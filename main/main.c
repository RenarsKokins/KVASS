/*
KVASS - firmware for RKBoard split keyboard
*/

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "include/comms_manager.h"


const char *TAG = "main";

/*
requirements
- Communications task (responisble for communication with PC by USB, bluetooth or ESP-NOW)
- HID packet creator task (compiles necessary info to send to PC by comms task)
- SPI task which recieves info and passes the buffer to HID packet creator task task
- Display task (responsible for GUI)
- keypress task (responsible for keypresses, microswitches and joystick)
- LED task (status led)
*/
void app_main(void)
{
    static uint8_t ucCommsParameters;
    TaskHandle_t commsHandle = NULL;

    ESP_LOGI(TAG, "RKBoard initializing...");

    xTaskCreate(vCommsTask, "commsTask", CONFIG_TINYUSB_TASK_STACK_SIZE, &ucCommsParameters, 9, &commsHandle);
    configASSERT(commsHandle);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
