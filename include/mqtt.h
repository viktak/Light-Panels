#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>

#include "network.h"

namespace mqtt
{
    extern PubSubClient PSclient;
    
    extern const char *mqttCustomer;
    extern const char *mqttProject;

    extern void ConnectToMQTTBroker();
    extern void SendHeartbeat();

    extern void setup();
    extern void loop();

}

#endif