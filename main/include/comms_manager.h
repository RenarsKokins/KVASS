#pragma once

#include <stdlib.h>

enum CommsProtocol {
    USB,
    BLUETOOTH,
    ESPNOW,
};

struct CommsParameters {
    uint8_t keycode[6];
    uint8_t delta_x;
    uint8_t delta_y;
    enum CommsProtocol protocol;
};

void vCommsTask(void *commsParameters);