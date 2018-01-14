 /**
 * File:   main.cpp
 * Author: btacklind
 *
 * Created on Nov 28, 2014, 2:02 PM
 */

#include <util/delay.h>
#include <avr/wdt.h>
#include "Board.h"

#include "ThreePhaseController.h"
#include "MLX90363.h"
// #include "Debug.h"
#include "Clock.h"
#include "Interpreter.h"
#include "ServoController.h"
#include "Demo.h"
#include "Calibration.h"
#include "Debug.h"
#include "HallWatcher.h"
#include "commutation.h"

using namespace AVR;
using namespace ThreePhaseControllerNamespace;

#include "mainHelper.inc"

// timer overflow interrupt for generating debug trigger
void TIMER4_OVF_vect() {
  Board::LED1::on();
  Board::LED1::off();
}

/**
 * All the init functions should go in here.
 * gcc will automatically call it for us because of the constructor attribute.
 */
void init() __attribute__((constructor));

u1 resetCause = MCUSR;

void init() {
  // Watch Dog Timer
  wdt_reset();
  wdt_disable();

  Debug::init();

  // Set up the driver pins
  ThreePhaseDriver::init();

  // Clear the MCU Status Register.  Indicates previous reset's source.
  MCUSR = 0;

  // Set up the hall sensor interrupts
  HallWatcher::init();

  // Temporary enable timer overflow interrupt for generating debug trigger
  TIMSK4 = 1 << TOIE4;

  // Set Enable Interrupts.
  sei();

  // Use the Clock that is outside the AVR++ namespace.
  ::Clock::init();

  Board::LED0::off(); // lED0 is used in debug routines.

  Debug::dout << 12345 << PSTR("Hello World!!\r\n");
  // End of init
}

Clock::MicroTime nextTime;



void printHallStateAndNumberIfHallChanged(u2 number) {
  u1 static lastState = HallWatcher::getState();

  const u1 state = HallWatcher::getState();

  if (state == lastState) return;

  lastState = state;
  Debug::dout << PSTR("New Hall State: ") << state << ',' << number << '\r' << '\n';
}

/**
 *
 */
int main() {

  // displayHallState();
  Debug::dout << PSTR("Main loop\r\n");

  // holdForButton();
  //doSquarePulse();

  // signed int command = 10;


  Clock::readTime(nextTime);
  ThreePhaseDriver::setDeadTimes(0xff);
  // ThreePhaseDriver::setAmplitude(255); // out of 0 to 255
  setPWM(-255);  // out of +/- 255


  u2 hallTest = ThreePhaseDriver::StepsPerCycle / 4;
  u2 constexpr fixedAngle = 704;
  hallTest = fixedAngle;

  while (1) {
    nextTime += 500_us;

    while (!nextTime.isInPast()) {
      // Do things here while we're waiting for the 1kHz tick

      // Cameron added this for clarity
      printHallStateIfChanged();
      // printHallStateAndNumberIfHallChanged(hallTest);
      // Debug::dout << PSTR("Testing hall state: ") << hallTest << PSTR("\r\n");
    }

    // setOverrideHallState(hallTest);
    Debug::dout << hallTest << PSTR("\r\n");
    // ThreePhaseDriver::advanceTo(hallTest);
    //updateCommutation();
    holdForButton();

    // hallTest = ((hallTest != 704) ? 704 : 576);

    // _delay_ms(4);


    hallTest += 128;
    while (hallTest >= 768) hallTest -= 768;

    // hallTest += 3;
    // while (hallTest >= 768) hallTest -= 768;
    //hallTest -= 3;
    //while (hallTest <= 0) hallTest += 768;

    // Do things here at 1Khz

  } // end main loop

  //loop in case main loop is disabled
  //allows for interrupts to continue
  while (1);
}
