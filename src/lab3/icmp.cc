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

//#define D_ICMP
#ifdef D_ICMP
#define trace cout
#else
#define trace if(false) cout
#endif


//---------------------------------------------------------------------------

ICMPInPacket::ICMPInPacket (byte*      theData,
                        udword    theLength,
                        InPacket* theFrame) :
InPacket(theData, theLength, theFrame)
{
}

//----------------------------------------------------------------------------

void
ICMPInPacket::decode() {
  ICMPHeader* icmp = (ICMPHeader*) myData;
  // Här använder man ICMPHeader, ICMPECHOHeader är datan i icmp-paketet, alltså det
  // som kommer precis efter checksum. Vi ska inte ändra något i ICMPECHOHeader
  // så jag försökte fråga varför den ens finns med i hh-filen men han verkade inte
  // fatta....
  //cout << "icmp decode" << endl;
  if(icmp->type == 0x8){ //Echo request
    icmp->type = 0x0; //set echo reply
    icmp->checksum += 0x8; // calculateChecksum(myData, myLength, icmp->checksum);
    this->answer((byte *) icmp, myLength);
  }
}

//-----------------------------------------------------------------------------

void
ICMPInPacket::answer(byte* theData, udword theLength) {
  //cout << "header offset" << myFrame->headerOffset() << endl;
  theData -= myFrame->headerOffset();
  theLength += myFrame->headerOffset();
  myFrame->answer((theData) , theLength);
  delete myFrame;
}

//-----------------------------------------------------------------------------

uword
ICMPInPacket::headerOffset(){
  return 8;
}
