#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#include <NTPClient.h>

#include "ntp.h"
#include "common.h"

#define NTP_UPDATE_INTERVAL_MS (unsigned long)3600000 //  synchronize time with NTP server once an hour

WiFiUDP ntpUDP;

#if __localNTP == 1
char timeServer[] = "192.168.123.2";
#else
char timeServer[] = "europe.pool.ntp.org";
#endif

NTPClient timeClient(ntpUDP, timeServer, 0, NTP_UPDATE_INTERVAL_MS);

void setupNTP()
{
    timeClient.begin();
}

void loopNTP()
{
    timeClient.update();
    if (timeClient.isTimeSet())
    {
        setTime(timeClient.getEpochTime());
    }
}
