#include "Button2.h"

#include "common.h"
#include "settings.h"
#include "ledstrip.h"

#define MODE_BUTTON_GPIO 2

namespace buttons
{

    Button2 btnMode = Button2(MODE_BUTTON_GPIO);

    void btnButtonClicked(Button2 &btn)
    {
        if (btn == btnMode)
        {
            switch (btn.getClickType())
            {
            case SINGLE_CLICK:
            {
                long m = settings::mode + 1;
                Serial.println("SINGLE_CLICK");
                settings::mode = m % settings::NUMBER_OF_OPERATION_MODES;
                settings::SaveSettings();
                ledstrip::StopAnimations();
                break;
            }
            case DOUBLE_CLICK:
                Serial.println("DOUBLE_CLICK");
                break;
            case TRIPLE_CLICK:
                Serial.println("TRIPLE_CLICK");
                break;
            case LONG_CLICK:
                Serial.println("LONG_CLICK");
                break;
            default:
                break;
            }
        }
    }

    void setup()
    {
        btnMode.setClickHandler(btnButtonClicked);
        btnMode.setLongClickHandler(btnButtonClicked);
        btnMode.setDoubleClickHandler(btnButtonClicked);
        btnMode.setTripleClickHandler(btnButtonClicked);
    }

    void loop()
    {
        btnMode.loop();
    }

}