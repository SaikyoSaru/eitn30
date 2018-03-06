/*!***************************************************************************
*!
*! FILE NAME  : http.cc
*!
*! DESCRIPTION: HTTP, Hyper text transfer protocol.
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}

#include "iostream.hh"
#include "tcpsocket.hh"
#include "http.hh"

//#define D_HTTP
#ifdef D_HTTP
#define trace cout
#else
#define trace if(true) cout
#endif

/****************** HTTPServer DEFINITION SECTION ***************************/



HTTPServer::HTTPServer(TCPSocket* theSocket) :
  mySocket(theSocket)
{
  //fs = new FileSystem();
}

//----------------------------------------------------------------------------
//
HTTPServer::~HTTPServer(){
  //delete fs;
}



//----------------------------------------------------------------------------
//
// Allocates a new null terminated string containing a copy of the data at
// 'thePosition', 'theLength' characters long. The string must be deleted by
// the caller.
//
char*
HTTPServer::extractString(char* thePosition, udword theLength)
{
  char* aString = new char[theLength + 1];
  strncpy(aString, thePosition, theLength);
  aString[theLength] = '\0';
  return aString;
}

//----------------------------------------------------------------------------
//
// Will look for the 'Content-Length' field in the request header and convert
// the length to a udword
// theData is a pointer to the request. theLength is the total length of the
// request.
//
udword
HTTPServer::contentLength(char* theData, udword theLength)
{
  udword index = 0;
  bool   lenFound = false;
  const char* aSearchString = "Content-Length: ";
  while ((index++ < theLength) && !lenFound)
  {
    lenFound = (strncmp(theData + index,
                        aSearchString,
                        strlen(aSearchString)) == 0);
  }
  if (!lenFound)
  {
    return 0;
  }
  trace << "Found Content-Length!" << endl;
  index += strlen(aSearchString) - 1;
  char* lenStart = theData + index;
  char* lenEnd = strchr(theData + index, '\r');
  char* lenString = this->extractString(lenStart, lenEnd - lenStart);
  udword contLen = atoi(lenString);
  trace << "lenString: " << lenString << " is len: " << contLen << endl;
  delete [] lenString;
  return contLen;
}

//----------------------------------------------------------------------------
//
// Decode user and password for basic authentication.
// returns a decoded string that must be deleted by the caller.
//
char*
HTTPServer::decodeBase64(char* theEncodedString)
{
  static const char* someValidCharacters =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  int aCharsToDecode;
  int k = 0;
  char  aTmpStorage[4];
  int aValue;
  char* aResult = new char[80];

  // Original code by JH, found on the net years later (!).
  // Modify on your own risk.

  for (unsigned int i = 0; i < strlen(theEncodedString); i += 4)
  {
    aValue = 0;
    aCharsToDecode = 3;
    if (theEncodedString[i+2] == '=')
    {
      aCharsToDecode = 1;
    }
    else if (theEncodedString[i+3] == '=')
    {
      aCharsToDecode = 2;
    }

    for (int j = 0; j <= aCharsToDecode; j++)
    {
      int aDecodedValue;
      aDecodedValue = strchr(someValidCharacters,theEncodedString[i+j])
        - someValidCharacters;
      aDecodedValue <<= ((3-j)*6);
      aValue += aDecodedValue;
    }
    for (int jj = 2; jj >= 0; jj--)
    {
      aTmpStorage[jj] = aValue & 255;
      aValue >>= 8;
    }
    aResult[k++] = aTmpStorage[0];
    aResult[k++] = aTmpStorage[1];
    aResult[k++] = aTmpStorage[2];
  }
  aResult[k] = 0; // zero terminate string

  return aResult;
}

