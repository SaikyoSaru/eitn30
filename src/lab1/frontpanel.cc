/*!***************************************************************************
*!
*! FILE NAME  : FrontPanel.cc
*!
*! DESCRIPTION: Handles the LED:s
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"

#include "iostream.hh"
#include "frontpanel.hh"

//#define D_FP
#ifdef D_FP
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** FrontPanel DEFINITION SECTION ***************************/

byte LED::writeOutRegisterShadow(0x38);


//----------------------------------------------------------------------------
//

LED::LED(byte theLedNumber)
	//writeOutRegisterShadow(0x38);
 {
	myLedBit = theLedNumber;
	iAmOn = false; //check the old value first?
}

void LED::on() {
	 byte led = 4 << myLedBit;  /* convert LED number to bit weight */
         *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow &= ~led ;
	iAmOn = true;
}

void LED::off() {
	byte led = 4 << myLedBit;  /* convert LED number to bit weight */
         *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow |= led ;
        iAmOn = false;
}

void LED::toggle() {
	if(iAmOn){
                off();
        } else {
                on();
        }

}

//-----------------------------------------------------------------------------

NetworkLEDTimer::NetworkLEDTimer(Duration blinkTime){
	myBlinkTime = blinkTime;
}

void NetworkLEDTimer::start(){
	this->timeOutAfter(myBlinkTime);
}

void NetworkLEDTimer::timeOut() {
	FrontPanel::instance().notifyLedEvent(FrontPanel::networkLedId);
	cout << "NetworkLED time out" << endl;
}

//-------------------------------------------------------------------------
//

CDLEDTimer::CDLEDTimer(Duration blinkPeriod) {
	this->timerInterval(blinkPeriod);
	startPeriodicTimer();
}

void CDLEDTimer::timerNotify() {
	FrontPanel::instance().notifyLedEvent(FrontPanel::cdLedId);
	//this->timerNotify();
	cout << "CDLEDTimer time out" << endl;
}

//----------------------------------------------------------------------------

StatusLEDTimer::StatusLEDTimer(Duration blinkPeriod) {
	this->timerInterval(blinkPeriod);
	startPeriodicTimer();
}

void StatusLEDTimer::timerNotify() {
	FrontPanel::instance().notifyLedEvent(FrontPanel::statusLedId);
	cout << "status time out" << endl;
}

//-------------------------------------------------------------------------

FrontPanel::FrontPanel() : 
	Job(),	
	mySemaphore(Semaphore::createQueueSemaphore("semaphore",0)),
	myNetworkLEDTimer(new NetworkLEDTimer(Clock::seconds*5)),
	myCDLEDTimer(new CDLEDTimer(Clock::seconds*2)),
	myStatusLEDTimer(new StatusLEDTimer(Clock::seconds*4)),	
	myNetworkLED(1),
	netLedEvent(false), 
	myCDLED(3), 
	cdLedEvent(false),
	myStatusLED(2),
	statusLedEvent(false)
{

cout << "FrontPanel created." << endl; 
 Job::schedule(this); 

}

void FrontPanel::doit() {
	
	while(1){
		mySemaphore->wait();
		if (netLedEvent) {
			myNetworkLED.toggle();
			netLedEvent = false;		
		}
		if (cdLedEvent)	{
			myCDLED.toggle();
			cdLedEvent = false;		
		}
		if (statusLedEvent) {
			myStatusLED.toggle();
			statusLedEvent = false;
		}
	}
}
//Hittar inte riktigt hur den skall skrivas^^
FrontPanel& FrontPanel::instance() {
	static FrontPanel INSTANCE;
	return INSTANCE;
}

void FrontPanel::packetReceived() {
	netLedEvent = true;
	myNetworkLEDTimer->start();
	//myNetworkLEDTimer::NetworkLEDTimer(Clock::seconds*5);
	//mySemaphore->wait();
}

void FrontPanel::notifyLedEvent(uword theLedId) {
	cout << theLedId << endl;
	switch(theLedId){
		case networkLedId: netLedEvent = true;
		case cdLedId: cdLedEvent = true;
		case statusLedId: statusLedEvent = true;
	}
	mySemaphore->signal();
}

//----------------------------------------------------------------------------
//

/****************** END OF FILE FrontPanel.cc ********************************/

