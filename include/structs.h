struct config{
    char ssid[22];
    char password[32];

    char friendlyName[30];
    u_int heartbeatInterval;

    unsigned long timeZone;

    char mqttServer[64];
    int mqttPort;
    char mqttTopic[32];

    bool dst;

    long mode;
};


struct digitalInput{
    char gpio;
    const char* name;
    char32_t buttonPattern;
    BUTTON_STATES buttonState;
};


#ifdef WS2812B
    struct ChaserAnimationState{
        RgbColor StartingColor;
        RgbColor EndingColor;
        uint16_t IndexPixel; // which pixel this animation is effecting
    };
#endif

#ifdef SK6812
    struct ChaserAnimationState{
        RgbwColor StartingColor;
        RgbwColor EndingColor;
        uint16_t IndexPixel; // which pixel this animation is effecting
    };
#endif

#ifdef WS2812B
    struct SlowPanelsAnimationState{
        RgbColor StartingColor;
        RgbColor EndingColor;
    };
#endif

#ifdef SK6812
    struct SlowPanelsAnimationState{
        RgbwColor StartingColor;
        RgbwColor EndingColor;
    };
#endif


#ifdef WS2812B
    struct FastSegmentsAnimationState{
        RgbColor StartingColor;
        RgbColor EndingColor;
        uint16_t Segment;
        bool dir;
    };
#endif

#ifdef SK6812
    struct FastSegmentsAnimationState{
        RgbwColor StartingColor;
        RgbwColor EndingColor;
        uint16_t Segment;
        bool dir;
    };
#endif

