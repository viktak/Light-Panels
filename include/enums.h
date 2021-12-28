#ifndef ENUMS_H
#define ENUMS_H

enum BUTTON_STATES {
  BUTTON_NOT_PRESSED,    //  button not pressed
  BUTTON_BOUNCING,       //  button in not stable state
  BUTTON_PRESSED,        //  button pressed
  BUTTON_LONG_PRESSED    //  button pressed for at least BUTTON_LONG_PRESS_TRESHOLD milliseconds
};

enum ENCODER_MODES {
  SET_NUMBER_OF_LEDS = 0,
  SET_CENTER_POINT,
  SET_INTENSITY
};

//  Mode operational states
// const long TOTAL_OPERATION_MODES = 3;

enum OPERATION_MODES{
    LED_CHASER,
    SLOW_PANELS,
    FAST_CHANGING_RANDOM_SEGMENTS,
    ROTATING_PANELS,
    NUMBER_OF_OPERATION_MODES
};

#endif