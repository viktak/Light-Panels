#define __debugSettings
#include "includes.h"


//  Web server
ESP8266WebServer server(80);

//  Initialize Wifi
WiFiClient wclient;
PubSubClient PSclient(wclient);

//  Timers and their flags
os_timer_t heartbeatTimer;
os_timer_t accessPointTimer;

//  Flags
bool needsHeartbeat = false;

//  Buttons/Switches
Button2 btnMode = Button2(MODE_BUTTON_GPIO);


//  NeoPixel
const uint16_t PixelCount = TOTAL_LEDS; // make sure to set this to the number of pixels in your strip
const uint16_t AnimCount = PixelCount / 5 * 2 + 1; // we only need enough animations for the tail and one extra

const uint16_t PixelFadeDuration = 500; // third of a second
// one second divide by the number of pixels = loop once a second
const uint16_t NextPixelMoveDuration = 2000 / PixelCount; // how fast we move through the pixels

NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);

NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

ChaserAnimationState chaserAnimationState[AnimCount];
SlowPanelsAnimationState slowPanelAnimationState[NUMBER_OF_PANELS];
FastSegmentsAnimationState fastSegmentsAnimationState;

uint16_t frontPixel = 0;  // the front of the loop
RgbColor frontColor;  // the color at the front of the loop

//  Other global variables
config appConfig;
bool isAccessPoint = false;
bool isAccessPointCreated = false;
TimeChangeRule *tcr;        // Pointer to the time change rule

char16_t buttonMillis;

bool ntpInitialized = false;
enum CONNECTION_STATE connectionState;

WiFiUDP Udp;

void LogEvent(int Category, int ID, String Title, String Data){
  if (PSclient.connected()){

    String msg = "{";

    msg += "\"Node\":" + (String)ESP.getChipId() + ",";
    msg += "\"Category\":" + (String)Category + ",";
    msg += "\"ID\":" + (String)ID + ",";
    msg += "\"Title\":\"" + Title + "\",";
    msg += "\"Data\":\"" + Data + "\"}";

    debugln(msg);

    PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/log").c_str(), msg.c_str(), false);
  }
}

void SetRandomSeed(){
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}



void FadeOutAnimUpdate(const AnimationParam& param){
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        chaserAnimationState[param.index].StartingColor,
        chaserAnimationState[param.index].EndingColor,
        param.progress);
    // apply the color to the strip
    strip.SetPixelColor(chaserAnimationState[param.index].IndexPixel, 
        colorGamma.Correct(updatedColor));
}

void ChaseAnimUpdate(const AnimationParam& param){
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);

        // pick the next pixel inline to start animating
        // 
        frontPixel = (frontPixel + 1) % PixelCount; // increment and wrap
        if (frontPixel == 0){
            // we looped, lets pick a new front color
            frontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
        }

        uint16_t indexAnim;
        // do we have an animation available to use to animate the next front pixel?
        // if you see skipping, then either you are going to fast or need to increase
        // the number of animation channels
        if (animations.NextAvailableAnimation(&indexAnim, 1))
        {
            chaserAnimationState[indexAnim].StartingColor = frontColor;
            chaserAnimationState[indexAnim].EndingColor = RgbColor(0, 0, 0);
            chaserAnimationState[indexAnim].IndexPixel = frontPixel;

            animations.StartAnimation(indexAnim, PixelFadeDuration, FadeOutAnimUpdate);
        }
    }
}

void BlendAnimUpdate(const AnimationParam& param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    for (size_t panel = 0; panel < NUMBER_OF_PANELS; panel++){
        RgbColor updatedColor = RgbColor::LinearBlend(
            slowPanelAnimationState[panel].StartingColor,
            slowPanelAnimationState[panel].EndingColor,
            param.progress);

        // apply the color to the strip
        for (uint16_t pixel = 0; pixel < LEDS_PER_PANEL; pixel++)
        {
            strip.SetPixelColor(panel * LEDS_PER_PANEL + pixel, updatedColor);
        }
    }

}

void FastSegmentsAnimUpdate(const AnimationParam& param){
    RgbColor updatedColor = RgbColor::LinearBlend(
        fastSegmentsAnimationState.StartingColor,
        fastSegmentsAnimationState.EndingColor,
        param.progress);

    for (uint16_t pixel = 0; pixel < LEDS_PER_SEGMENT; pixel++){
        strip.SetPixelColor(fastSegmentsAnimationState.Segment * LEDS_PER_SEGMENT + pixel, updatedColor);
    }

}

void StopAnimations(){

    animations.StopAll();               //  Stop all animations
    strip.ClearTo(RgbColor(0, 0, 0));   //  Clear all pixels
    strip.Show();

}


void accessPointTimerCallback(void *pArg) {
  ESP.reset();
}

