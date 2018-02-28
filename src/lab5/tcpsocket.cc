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
  myWriteSemaphore(Semaphore::createQueueSemaphore("write",0))
{
}

//----------------------------------------------------------------------------
//
TCPSocket::~TCPSocket()
{
  //delete myConnection;
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
  //cout << "the len: sent" << theLength << " pointer: "<<(udword)theData <<  endl;

  myConnection->Send(theData, theLength);
  myWriteSemaphore->wait(); // Wait until the data is acknowledged
}

//----------------------------------------------------------------------------
//
void
TCPSocket::Close()
{
  //myConnection->myState->FinWait1State::instance();
  myConnection->AppClose();
  //delete this;
}
//----------------------------------------------------------------------------
//
void
TCPSocket::socketDataReceived(byte* theData, udword theLength)//Copied
{
  //cout << "the len received: " << theLength << " pointer: "<<(udword)theData <<  endl;
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
  //cout << done << " and " << mySocket->isEof() << endl;

  while (!done && !mySocket->isEof())
  {
    //cout << "Core in socket" << ax_coreleft_total() << endl;
    aData = mySocket->Read(aLength);
    if (aLength > 0)
    {
      mySocket->Write(aData, aLength);
      if ((char)*aData == 'q') {
        done = true;
      } else if ((char)*aData == 's'){
        udword len = 10000;
        byte* data = new byte[len];
        generateData(data, len);
        mySocket->Write(data, len);
        delete data;
      }
      delete aData;
    }
  }
  mySocket->Close();
}


void
SimpleApplication::generateData(byte* data, udword len) {
    //data = new byte[len];
    for (udword i=0; i<len; i++) {
      data[i] = 'b';
      if(i%5 == 0){
        data[i] = 'n';
      }
    }
    // for (udword i=0; i<len; i++) {
    //   cout << data[i] << endl;
    // }

}
