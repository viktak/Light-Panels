#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <TimeLib.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

#include "version.h"
#include "settings.h"
#include "common.h"
#include "TimeChangeRules.h"
#include "ledstrip.h"
#include "mqtt.h"

#define ADMIN_USERNAME "admin"
#define ESP_ACCESS_POINT_NAME_SIZE 63
#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF

#define __debugSettings

namespace network
{

    WiFiClient client;
    ESP8266WebServer webServer(80);

    bool isAccessPoint = false;

    char accessPointSSID[32];

    os_timer_t accessPointTimer;

    TimeChangeRule *tcr;

    bool is_authenticated()
    {
#ifdef __debugSettings
        return true;
#endif
        if (webServer.hasHeader("Cookie"))
        {
            String cookie = webServer.header("Cookie");
            if (cookie.indexOf("EspAuth=1") != -1)
            {
                return true;
            }
        }
        return false;
    }

    void handleLogin()
    {
        String msg = "";
        if (webServer.hasHeader("Cookie"))
        {
            String cookie = webServer.header("Cookie");
        }
        if (webServer.hasArg("DISCONNECT"))
        {
            String header = "HTTP/1.1 301 OK\r\nSet-Cookie: EspAuth=0\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
            webServer.sendContent(header);
            return;
        }
        if (webServer.hasArg("username") && webServer.hasArg("password"))
        {
            if (webServer.arg("username") == ADMIN_USERNAME && webServer.arg("password") == settings::adminPassword)
            {
                String header = "HTTP/1.1 301 OK\r\nSet-Cookie: EspAuth=1\r\nLocation: /status.html\r\nCache-Control: no-cache\r\n\r\n";
                webServer.sendContent(header);
                return;
            }
            msg = "<div class=\"alert alert-danger\"><strong>Error!</strong> Wrong user name and/or password specified.<a href=\"#\" class=\"close\" data-dismiss=\"alert\" aria-label=\"close\">&times;</a></div>";
        }

        File f = LittleFS.open("/pageheader.html", "r");
        String headerString;
        if (f.available())
            headerString = f.readString();
        f.close();

        time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);

        f = LittleFS.open("/login.html", "r");

        String s, htmlString;