void heartbeatTimerCallback(void *pArg) {
  needsHeartbeat = true;
}

bool loadSettings(config& data) {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    debugln("Failed to open config file");
    LogEvent(EVENTCATEGORIES::System, 1, "FS failure", "Failed to open config file.");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    debugln("Config file size is too large");
    LogEvent(EVENTCATEGORIES::System, 2, "FS failure", "Config file size is too large.");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  configFile.close();

  StaticJsonDocument<JSON_SETTINGS_SIZE> doc;
  DeserializationError error = deserializeJson(doc, buf.get());

  if (error) {
    debugln("Failed to parse config file");
    LogEvent(EVENTCATEGORIES::System, 3, "FS failure", "Failed to parse config file.");
    debugln(error.c_str());
    return false;
  }

  #ifdef __debugSettings
  serializeJsonPretty(doc,Serial);
  Serial.println();
  #endif

  if (doc["ssid"]){
    strcpy(appConfig.ssid, doc["ssid"]);
  }
  else
  {
    strcpy(appConfig.ssid, defaultSSID);
  }

  if (doc["password"]){
    strcpy(appConfig.password, doc["password"]);
  }
  else
  {
    strcpy(appConfig.password, DEFAULT_PASSWORD);
  }

  if (doc["mqttServer"]){
    strcpy(appConfig.mqttServer, doc["mqttServer"]);
  }
  else
  {
    strcpy(appConfig.mqttServer, DEFAULT_MQTT_SERVER);
  }

  if (doc["mqttPort"]){
    appConfig.mqttPort = doc["mqttPort"];
  }
  else
  {
    appConfig.mqttPort = DEFAULT_MQTT_PORT;
  }

  if (doc["mqttTopic"]){
    strcpy(appConfig.mqttTopic, doc["mqttTopic"]);
  }
  else
  {
    sprintf(appConfig.mqttTopic, "%s-%u", DEFAULT_MQTT_TOPIC, ESP.getChipId());
  }

  if (doc["friendlyName"]){
    strcpy(appConfig.friendlyName, doc["friendlyName"]);
  }
  else
  {
    strcpy(appConfig.friendlyName, NODE_DEFAULT_FRIENDLY_NAME);
  }

  if (doc["timezone"]){
    appConfig.timeZone = doc["timezone"];
  }
  else
  {
    appConfig.timeZone = 0;
  }

  if (doc["heartbeatInterval"]){
    appConfig.heartbeatInterval = doc["heartbeatInterval"];
  }
  else
  {
    appConfig.heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
  }


  if (doc["mode"]){
    appConfig.mode = doc["mode"];
  }
  else
  {
    appConfig.mode = OPERATION_MODES::LED_CHASER;
  }

  return true;
}

bool saveSettings() {
  StaticJsonDocument<1024> doc;

  doc["ssid"] = appConfig.ssid;
  doc["password"] = appConfig.password;

  doc["heartbeatInterval"] = appConfig.heartbeatInterval;

  doc["timezone"] = appConfig.timeZone;

  doc["mqttServer"] = appConfig.mqttServer;
  doc["mqttPort"] = appConfig.mqttPort;
  doc["mqttTopic"] = appConfig.mqttTopic;

  doc["friendlyName"] = appConfig.friendlyName;

  doc["mode"] = appConfig.mode;

  #ifdef __debugSettings
  serializeJsonPretty(doc,Serial);
  Serial.println();
  #endif

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    debugln("Failed to open config file for writing");
    LogEvent(System, 4, "FS failure", "Failed to open config file for writing.");
    return false;
  }
  serializeJson(doc, configFile);
  configFile.close();

  return true;
}

void defaultSettings(){
  #ifdef __debugSettings
  strcpy(appConfig.ssid, DEBUG_WIFI_SSID);
  strcpy(appConfig.password, DEBUG_WIFI_PASSWORD);
  strcpy(appConfig.mqttServer, DEBUG_MQTT_SERVER);
  #else
  strcpy(appConfig.ssid, "ESP");
  strcpy(appConfig.password, "password");
  strcpy(appConfig.mqttServer, "test.mosquitto.org");
  #endif

  appConfig.mqttPort = DEFAULT_MQTT_PORT;

  sprintf(defaultSSID, "%s-%u", DEFAULT_MQTT_TOPIC, ESP.getChipId());
  strcpy(appConfig.mqttTopic, defaultSSID);

  appConfig.timeZone = 2;

  strcpy(appConfig.friendlyName, NODE_DEFAULT_FRIENDLY_NAME);
  appConfig.heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;

  appConfig.mode = OPERATION_MODES::LED_CHASER;

  if (!saveSettings()) {
    debugln("Failed to save config");
  } else {
    debugln("Config saved");
  }
}

