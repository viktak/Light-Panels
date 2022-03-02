#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>

namespace mqtt
{
    extern PubSubClient PSclient();

    extern void ConnectToMQTTBroker();
    extern void SendHeartbeat();

    extern void setup();
    extern void loop();

}

#endif