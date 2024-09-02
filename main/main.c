/*
KVASS - firmware for RKBoard split keyboard
*/

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "include/common_kvass.h"
#include "include/comms_manager.h"
#include "include/gpio_manager.h"
#include "include/config_manager.h"
#include "include/kb_interconnect_manager.h"

const char *TAG = "main";

/*
requirements
- Communications task (responisble for communication with PC by USB, bluetooth or ESP-NOW)
- SPI task which recieves info and passes the buffer to HID packet creator task task
- Display task (responsible for GUI)
- keypress task (responsible for keypresses, microswitches and joystick)
- LED task (status led)
*/
void app_main(void)
{
    TaskStatus_t taskStatus = {0};

    volatile static struct GodParameters uGodParameters = {0};
    volatile static struct GpioParameters uGpioParameters = {0};
    volatile static struct CommsParameters uCommsParameters = {0};
    volatile static struct InterconnectParameters uInterconnectParameters = {0};

    // define task handles
    TaskHandle_t gpioHandle = NULL;
    TaskHandle_t commsHandle = NULL;
    TaskHandle_t interconnectHandle = NULL;

    ESP_LOGI(TAG, "RKBoard initializing...");

    // read board config
    esp_err_t err = initConfigManager();

    if (err)
    {
        ESP_LOGE(TAG, "Cannot initialize NVS!");
    }

    // Init parameters
    uCommsParameters.protocol = USB;

    uGpioParameters.gpioTask = &gpioHandle;
    uCommsParameters.commsTask = &commsHandle;
    uInterconnectParameters.interconnectTask = &interconnectHandle;

    uGodParameters.gpioParameters = (void*)&uGpioParameters;
    uGodParameters.commsParameters = (void*)&uCommsParameters;
    uGodParameters.interconnectParameters = (void*)&uInterconnectParameters;


    xTaskCreate(vCommsTask, "commsTask", CONFIG_TINYUSB_TASK_STACK_SIZE, &uGodParameters, 9, &commsHandle);
    configASSERT(commsHandle);
    xTaskCreate(vGpioTask, "gpioTask", 3072, &uGodParameters, 9, &gpioHandle);
    configASSERT(gpioHandle);
    xTaskCreate(vInterconnectTask, "interconnectTask", 3072, &uGodParameters, 9, &interconnectHandle);
    configASSERT(interconnectHandle);

    while (1)
    {
        vTaskGetInfo(commsHandle, &taskStatus, pdTRUE, eInvalid);
        ESP_LOGI(TAG, "commsTask state: %d, freeStackSpace: %lu, runtime: %lu", taskStatus.eCurrentState, taskStatus.usStackHighWaterMark, taskStatus.ulRunTimeCounter);

        vTaskGetInfo(gpioHandle, &taskStatus, pdTRUE, eInvalid);
        ESP_LOGI(TAG, "gpioTask state: %d, freeStackSpace: %lu, runtime: %lu", taskStatus.eCurrentState, taskStatus.usStackHighWaterMark, taskStatus.ulRunTimeCounter);

        vTaskGetInfo(interconnectHandle, &taskStatus, pdTRUE, eInvalid);
        ESP_LOGI(TAG, "interconnectTask state: %d, freeStackSpace: %lu, runtime: %lu", taskStatus.eCurrentState, taskStatus.usStackHighWaterMark, taskStatus.ulRunTimeCounter);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