String DateTimeToString(time_t time){

  String myTime = "";
  char s[2];

  //  years
  itoa(year(time), s, DEC);
  myTime+= s;
  myTime+="-";


  //  months
  itoa(month(time), s, DEC);
  myTime+= s;
  myTime+="-";

  //  days
  itoa(day(time), s, DEC);
  myTime+= s;

  myTime+=" ";

  //  hours
  itoa(hour(time), s, DEC);
  myTime+= s;
  myTime+=":";

  //  minutes
  if(minute(time) <10)
    myTime+="0";

  itoa(minute(time), s, DEC);
  myTime+= s;
  myTime+=":";

  //  seconds
  if(second(time) <10)
    myTime+="0";

  itoa(second(time), s, DEC);
  myTime+= s;

  return myTime;
}

String TimeIntervalToString(time_t time){

  String myTime = "";
  char s[2];

  //  hours
  itoa((time/3600), s, DEC);
  myTime+= s;
  myTime+=":";

  //  minutes
  if(minute(time) <10)
    myTime+="0";

  itoa(minute(time), s, DEC);
  myTime+= s;
  myTime+=":";

  //  seconds
  if(second(time) <10)
    myTime+="0";

  itoa(second(time), s, DEC);
  myTime+= s;
  return myTime;
}

void btnButtonClicked(Button2& btn){

    long m = appConfig.mode + 1;

    if (btn == btnMode){
        switch (btn.getClickType()){
            case SINGLE_CLICK:
                debugln("SINGLE_CLICK");
                appConfig.mode = m % NUMBER_OF_OPERATION_MODES;
                saveSettings();
                StopAnimations();
                break;
            case DOUBLE_CLICK:
                debugln("DOUBLE_CLICK");
                break;
            case TRIPLE_CLICK:
                debugln("TRIPLE_CLICK");
                break;
            case LONG_CLICK:
                debugln("LONG_CLICK");
                break;
            default:
                break;
        }
    }

}

bool is_authenticated(){
  #ifdef __debugSettings
  return true;
  #endif
  if (server.hasHeader("Cookie")){
    String cookie = server.header("Cookie");
    if (cookie.indexOf("EspAuth=1") != -1) {
      LogEvent(EVENTCATEGORIES::Authentication, 1, "Success", "");
      return true;
    }
  }
  LogEvent(EVENTCATEGORIES::Authentication, 2, "Failure", "");
  return false;
}

void handleLogin(){
  String msg = "";
  if (server.hasHeader("Cookie")){
    String cookie = server.header("Cookie");
  }
  if (server.hasArg("DISCONNECT")){
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: EspAuth=0\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    LogEvent(EVENTCATEGORIES::Login, 1, "Logout", "");
    return;
  }
  if (server.hasArg("username") && server.hasArg("password")){
    if (server.arg("username") == ADMIN_USERNAME &&  server.arg("password") == ADMIN_PASSWORD ){
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: EspAuth=1\r\nLocation: /status.html\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      LogEvent(EVENTCATEGORIES::Login, 2, "Success", "User name: " + server.arg("username"));
      return;
    }
    msg = "<div class=\"alert alert-danger\"><strong>Error!</strong> Wrong user name and/or password specified.<a href=\"#\" class=\"close\" data-dismiss=\"alert\" aria-label=\"close\">&times;</a></div>";
    LogEvent(EVENTCATEGORIES::Login, 2, "Failure", "User name: " + server.arg("username") + " - Password: " + server.arg("password"));
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = timezones[appConfig.timeZone]->toLocal(now(), &tcr);

  f = LittleFS.open("/login.html", "r");

  String s, htmlString;

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%alert%")>-1) s.replace("%alert%", msg);

    htmlString+=s;
  }
  f.close();
  server.send(200, "text/html", htmlString);
  LogEvent(PageHandler, 2, "Page served", "/");
}

void handleRoot() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "/");

  if (!is_authenticated()){
    String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = timezones[appConfig.timeZone]->toLocal(now(), &tcr);

  f = LittleFS.open("/index.html", "r");

  String FirmwareVersionString = String(FIRMWARE_VERSION);

  String s, htmlString;

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%espid%")>-1) s.replace("%espid%", (String)ESP.getChipId());
    if (s.indexOf("%hardwareid%")>-1) s.replace("%hardwareid%", HARDWARE_ID);
    if (s.indexOf("%hardwareversion%")>-1) s.replace("%hardwareversion%", HARDWARE_VERSION);
    if (s.indexOf("%softwareid%")>-1) s.replace("%softwareid%", SOFTWARE_ID);
    if (s.indexOf("%firmwareversion%")>-1) s.replace("%firmwareversion%", FirmwareVersionString);

    htmlString+=s;
  }
  f.close();
  server.send(200, "text/html", htmlString);
  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "/");
}

