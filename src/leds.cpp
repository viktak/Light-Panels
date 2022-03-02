#include <Esp.h>

namespace leds
{
#define CONNECTION_STATUS_LED_GPIO 0

    void connectionLED_TOGGLE()
    {
        if (digitalRead(CONNECTION_STATUS_LED_GPIO) == LOW)
            digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
        else
            digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
    }

    void connectionLED_ON()
    {
        digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
    }

    void connectionLED_OFF()
    {
        digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
    }

    void setup()
    {
        pinMode(CONNECTION_STATUS_LED_GPIO, OUTPUT);
        digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
    }

}