        while (f.available())
        {
            s = f.readStringUntil('\n');

            if (s.indexOf("%pageheader%") > -1)
                s.replace("%pageheader%", headerString);
            if (s.indexOf("%year%") > -1)
                s.replace("%year%", (String)year(localTime));
            if (s.indexOf("%alert%") > -1)
                s.replace("%alert%", msg);

            htmlString += s;
        }
        f.close();
        webServer.send(200, "text/html", htmlString);
    }

    void handleStatus()
    {

        if (!is_authenticated())
        {
            String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
            webServer.sendContent(header);
            return;
        }

        File f = LittleFS.open("/pageheader.html", "r");
        String headerString;
        if (f.available())
            headerString = f.readString();
        f.close();

        time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);

        String s;

        f = LittleFS.open("/status.html", "r");

        String htmlString;

        while (f.available())
        {
            s = f.readStringUntil('\n');

            //  System information
            if (s.indexOf("%pageheader%") > -1)
                s.replace("%pageheader%", headerString);
            if (s.indexOf("%year%") > -1)
                s.replace("%year%", (String)year(localTime));
            if (s.indexOf("%espid%") > -1)
                s.replace("%espid%", (String)ESP.getChipId());
            if (s.indexOf("%hardwareid%") > -1)
                s.replace("%hardwareid%", common::HARDWARE_ID);
            if (s.indexOf("%hardwareversion%") > -1)
                s.replace("%hardwareversion%", common::HARDWARE_VERSION);
            if (s.indexOf("%firmwareid%") > -1)
                s.replace("%firmwareid%", common::FIRMWARE_ID);
            if (s.indexOf("%firmwareversion%") > -1)
                s.replace("%firmwareversion%", String(FIRMWARE_VERSION));
            if (s.indexOf("%chipid%") > -1)
                s.replace("%chipid%", String((String)ESP.getChipId()));
            if (s.indexOf("%uptime%") > -1)
                s.replace("%uptime%", common::TimeIntervalToString(millis() / 1000));
            if (s.indexOf("%currenttime%") > -1)
                s.replace("%currenttime%", common::DateTimeToString(localTime));
            if (s.indexOf("%lastresetreason%") > -1)
                s.replace("%lastresetreason%", ESP.getResetReason());
            if (s.indexOf("%flashchipsize%") > -1)
                s.replace("%flashchipsize%", String(ESP.getFlashChipSize()));
            if (s.indexOf("%flashchipspeed%") > -1)
                s.replace("%flashchipspeed%", String(ESP.getFlashChipSpeed()));
            if (s.indexOf("%freeheapsize%") > -1)
                s.replace("%freeheapsize%", String(ESP.getFreeHeap()));
            if (s.indexOf("%freesketchspace%") > -1)
                s.replace("%freesketchspace%", String(ESP.getFreeSketchSpace()));
            if (s.indexOf("%friendlyname%") > -1)
                s.replace("%friendlyname%", settings::nodeFriendlyName);
            if (s.indexOf("%mqtt-topic%") > -1)
                s.replace("%mqtt-topic%", settings::mqttTopic);

            //  Network settings
            switch (WiFi.getMode())
            {
            case WIFI_AP:
                if (s.indexOf("%wifimode%") > -1)
                    s.replace("%wifimode%", "Access Point");
                if (s.indexOf("%macaddress%") > -1)
                    s.replace("%macaddress%", String(WiFi.softAPmacAddress()));
                if (s.indexOf("%networkaddress%") > -1)
                    s.replace("%networkaddress%", WiFi.softAPIP().toString());
                if (s.indexOf("%ssid%") > -1)
                    s.replace("%ssid%", String(WiFi.SSID()));
                if (s.indexOf("%subnetmask%") > -1)
                    s.replace("%subnetmask%", "n/a");
                if (s.indexOf("%gateway%") > -1)
                    s.replace("%gateway%", "n/a");
                break;
            case WIFI_STA:
                if (s.indexOf("%wifimode%") > -1)
                    s.replace("%wifimode%", "Station");
                if (s.indexOf("%macaddress%") > -1)
                    s.replace("%macaddress%", WiFi.macAddress());
                if (s.indexOf("%networkaddress%") > -1)
                    s.replace("%networkaddress%", WiFi.localIP().toString());
                if (s.indexOf("%ssid%") > -1)
                    s.replace("%ssid%", String(WiFi.SSID()));
                if (s.indexOf("%channel%") > -1)
                    s.replace("%channel%", String(WiFi.channel()));
                if (s.indexOf("%subnetmask%") > -1)
                    s.replace("%subnetmask%", WiFi.subnetMask().toString());
                if (s.indexOf("%gateway%") > -1)
                    s.replace("%gateway%", WiFi.gatewayIP().toString());
                break;
            default:
                //  This should not happen...
                break;
            }

            htmlString += s;
        }
        f.close();
        webServer.send(200, "text/html", htmlString);
    }

    void handleGeneralSettings()
    {

        if (!is_authenticated())
        {
            String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
            webServer.sendContent(header);
            return;
        }

        if (webServer.method() == HTTP_POST)
        { //  POST
#ifdef __debugSettings
            Serial.println("================= Submitted data =================");
            for (int i = 0; i < webServer.args(); i++)
                Serial.printf("%s: %s\r\n", webServer.argName(i).c_str(), webServer.arg(i).c_str());
            Serial.println("==================================================");
#endif
            //  System settings
            if (webServer.hasArg("friendlyname"))
                strcpy(settings::nodeFriendlyName, webServer.arg("friendlyname").c_str());

            if (webServer.hasArg("heartbeatinterval"))
            {
                os_timer_disarm(&mqtt::heartbeatTimer);
                settings::heartbeatInterval = atoi(webServer.arg("heartbeatinterval").c_str());
                os_timer_arm(&mqtt::heartbeatTimer, settings::heartbeatInterval * 1000, true);
            }

            if (webServer.hasArg("timezoneselector"))
            {
                settings::timeZone = atoi(webServer.arg("timezoneselector").c_str());
            }

            //  MQTT settings
            if (webServer.hasArg("mqttbroker"))
            {
                sprintf(settings::mqttServer, "%s", webServer.arg("mqttbroker").c_str());
            }

            if (webServer.hasArg("mqttport"))
            {
                settings::mqttPort = atoi(webServer.arg("mqttport").c_str());
            }

            if (webServer.hasArg("mqtttopic"))
            {
                sprintf(settings::mqttTopic, "%s", webServer.arg("mqtttopic").c_str());
            }

            settings::SaveSettings();
            ESP.restart();
        }

        File f = LittleFS.open("/pageheader.html", "r");
        String headerString;
        if (f.available())
            headerString = f.readString();
        f.close();

        f = LittleFS.open("/generalsettings.html", "r");

        time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);

        String s, htmlString, timezoneslist = "";

        char ss[4];

        for (signed char i = 0; i < (signed char)(sizeof(timechangerules::tzDescriptions) / sizeof(timechangerules::tzDescriptions[0])); i++)
        {
            itoa(i, ss, DEC);
            timezoneslist += "<option ";
            if (settings::timeZone == i)
            {
                timezoneslist += "selected ";
            }
            timezoneslist += "value=\"";
            timezoneslist += ss;
            timezoneslist += "\">";

            timezoneslist += timechangerules::tzDescriptions[i];

            timezoneslist += "</option>";
            timezoneslist += "\n";
        }

        while (f.available())
        {
            s = f.readStringUntil('\n');

            if (s.indexOf("%pageheader%") > -1)
                s.replace("%pageheader%", headerString);
            if (s.indexOf("%year%") > -1)
                s.replace("%year%", (String)year(localTime));
            if (s.indexOf("%mqtt-servername%") > -1)
                s.replace("%mqtt-servername%", settings::mqttServer);
            if (s.indexOf("%mqtt-port%") > -1)
                s.replace("%mqtt-port%", String(settings::mqttPort));
            if (s.indexOf("%mqtt-topic%") > -1)
                s.replace("%mqtt-topic%", settings::mqttTopic);
            if (s.indexOf("%timezoneslist%") > -1)
                s.replace("%timezoneslist%", timezoneslist);
            if (s.indexOf("%friendlyname%") > -1)
                s.replace("%friendlyname%", settings::nodeFriendlyName);
            if (s.indexOf("%heartbeatinterval%") > -1)
                s.replace("%heartbeatinterval%", (String)settings::heartbeatInterval);
            if (s.indexOf("%accesspointpassword%") > -1)
                s.replace("%accesspointpassword%", "");
            if (s.indexOf("%confirmaccesspointpassword%") > -1)
                s.replace("%confirmaccesspointpassword%", "");
            if (s.indexOf("%deviceadminpassword%") > -1)
                s.replace("%deviceadminpassword%", "");
            if (s.indexOf("%confirmdeviceadminpassword%") > -1)
                s.replace("%confirmdeviceadminpassword%", "");
            if (s.indexOf("%deviceadmin%") > -1)
                s.replace("%deviceadmin%", "admin");

            htmlString += s;
        }
        f.close();
        webServer.send(200, "text/html", htmlString);
    }

    void handleLightSettings()
    {
        if (!is_authenticated())
        {
            String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
            webServer.sendContent(header);
            return;
        }

        if (webServer.method() == HTTP_POST)
        { //  POST

            // for (size_t i = 0; i < server.args(); i++) {
            //   Serial.print(server.argName(i));
            //   Serial.print(": ");
            //   Serial.println(server.arg(i));
            // }

            settings::mode = webServer.arg("optSelectMode").toInt();
            settings::SaveSettings();
            ledstrip::StopAnimations();
        }

        File f = LittleFS.open("/pageheader.html", "r");
        String headerString;
        if (f.available())
            headerString = f.readString();
        f.close();

        time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);

        f = LittleFS.open("/lightsettings.html", "r");

        String s, htmlString, chkReversed;

        while (f.available())
        {
            s = f.readStringUntil('\n');

            if (s.indexOf("%pageheader%") > -1)
                s.replace("%pageheader%", headerString);
            if (s.indexOf("%year%") > -1)
                s.replace("%year%", (String)year(localTime));
            if (s.indexOf("%mqtt-servername%") > -1)
                s.replace("%mqtt-servername%", settings::mqttServer);
            if (s.indexOf("%mqtt-port%") > -1)
                s.replace("%mqtt-port%", String(settings::mqttPort));
            if (s.indexOf("%mqtt-topic%") > -1)
                s.replace("%mqtt-topic%", settings::mqttTopic);
            if (s.indexOf("%friendlyname%") > -1)
                s.replace("%friendlyname%", settings::nodeFriendlyName);
            if (s.indexOf("%heartbeatinterval%") > -1)
                s.replace("%heartbeatinterval%", (String)settings::heartbeatInterval);

            if (s.indexOf("%checked0%") > -1)
                s.replace("%checked0%", settings::mode == settings::OPERATION_MODES::LED_CHASER ? "checked" : "");
            if (s.indexOf("%checked1%") > -1)
                s.replace("%checked1%", settings::mode == settings::OPERATION_MODES::SLOW_PANELS ? "checked" : "");
            if (s.indexOf("%checked2%") > -1)
                s.replace("%checked2%", settings::mode == settings::OPERATION_MODES::FAST_CHANGING_RANDOM_SEGMENTS ? "checked" : "");

            htmlString += s;
        }
        f.close();
        webServer.send(200, "text/html", htmlString);
    }

    void handleNetworkSettings()
    {

        if (!is_authenticated())
        {
            String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
            webServer.sendContent(header);
            return;
        }

        if (webServer.method() == HTTP_POST)
        { //  POST
            if (webServer.hasArg("ssid"))
            {
                strcpy(settings::wifiSSID, webServer.arg("ssid").c_str());
                strcpy(settings::wifiPassword, webServer.arg("password").c_str());
                settings::SaveSettings();
                ESP.restart();
            }
        }

        File f = LittleFS.open("/pageheader.html", "r");

        String headerString;

        if (f.available())
            headerString = f.readString();
        f.close();

        time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);

        f = LittleFS.open("/networksettings.html", "r");
        String s, htmlString, wifiList;

        byte numberOfNetworks = WiFi.scanNetworks();
        for (size_t i = 0; i < numberOfNetworks; i++)
        {
            wifiList += "<div class=\"radio\"><label><input ";
            if (i == 0)
                wifiList += "id=\"ssid\" ";

            wifiList += "type=\"radio\" name=\"ssid\" value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</label></div>";
        }

        while (f.available())
        {
            s = f.readStringUntil('\n');

            if (s.indexOf("%pageheader%") > -1)
                s.replace("%pageheader%", headerString);
            if (s.indexOf("%year%") > -1)
                s.replace("%year%", (String)year(localTime));
            if (s.indexOf("%wifilist%") > -1)
                s.replace("%wifilist%", wifiList);
            htmlString += s;
        }
        f.close();
        webServer.send(200, "text/html", htmlString);
    }

    void handleTools()
    {

        if (!is_authenticated())
        {
            String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
            webServer.sendContent(header);
            return;
        }

        if (webServer.method() == HTTP_POST)
        { //  POST

            if (webServer.hasArg("reset"))
            {
                settings::DefaultSettings();
                ESP.restart();
            }

            if (webServer.hasArg("restart"))
            {
                ESP.restart();
            }
        }

        File f = LittleFS.open("/pageheader.html", "r");
        String headerString;
        if (f.available())
            headerString = f.readString();
        f.close();

        f = LittleFS.open("/tools.html", "r");

        time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);

        String s, htmlString;

        while (f.available())
        {
            s = f.readStringUntil('\n');

            if (s.indexOf("%pageheader%") > -1)
                s.replace("%pageheader%", headerString);
            if (s.indexOf("%year%") > -1)
                s.replace("%year%", (String)year(localTime));

            htmlString += s;
        }
        f.close();
        webServer.send(200, "text/html", htmlString);
    }

    void handleNotFound()
    {
        if (!is_authenticated())
        {
            String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
            webServer.sendContent(header);
            return;
        }

        File f = LittleFS.open("/pageheader.html", "r");
        String headerString;
        if (f.available())
            headerString = f.readString();
        f.close();

        f = LittleFS.open("/badrequest.html", "r");

        time_t localTime = timechangerules::timezones[settings::timeZone]->toLocal(now(), &tcr);

        String s, htmlString;

        while (f.available())
        {
            s = f.readStringUntil('\n');

            if (s.indexOf("%pageheader%") > -1)
                s.replace("%pageheader%", headerString);
            if (s.indexOf("%year%") > -1)
                s.replace("%year%", (String)year(localTime));

            htmlString += s;
        }
        f.close();
        webServer.send(200, "text/html", htmlString);
    }

    void InitWifiWebServer()
    {
        //  Web server
        if (MDNS.begin(settings::localHost))
        {
            Serial.printf("MDNS responder with hostname %s started.\r\n", settings::localHost);
        }

        //  Page handles
        webServer.on("/", handleStatus);
        webServer.on("/login.html", handleLogin);
        webServer.on("/status.html", handleStatus);
        webServer.on("/generalsettings.html", handleGeneralSettings);
        webServer.on("/lightsettings.html", handleLightSettings);
        webServer.on("/networksettings.html", handleNetworkSettings);
        webServer.on("/tools.html", handleTools);

        webServer.onNotFound(handleNotFound);

        /*handling uploading firmware file */
        webServer.on(
            "/update", HTTP_POST, []()
            {
        webServer.sendHeader("Connection", "close");
        webServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK"); },
            []()
            {
                HTTPUpload &upload = webServer.upload();
                if (upload.status == UPLOAD_FILE_START)
                {
                    Serial.printf("Update: %s\n", upload.filename.c_str());
                    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                    { // start with max available size
                        Update.printError(Serial);
                    }
                }
                else if (upload.status == UPLOAD_FILE_WRITE)
                {
                    /* flashing firmware to ESP*/
                    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                    {
                        Update.printError(Serial);
                    }
                }
                else if (upload.status == UPLOAD_FILE_END)
                {
                    if (Update.end(true))
                    { // true to set the size to the current progress
                        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                        ESP.restart();
                    }
                    else
                    {
                        Update.printError(Serial);
                    }
                }
                yield();
            });

        //  Start HTTP (web) server
        webServer.begin();
        Serial.println("HTTP server started.");

        //  Authenticate HTTP requests
        const char *headerkeys[] = {"User-Agent", "Cookie"};
        size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
        webServer.collectHeaders(headerkeys, headerkeyssize);
    }

    void setup()
    {
        InitWifiWebServer();
    }

    void loop()
    {
        webServer.handleClient();
    }

}