void handleStatus() {

  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "status.html");
  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = timezones[appConfig.timeZone]->toLocal(now(), &tcr);

  String FirmwareVersionString = String(FIRMWARE_VERSION);
  String s;

  f = LittleFS.open("/status.html", "r");

  String htmlString, ds18b20list;

  while (f.available()){
    s = f.readStringUntil('\n');

    //  System information
    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%chipid%")>-1) s.replace("%chipid%", (String)ESP.getChipId());
    if (s.indexOf("%hardwareid%")>-1) s.replace("%hardwareid%", HARDWARE_ID);
    if (s.indexOf("%hardwareversion%")>-1) s.replace("%hardwareversion%", HARDWARE_VERSION);
    if (s.indexOf("%firmwareid%")>-1) s.replace("%firmwareid%", SOFTWARE_ID);
    if (s.indexOf("%firmwareversion%")>-1) s.replace("%firmwareversion%", FirmwareVersionString);
    if (s.indexOf("%uptime%")>-1) s.replace("%uptime%", TimeIntervalToString(millis()/1000));
    if (s.indexOf("%currenttime%")>-1) s.replace("%currenttime%", DateTimeToString(localTime));
    if (s.indexOf("%lastresetreason%")>-1) s.replace("%lastresetreason%", ESP.getResetReason());
    if (s.indexOf("%flashchipsize%")>-1) s.replace("%flashchipsize%",String(ESP.getFlashChipSize()));
    if (s.indexOf("%flashchipspeed%")>-1) s.replace("%flashchipspeed%",String(ESP.getFlashChipSpeed()));
    if (s.indexOf("%freeheapsize%")>-1) s.replace("%freeheapsize%",String(ESP.getFreeHeap()));
    if (s.indexOf("%freesketchspace%")>-1) s.replace("%freesketchspace%",String(ESP.getFreeSketchSpace()));
    if (s.indexOf("%friendlyname%")>-1) s.replace("%friendlyname%",appConfig.friendlyName);
    if (s.indexOf("%mqtt-topic%")>-1) s.replace("%mqtt-topic%",appConfig.mqttTopic);

    //  Network settings
    switch (WiFi.getMode()) {
      case WIFI_AP:
        if (s.indexOf("%wifimode%")>-1) s.replace("%wifimode%", "Access Point");
        if (s.indexOf("%macaddress%")>-1) s.replace("%macaddress%",String(WiFi.softAPmacAddress()));
        if (s.indexOf("%networkaddress%")>-1) s.replace("%networkaddress%",WiFi.softAPIP().toString());
        if (s.indexOf("%ssid%")>-1) s.replace("%ssid%",String(WiFi.SSID()));
        if (s.indexOf("%subnetmask%")>-1) s.replace("%subnetmask%","n/a");
        if (s.indexOf("%gateway%")>-1) s.replace("%gateway%","n/a");
        break;
      case WIFI_STA:
        if (s.indexOf("%wifimode%")>-1) s.replace("%wifimode%", "Station");
        if (s.indexOf("%macaddress%")>-1) s.replace("%macaddress%",String(WiFi.macAddress()));
        if (s.indexOf("%networkaddress%")>-1) s.replace("%networkaddress%",WiFi.localIP().toString());
        if (s.indexOf("%ssid%")>-1) s.replace("%ssid%",String(WiFi.SSID()));
        if (s.indexOf("%subnetmask%")>-1) s.replace("%subnetmask%",WiFi.subnetMask().toString());
        if (s.indexOf("%gateway%")>-1) s.replace("%gateway%",WiFi.gatewayIP().toString());
        break;
      default:
        //  This should not happen...
        break;
    }

      htmlString+=s;
    }
    f.close();
  server.send(200, "text/html", htmlString);
  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "status.html");
}

