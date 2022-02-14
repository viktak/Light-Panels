#ifndef DEFINES_H
#define DEFINES_H

#define MQTT_CUSTOMER "viktak"
#define MQTT_PROJECT  "spiti"

#define HARDWARE_ID "LightPanel Controller"
#define HARDWARE_VERSION "1.0"
#define SOFTWARE_ID "LightPanel - Triangle"

#define SerialMon Serial

#define DEBUG_SPEED 921600

#define JSON_SETTINGS_SIZE (JSON_OBJECT_SIZE(10) + 200)
#define JSON_MQTT_COMMAND_SIZE 300

#define CONNECTION_STATUS_LED_GPIO 0
#define MODE_BUTTON_GPIO 2

#define DEFAULT_TIMEZONE                        2

#define WIFI_CONNECTION_TIMEOUT                 30
#define NTP_REFRESH_INTERVAL 3600

#define DEFAULT_MQTT_SERVER                     "test.mosquitto.org"
#define DEFAULT_MQTT_PORT                       1883
#define DEFAULT_MQTT_TOPIC                      "vnode"

#define DEFAULT_HEARTBEAT_INTERVAL              300 //  seconds

#define NUMBER_OF_PANELS 6
#define SEGMENTS_PER_PANEL 3
#define LEDS_PER_SEGMENT 8

#define LEDS_PER_PANEL SEGMENTS_PER_PANEL * LEDS_PER_SEGMENT
#define TOTAL_SEGMENTS SEGMENTS_PER_PANEL * NUMBER_OF_PANELS
#define TOTAL_LEDS NUMBER_OF_PANELS * SEGMENTS_PER_PANEL * LEDS_PER_SEGMENT


#endif