//------------------------------------------------------------------------
//
// Decode the URL encoded data submitted in a POST.
//
char*
HTTPServer::decodeForm(char* theEncodedForm)
{
  char* anEncodedFile = strchr(theEncodedForm,'=');
  anEncodedFile++;
  char* aForm = new char[strlen(anEncodedFile) * 2];
  // Serious overkill, but what the heck, we've got plenty of memory here!
  udword aSourceIndex = 0;
  udword aDestIndex = 0;

  while (aSourceIndex < strlen(anEncodedFile))
  {
    char aChar = *(anEncodedFile + aSourceIndex++);
    switch (aChar)
    {
     case '&':
       *(aForm + aDestIndex++) = '\r';
       *(aForm + aDestIndex++) = '\n';
       break;
     case '+':
       *(aForm + aDestIndex++) = ' ';
       break;
     case '%':
       char aTemp[5];
       aTemp[0] = '0';
       aTemp[1] = 'x';
       aTemp[2] = *(anEncodedFile + aSourceIndex++);
       aTemp[3] = *(anEncodedFile + aSourceIndex++);
       aTemp[4] = '\0';
       udword anUdword;
       anUdword = strtoul((char*)&aTemp,0,0);
       *(aForm + aDestIndex++) = (char)anUdword;
       break;
     default:
       *(aForm + aDestIndex++) = aChar;
       break;
    }
  }
  *(aForm + aDestIndex++) = '\0';
  return aForm;
}
//------------------------------------------------------------------------
//
//
char*
HTTPServer::findPathName(char* str)
//Jossan hade flera liknande metoder här som de kallade i doit.
{
  char* firstPos = strchr(str, ' ');     // First space on line
  firstPos++;                            // Pointer to first /
  char* lastPos = strchr(firstPos, ' '); // Last space on line
  char* thePath = 0;                     // Result path
  if ((lastPos - firstPos) == 1)
  {
    // Is / only
    thePath = 0;                         // Return NULL
  }
  else
  {
    // Is an absolute path. Skip first /.
    thePath = extractString((char*)(firstPos+1),
                            lastPos-firstPos);
    if ((lastPos = strrchr(thePath, '/')) != 0)
    {
      // Found a path. Insert -1 as terminator.
      *lastPos = '\xff';
      *(lastPos+1) = '\0';
      while ((firstPos = strchr(thePath, '/')) != 0)
      {
        // Insert -1 as separator.
        *firstPos = '\xff';
      }
    }
    else
    {
      // Is /index.html
      delete thePath; thePath = 0; // Return NULL
    }
  }
  return thePath;
}
//-----------------------------------------------------------------------------
//
//fin the name of the requested file
char*
HTTPServer::findFileName(char* str)
{
  char* firstPos = strchr(str, '/');
  firstPos++;
  char* lastPos = strchr(firstPos, ' ');
  char* fileName = extractString(firstPos, (udword)(lastPos - firstPos));
  fileName = strrchr(fileName, '/');
  fileName++;
  //no filename found -> index page
  if (strlen(fileName) == 0) {
    fileName = "index.htm";
  }
  return fileName;
}

//-----------------------------------------------------------------------------
//

//the job to schedule
void HTTPServer::doit()
{
  char* notF = "HTTP/1.0 404 Not found\r\nContent-type: text/html\r\n\r\n
  <html><head><title>File not found</title></head>
  <body><h1>404 Not found</h1></body></html>";
  cout << "this is a job" << endl;



  udword aLength;
  char* aData;
  char* header;
  char* path;
  char* sendType;
  //cout << done << " and " << mySocket->isEof() << endl;

  //  while (!done && !mySocket->isEof())
  //  {
    //cout << "Core in socket" << ax_coreleft_total() << endl;
    aData = (char*)mySocket->Read(aLength);
    header = extractString(aData, aLength);
    path = findPathName(header); //Eventuellt dela upp header och enbart använda första raden
  //  cout << "path: " << path <<" end path"<< endl;
    if(strncmp(aData,"GET", 3) == 0){
      //cout << "get request" << endl;
      //cout << aData << endl;
      //mySocket->Write((byte*)stdget, (uint)strlen(stdget));
      char* file = findFileName(header); //check this out
    //  cout << "filename: " << file << endl;
      char* type = strchr(file, '.');
      type++;

        if ( strcmp(type,"gif") == 0){
    //      cout << "gif" << endl;
          sendType = "HTTP/1.0 200 OK\r\nContent-type: image/gif\r\n\r\n";
          // path = "pict";
          // file = "small1.gif";
        } else if (strcmp(type, "jpg") == 0) {
    //      cout << "jpg" << endl;
          sendType = "HTTP/1.0 200 OK\r\nContent-type: image/jpeg\r\n\r\n";
        } else {
      //    cout << "htm" << endl;
          sendType = "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n";

        }



      byte* answer = FileSystem::instance().readFile(path, file, aLength);
      if (answer == NULL) {
        mySocket->Write((byte*)notF, strlen(notF));
      } else {
    //  cout << "len: " << aLength << endl;
    //  cout << (char*)answer << endl;
      mySocket->Write((byte*)sendType, (uint) strlen(sendType));
      cout << "trams" << endl;
      mySocket->Write(answer, (uint) aLength);
  //    cout << "sent answer" << endl;
      }
      delete answer;
      delete header;

    //  done = true; //wip
    } else if(strncmp(aData,"POST", 4) == 0){
  //    cout << "post request" << endl;
    }

//  }
  //close after every
  //cout << "close socket" << endl;
  mySocket->Close();

}

//HTTPServer::requestChecker()

/************** END OF FILE http.cc *************************************/
