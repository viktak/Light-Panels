#include "Button2.h"

#include "common.h"
#include "settings.h"
#include "ledstrip.h"

#define MODE_BUTTON_GPIO 2

Button2 btnMode = Button2(MODE_BUTTON_GPIO);

void btnButtonClicked(Button2 &btn)
{
    if (btn == btnMode)
    {
        switch (btn.getType())
        {
        case single_click:
        {
            long m = settings::mode + 1;
            Serial.println("SINGLE_CLICK");
            settings::mode = m % settings::NUMBER_OF_OPERATION_MODES;
            settings::SaveSettings();
            StopAnimations();
            break;
        }
        case double_click:
            Serial.println("DOUBLE_CLICK");
            break;
        case triple_click:
            Serial.println("TRIPLE_CLICK");
            break;
        case long_click:
            Serial.println("LONG_CLICK");
            break;
        default:
            break;
        }
    }
}

void setupButtons()
{
    btnMode.setClickHandler(btnButtonClicked);
    btnMode.setLongClickHandler(btnButtonClicked);
    btnMode.setDoubleClickHandler(btnButtonClicked);
    btnMode.setTripleClickHandler(btnButtonClicked);
}

void loopButtons()
{
    btnMode.loop();
}
