#pragma once

#include <stdlib.h>
#include "driver/gpio.h"

#include "comms_manager.h"

#define JOYSTICK_LR_ADC ADC1_CHANNEL_7
#define JOYSTICK_UD_ADC ADC2_CHANNEL_3
#define JOYSTICK_BTN_GPIO GPIO_NUM_9

#define KB_ROW_0_GPIO GPIO_NUM_33
#define KB_ROW_1_GPIO GPIO_NUM_37
#define KB_ROW_2_GPIO GPIO_NUM_38
#define KB_ROW_3_GPIO GPIO_NUM_34
#define KB_ROW_4_GPIO GPIO_NUM_21

#define KB_COL_0_GPIO GPIO_NUM_17
#define KB_COL_1_GPIO GPIO_NUM_7
#define KB_COL_2_GPIO GPIO_NUM_6
#define KB_COL_3_GPIO GPIO_NUM_5
#define KB_COL_4_GPIO GPIO_NUM_3
#define KB_COL_5_GPIO GPIO_NUM_1
#define KB_COL_6_GPIO GPIO_NUM_16

#define KB_COLS 7
#define KB_ROWS 5

struct GpioParameters
{
    struct CommsParameters *commsParameters;
};

void vGpioTask(void *gpioParameters);