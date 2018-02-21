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


#define D_TCP
#ifdef D_TCP
#define trace cout
#else
#define trace if(false) cout
#endif

//----------------------------------------------------------------------------
//
TCPSocket::TCPSocket(TCPConnection* theConnection):
  myConnection(theConnection),
  myReadSemaphore(Semaphore::createQueueSemaphore("read",0)),
  myWriteSemaphore(Semaphore::createQueueSemaphore("write",0))
{
}

//----------------------------------------------------------------------------
//
TCPSocket::~TCPSocket()
{
  delete myConnection;
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
  //~TCPSocket(); //maybe?
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
//  cout << "work work" << endl;
  udword aLength;
  byte* aData;
  bool done = false;
//  cout << done << " and " << mySocket->isEof() << endl;

  while (!done && !mySocket->isEof())
  {
    aData = mySocket->Read(aLength);
    if (aLength > 0)
    {
      mySocket->Write(aData, aLength);
      if ((char)*aData == 'q')
      {
        done = true;
        //cout << "work done true" << endl;
      }
      delete aData;
    }
  }
  mySocket->Close();
}
