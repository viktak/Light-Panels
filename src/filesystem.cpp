#include <LittleFS.h>

#define FORMAT_SPIFFS_IF_FAILED true

void setupFileSystem()
{
    if (!LittleFS.begin())
    {
        Serial.println("Error: Failed to initialize the filesystem!");
    }
}
