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
  trace << "TCP created." << endl;
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
    trace << "Connection not found!" << endl;
    aConnection = NULL;
  }
  else
  {
    trace << "Found connection in queue" << endl;
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
  cout << ax_coreleft_total() << " create1 " << endl;

  TCPConnection* aConnection =  new TCPConnection(theSourceAddress,
                                                  theSourcePort,
                                                  theDestinationPort,
                                                  theCreator);
  
  cout << ax_coreleft_total() << " create2 " << endl;

  myConnectionList.Append(aConnection);
  return aConnection;
}

//----------------------------------------------------------------------------
//
void
TCP::deleteConnection(TCPConnection* theConnection)
{
cout << ax_coreleft_total() << " delete1 " << endl;
  myConnectionList.Remove(theConnection);
  delete theConnection;
  cout << ax_coreleft_total() << " delete2" << endl;
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
  cout << ax_coreleft_total() << " 1" << endl;
  trace << "TCP connection created" << endl;
  myTCPSender = new TCPSender(this, theCreator),
  cout << ax_coreleft_total() << " 2" << endl;
  myState = ListenState::instance();

}

//----------------------------------------------------------------------------
//
TCPConnection::~TCPConnection()
{
  trace << "TCP connection destroyed" << endl;
  delete myTCPSender;
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
TCPConnection::Send(byte*  theData, udword theLength){

}
// Send outgoing data


//----------------------------------------------------------------------------
// TCPState contains dummies for all the operations, only the interesting ones
// gets overloaded by the various sub classes.


//----------------------------------------------------------------------------
//

void
TCPState::Synchronize(TCPConnection* theConnection,
                         udword theSynchronizationNumber)
{
  //theConnection->theSynchronizationNumber = theSynchronizationNumber;
  ListenState::instance()->Synchronize(theConnection, theSynchronizationNumber);
  // Behövs detta? Förstår inte riktigt när detta ska köras, tänker att den alltid
  // anropar submetoden... eller?
}
// Handle an incoming SYN segment
void
TCPState::NetClose(TCPConnection* theConnection)
{
  //EstablishedState::instance()->NetClose(theConnection);
}
// Handle an incoming FIN segment
void
TCPState::AppClose(TCPConnection* theConnection)
{
//  CloseWaitState::instance()->AppClose(theConnection);
}
// Handle close from application
void
TCPState::Kill(TCPConnection* theConnection)
{
  trace << "TCPState::Kill" << endl;
  TCP::instance().deleteConnection(theConnection);
}

void
TCPState::Receive(TCPConnection* theConnection,
                     udword theSynchronizationNumber,
                     byte*  theData,
                     udword theLength)
{
  // EstablishedState::instance()->Receive(theConnection,
  //              theSynchronizationNumber,
  //              theData,
  //              theLength);
}
// Handle incoming data
void
TCPState::Acknowledge(TCPConnection* theConnection,
                         udword theAcknowledgementNumber)
{
  //EstablishedState::instance()->Acknowledge(theConnection, theAcknowledgementNumber);
}
// Handle incoming Acknowledgement
void
TCPState::Send(TCPConnection* theConnection,
                  byte*  theData,
                  udword theLength)
{
  //EstablishedState::instance()->Send(theConnection, theData, theLength);
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
  switch (theConnection->myPort)
  {
   case 7:
     trace << "got SYN on ECHO port" << endl;
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
     break;
   default:
     trace << "send RST..." << endl;
     theConnection->sendNext = 0;
     // Send a segment with the RST flag set.
     theConnection->myTCPSender->sendFlags(0x04);
     TCP::instance().deleteConnection(theConnection);
     break;
  }
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
  switch (theConnection->myPort)
  {
   case 7:
     trace << "got ACK on ECHO port" << endl;
     theConnection->receiveNext = theAcknowledgementNumber;
     // Next reply to be sent.
     theConnection->sentUnAcked = theConnection->sendNext; // prob unnecessary
     // Send a segment with the ACK flag set.
     // Prepare for the next send operation.
     // Change state
     theConnection->myState = EstablishedState::instance(); //SynRecvdState::instance();
     break;
   default:
     trace << "send RST..." << endl;
     theConnection->sendNext = 0;
     // Send a segment with the RST flag set.
     theConnection->myTCPSender->sendFlags(0x04);
     TCP::instance().deleteConnection(theConnection);
     break;
  }

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
  trace << "EstablishedState::NetClose" << endl;

  // Update connection variables and send an ACK

  // Go to NetClose wait state, inform application
  theConnection->myState = CloseWaitState::instance();

  // Normally the application would be notified next and nothing
  // happen until the application calls appClose on the connection.
  // Since we don't have an application we simply call appClose here instead.

  // Simulate application Close...
  theConnection->AppClose();
}

//----------------------------------------------------------------------------
//
void
EstablishedState::Receive(TCPConnection* theConnection,
                          udword theSynchronizationNumber,
                          byte*  theData,
                          udword theLength)
{
  trace << "EstablishedState::Receive" << endl;

  // Delayed ACK is not implemented, simply acknowledge the data
  // by sending an ACK segment, then echo the data using Send.
  theConnection->receiveNext = theSynchronizationNumber + (theLength - 20);
  this->Send(theConnection, theData, theLength);
}


void
EstablishedState::Acknowledge(TCPConnection* theConnection,
                 udword theAcknowledgementNumber)
{

    switch (theConnection->myPort)
    {
     case 7:
       // Next reply to be sent.
       // Send a segment with the ACK flag set.
       // Prepare for the next send operation.
       theConnection->sendNext = theAcknowledgementNumber;
       // Change state
       break;
     default:
       trace << "send RST..." << endl;
       theConnection->sendNext = 0;
       // Send a segment with the RST flag set.
       theConnection->myTCPSender->sendFlags(0x04);
       TCP::instance().deleteConnection(theConnection);
       break;
    }


}
// Handle incoming Acknowledgement

void
EstablishedState::Send(TCPConnection* theConnection,
          byte*  theData,
          udword theLength)
{
  theConnection->myTCPSender->sendData(theData, theLength);
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
  theConnection->myState->Acknowledge(theConnection, theConnection->receiveNext + 1);
//  theConnection->myState = TCPState::instance();
  theConnection->Kill(); // prob wrong?? first get to LastAckState??
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
  //theConnection->Acknowledge(theAcknowledgementNumber);
    switch (theConnection->myPort)
    {
     case 7:
       // Next reply to be sent.
       // Send a segment with the ACK flag set.
       // Prepare for the next send operation.
       theConnection->receiveNext = theAcknowledgementNumber;
       theConnection->myTCPSender->sendFlags(0x10);
       // Change state
       break;
     default:
       trace << "send RST..." << endl;
       theConnection->sendNext = 0;
       // Send a segment with the RST flag set.
       theConnection->myTCPSender->sendFlags(0x04);
       TCP::instance().deleteConnection(theConnection);
       break;
    }
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
  cout << ax_coreleft_total() << " 1 sender" << endl;

  myAnswerChain->deleteAnswerChain();
  cout << ax_coreleft_total() << " 2 sender" << endl;

}

void
TCPSender::sendFlags(byte theFlags)
{
  // Decide on the value of the length totalSegmentLength.
  // Allocate a TCP segment.
  uword totalSegmentLength = 20; //TODO: Default segment length, might change
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
  //delete aTCPHeader;
}

// ---------------------------------------------------------------------------

void
TCPSender::sendData(byte*  theData, udword theLength) {
/*
  cout << "the length: " << theLength << endl;
  byte* pM = (byte*) theData;
      for (int i=0; i<theLength; i++)
        ax_printf(" %2.2X",pM[i]);
      ax_printf("\r\n");
      */
  // Calculate the pseudo header checksum

  TCPPseudoHeader* aPseudoHeader =
    new TCPPseudoHeader(myConnection->hisAddress,
                        theLength);
  uword pseudosum = aPseudoHeader->checksum();

  delete aPseudoHeader;


  // Create the TCP segment.
  // Calculate the final checksum.
  TCPHeader* aTCPHeader = (TCPHeader*) theData;
  aTCPHeader->flags = 0x18;
  aTCPHeader->sourcePort = HILO(myConnection->myPort);
  aTCPHeader->destinationPort = HILO(myConnection->hisPort);
  aTCPHeader->sequenceNumber = LHILO(myConnection->sendNext);
  aTCPHeader->acknowledgementNumber = LHILO(myConnection->receiveNext); // done!!
  aTCPHeader->headerLength = 0x50;
  aTCPHeader->checksum = 0;
  aTCPHeader->urgentPointer = 0;
  aTCPHeader->windowSize = HILO(myConnection->receiveWindow); //???
  aTCPHeader->checksum = calculateChecksum((byte*)aTCPHeader,
                                           theLength,
                                           pseudosum);
  myAnswerChain->answer((byte*)aTCPHeader, //(byte*)aTCPHeader
                        theLength);
  // Deallocate the dynamic memory


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
    if ((aTCPHeader->flags & 0x02) != 0)
    {
      // State LISTEN. Received a SYN flag.
      aConnection->Synchronize(mySequenceNumber);
    }
    else
    {
      // State LISTEN. No SYN flag. Impossible to continue.
      aConnection->Kill();
    }
  }
  else
  {
    // Connection was established. Handle all states.
if ((aTCPHeader->flags & 0x10 ) != 0) {
    // Received a ACK flag

    if(aTCPHeader->flags & 0x01) {//FIN
      //myState = CloseWaitState
      aConnection->NetClose();

    }
      else if ((aTCPHeader->flags & 0x08) != 0) {
        //got data
        aConnection->Receive(mySequenceNumber,
                          myData,
                          myLength);
    } else { // check for ack
      aConnection->Acknowledge(myAcknowledgementNumber);
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
  delete myFrame;
}

uword
TCPInPacket::headerOffset(){ //TODO
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
