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
  //cast data to ip header
  IPHeader* myIpHeader = (IPHeader*) myData;
  // check if its for us
  if ((myIpHeader->destinationIPAddress) == IP::instance().myAddress())
  {

    if (myIpHeader->versionNHeaderLength == 0x45){ // version and headerelength ok
      if ((HILO(myIpHeader->fragmentFlagsNOffset) & 0x3FFF) == 0 ) { // fragment ok
        myProtocol = myIpHeader->protocol;
        if(myProtocol == 0x1) {
          //cout << "icmp" << endl;
          mySourceIPAddress = (myIpHeader->sourceIPAddress);
          uword realPacketLength = HILO(myIpHeader->totalLength);
          byte* icmp = myData + headerOffset();
          ICMPInPacket* aICMP = new ICMPInPacket(icmp, realPacketLength - headerOffset(), this);
          aICMP->decode();
          delete aICMP; // get rid of memory leak
        } else {
          cout << "not ping" << endl;
        }

      }
    }
  } else {
    cout << "yo shit is wrong" << endl;
  }

}

//-----------------------------------------------------------------------------

void
IPInPacket::answer(byte* theData, udword theLength) {
  // cast to ip header again, and set the values
  IPHeader * myIpHeader = (IPHeader*) (theData);
  myIpHeader->identification = HILO(sequenceNumber++);
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
  // call answer in llc
  myFrame->answer((byte*)myIpHeader, theLength);
  delete myFrame;
}

//-----------------------------------------------------------------------------

uword
IPInPacket::headerOffset(){
  return 20;
}
