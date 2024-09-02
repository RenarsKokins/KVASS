#pragma once

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define CFG_KB_SIDE "side"

enum {
    CFG_KB_SIDE_LEFT,
    CFG_KB_SIDE_RIGHT,
};

esp_err_t initConfigManager();
esp_err_t getKbSide(int8_t *side);
esp_err_t setKbSide(int8_t side);

