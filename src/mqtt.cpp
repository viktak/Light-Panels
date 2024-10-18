#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

#include "version.h"
#include "settings.h"
#include "common.h"
#include "TimeChangeRules.h"

bool needsHeartbeat = false;
os_timer_t heartbeatTimer;

WiFiClient client;

PubSubClient PSclient(client);

const char *mqttCustomer = MQTT_CUSTOMER;
const char *mqttProject = MQTT_PROJECT;

void heartbeatTimerCallback(void *pArg)
{
    needsHeartbeat = true;
}

void ConnectToMQTTBroker()
{
    if (!PSclient.connected())
    {
#ifdef __debugSettings
        Serial.printf("Connecting to MQTT broker %s... ", settings::mqttServer);
#endif
        if (PSclient.connect(settings::localHost, (mqttCustomer + String("/") + mqttProject + String("/") + settings::mqttTopic + "/STATE").c_str(), 0, true, "offline"))
        {
#ifdef __debugSettings
            Serial.println(" success.");
#endif
            PSclient.subscribe((mqttCustomer + String("/") + mqttProject + String("/") + settings::mqttTopic + "/cmnd").c_str(), 0);
            PSclient.publish((mqttCustomer + String("/") + mqttProject + String("/") + settings::mqttTopic + "/STATE").c_str(), "online", true);

            PSclient.setBufferSize(1024 * 5);
        }
        else
        {
#ifdef __debugSettings
            Serial.println(" failure!");
#endif
        }
    }
}

void PublishData(const char *topic, const char *payload, bool retained)
{
    ConnectToMQTTBroker();

    if (PSclient.connected())
    {
        PSclient.publish((mqttCustomer + String("/") + mqttProject + String("/") + settings::mqttTopic + String("/") + topic).c_str(), payload, retained);
    }
}

void SendHeartbeat()
{

    StaticJsonDocument<768> doc;

    JsonObject sysDetails = doc.createNestedObject("System");
    sysDetails["ChipID"] = (String)ESP.getChipId();

    TimeChangeRule *tcr;
    time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);
    char myDate[20];
    DateTimeToString(myDate, localTime);
    sysDetails["Time"] = myDate;

    sysDetails["Node"] = settings::localHost;
    sysDetails["Freeheap"] = ESP.getFreeHeap();

    sysDetails["HardwareID"] = HARDWARE_ID;
    sysDetails["HardwareVersion"] = HARDWARE_VERSION;
    sysDetails["FirmwareID"] = FIRMWARE_ID;
    sysDetails["FirmwareVersion"] = FIRMWARE_VERSION;
    sysDetails["UpTime"] = TimeIntervalToString(millis() / 1000);
    sysDetails["CPU0_ResetReason"] = ESP.getResetReason();

    sysDetails["FriendlyName"] = settings::nodeFriendlyName;
    sysDetails["TIMEZONE"] = settings::timeZone;

    JsonObject mqttDetails = doc.createNestedObject("MQTT");

    mqttDetails["MQTT_SERVER"] = settings::mqttServer;
    mqttDetails["MQTT_PORT"] = settings::mqttPort;
    mqttDetails["MQTT_TOPIC"] = settings::mqttTopic;

    JsonObject wifiDetails = doc.createNestedObject("WiFi");
    wifiDetails["APP_NAME"] = settings::localHost;
    wifiDetails["SSID"] = settings::wifiSSID;
    wifiDetails["Channel"] = WiFi.channel();
    wifiDetails["IP_Address"] = WiFi.localIP().toString();
    wifiDetails["MAC_Address"] = WiFi.macAddress();

    String myJsonString;

    serializeJson(doc, myJsonString);

#ifdef __debugSettings
    serializeJsonPretty(doc, Serial);
    Serial.println();
#endif

    ConnectToMQTTBroker();

    if (PSclient.connected())
    {
        PSclient.publish((mqttCustomer + String("/") + mqttProject + String("/") + settings::mqttTopic + "/HEARTBEAT").c_str(), myJsonString.c_str(), false);
#ifdef __debugSettings
        Serial.println("Heartbeat sent.");
#endif
        needsHeartbeat = false;
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{
    const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(14) + 300;

    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, payload);

#ifdef __debugSettings
    Serial.print("Message arrived in topic [");
    Serial.print(topic);
    Serial.println("]: ");

    serializeJsonPretty(doc, Serial);
    Serial.println();
#endif

    const char *command = doc["command"];
    if (!strcmp(command, "ListSDCardFiles"))
    {
        // SendFileList();
    }
    else if (!strcmp(command, "SetSettings"))
    {
        // ChangeSettings_JSON(doc.getMember("params"));
    }
    else if (!strcmp(command, "ResetAllSettingsToDefault"))
    {
        settings::DefaultSettings();
        ESP.restart();
    }
}

void setupMqtt()
{
    PSclient.setServer(settings::mqttServer, settings::mqttPort);
    PSclient.setCallback(mqttCallback);

    os_timer_setfn(&heartbeatTimer, heartbeatTimerCallback, NULL);
    os_timer_arm(&heartbeatTimer, settings::heartbeatInterval * 1000, true);
}

void loopMqtt()
{
    ConnectToMQTTBroker();
    if (PSclient.connected())
        PSclient.loop();
}