void handleGeneralSettings() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "generalsettings.html");

  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
   }

  if (server.method() == HTTP_POST){  //  POST
    bool mqttDirty = false;

    if (server.hasArg("timezoneselector")){
      signed char oldTimeZone = appConfig.timeZone;
      appConfig.timeZone = atoi(server.arg("timezoneselector").c_str());

      adjustTime((appConfig.timeZone - oldTimeZone) * SECS_PER_HOUR);

      LogEvent(EVENTCATEGORIES::TimeZoneChange, 1, "New time zone", "UTC " + server.arg("timezoneselector"));
    }

    if (server.hasArg("friendlyname")){
      strcpy(appConfig.friendlyName, server.arg("friendlyname").c_str());
      LogEvent(EVENTCATEGORIES::FriendlyNameChange, 1, "New friendly name", appConfig.friendlyName);
    }

    if (server.hasArg("heartbeatinterval")){
      os_timer_disarm(&heartbeatTimer);
      appConfig.heartbeatInterval = server.arg("heartbeatinterval").toInt();
      LogEvent(EVENTCATEGORIES::HeartbeatIntervalChange, 1, "New Heartbeat interval", (String)appConfig.heartbeatInterval);
      os_timer_arm(&heartbeatTimer, appConfig.heartbeatInterval * 1000, true);
    }

    //  MQTT settings
    if (server.hasArg("mqttbroker")){
        if ((String)appConfig.mqttServer != server.arg("mqttbroker")){
            mqttDirty = true;
            sprintf(appConfig.mqttServer, "%s", server.arg("mqttbroker").c_str());
            LogEvent(EVENTCATEGORIES::MqttParamChange, 1, "New MQTT broker", appConfig.mqttServer);
        }
    }

    if (server.hasArg("mqttport")){
        if (appConfig.mqttPort != atoi(server.arg("mqttport").c_str())){
            mqttDirty = true;
            appConfig.mqttPort = atoi(server.arg("mqttport").c_str());
            LogEvent(EVENTCATEGORIES::MqttParamChange, 2, "New MQTT port", server.arg("mqttport").c_str());
        }
    }

    if (server.hasArg("mqtttopic")){
        if ((String)appConfig.mqttTopic != server.arg("mqtttopic")){
            mqttDirty = true;
            sprintf(appConfig.mqttTopic, "%s", server.arg("mqtttopic").c_str());
            LogEvent(EVENTCATEGORIES::MqttParamChange, 1, "New MQTT topic", appConfig.mqttTopic);
        }
    }

    if (mqttDirty)
      PSclient.disconnect();

    saveSettings();
    ESP.reset();

  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = timezones[appConfig.timeZone]->toLocal(now(), &tcr);

  f = LittleFS.open("/generalsettings.html", "r");

  String s, htmlString, timezoneslist;

  char ss[sizeof(int)];

  for (unsigned long i = 0; i < sizeof(tzDescriptions)/sizeof(tzDescriptions[0]); i++) {
    itoa(i, ss, DEC);
    timezoneslist+="<option ";
    if (appConfig.timeZone == i){
      timezoneslist+= "selected ";
    }
    timezoneslist+= "value=\"";
    timezoneslist+=ss;
    timezoneslist+="\">";

    timezoneslist+= tzDescriptions[i];

    timezoneslist+="</option>";
    timezoneslist+="\n";
  }

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%mqtt-servername%")>-1) s.replace("%mqtt-servername%", appConfig.mqttServer);
    if (s.indexOf("%mqtt-port%")>-1) s.replace("%mqtt-port%", String(appConfig.mqttPort));
    if (s.indexOf("%mqtt-topic%")>-1) s.replace("%mqtt-topic%", appConfig.mqttTopic);
    if (s.indexOf("%timezoneslist%")>-1) s.replace("%timezoneslist%", timezoneslist);
    if (s.indexOf("%friendlyname%")>-1) s.replace("%friendlyname%", appConfig.friendlyName);
    if (s.indexOf("%heartbeatinterval%")>-1) s.replace("%heartbeatinterval%", (String)appConfig.heartbeatInterval);

    htmlString+=s;
  }
  f.close();
  server.send(200, "text/html", htmlString);

  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "generalsettings.html");
}

void handleLightSettings() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "lightsettings.html");

  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
   }

  if (server.method() == HTTP_POST){  //  POST


    // for (size_t i = 0; i < server.args(); i++) {
    //   Serial.print(server.argName(i));
    //   Serial.print(": ");
    //   Serial.println(server.arg(i));
    // }

    appConfig.mode = server.arg("optSelectMode").toInt();
    saveSettings();
    StopAnimations();
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = timezones[appConfig.timeZone]->toLocal(now(), &tcr);

  f = LittleFS.open("/lightsettings.html", "r");

  String s, htmlString, chkReversed;

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%mqtt-servername%")>-1) s.replace("%mqtt-servername%", appConfig.mqttServer);
    if (s.indexOf("%mqtt-port%")>-1) s.replace("%mqtt-port%", String(appConfig.mqttPort));
    if (s.indexOf("%mqtt-topic%")>-1) s.replace("%mqtt-topic%", appConfig.mqttTopic);
    if (s.indexOf("%friendlyname%")>-1) s.replace("%friendlyname%", appConfig.friendlyName);
    if (s.indexOf("%heartbeatinterval%")>-1) s.replace("%heartbeatinterval%", (String)appConfig.heartbeatInterval);

    if (s.indexOf("%checked0%")>-1) s.replace("%checked0%", appConfig.mode==OPERATION_MODES::LED_CHASER?"checked":"");
    if (s.indexOf("%checked1%")>-1) s.replace("%checked1%", appConfig.mode==OPERATION_MODES::SLOW_PANELS?"checked":"");
    if (s.indexOf("%checked2%")>-1) s.replace("%checked2%", appConfig.mode==OPERATION_MODES::FAST_CHANGING_RANDOM_SEGMENTS?"checked":"");


    htmlString+=s;
  }
  f.close();
  server.send(200, "text/html", htmlString);

  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "lightsettings.html");
}

