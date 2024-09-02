/*
KVASS - firmware for RKBoard split keyboard
*/

#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

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
    static struct CommsParameters uCommsParameters = {0};
    static struct GpioParameters uGpioParameters = {0};
    static struct InterconnectParameters uInterconnectParameters = {0};

    // Init parameters
    uCommsParameters.protocol = USB;
    uCommsParameters.protocolChanged = xSemaphoreCreateBinary();
    uCommsParameters.keyboardChanged = xSemaphoreCreateBinary();
    uCommsParameters.mouseChanged = xSemaphoreCreateBinary();
    uCommsParameters.hidChanged = xSemaphoreCreateBinary();

    uGpioParameters.commsParameters = &uCommsParameters;
    uInterconnectParameters.commsParameters = &uCommsParameters;

    // define task handles
    TaskHandle_t commsHandle = NULL;
    TaskHandle_t gpioHandle = NULL;
    TaskHandle_t interconnectHandle = NULL;

    TaskStatus_t taskStatus = {0};

    ESP_LOGI(TAG, "RKBoard initializing...");

    // read board config
    esp_err_t err = initConfigManager();

    if (err)
    {
        ESP_LOGE(TAG, "Cannot initialize NVS!");
    }

    xTaskCreate(vCommsTask, "commsTask", CONFIG_TINYUSB_TASK_STACK_SIZE, &uCommsParameters, 9, &commsHandle);
    configASSERT(commsHandle);
    xTaskCreate(vGpioTask, "gpioTask", 3072, &uGpioParameters, 9, &gpioHandle);
    configASSERT(gpioHandle);
    xTaskCreate(vInterconnectTask, "interconnectTask", 3072, &uInterconnectParameters, 9, &interconnectHandle);
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
