/**
 * File:   main.cpp
 * Author: btacklind
 *
 * Created on Nov 28, 2014, 2:02 PM
 */

#include "Board.h"
#include <avr/wdt.h>
#include <util/delay.h>

#include "MLX90363.h"
#include "ThreePhaseController.h"
#include "Calibration.h"
#include "Clock.h"
#include "Debug.h"
#include "Demo.h"
#include "ServoController.h"
#include "MLXDebug.h"
#include "SerialInterface.h"

using namespace AVR;
using namespace ThreePhaseControllerNamespace;

/**
 * All the init functions should go in here.
 * gcc will automatically call it for us because of the constructor attribute.
 */
void init() __attribute__((constructor));

// Save reset cause
u1 resetCause;

void init() {
  // Watch Dog Timer
  wdt_reset();
  wdt_disable();

  Debug::init();
  Debug::dout << PSTR("Beginning Inits \r\n");

  // Set up the driver pins
  ThreePhaseDriver::init();

  // Save and Clear the MCU Status Register. Indicates previous reset's source.
  resetCause = MCUSR;
  MCUSR = 0;

  // Set Enable Interrupts.
  sei();

  // Use the Clock that is outside the AVR++ namespace.
  ::Clock::init();
}

/**
 *
 */
int main() {
  // These don't do anything if they're not enabled
  Calibration::main();
  Demo::main();

  SerialInterface::init();
  ThreePhaseController::init();

  while (1) {
    if (auto msg = SerialInterface::getMessage()) {
      ThreePhaseController::setAmplitude(msg->getCommand());
    }
  }

  // loop in case main loop is disabled
  // allows for interrupts to continue
  while (1);
}
