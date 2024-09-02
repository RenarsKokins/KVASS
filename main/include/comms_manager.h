#pragma once

#include <stdlib.h>
#include <semphr.h>

#define KB_BUFFER_SIZE 6

enum CommsProtocol
{
    USB,
    BLUETOOTH,
    ESPNOW,
    NONE,
};

struct CommsData
{
    // Keyboard
    uint8_t modifier;
    uint8_t keycode[KB_BUFFER_SIZE];

    // Mouse
    uint8_t button;
    uint8_t delta_x;
    uint8_t delta_y;
    uint8_t scroll_vertical;
    uint8_t scroll_horizontal;
};

struct CommsParameters
{
    SemaphoreHandle_t protocolChanged;
    SemaphoreHandle_t hidChanged;
    SemaphoreHandle_t mouseChanged;
    SemaphoreHandle_t keyboardChanged;

    enum CommsProtocol protocol;
    struct CommsData commsData;
};

void vCommsTask(void *commsParameters);