void handleNetworkSettings() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "networksettings.html");

  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
   }

  if (server.method() == HTTP_POST){  //  POST
    if (server.hasArg("ssid")){
      strcpy(appConfig.ssid, server.arg("ssid").c_str());
      strcpy(appConfig.password, server.arg("password").c_str());
      saveSettings();

      isAccessPoint=false;
      connectionState = STATE_CHECK_WIFI_CONNECTION;
      WiFi.disconnect(false);

      ESP.reset();
    }
  }

  File f = LittleFS.open("/pageheader.html", "r");

  String headerString;

  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = timezones[appConfig.timeZone]->toLocal(now(), &tcr);

  f = LittleFS.open("/networksettings.html", "r");
  String s, htmlString, wifiList;

  byte numberOfNetworks = WiFi.scanNetworks();
  for (size_t i = 0; i < numberOfNetworks; i++) {
    wifiList+="<div class=\"radio\"><label><input ";
    if (i==0) wifiList+="id=\"ssid\" ";

    wifiList+="type=\"radio\" name=\"ssid\" value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</label></div>";
  }

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%wifilist%")>-1) s.replace("%wifilist%", wifiList);
      htmlString+=s;
    }
    f.close();
  server.send(200, "text/html", htmlString);

  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "networksettings.html");
}

void handleTools() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "tools.html");

  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
   }

  if (server.method() == HTTP_POST){  //  POST

    if (server.hasArg("reset")){
      LogEvent(EVENTCATEGORIES::Reboot, 1, "Reset", "");
      defaultSettings();
      ESP.reset();
    }

    if (server.hasArg("restart")){
      LogEvent(EVENTCATEGORIES::Reboot, 2, "Restart", "");
      ESP.reset();
    }
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = timezones[appConfig.timeZone]->toLocal(now(), &tcr);

  f = LittleFS.open("/tools.html", "r");

  String s, htmlString;

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));

      htmlString+=s;
    }
    f.close();
  server.send(200, "text/html", htmlString);

  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "tools.html");
}

/*
    for (size_t i = 0; i < server.args(); i++) {
      Serial.print(server.argName(i));
      Serial.print(": ");
      Serial.println(server.arg(i));
    }
*/

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void SendHeartbeat(){

  if (PSclient.connected()){

    time_t localTime = timezones[appConfig.timeZone]->toLocal(now(), &tcr);

    const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 200;
    DynamicJsonDocument doc(capacity);

    JsonObject sysDetails = doc.createNestedObject("System");
    sysDetails["Time"] = DateTimeToString(localTime);
    sysDetails["Node"] = ESP.getChipId();
    sysDetails["Freeheap"] = ESP.getFreeHeap();
    sysDetails["FriendlyName"] = appConfig.friendlyName;
    sysDetails["HeartbeatInterval"] = appConfig.heartbeatInterval;

    JsonObject wifiDetails = doc.createNestedObject("Wifi");
    wifiDetails["SSId"] = String(WiFi.SSID());
    wifiDetails["MACAddress"] = String(WiFi.macAddress());
    wifiDetails["IPAddress"] = WiFi.localIP().toString();

    #ifdef __debugSettings
    serializeJsonPretty(doc,Serial);
    Serial.println();
    #endif

    String myJsonString;

    serializeJson(doc, myJsonString);
    PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/HEARTBEAT").c_str(), myJsonString.c_str(), false);

  }

  needsHeartbeat = false;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  debug("Topic:\t\t");
  debugln(topic);

  debug("Payload:\t");
  for (unsigned int i = 0; i < length; i++) {
    debug((char)payload[i]);
  }
  debugln();

  StaticJsonDocument<JSON_MQTT_COMMAND_SIZE> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    debugln("Failed to parse incoming string.");
    debugln(error.c_str());
    for (size_t i = 0; i < 10; i++) {
      digitalWrite(CONNECTION_STATUS_LED_GPIO, !digitalRead(CONNECTION_STATUS_LED_GPIO));
      delay(50);
    }
  }
  else{
    //  It IS a JSON string

    #ifdef __debugSettings
    serializeJsonPretty(doc,Serial);
    Serial.println();
    #endif

    //  reset
    if (doc.containsKey("reset")){
      LogEvent(EVENTCATEGORIES::MqttMsg, 1, "Reset", "");
      defaultSettings();
      ESP.reset();
    }

    //  restart
    if (doc.containsKey("restart")){
      LogEvent(EVENTCATEGORIES::MqttMsg, 2, "Restart", "");
      ESP.reset();
    }
  }

}

