#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "include/kb_interconnect_manager.h"

const char *TAG_INTERCONN = "Interconnect";

void vInterconnectTask(void *godParameters)
{
    struct GodParameters *params = (struct GodParameters*) godParameters;

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}