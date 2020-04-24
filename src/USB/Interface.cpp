/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   USBInterface.cpp
 * Author: Cameron
 *
 * Created on April 18, 2018, 12:10 PM
 */

#include "PacketFormats.h"
#include "ServoController.hpp"
#include "ThreePhase/Controller.hpp"

#include <AVR++/WDT.hpp>

#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

using namespace ThreePhaseControllerNamespace;

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void) {
  //	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void) {
  //	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
  bool ConfigSuccess = true;

  ConfigSuccess &= HID_Device_ConfigureEndpoints(&Generic_HID_Interface);

  USB_Device_EnableSOFEvents();

  //	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) { HID_Device_ProcessControlRequest(&Generic_HID_Interface); }

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void) { HID_Device_MillisecondElapsed(&Generic_HID_Interface); }

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the
 * generated report ID \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or
 * HID_REPORT_ITEM_Feature \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be
 * sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
                                         const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  USBDataINShape *const data = (USBDataINShape *)ReportData;

  *ReportSize = REPORT_SIZE;

  data->state = state;
  switch (data->state) {
  case State::Fault:
    data->fault.fault = fault;

    if (fault == Fault::Init) {
      data->fault.init.cyclesPerRevolution = ThreePhaseDriver::CyclesPerRevolution;

      data->fault.init.deadTimes = DT4;

      // TODO:
      data->fault.init.currentLimit = 0;

      data->fault.init.validCalibration = Lookup::isValid;
      if (Lookup::isValid) {
        // TODO:
        // data->fault.init.calibration.time = ;
        // data->fault.init.calibration.version = ;
      }
    }
    break;

  case State::Manual:
    data->manual.drivePosition = ThreePhaseControllerNamespace::getManualPosition();
    data->manual.realPosition = Lookup::isValid ? ThreePhasePositionEstimator::getMagnetometerPhaseEstimate() : true;
    data->manual.velocity = ThreePhaseControllerNamespace::getSynchronous();
    data->manual.amplitude = ThreePhaseDriver::getAmplitude();

    data->manual.mlxResponseState = MLX90363::getResponseState();
    memcpy(data->manual.mlxResponse, MLX90363::getRxBuffer(), MLX90363::messageLength);

    break;
  case State::Normal:
    data->normal.position = ServoController::getPosition();
    data->normal.velocity = ThreePhasePositionEstimator::getMagnetometerVelocityEstimate();

    data->normal.amplitude = ThreePhaseController::getAmplitudeTarget();

    data->normal.mlxFailedCRCs = MLX90363::getCRCFailures();
    data->normal.controlLoops = ThreePhaseController::getLoopCount();
    break;
  }

  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    data->cpuTemp = ADCValues::temperature.getUnsafe();
    data->current = ADCValues::current.getUnsafe();
    data->VBatt = ADCValues::battery.getUnsafe();
    data->VDD = ADCValues::drive.getUnsafe();
    data->AS = ADCValues::AS.getUnsafe();
    data->BS = ADCValues::BS.getUnsafe();
    data->CS = ADCValues::CS.getUnsafe();
  }

  return true;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or
 * HID_REPORT_ITEM_Feature \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
                                          const uint8_t ReportType, const void *ReportData, const uint16_t ReportSize) {
  USBDataOUTShape const *const data = (USBDataOUTShape *)ReportData;

  switch (data->mode) {
  default:
    return;

  case CommandMode::ClearFault:
    WDT::stop();
    clearFault();
    break;

  case CommandMode::MLXDebug:
    if (state == State::Fault && fault != Fault::Init)
      return;

    if (setState(State::Manual))
      WDT::stop();

    memcpy(MLX90363::getTxBuffer(), data->mlx.mlxData, MLX90363::messageLength);
    if (data->mlx.crc)
      MLX90363::fillTxBufferCRC();

    MLX90363::startTransmittingUnsafe();
    break;

  case CommandMode::ThreePhase:
    if (state == State::Fault && fault != Fault::Init)
      return;

    if (setState(State::Manual))
      WDT::start(WDT::T1000ms);

    // TODO: Implement body of this "method"
    break;

  case CommandMode::Calibration:
    if (state == State::Fault && fault != Fault::Init)
      return;

    if (setState(State::Manual))
      WDT::start(WDT::T1000ms);

    ThreePhaseDriver::setAmplitude(data->calibrate.amplitude);
    ThreePhaseDriver::advanceTo(data->calibrate.angle);
    break;

  case CommandMode::Push:
    if (state == State::Fault && fault != Fault::Init)
      return;

    if (setState(State::Normal))
      WDT::start(WDT::T_120ms);

    ServoController::setEnable(false);
    ThreePhaseController::setAmplitudeTarget(data->push.command);
    break;

  case CommandMode::Servo:
    if (state == State::Fault && fault != Fault::Init)
      return;

    if (setState(State::Normal))
      WDT::start(WDT::T_250ms);

    switch (data->servo.mode) {
    default:
      return;

    case ServoController::Mode::Disabled:
      ServoController::setEnable(false);
      break;

    case ServoController::Mode::Amplitude: {
      const auto &parameters = data->servo.amplitude;
      ServoController::setAmplitude(ThreePhaseController::Amplitude(parameters.forward, parameters.amplitude));
    } break;

    case ServoController::Mode::Velocity:
      // TODO: Implement
      ServoController::setVelocity(0);
      break;

    case ServoController::Mode::Position: {
      const auto &parameters = data->servo.position;
      ServoController::setPosition(ServoController::MultiTurn(parameters.turns, parameters.commutation));
      ServoController::setPosition_P(parameters.kP);
      ServoController::setPosition_I(parameters.kI);
      ServoController::setPosition_D(parameters.kD);
    } break;
    }

    break;

  case CommandMode::SynchronousDrive:
    if (setState(State::Manual))
      WDT::start(WDT::T_500ms);

    ThreePhaseControllerNamespace::setSynchronous(data->synchronous.velocity);
    ThreePhaseDriver::setAmplitude(data->synchronous.amplitude);
    break;

  case CommandMode::Bootloader:
    ThreePhaseDriver::emergencyDisable();
    WDT::stop();
    ServoController::setEnable(false);
    bootloaderJump();
    break;
  }

  WDT::tick();
}