void setup() {
    delay(1); //  Needed for PlatformIO serial monitor
    Serial.begin(DEBUG_SPEED);
    Serial.setDebugOutput(false);
    Serial.printf("\n\n\n\rBooting node:\t%u...\r\n", ESP.getChipId());
    Serial.printf("Hardware ID:\t\t%s\r\nHardware version:\t%s\r\nSoftware ID:\t\t%s\r\nSoftware version:\t%s\r\n\n", HARDWARE_ID, HARDWARE_VERSION, SOFTWARE_ID, FIRMWARE_VERSION);


    //  File system
    if (!LittleFS.begin()){
        Serial.printf("Error: Failed to initialize the filesystem!\r\n");
    }

    if (!loadSettings(appConfig)) {
        Serial.printf("Failed to load config, creating default settings...");
        defaultSettings();
    } else {
        debugln("Config loaded.");
    }

    //  GPIO
    pinMode(CONNECTION_STATUS_LED_GPIO, OUTPUT);
    digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);

    //  Buttons
    btnMode.setClickHandler(btnButtonClicked);
    btnMode.setLongClickHandler(btnButtonClicked);
    btnMode.setDoubleClickHandler(btnButtonClicked);
    btnMode.setTripleClickHandler(btnButtonClicked);


    //  OTA
    ArduinoOTA.onStart([]() {
    Serial.printf("OTA started.\r\n");
    });

    ArduinoOTA.onEnd([]() {
    Serial.printf("\nOTA finished.\r\n");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        if (progress % OTA_BLINKING_RATE == 0){
        if (digitalRead(CONNECTION_STATUS_LED_GPIO)==HIGH)
            digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
            else
            digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Authentication failed.");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin failed.");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect failed.");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive failed.");
        else if (error == OTA_END_ERROR) Serial.println("End failed.");
    });

    ArduinoOTA.begin();

    debugln();

    server.on("/", handleStatus);
    server.on("/status.html", handleStatus);
    server.on("/generalsettings.html", handleGeneralSettings);
    server.on("/lightsettings.html", handleLightSettings);
    server.on("/networksettings.html", handleNetworkSettings);
    server.on("/tools.html", handleTools);
    server.on("/login.html", handleLogin);

    server.onNotFound(handleNotFound);

    //  Start HTTP (web) server
    server.begin();
    debugln("HTTP server started.");

    //  Authenticate HTTP requests
    const char * headerkeys[] = {"User-Agent","Cookie"} ;
    size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
    server.collectHeaders(headerkeys, headerkeyssize );

    //  Timers
    os_timer_setfn(&heartbeatTimer, heartbeatTimerCallback, NULL);
    os_timer_arm(&heartbeatTimer, appConfig.heartbeatInterval * 1000, true);

    //  Randomizer
    SetRandomSeed();

    //  Neopixel
    strip.Begin();
    for (size_t i = 0; i < TOTAL_LEDS; i++){
        strip.ClearTo(RgbColor(0));
    }
    strip.Show();


    // Set the initial connection state
    connectionState = STATE_CHECK_WIFI_CONNECTION;

    Serial.println("Setup complete.");

}

