#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>

#include "ArduinoJson.h"

#include "common.h"
#include "settings.h"
#include "logger.h"

#define __debugSettings

#define DEFAULT_ADMIN_PASSWORD "admin"
#define DEFAULT_NODE_FRIENDLY_NAME "Light panel"
#define DEFAULT_AP_PASSWORD "esp12345678"

#define DEFAULT_TIMEZONE 2

#define DEFAULT_MQTT_SERVER "test.mosquitto.org"
#define DEFAULT_MQTT_PORT 1883
#define DEFAULT_MQTT_TOPIC "light-panel"

#define DEFAULT_HEARTBEAT_INTERVAL 300 //  seconds

namespace settings
{
    //  Saved values
    char wifiSSID[22];
    char wifiPassword[32];

    char adminPassword[32];

    char nodeFriendlyName[32];
    u_int heartbeatInterval;

    signed char timeZone;

    char mqttServer[64];
    int mqttPort;
    char mqttTopic[32];

    bool dst;

    long mode;

    //  Calculated values
    char accessPointPassword[32];
    char localHost[24];

    static void PrintSettings()
    {
        Serial.println("==========================App settings==========================");
        Serial.printf("App name\t\t%s\r\nAdmin password\t\t%s\r\nSSID\t\t\t%s\r\nPassword\t\t%s\r\nAP Password\t\t%s\r\nTimezone\t\t%i\r\nMQTT Server\t\t%s\r\nMQTT Port\t\t%u\r\nMQTT TOPIC\t\t%s\r\nHearbeat interval\t%u\r\nMode:\t\t\t%li\r\n",
                      nodeFriendlyName, adminPassword, wifiSSID, wifiPassword, accessPointPassword, timeZone, mqttServer, mqttPort, mqttTopic, heartbeatInterval, mode);
        Serial.println("====================================================================");
    }

    bool LoadSettings()
    {
        File configFile = LittleFS.open("/config.json", "r");
        if (!configFile)
        {
            Serial.println("Failed to open config file");
            return false;
        }

        size_t size = configFile.size();
        if (size > 1024)
        {
            Serial.println("Config file size is too large");
            logger::LogEvent(logger::EVENTCATEGORIES::System, 2, "FS failure", "Config file size is too large.");
            return false;
        }

        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        // We don't use String here because ArduinoJson library requires the input
        // buffer to be mutable. If you don't use ArduinoJson, you may as well
        // use configFile.readString instead.
        configFile.readBytes(buf.get(), size);
        configFile.close();

        StaticJsonDocument<192> doc;
        DeserializationError error = deserializeJson(doc, buf.get());

        if (error)
        {
            Serial.println("Failed to parse config file");
            logger::LogEvent(logger::EVENTCATEGORIES::System, 3, "FS failure", "Failed to parse config file.");
            Serial.println(error.c_str());
            return false;
        }

#ifdef __debugSettings
        serializeJsonPretty(doc, Serial);
        Serial.println();
#endif

        sprintf(localHost, "%s-%s", DEFAULT_MQTT_TOPIC, common::GetDeviceMAC().substring(6).c_str());

        if (doc["ssid"])
        {
            strcpy(wifiSSID, doc["ssid"]);
        }
        else
        {
            strcpy(wifiSSID, localHost);
        }

        if (doc["password"])
        {
            strcpy(wifiPassword, doc["password"]);
        }
        else
        {
            strcpy(wifiPassword, DEFAULT_ADMIN_PASSWORD);
        }

        if (doc["accessPointPassword"])
        {
            strcpy(accessPointPassword, doc["accessPointPassword"]);
        }
        else
        {
            strcpy(accessPointPassword, DEFAULT_AP_PASSWORD);
        }

        if (doc["mqttServer"])
        {
            strcpy(mqttServer, doc["mqttServer"]);
        }
        else
        {
            strcpy(mqttServer, DEFAULT_MQTT_SERVER);
        }

        if (doc["mqttPort"])
        {
            mqttPort = doc["mqttPort"];
        }
        else
        {
            mqttPort = DEFAULT_MQTT_PORT;
        }

        if (doc["mqttTopic"])
        {
            strcpy(mqttTopic, doc["mqttTopic"]);
        }
        else
        {
            sprintf(mqttTopic, "%s-%u", localHost, ESP.getChipId());
        }

        if (doc["friendlyName"])
        {
            strcpy(nodeFriendlyName, doc["friendlyName"]);
        }
        else
        {
            strcpy(nodeFriendlyName, DEFAULT_NODE_FRIENDLY_NAME);
        }

        if (doc["timezone"])
        {
            timeZone = doc["timezone"];
        }
        else
        {
            timeZone = 0;
        }

        if (doc["heartbeatInterval"])
        {
            heartbeatInterval = doc["heartbeatInterval"];
        }
        else
        {
            heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
        }

        if (doc["mode"])
        {
            mode = doc["mode"];
        }
        else
        {
            mode = OPERATION_MODES::LED_CHASER;
        }

        return true;
    }

    bool SaveSettings()
    {
        StaticJsonDocument<256> doc;

        doc["ssid"] = wifiSSID;
        doc["password"] = wifiPassword;
        doc["accessPointPassword"] = accessPointPassword;

        doc["heartbeatInterval"] = heartbeatInterval;

        doc["timezone"] = timeZone;

        doc["mqttServer"] = mqttServer;
        doc["mqttPort"] = mqttPort;
        doc["mqttTopic"] = mqttTopic;

        doc["friendlyName"] = nodeFriendlyName;

        doc["mode"] = mode;

#ifdef __debugSettings
        serializeJsonPretty(doc, Serial);
        Serial.println();
#endif

        File configFile = LittleFS.open("/config.json", "w");
        if (!configFile)
        {
            Serial.println("Failed to open config file for writing");
            logger::LogEvent(logger::EVENTCATEGORIES::System, 4, "FS failure", "Failed to open config file for writing.");
            return false;
        }
        serializeJson(doc, configFile);
        configFile.close();

        return true;
    }

    void DefaultSettings()
    {
        sprintf(localHost, "%s-%s", DEFAULT_MQTT_TOPIC, common::GetDeviceMAC().substring(6).c_str());

        strcpy(wifiSSID, localHost);
        strcpy(wifiPassword, DEFAULT_AP_PASSWORD);
        strcpy(mqttServer, DEFAULT_MQTT_SERVER);

        strcpy(accessPointPassword, DEFAULT_AP_PASSWORD);

        mqttPort = DEFAULT_MQTT_PORT;

        strcpy(mqttTopic, localHost);

        timeZone = 2;

        strcpy(nodeFriendlyName, DEFAULT_NODE_FRIENDLY_NAME);
        heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;

        mode = OPERATION_MODES::LED_CHASER;

        if (!SaveSettings())
        {
            Serial.println("Failed to save config!");
        }
        else
        {
            ESP.restart();
#ifdef __debugSettings
            Serial.println("Settings saved.");
#endif
        }
    }

    void setup()
    {
        if ( !LoadSettings() )
            DefaultSettings();

#ifdef __debugSettings
        PrintSettings();
#endif
    }
}