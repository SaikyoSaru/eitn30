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
  myReadLength(0)
{
}

//----------------------------------------------------------------------------
//
TCPSocket::~TCPSocket()
{
  //delete myConnection;
  delete myReadData;
  //cout << "delete socket" << endl;
  delete myReadSemaphore;
  delete myWriteSemaphore;
}

//----------------------------------------------------------------------------
//
byte*
TCPSocket::Read(udword& theLength)//Copied
{
  myReadSemaphore->wait(); // Wait for available data
//  cout << "read" << endl;
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
  cout << "the data got acked" << endl;
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
  //cout << "socketDataReceived" << endl;
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
  cout << "SimpleApplication" << endl;
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
    //data = new byte[len];
    // char mats [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZMATSHARENKURSS";
    // for(udword i=0; i<len; i+=40){
    //   memcpy(data[i], mats, 40);
    //  }
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