void loop(){

  if (isAccessPoint){
    if (!isAccessPointCreated){
        Serial.printf("Could not connect to WiFI network %s.\r\nReverting to Access Point mode.\r\n", appConfig.ssid);

        delay(500);

        WiFi.mode(WiFiMode::WIFI_AP);
        sprintf(defaultSSID, "%s-%u", DEFAULT_MQTT_TOPIC, ESP.getChipId());
        WiFi.softAP(defaultSSID, DEFAULT_PASSWORD);

        IPAddress myIP;
        myIP = WiFi.softAPIP();
        isAccessPointCreated = true;

        if (MDNS.begin(defaultSSID)) debugln("MDNS responder started.");

        Serial.printf("Access point created. Use the following information to connect to the ESP device, then follow the on-screen instructions to connect to a different wifi network:\r\n\r\nSSID:\t\t\t%s\r\nPassword:\t\t%s\r\nAccess point address:\t", defaultSSID, DEFAULT_PASSWORD);
        Serial.println(myIP);

        Serial.println();
        Serial.println("Note: The device will reset in 5 minutes.");

        os_timer_setfn(&accessPointTimer, accessPointTimerCallback, NULL);
        os_timer_arm(&accessPointTimer, ACCESS_POINT_TIMEOUT, true);
        os_timer_disarm(&heartbeatTimer);
    }
    server.handleClient();
  }
  else{
    switch (connectionState) {

      // Check the WiFi connection
      case STATE_CHECK_WIFI_CONNECTION:

        // Are we connected ?
        if (WiFi.status() != WL_CONNECTED) {
          // Wifi is NOT connected
          digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
          connectionState = STATE_WIFI_CONNECT;
        } else  {
          // Wifi is connected so check Internet
          digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
          connectionState = STATE_CHECK_INTERNET_CONNECTION;

          server.handleClient();
        }
        break;

      // No Wifi so attempt WiFi connection
      case STATE_WIFI_CONNECT:
        {
          // Indicate NTP no yet initialized
          ntpInitialized = false;

          digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
          Serial.printf("Trying to connect to WIFI network: %s", appConfig.ssid);

          // Set station mode
          WiFi.mode(WIFI_STA);

          // Start connection process
          WiFi.hostname((String)appConfig.mqttTopic);
          WiFi.begin(appConfig.ssid, appConfig.password);

          // Initialize iteration counter
          uint8_t attempt = 0;

          while ((WiFi.status() != WL_CONNECTED) && (attempt++ < WIFI_CONNECTION_TIMEOUT)) {
            digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
            Serial.print(".");
            delay(50);
            digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
            delay(950);
          }
          if (attempt >= WIFI_CONNECTION_TIMEOUT) {
            Serial.println("\r\nCould not connect to WiFi");
            delay(100);

            isAccessPoint=true;

            break;
          }
          digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
          Serial.println(" Success!");
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          if (MDNS.begin(appConfig.mqttTopic)) debugln("MDNS responder started.");

          connectionState = STATE_CHECK_INTERNET_CONNECTION;
        }
        break;

      case STATE_CHECK_INTERNET_CONNECTION:

        // Do we have a connection to the Internet ?
        if (checkInternetConnection()) {
          // We have an Internet connection

          if (!ntpInitialized) {
            // We are connected to the Internet for the first time so set NTP provider
            initNTP();

            ntpInitialized = true;

            debugln("Connected to the Internet.");
          }

          connectionState = STATE_INTERNET_CONNECTED;
        } else  {
          connectionState = STATE_CHECK_WIFI_CONNECTION;
        }
        break;

      case STATE_INTERNET_CONNECTED:

        ArduinoOTA.handle();

        if (!PSclient.connected()) {
          PSclient.setServer(appConfig.mqttServer, appConfig.mqttPort);
          if (PSclient.connect(defaultSSID, (MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/STATE").c_str(), 0, true, "offline" )){
                PSclient.setBufferSize(10240);
                PSclient.setCallback(mqtt_callback);

                PSclient.subscribe((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/cmnd").c_str(), 0);

                PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/STATE").c_str(), "online", true);
                LogEvent(EVENTCATEGORIES::Conn, 1, "Node online", WiFi.localIP().toString());
          }
        }


        //  Button(s)
        btnMode.loop();    

        if (PSclient.connected()){
          PSclient.loop();
        }

        if (needsHeartbeat){
          SendHeartbeat();
          needsHeartbeat = false;
        }


        if (animations.IsAnimating()){
            animations.UpdateAnimations();
            strip.Show();
        }
        else
        {
            switch (appConfig.mode){
                case OPERATION_MODES::LED_CHASER:
                    animations.StartAnimation(0, NextPixelMoveDuration, ChaseAnimUpdate);
                    break;
                
                case OPERATION_MODES::SLOW_PANELS:
                    for (size_t panel = 0; panel < NUMBER_OF_PANELS; panel++){
                        RgbColor target = HslColor(random(360) / 360.0f, 1.0f, .5f);

                        slowPanelAnimationState[panel].StartingColor = strip.GetPixelColor(panel * LEDS_PER_PANEL);
                        slowPanelAnimationState[panel].EndingColor = target;

                        animations.StartAnimation(panel, 5000, BlendAnimUpdate);
                    }
                    break;

                case OPERATION_MODES::FAST_CHANGING_RANDOM_SEGMENTS:{
                    if ( fastSegmentsAnimationState.dir ){
                        fastSegmentsAnimationState.Segment = random(NUMBER_OF_PANELS) * SEGMENTS_PER_PANEL + random(SEGMENTS_PER_PANEL);
                        fastSegmentsAnimationState.StartingColor = RgbColor(0, 0, 0);
                        fastSegmentsAnimationState.EndingColor = HslColor(random(360) / 360.0f, 1.0f, .5f);
                    }
                    else{
                        fastSegmentsAnimationState.StartingColor = fastSegmentsAnimationState.EndingColor;
                        fastSegmentsAnimationState.EndingColor = RgbColor(0, 0, 0);
                    }

                    animations.StartAnimation(0, 50, FastSegmentsAnimUpdate);
                    fastSegmentsAnimationState.dir = !fastSegmentsAnimationState.dir;
                    break;
                }
                case OPERATION_MODES::ROTATING_PANELS:

                default:
                    break;
            }
        }

        // Set next connection state
        connectionState = STATE_CHECK_WIFI_CONNECTION;
        break;
    }

  }
}
