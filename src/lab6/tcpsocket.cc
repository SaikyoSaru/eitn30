#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
#include "timr.h"
}

#include "iostream.hh"
#include "tcp.hh"
#include "ip.hh"
#include "tcpsocket.hh"


#define D_TCPSOCKET
#ifdef D_TCPSOCKET
#define trace cout
#else
#define trace if(false) cout
#endif

//----------------------------------------------------------------------------
//
TCPSocket::TCPSocket(TCPConnection* theConnection):
  myConnection(theConnection),
  myReadSemaphore(Semaphore::createQueueSemaphore("read",0)),
  myWriteSemaphore(Semaphore::createQueueSemaphore("write",0)),
  myReadLength(0),
  eofFound(false)
{
}

//----------------------------------------------------------------------------
//
TCPSocket::~TCPSocket()
{
  delete myReadData;
  delete myReadSemaphore;
  delete myWriteSemaphore;
}

//----------------------------------------------------------------------------
//
byte*
TCPSocket::Read(udword& theLength)//Copied
{
  myReadSemaphore->wait(); // Wait for available data
  theLength = myReadLength;
  byte* aData = myReadData;
  myReadLength = 0;
  myReadData = 0;
  return aData;
}

//----------------------------------------------------------------------------
//
bool
TCPSocket::isEof(){
  return eofFound;
}

//----------------------------------------------------------------------------
//
void
TCPSocket::Write(byte* theData, udword theLength)//Copied
{
  myConnection->Send(theData, theLength);
  myWriteSemaphore->wait(); // Wait until the data is acknowledged
}

//----------------------------------------------------------------------------
//
void
TCPSocket::Close()
{
  myConnection->AppClose();
}
//----------------------------------------------------------------------------
//
void
TCPSocket::socketDataReceived(byte* theData, udword theLength)//Copied
{
  myReadData = new byte[theLength];
  memcpy(myReadData, theData, theLength);
  myReadLength = theLength;
  myReadSemaphore->signal(); // Data is available
}

//----------------------------------------------------------------------------
//
void
TCPSocket::socketDataSent() //Copied
{
  myWriteSemaphore->signal(); // The data has been acknowledged
}

//----------------------------------------------------------------------------
//
void
TCPSocket::socketEof() //Copied
{

  eofFound = true;
  myReadSemaphore->signal();
}


//----------------------------------------------------------------------------
//
SimpleApplication::SimpleApplication(TCPSocket* theSocket):
  mySocket(theSocket)
{
}

//----------------------------------------------------------------------------
//
void
SimpleApplication::doit() //Copied
{
  udword aLength;
  byte* aData;
  bool done = false;

  while (!done && !mySocket->isEof())
  {
    aData = mySocket->Read(aLength);
    if (aLength > 0)
    {

      if ((char)*aData == 'q') {
        done = true;
      } else if ((char)*aData == 's'){
        udword len = 10000;
        byte* data = new byte[len];
        generateData(data, len);
        mySocket->Write(data, len);
        delete data; //wip
      } else if ((char)*aData == 't'){
        udword len = 1000000;
        byte* data = new byte[len];
        generateData(data, len);
        mySocket->Write(data, len);
        delete data; //wip
      } else {
          mySocket->Write(aData, aLength);
      }
      delete aData;
    }
  }
  mySocket->Close();
}


void
SimpleApplication::generateData(byte* data, udword len) {
    byte c = 'A' - 1;
    for (udword i=0; i<len; i++) {
      if((i%1460) == 0){
        if(c == 'Z'){
          c = 'A';
        } else {
          c++;
        }
      }
      data[i] = c;
    }
}
