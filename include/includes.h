#ifndef INCLUDES_H
#define INCLUDES_H

#include "version.h"

#include "../../../ActoSenso/Nodes/_common/debug.h"

#include "defines.h"
#include "../../../ActoSenso/Nodes/_common/defines.h"

#include "enums.h"
#include "../../../ActoSenso/Nodes/_common/enums.h"

#include "../../../ActoSenso/Nodes/_common/variables.cpp"

#include <cstdlib>
#include <string>

#include <ESP8266WiFi.h>
// #include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

#include <LittleFS.h>

#include <PubSubClient.h>
#include <EEPROM.h>

#include <ArduinoJson.h>
#include <TimeLib.h>
#include <Time.h>
#include <TimeChangeRules.h>
#include <NTP.h>
#include "Button2.h"

#include "structs.h"

#include <user_interface.h>

#endif