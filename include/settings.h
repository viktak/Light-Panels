#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

#include "common.h"

namespace settings
{
    //  Saved values
    extern char wifiSSID[22];
    extern char wifiPassword[32];

    extern char adminPassword[32];

    extern char accessPointPassword[32];

    extern char nodeFriendlyName[32];
    extern u_int heartbeatInterval;

    extern signed char timeZone;

    extern char mqttServer[64];
    extern int mqttPort;
    extern char mqttTopic[32];

    extern bool dst;

    extern long mode;

    //  Calculated values
    extern char localHost[48];

    enum OPERATION_MODES
    {
        LED_CHASER,
        SLOW_PANELS,
        FAST_CHANGING_RANDOM_SEGMENTS,
        ROTATING_PANELS,
        NUMBER_OF_OPERATION_MODES
    };

    extern void setup();

    extern bool LoadSettings();
    extern bool SaveSettings();
    extern void DefaultSettings();
}

#endif