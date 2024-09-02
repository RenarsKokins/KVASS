#pragma once

#include <stdlib.h>

#define KB_BUFFER_SIZE 6
#define KB_QUEUE_SIZE 10
#define MOUSE_QUEUE_SIZE 10

enum CommsProtocol
{
    USB,
    BLUETOOTH,
    ESPNOW,
    NONE,
};

struct KeyboardData
{
    uint8_t modifier;
    uint8_t keycode[KB_BUFFER_SIZE];
};

struct MouseData
{
    uint8_t button;
    uint8_t delta_x;
    uint8_t delta_y;
    uint8_t scroll_vertical;
    uint8_t scroll_horizontal;
};

struct CommsData
{
    QueueHandle_t mouseQueue;
    QueueHandle_t keyboardQueue;
};

struct CommsParameters
{
    TaskHandle_t *commsTask;

    enum CommsProtocol protocol;
    struct CommsData commsData;
};

void vCommsTask(void *godParameters);