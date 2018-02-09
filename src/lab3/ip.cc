#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}

#include "iostream.hh"
#include "ethernet.hh"
#include "ip.hh"
#include "icmp.hh"

//#define D_ip
#ifdef D_ip
#define trace cout
#else
#define trace if(false) cout
#endif


IP&
IP::instance(){
  static IP ip;
	return ip;
}

IP::IP()
: myIPAddress(new IPAddress(130, 235, 200, 119))
{
}


//-------------------------------------------------------------------------

const
IPAddress&
IP::myAddress(){
  return *myIPAddress;
}


//---------------------------------------------------------------------------

IPInPacket::IPInPacket (byte*      theData,
                        udword    theLength,
                        InPacket* theFrame) :
InPacket(theData, theLength, theFrame)
{
}

//----------------------------------------------------------------------------

void
IPInPacket::decode() {
  // check if is an ICMP ECHO request skip IP totally...
  // uword icmpSeq = *(uword*)(myData + 26);
  // icmpSeq = ((icmpSeq & 0xff00) >> 8) | ((icmpSeq & 0x00ff) << 8);
  // trace << "icmp echo, icmp_seq=" << icmpSeq << endl;
  // create a response...
  uword hoffs = myFrame->headerOffset();
  byte* temp = new byte[myLength + hoffs];
  byte* aReply = temp + hoffs;
  //delete temp;
  memcpy(aReply, myData, myLength);
  IPHeader * myIpHeader = (IPHeader*) aReply;

  if ((myIpHeader->destinationIPAddress) == IP::instance().myAddress()) // Our address, should be HILO?
  {
  //  cout << "ip packet" << endl;
    if (myIpHeader->versionNHeaderLength == 0x45){ //ok
      if ((HILO(myIpHeader->fragmentFlagsNOffset) & 0x3FFF) == 0 ) { // fragment ok
        myProtocol = myIpHeader->protocol;
        if(myProtocol == 0x1) {
          //cout << "icmp" << endl;
          mySourceIPAddress = (myIpHeader->sourceIPAddress);
          uword realPacketLength = HILO(myIpHeader->totalLength);
          byte* icmp = aReply + headerOffset();
          ICMPInPacket* aICMP = new ICMPInPacket(icmp, realPacketLength - headerOffset(), this);
          aICMP->decode();
          delete aICMP; // could be an issue??
        } else {
        //  this->answer(aReply, myLength);
          cout << "not ping" << endl;
        }

      }
    }
  } else {
    cout << "yo shit is wrong" << endl;
  }
  delete aReply;
}

//-----------------------------------------------------------------------------

void
IPInPacket::answer(byte* theData, udword theLength) {

  IPHeader * myIpHeader = (IPHeader*) (theData);//prova hilo
  myIpHeader->identification = HILO(sequenceNumber++);
  //cout << "seq:" << sequenceNumber << endl;
  myIpHeader->versionNHeaderLength = 0x45;
  myIpHeader->TypeOfService = 0x0;
  myIpHeader->fragmentFlagsNOffset = 0x0;
  myIpHeader->timeToLive = 0x40;
  myIpHeader->protocol = myProtocol;
  myIpHeader->headerChecksum = 0;
  myIpHeader->totalLength = HILO(theLength);
  myIpHeader->sourceIPAddress = IP::instance().myAddress();
  myIpHeader->destinationIPAddress = mySourceIPAddress;
  myIpHeader->headerChecksum = calculateChecksum(theData, theLength, 0);

  // Change to reply
  //theData[20] = 0;
  // Adjust ICMP checksum...
  //uword oldSum = *(uword*)(myData + 22);
  //uword newSum = oldSum + 0x8;
  //*(uword*)(aReply + 22) = newSum;

  //theData = (theData - myFrame->headerOffset());
  //theLength += myFrame->headerOffset();

  myFrame->answer((byte*)myIpHeader, theLength);
  delete myFrame;
}

//-----------------------------------------------------------------------------

uword
IPInPacket::headerOffset(){
  return 20;
}
