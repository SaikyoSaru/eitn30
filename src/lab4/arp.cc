#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}
#include "ip.hh"
#include "iostream.hh"
#include "ethernet.hh"
#include "arp.hh"

//#define D_LLC
#ifdef D_ARP
#define trace cout
#else
#define trace if(false) cout
#endif

//---------------------------------------------------------------------------

ARPInPacket::ARPInPacket(byte*      theData,
                        udword     theLength,
                        InPacket*  theFrame) :
InPacket(theData, theLength, theFrame)
{
}

//----------------------------------------------------------------------------

void
ARPInPacket::decode()
{
  // by reusing his ip packet (including id nr) we get the same checksum :)
    // Just reverse IP addresses.
 ARPHeader* arpHeader = (ARPHeader*) (myData);
/*
 byte* pM = (byte*) arpHeader;
 for (int i=0;i<28;i++)
   ax_printf("%2.2X ",pM[i]);
 ax_printf("\r\n");
*/
// check for our ethernet address
 if(arpHeader->targetIPAddress == IP::instance().myAddress()){
   arpHeader->targetEthAddress = arpHeader->senderEthAddress;
   arpHeader->targetIPAddress = arpHeader->senderIPAddress;
   arpHeader->senderEthAddress = Ethernet::instance().myAddress();
   arpHeader->senderIPAddress = IP::instance().myAddress();
   arpHeader->op = HILO(2);
   this->answer((byte*)arpHeader, myLength);
  }
}

//--------------------------------------------------------------------------

void
ARPInPacket::answer(byte* theData, udword theLength){
  //cout << "answer" << endl;
  myFrame->answer(theData, theLength);
  delete myFrame;
}

//-----------------------------------------------------------------------------

uword
ARPInPacket::headerOffset(){
  return 28;
}
