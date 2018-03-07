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
  udword aLength;
  char* aData;
  char* header;

  //cout << done << " and " << mySocket->isEof() << endl;

  //  while (!done && !mySocket->isEof())
  //  {
    //cout << "Core in socket" << ax_coreleft_total() << endl;
    aData = (char*)mySocket->Read(aLength);
    header = extractString(aData, aLength);
  //  cout << "path: " << path <<" end path"<< endl;
  if(strncmp(aData,"GET", 3) == 0 || strncmp(aData,"HEAD", 4) == 0){
    getRequest(header, aLength);

      }  else if (strncmp(aData, "POST", 4) == 0){
        //Post
        postRequest(header, aLength);
       }
    delete header;
    delete aData;
    mySocket->Close();

}
bool
HTTPServer::authentication(char* header) {
  char* unAuth = "HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\nWWW-Authenticate: Basic realm=\"private\"\r\n\r\n
<html><head><title>401 Unauthorized</title></head>\r\n
<body><h1>401 Unauthorized</h1></body></html>";
  char* usrpwd1 = "marcus:222";
  char* auth = "Basic";

  //cout << header << endl;
  if (strstr(header, auth) == NULL) {//not auth
    cout << "privjet, suka" << endl;

    mySocket->Write((byte*)unAuth, strlen(unAuth));
    return false;


  } else {
    cout << "check pass" << endl;
    char* secCheck = strstr(header, auth);
    secCheck = strchr(secCheck, ' ');
    secCheck++;
    char* secEnd = strchr(secCheck, ' ');
    secCheck = extractString(secCheck, (udword)(secEnd - secCheck));
    secCheck = decodeBase64(secCheck);
//    cout << "the found credentials"
    cout << "their credentials: " << secCheck << " - stored credentials : " << usrpwd1 << endl;
    if (strcmp(secCheck, usrpwd1) == 0) {
      cout << "check cleared" << endl;

    } else {
      mySocket->Write((byte*)unAuth, strlen(unAuth));
      delete secCheck;
      return false;
    }
    delete secCheck;
    return true;

  }
}
void
HTTPServer::getRequest(char* header, udword aLength) {
  char* notF = "HTTP/1.0 404 Not found\r\nContent-type: text/html\r\n\r\n
  <html><head><title>File not found</title></head>
  <body><h1>404 Not found</h1></body></html>";
  //cout << "this is a job" << endl;

  char* sendType = "Content-type: text/html\r\n\r\n";
  char* ok = "HTTP/1.0 200 OK\r\n"; //the first part of a ok
  bool privCheck = true;
  char* path;

  path = findPathName(header); //Eventuellt dela upp header och enbart använda första raden
  char* file = findFileName(header); //check this out
  char* type = strchr(file, '.');

  type++;
  if (strncmp(path,"private", 7) == 0) {
    //check for authentication
    privCheck = authentication(header);

    // if (privCheck) { wip cleared the double auth problem
    //   privCheck = authentication(header);
    // }
  }
  if (privCheck) {
    if ( strcmp(type,"gif") == 0){
      sendType = "Content-type: image/gif\r\n\r\n";
    } else if (strcmp(type, "jpg") == 0) {
      sendType = "Content-type: image/jpeg\r\n\r\n";
    } else {
      sendType = "Content-type: text/html\r\n\r\n";
    }
    mySocket->Write((byte*)ok, strlen(ok));
    mySocket->Write((byte*)sendType, (uint) strlen(sendType));

    if (strncmp(header,"GET", 3) == 0) {

      byte* answer = FileSystem::instance().readFile(path, file, aLength);
      if (answer == NULL) {
        //if data not found
        mySocket->Write((byte*)notF, strlen(notF));
      } else {
        //write the data
        mySocket->Write(answer, (uint) aLength);
      }
      delete answer; // wip

    }
  }
  delete path;
  delete file;
  delete type; //maybe worry bout this

}
void
HTTPServer::postRequest(char* header, udword aLength) {
  //post something
  char* ok = "HTTP/1.0 200 OK\r\n"; //the first part of a ok
  char* sendType = "Content-type: text/html\r\n\r\n";
  char* accepted = "<html><head><title>Accepted</title></head>
<body><h1>The file dynamic.htm was updated successfully.</h1></body></html>";
  char* path;

  path = findPathName(header); //Eventuellt dela upp header och enbart använda första raden
  char* file = findFileName(header); //check this out

  char* postReq = strstr(header, "dynamic");
  udword contLen = contentLength(header, aLength);
  postReq = decodeForm(postReq);
  byte* toWrite = new byte[contLen + 1]; // dunno if append null or not
  memcpy(toWrite, postReq, contLen + 1);
  cout << "path: " << path << " file: " << file << endl;
  cout << (char*)toWrite << endl;
  FileSystem::instance().writeFile(path, file, toWrite, contLen+1);

  mySocket->Write((byte*)ok, strlen(ok));
  mySocket->Write((byte*)sendType, strlen(sendType));
  mySocket->Write((byte*)accepted, strlen(accepted));
  delete toWrite;
  delete path;
  delete file;
  delete postReq;

}


/************** END OF FILE http.cc *************************************/
