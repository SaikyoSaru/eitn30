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
  //uword icmpSeq = *(uword*)(myData + 22);//prob wrong
  //icmpSeq = ((icmpSeq & 0xff00) >> 8) | ((icmpSeq & 0x00ff) << 8);
  uword hoffs = myFrame->headerOffset();
  byte* temp = new byte[myLength + hoffs];
  byte* aReply = temp + hoffs;
  memcpy(aReply, myData, myLength);
  // by reusing his ip packet (including id nr) we get the same checksum :)
    // Just reverse IP addresses.
 ARPHeader* arpHeader = (ARPHeader*) (aReply);
/*
 byte* pM = (byte*) arpHeader;
 for (int i=0;i<28;i++)
   ax_printf("%2.2X ",pM[i]);
 ax_printf("\r\n");
*/// cout << arpHeader->targetIPAddress << endl;
 if(arpHeader->targetIPAddress == IP::instance().myAddress()){
   //cout << "arp packet" << endl;
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
