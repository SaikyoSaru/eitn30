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
#include "tcp.hh"

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
IPInPacket::decode()
{
  //cast data to ip header
  IPHeader* myIpHeader = (IPHeader*) myData;
  // check if its for us
  if ((myIpHeader->destinationIPAddress) == IP::instance().myAddress())
  {
    if (myIpHeader->versionNHeaderLength == 0x45){
      // version and headerelength ok
      if ((HILO(myIpHeader->fragmentFlagsNOffset) & 0x3FFF) == 0 ) {
         // fragment ok
        myProtocol = myIpHeader->protocol;
        uword realPacketLength = HILO(myIpHeader->totalLength);
        mySourceIPAddress = (myIpHeader->sourceIPAddress);

        if(myProtocol == 0x1) {
          byte* icmp = myData + headerOffset();
          ICMPInPacket* aICMP = new ICMPInPacket(icmp, realPacketLength - headerOffset(), this);
          aICMP->decode();
          delete aICMP; // get rid of memory leak
        } else if(myProtocol == 0x06){

          byte *tcpP = myData + headerOffset();
          TCPInPacket* tcp = new TCPInPacket((tcpP),
                                realPacketLength - headerOffset(),
                                this,
                                mySourceIPAddress);
          tcp->decode();
          delete tcp;
        } else {
          cout << "not tcp and not icmp" << endl;
        }

      }
    }
  }

}

//-----------------------------------------------------------------------------

void
IPInPacket::answer(byte* theData, udword theLength)
{

  // cast to ip header again, and set the values
  byte * iphead = new byte[theLength + headerOffset()];
  memcpy(iphead + headerOffset(), theData, theLength);
  IPHeader * myIpHeader = (IPHeader*) (iphead);
  myIpHeader->identification = HILO(sequenceNumber++);
  myIpHeader->versionNHeaderLength = 0x45;
  myIpHeader->TypeOfService = 0x0;
  myIpHeader->fragmentFlagsNOffset = 0x0;
  myIpHeader->timeToLive = 0x40;
  myIpHeader->protocol = myProtocol;
  myIpHeader->headerChecksum = 0;
  myIpHeader->totalLength = HILO(theLength + headerOffset());
  myIpHeader->sourceIPAddress = IP::instance().myAddress();
  myIpHeader->destinationIPAddress = mySourceIPAddress;
  myIpHeader->headerChecksum = calculateChecksum(iphead, this->headerOffset(), 0);
  // call answer in llc


  myFrame->answer((byte*)myIpHeader, theLength + headerOffset());
  delete iphead;
  delete theData;
}

//-----------------------------------------------------------------------------

uword
IPInPacket::headerOffset(){
  return 20;
}
InPacket*
IPInPacket::copyAnswerChain()
{
  IPInPacket* anAnswerPacket = new IPInPacket(*this);
  anAnswerPacket->setNewFrame(myFrame->copyAnswerChain());
  return anAnswerPacket;
}
