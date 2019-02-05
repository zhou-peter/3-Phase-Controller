/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   USBPacketFormats.h
 * Author: Cameron
 *
 * Created on May 14, 2018, 2:18 PM
 */

#ifndef USBPACKETFORMATS_H
#define USBPACKETFORMATS_H

#include "Descriptors.h"
#include "main.hpp"
#include <stdint.h>

namespace ThreePhaseControllerNamespace {

enum class CommandMode : u1 { Calibration, Push, Servo };

/**
 * Shape of data going OUT of host.
 */
typedef struct {
  CommandMode mode;
  union {
    struct {
      u2 angle;
      u1 amplitude;
    } calibrate;
    struct {
      s2 command;
    } push;
    struct {
      u1 mode;
      s2 command;
    } servo;
  } command;

} USBDataOUTShape;

/**
 * Shape of data going IN to host
 */
typedef struct {
  State state;
  Fault fault;
  u4 position;
  s2 velocity;
  u2 cpuTemp;
  s2 current;
  u2 rawAngle;
} USBDataINShape;

static_assert(sizeof(USBDataOUTShape) <= REPORT_SIZE, "Data going OUT of HOST must be shorter than REPORT_SIZE");
static_assert(sizeof(USBDataINShape) <= REPORT_SIZE, "Data going IN to HOST must be shorter than REPORT_SIZE");

} // namespace ThreePhaseControllerNamespace

#endif /* USBPACKETFORMATS_H */
