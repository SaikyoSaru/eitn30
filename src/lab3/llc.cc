/*!***************************************************************************
*!
*! FILE NAME  : llc.cc
*!
*! DESCRIPTION: LLC dummy
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
}

#include "iostream.hh"
#include "ethernet.hh"
#include "llc.hh"
#include "ip.hh"
#include "arp.hh"

//#define D_LLC
#ifdef D_LLC
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** LLC DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//
LLCInPacket::LLCInPacket(byte*           theData,
                         udword          theLength,
						             InPacket*       theFrame,
                         EthernetAddress theDestinationAddress,
                         EthernetAddress theSourceAddress,
                         uword           theTypeLen):
InPacket(theData, theLength, theFrame),
myDestinationAddress(theDestinationAddress),
mySourceAddress(theSourceAddress),
myTypeLen(theTypeLen)
{
}

//----------------------------------------------------------------------------
//
void
LLCInPacket::decode()
{
  if ((myTypeLen == 0x800) &&
	(myLength > 28) &&
	(*(myData + 20) == 8)) //ip packet
  {

    IPInPacket* ipPacket = new IPInPacket(myData, myLength, myFrame);
    ipPacket->decode();
    delete ipPacket;

	} else if (myTypeLen == 0x806) { // arp packet, Maybe add mylength == 28 here
    //cout << "arp packet" << endl;
    ARPInPacket* arpPacket = new ARPInPacket(myData, myLength, myFrame);
    arpPacket->decode();
    delete arpPacket;
  }
  //cout << "stuff" << endl;

}

//----------------------------------------------------------------------------
//
void
LLCInPacket::answer(byte *theData, udword theLength)
{
  myFrame->answer(theData, theLength);
  delete myFrame;
}

uword
LLCInPacket::headerOffset()
{
  return myFrame->headerOffset() + 0;
}

/****************** END OF FILE Ethernet.cc *************************************/
