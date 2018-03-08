/*!***************************************************************************
*!
*! FILE NAME  : tcp.cc
*!
*! DESCRIPTION: TCP, Transport control protocol
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
#include "timr.h"
}

#include "iostream.hh"
#include "tcp.hh"
#include "ip.hh"
#include "tcpsocket.hh"
#include "http.hh"


#define D_TCP
#ifdef D_TCP
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** TCP DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//
TCP::TCP()
{
}

//----------------------------------------------------------------------------
//
TCP&
TCP::instance()
{
  static TCP myInstance;
  return myInstance;
}

//----------------------------------------------------------------------------
//
TCPConnection*
TCP::getConnection(IPAddress& theSourceAddress,
                   uword      theSourcePort,
                   uword      theDestinationPort)
{
  TCPConnection* aConnection = NULL;
  // Find among open connections
  uword queueLength = myConnectionList.Length();
  myConnectionList.ResetIterator();
  bool connectionFound = false;
  while ((queueLength-- > 0) && !connectionFound)
  {
    aConnection = myConnectionList.Next();
    connectionFound = aConnection->tryConnection(theSourceAddress,
                                                 theSourcePort,
                                                 theDestinationPort);
  }
  if (!connectionFound)
  {
    aConnection = NULL;
  }
  return aConnection;
}

//----------------------------------------------------------------------------
//
TCPConnection*
TCP::createConnection(IPAddress& theSourceAddress,
                      uword      theSourcePort,
                      uword      theDestinationPort,
                      InPacket*  theCreator)
{
  TCPConnection* aConnection =  new TCPConnection(theSourceAddress,
                                                  theSourcePort,
                                                  theDestinationPort,
                                                  theCreator);

  myConnectionList.Append(aConnection);
  return aConnection;
}

//----------------------------------------------------------------------------
//
void
TCP::deleteConnection(TCPConnection* theConnection)
{
  myConnectionList.Remove(theConnection);
  delete theConnection;
}

//----------------------------------------------------------------------------
//
bool
TCP::acceptConnection(uword portNo) {
  if (portNo == 7 || portNo == 80) {
    return true;
  }
  return false;
}



//----------------------------------------------------------------------------
//
void
TCP::connectionEstablished(TCPConnection *theConnection)
{
  if (theConnection->serverPortNumber() == 7 || theConnection->serverPortNumber() == 80)
  {
    TCPSocket* aSocket = new TCPSocket(theConnection);
    // Create a new TCPSocket.
    theConnection->registerSocket(aSocket);
    // Register the socket in the TCPConnection.
    if (theConnection->serverPortNumber() == 7) {
      Job::schedule(new SimpleApplication(aSocket)); //delete this job?
    } else {
      Job::schedule(new HTTPServer(aSocket)); //delete this job?

    }
    // Create and start an application for the connection.
  }
}
//----------------------------------------------------------------------------
//
TCPConnection::TCPConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort,
                             InPacket*  theCreator):
        hisAddress(theSourceAddress),
        hisPort(theSourcePort),
        myPort(theDestinationPort)
{
  myTCPSender = new TCPSender(this, theCreator),
  timer = new retransmitTimer(this, Clock::seconds * 1), // new, in seconds or millis?
  myState = ListenState::instance();
  gotRST = false;
  mySocket = NULL;

}

//----------------------------------------------------------------------------
//
TCPConnection::~TCPConnection()
{
  delete myTCPSender;
  if (timer) {
    delete timer;
  }
  if (mySocket != NULL) {
    delete mySocket;  //wip testing

  }
}

//----------------------------------------------------------------------------
//
bool
TCPConnection::tryConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort)
{
  return (theSourcePort      == hisPort   ) &&
         (theDestinationPort == myPort    ) &&
         (theSourceAddress   == hisAddress);
}


// TCPConnection cont...
void
TCPConnection::Synchronize(udword theSynchronizationNumber)
{
  myState->Synchronize(this, theSynchronizationNumber);
}

void
TCPConnection::NetClose(){
  myState->NetClose(this);
}
// Handle an incoming FIN segment

void
TCPConnection::AppClose(){
  //myState->Acknowledgement(this);
  myState->AppClose(this);

}
// Handle close from application

void
TCPConnection::Kill(){
  myState->Kill(this);
}
// Handle an incoming RST segment, can also called in other error conditions

void
TCPConnection::Receive(udword theSynchronizationNumber,
             byte*  theData,
             udword theLength){

  myState->Receive(this,
                  theSynchronizationNumber,
                  theData,
                  theLength);

}
// Handle incoming data

void
TCPConnection::Acknowledge(udword theAcknowledgementNumber){
  myState->Acknowledge(this, theAcknowledgementNumber);
}
// Handle incoming Acknowledgement

void
TCPConnection::Send(byte*  theData, udword theLength){ //TODO WHY empty fam?
 myState->Send(this, theData, theLength);
}
// Send outgoing data

//----------------------------------------------------------------------------
//
uword
TCPConnection::serverPortNumber(){
  return myPort;
}

//----------------------------------------------------------------------------
//
void
TCPConnection::registerSocket(TCPSocket* theSocket)
{
  mySocket = theSocket;
}

//----------------------------------------------------------------------------
// TCPState contains dummies for all the operations, only the interesting ones
// gets overloaded by the various sub classes.


//----------------------------------------------------------------------------
//

void
TCPState::Synchronize(TCPConnection* theConnection,
                         udword theSynchronizationNumber)
{
}
// Handle an incoming SYN segment
void
TCPState::NetClose(TCPConnection* theConnection)
{
}
// Handle an incoming FIN segment
void
TCPState::AppClose(TCPConnection* theConnection)
{
}

void
TCPState::Kill(TCPConnection* theConnection)
{
  TCP::instance().deleteConnection(theConnection);
}

void
TCPState::Receive(TCPConnection* theConnection,
                     udword theSynchronizationNumber,
                     byte*  theData,
                     udword theLength)
{
}
// Handle incoming data
void
TCPState::Acknowledge(TCPConnection* theConnection,
                         udword theAcknowledgementNumber)
{
}
// Handle incoming Acknowledgement
void
TCPState::Send(TCPConnection* theConnection,
                  byte*  theData,
                  udword theLength)
{
}





//----------------------------------------------------------------------------
//
ListenState*
ListenState::instance()
{
  static ListenState myInstance;
  return &myInstance;
}

void
ListenState::Synchronize(TCPConnection* theConnection,
                         udword theSynchronizationNumber)
{

   theConnection->receiveNext = theSynchronizationNumber + 1; // look below
   // acknumber , we ack on their sequenceNumber -> acknowledgementNumber = receiveNext
   theConnection->receiveWindow = 8*1024;
   theConnection->sendNext = get_time();
   // Next reply to be sent.
   theConnection->sentUnAcked = theConnection->sendNext;
   // Send a segment with the SYN and ACK flags set.
   theConnection->myTCPSender->sendFlags(0x12);
   // Prepare for the next send operation.
   theConnection->sendNext += 1;
   // Change state
   theConnection->myState = SynRecvdState::instance();

}






//----------------------------------------------------------------------------
//
SynRecvdState*
SynRecvdState::instance()
{
  static SynRecvdState myInstance;
  return &myInstance;
}

void
SynRecvdState::Acknowledge(TCPConnection* theConnection,
                 udword theAcknowledgementNumber){
                   
   // Next reply to be sent.
   theConnection->sentUnAcked = theAcknowledgementNumber; //update the acknumber
   theConnection->sentMaxSeq = theAcknowledgementNumber; //wip, dont know where else
   // Change state....
   TCP::instance().connectionEstablished(theConnection); //ok connection
   theConnection->myState = EstablishedState::instance();

}





//----------------------------------------------------------------------------
//
EstablishedState*
EstablishedState::instance()
{
  static EstablishedState myInstance;
  return &myInstance;
}


//----------------------------------------------------------------------------
//
void
EstablishedState::NetClose(TCPConnection* theConnection)
{

  // Update connection variables and send an ACK

  // Go to NetClose wait state, inform application
  theConnection->myState = CloseWaitState::instance();
  theConnection->mySocket->socketEof();

  // Normally the application would be notified next and nothing
  // happen until the application calls appClose on the connection.
  // Since we don't have an application we simply call appClose here instead.

}

void
EstablishedState::AppClose(TCPConnection* theConnection ) {
  theConnection->myState = FinWait1State::instance();
  theConnection->myTCPSender->sendFlags(0x11); //fin/ack maybe only ack
}



//----------------------------------------------------------------------------
//
void
EstablishedState::Receive(TCPConnection* theConnection,
                          udword theSynchronizationNumber,
                          byte*  theData,
                          udword theLength)
{
  // Delayed ACK is not implemented, simply acknowledge the data
  // by sending an ACK segment, then echo the data using Send.
  theConnection->receiveNext = theSynchronizationNumber + theLength; // WIP
  theConnection->myTCPSender->sendFlags(0x10);
  theConnection->mySocket->socketDataReceived(theData, theLength);
}


void
EstablishedState::Acknowledge(TCPConnection* theConnection,
                 udword theAcknowledgementNumber)
{


   // Next reply to be sent.
   // Send a segment with the ACK flag set.
   // Prepare for the next send operation.
   // Change state
    if (theConnection->sentUnAcked < theAcknowledgementNumber) {
     theConnection->sentUnAcked = theAcknowledgementNumber;
   }
   if(theConnection->sendNext == theAcknowledgementNumber) {
     theConnection->timer->cancel();
   }
   if (theConnection->sendNext < theAcknowledgementNumber) { // && theConnection->sentMaxSeq != theConnection->sendNext
       theConnection->sendNext = theAcknowledgementNumber;
       theConnection->timer->cancel();
   }
   theConnection->myTCPSender->sendFromQueue();

}
// Handle incoming Acknowledgement

void
EstablishedState::Send(TCPConnection* theConnection,
          byte*  theData,
          udword theLength)
{
  //initialize the sending queue
  theConnection->transmitQueue = theData;
  theConnection->queueLength = theLength;
  theConnection->firstSeq = theConnection->sendNext;
  theConnection->theFirst = theData;
  theConnection->theOffset = 0;
  theConnection->theSendLength = 0;
  theConnection->myTCPSender->sendFromQueue();
  //all data sent -> notify and release sem
  theConnection->mySocket->socketDataSent(); //kolla på den sen?
}
// Send outgoing data





//----------------------------------------------------------------------------
//
CloseWaitState*
CloseWaitState::instance()
{
  static CloseWaitState myInstance;
  return &myInstance;
}


void
CloseWaitState::AppClose(TCPConnection* theConnection) {
  theConnection->myState = LastAckState::instance();
  theConnection->myState->Acknowledge(theConnection, theConnection->receiveNext);
  theConnection->Kill(); // prob not wrong?? first get to LastAckState??
}
// Handle close from application


//----------------------------------------------------------------------------
//
LastAckState*
LastAckState::instance()
{
  static LastAckState myInstance;
  return &myInstance;
}


void
LastAckState::Acknowledge(TCPConnection* theConnection,
                 udword theAcknowledgementNumber)
{

    // Send a segment with the ACK flag set.
   // Prepare for the next send operation.
   theConnection->receiveNext += 1;
   theConnection->sentUnAcked = theAcknowledgementNumber;
   theConnection->myTCPSender->sendFlags(0x10);
   // Change state
}

//----------------------------------------------------------------------------
//



FinWait1State*
FinWait1State::instance(){
  static FinWait1State myInstance;
  return &myInstance;
}

void
FinWait1State::Acknowledge(TCPConnection* theConnection, udword acknowledgementNumber){
  theConnection->myState = FinWait2State::instance();
}
void
FinWait1State::NetClose(TCPConnection* theConnection) {
  theConnection->receiveNext += 1;
  theConnection->sendNext += 1;
  theConnection->myTCPSender->sendFlags(0x10);
  theConnection->Kill();
}

//----------------------------------------------------------------------------
//


FinWait2State*
FinWait2State::instance(){
  static FinWait2State myInstance;
  return &myInstance;
}

void
FinWait2State::NetClose(TCPConnection* theConnection){
  theConnection->receiveNext += 1;
  theConnection->sendNext += 1;
  theConnection->myTCPSender->sendFlags(0x10);
  theConnection->Kill();
}





//----------------------------------------------------------------------------
//
TCPSender::TCPSender(TCPConnection* theConnection,
                     InPacket*      theCreator):
        myConnection(theConnection),
        myAnswerChain(theCreator->copyAnswerChain()) // Copies InPacket chain!
{
}

//----------------------------------------------------------------------------
//
TCPSender::~TCPSender()
{
  myAnswerChain->deleteAnswerChain();
}

void
TCPSender::sendFlags(byte theFlags)
{
  if(!myConnection->gotRST){
    // Decide on the value of the length totalSegmentLength.
    // Allocate a TCP segment.
    uword totalSegmentLength = 20;
    byte* anAnswer = new byte[totalSegmentLength];
    // Calculate the pseudo header checksum
    TCPPseudoHeader* aPseudoHeader =
      new TCPPseudoHeader(myConnection->hisAddress,
                          totalSegmentLength);
    uword pseudosum = aPseudoHeader->checksum();
    delete aPseudoHeader;
    // Create the TCP segment.
    // Calculate the final checksum.
    TCPHeader* aTCPHeader = (TCPHeader*) anAnswer;
    aTCPHeader->flags = theFlags;
    aTCPHeader->sourcePort = HILO(myConnection->myPort);
    aTCPHeader->destinationPort = HILO(myConnection->hisPort);
    aTCPHeader->sequenceNumber = LHILO(myConnection->sendNext);
    aTCPHeader->acknowledgementNumber = LHILO(myConnection->receiveNext); // done!!
    aTCPHeader->headerLength = 0x50;
    aTCPHeader->checksum = 0;
    aTCPHeader->urgentPointer = 0;
    aTCPHeader->windowSize = HILO(myConnection->receiveWindow); //???
    aTCPHeader->checksum = calculateChecksum((byte*)aTCPHeader,
                                             totalSegmentLength,
                                             pseudosum);

    myAnswerChain->answer((byte*)aTCPHeader,
                          totalSegmentLength);
    // Deallocate the dynamic memory
    delete anAnswer;
  }
}

// ---------------------------------------------------------------------------

void
TCPSender::sendData(byte*  theData, udword theLength) {
  // Calculate the pseudo header checksum
  if(!myConnection->gotRST){
    myConnection->timer->start();
    udword totalSegmentLength = 20 + theLength;
    byte* anAnswer = new byte[totalSegmentLength];

    //create header and memcpy
    TCPPseudoHeader* aPseudoHeader =
      new TCPPseudoHeader(myConnection->hisAddress,
                          totalSegmentLength);
    uword pseudosum = aPseudoHeader->checksum();

    delete aPseudoHeader;
    // Copy over the data to anAnswer
    memcpy((byte*)(anAnswer + 20), theData, theLength);

    // Create the TCP segment.
    // Calculate the final checksum.
    TCPHeader* aTCPHeader = (TCPHeader*) anAnswer;
    aTCPHeader->flags = 0x18;
    aTCPHeader->sourcePort = HILO(myConnection->myPort);
    aTCPHeader->destinationPort = HILO(myConnection->hisPort);
    aTCPHeader->sequenceNumber = LHILO(myConnection->sendNext);
    aTCPHeader->acknowledgementNumber = LHILO(myConnection->receiveNext); // done!!
    aTCPHeader->headerLength = 0x50;
    aTCPHeader->checksum = 0;
    aTCPHeader->urgentPointer = 0;
    aTCPHeader->windowSize = HILO(myConnection->receiveWindow); //???
    aTCPHeader->checksum = calculateChecksum(anAnswer,
                                             totalSegmentLength,
                                             pseudosum);

    myAnswerChain->answer(anAnswer, totalSegmentLength);

    delete anAnswer;
      // Deallocate the dynamic memory
  }
}


//-----------------------------------------------------------------------------
//

void
TCPSender::sendFromQueue(){
  if (myConnection->sentMaxSeq > myConnection->sendNext || myConnection->timer->retransmit){
    myConnection->timer->retransmit = false;
    myConnection->timer->cancel();
    //retrsansmission
    udword send_l = MIN(TCP::maxSegmentLength, myConnection->queueLength -
      (myConnection->sentUnAcked - myConnection->firstSeq));
    sendData(myConnection->theFirst + (myConnection->sentUnAcked - myConnection->firstSeq), send_l); //the one thats missing
    myConnection->sendNext = myConnection->sentMaxSeq; //wip prob ok
  } else {
    //not retransmission
    udword theWindowSize = myConnection->myWindowSize -
        (myConnection->sendNext - myConnection->sentUnAcked);
    // if the segment is over 1460 bytes

    while (myConnection->queueLength - myConnection->theOffset > 0 && theWindowSize > 0) {
      // We dont handle the case where
      udword send_l = MIN(theWindowSize, myConnection->queueLength - myConnection->theOffset);
      send_l = MIN(send_l, TCP::maxSegmentLength);
      myConnection->theSendLength = send_l;
      sendData(myConnection->theFirst + myConnection->theOffset, myConnection->theSendLength);
      myConnection->sentMaxSeq = myConnection->sendNext; //new
      myConnection->sendNext += send_l;
      myConnection->theOffset += myConnection->theSendLength;

    }
  }

}

//----------------------------------------------------------------------------
//

retransmitTimer::retransmitTimer(TCPConnection* theConnection,
               Duration retransmitTime):
  myConnection(theConnection),
  myRetransmitTime(retransmitTime)
{
}

void
retransmitTimer::start(){
  this->timeOutAfter(myRetransmitTime);
}

void
retransmitTimer::cancel(){
  this->resetTimeOut();
}

void
retransmitTimer::timeOut(){
  retransmit = true;
  myConnection->sendNext = myConnection->sentUnAcked;
  myConnection->myTCPSender->sendFromQueue();

}

//----------------------------------------------------------------------------
//
TCPInPacket::TCPInPacket(byte*           theData,
                         udword          theLength,
                         InPacket*       theFrame,
                         IPAddress&      theSourceAddress):
        InPacket(theData, theLength, theFrame),
        mySourceAddress(theSourceAddress)
{
}

void
TCPInPacket::decode()
{
  // Extract the parameters from the TCP header which define the
  // connection.
  //create a tcpheader
  TCPHeader* aTCPHeader = (TCPHeader*)myData;
  mySourcePort = HILO(aTCPHeader->sourcePort);
  myDestinationPort = HILO(aTCPHeader->destinationPort);
  myAcknowledgementNumber = LHILO(aTCPHeader->acknowledgementNumber);
  mySequenceNumber = LHILO(aTCPHeader->sequenceNumber);

  TCPConnection* aConnection =
         TCP::instance().getConnection(mySourceAddress,
                                       mySourcePort,
                                       myDestinationPort);

  if (!aConnection)
  {
    // Establish a new connection.
    aConnection =
         TCP::instance().createConnection(mySourceAddress,
                                          mySourcePort,
                                          myDestinationPort,
                                          this);

    if ((aTCPHeader->flags & 0x10) != 0) {
    }
    if ((aTCPHeader->flags & 0x02) != 0)
    {
      // State LISTEN. Received a SYN flag.
      if(TCP::instance().acceptConnection(aConnection->myPort)){
        //cout << "Synchronize" << endl;
        aConnection->Synchronize(mySequenceNumber);
      }
    }
    else
    {
      // State LISTEN. No SYN flag. Impossible to continue.
      aConnection->Kill();
    }
  }
  else
  {
    aConnection->myWindowSize = HILO(aTCPHeader->windowSize); //maybe also add when creating connection
    // Connection was established. Handle all states.
    if ((aTCPHeader->flags & 0x10 ) != 0) {
    // Received a ACK flag

      if(aTCPHeader->flags & 0x01) { //FIN
        aConnection->NetClose();

      } else if ((aTCPHeader->flags & 0x08) != 0) {
          aConnection->Receive(mySequenceNumber,
                            myData + headerOffset(),
                            myLength - headerOffset());

      } else if ((aTCPHeader->flags & 0x04) != 0) {
        //in case of RST flag
        aConnection->gotRST = true;
        aConnection->Kill();

      } else { // check for ack

        if (myLength - headerOffset() > 0) { // wip check for data in the ack before push
          aConnection->Receive(mySequenceNumber,
                            myData + headerOffset(),
                            myLength - headerOffset());
        }  else {
            aConnection->Acknowledge(myAcknowledgementNumber);
        } // prob solution to data with ack??
    }
    }
  }
}

//----------------------------------------------------------------------------
//
InPacket*
TCPInPacket::copyAnswerChain()
{
  return myFrame->copyAnswerChain();
}

void
TCPInPacket::answer(byte* theData, udword theLength){
  // we now move the headeroffset in the ip class instead
  myFrame->answer(theData, theLength);
}

uword
TCPInPacket::headerOffset(){
  return 20;
}
//----------------------------------------------------------------------------
//
TCPPseudoHeader::TCPPseudoHeader(IPAddress& theDestination,
                                 uword theLength):
        sourceIPAddress(IP::instance().myAddress()),
        destinationIPAddress(theDestination),
        zero(0),
        protocol(6)
{
  tcpLength = HILO(theLength);
}

//----------------------------------------------------------------------------
//
uword
TCPPseudoHeader::checksum()
{
  return calculateChecksum((byte*)this, 12);
}

/****************** END OF FILE tcp.cc *************************************/
