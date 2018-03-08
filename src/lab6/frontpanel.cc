/*!***************************************************************************
 *!
 *! FILE NAME  : FrontPanel.cc
 *!
 *! DESCRIPTION: Handles the LED:s
 *!
 *!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
//#include "sp_alloc.h" //wip

#include "frontpanel.hh"
#include "iostream.hh"

//#define D_FP
#ifdef D_FP
#define trace cout
#else
#define trace                                                                  \
  if (false)                                                                   \
  cout
#endif

/****************** FrontPanel DEFINITION SECTION ***************************/

/****************** LED ******************/

// needs to be accessed everywhere
byte LED::writeOutRegisterShadow = 0x38;

LED::LED(byte theLedNumber) : iAmOn(false), myLedBit(4 << theLedNumber) {}

void LED::on() {
  *(VOLATILE byte *)0x80000000 = writeOutRegisterShadow &= ~myLedBit;
  iAmOn = true;
}

void LED::off() {
  *(VOLATILE byte *)0x80000000 = writeOutRegisterShadow |= myLedBit;
  iAmOn = false;
}

void LED::toggle() {
  if (iAmOn)
    off();
  else
    on();
}

/****************** NetworkLEDTimer ******************/
NetworkLEDTimer::NetworkLEDTimer(Duration blinkTime) : myBlinkTime(blinkTime) {}

void NetworkLEDTimer::start() { this->timeOutAfter(myBlinkTime); }
void NetworkLEDTimer::timeOut() {
  FrontPanel::instance().notifyLedEvent(FrontPanel::networkLedId);
}

/****************** StatusLEDTimer ******************/
StatusLEDTimer::StatusLEDTimer(Duration blinkPeriod) {
  this->timerInterval(blinkPeriod);
  this->startPeriodicTimer();
}

void StatusLEDTimer::timerNotify() {
  FrontPanel::instance().notifyLedEvent(FrontPanel::statusLedId);
}

/****************** CDLEDTimer ******************/
CDLEDTimer::CDLEDTimer(Duration blinkPeriod) {
  this->timerInterval(blinkPeriod);
  this->startPeriodicTimer();
}

void CDLEDTimer::timerNotify() {
  FrontPanel::instance().notifyLedEvent(FrontPanel::cdLedId);
  cout << "Core" << ax_coreleft_total() << endl;
}

/****************** FrontPanel ******************/
FrontPanel &FrontPanel::instance() {
  static FrontPanel fp;
  return fp;
}

void FrontPanel::packetReceived() {
  myNetworkLED.on();
  myNetworkLEDTimer->start();
  trace << "packetReceived called (FrontPanel)" << endl;
}

void FrontPanel::notifyLedEvent(uword theLedId) {
  switch (theLedId) {
  case networkLedId:
    netLedEvent = true;
    break;
  case statusLedId:
    statusLedEvent = true;
    break;
  case cdLedId:
    cdLedEvent = true;
    break;
  }
  mySemaphore->signal();
}

FrontPanel::FrontPanel()
    : Job(), mySemaphore(Semaphore::createQueueSemaphore("FP", 0)),
      myNetworkLED(networkLedId), netLedEvent(false), myCDLED(cdLedId),
      cdLedEvent(false), myStatusLED(statusLedId), statusLedEvent(false) {
  Job::schedule(this);
}

void FrontPanel::doit() {
  myNetworkLEDTimer = new NetworkLEDTimer(1);
  myCDLEDTimer = new CDLEDTimer(Clock::seconds * 2);
  myStatusLEDTimer = new StatusLEDTimer(Clock::seconds * 3);

  while (1) {
    mySemaphore->wait();
    if (netLedEvent) {
      myNetworkLED.toggle();
      netLedEvent = false;
    }
    if (statusLedEvent) {
      myStatusLED.toggle();
      statusLedEvent = false;
    }
    if (cdLedEvent) {
      myCDLED.toggle();
      cdLedEvent = false;
    }
  }
}

/****************** END OF FILE FrontPanel.cc ********************************/
