#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <os_type.h>

#define __debugSettings
#define SerialMon Serial1

static const int32_t DEBUG_SPEED = 115200;
static const String HARDWARE_ID = "light-panel-controller-tri";
static const String HARDWARE_VERSION = "1.0";
static const String FIRMWARE_ID = "lp-triangle";

extern String GetDeviceMAC();
extern void DateTimeToString(char *dest, time_t localTime);
extern String TimeIntervalToString(const time_t time);
extern String GetDeviceMAC();

extern void setupCommon();

#endif