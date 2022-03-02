#include <Arduino.h>

namespace logger
{
    void LogEvent(int Category, int ID, String Title, String Data)
    {
        //   todo
        // if (PSclient.connected())
        // {

        //     String msg = "{";

        //     msg += "\"Node\":" + (String)ESP.getChipId() + ",";
        //     msg += "\"Category\":" + (String)Category + ",";
        //     msg += "\"ID\":" + (String)ID + ",";
        //     msg += "\"Title\":\"" + Title + "\",";
        //     msg += "\"Data\":\"" + Data + "\"}";

        //     debugln(msg);

        //     PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/log").c_str(), msg.c_str(), false);
        // }
    }

}