#include "esp_log.h"

#include "config_manager.h"

nvs_handle_t handle;
const char *TAG_CFG = "config";

esp_err_t initConfigManager()
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    return err;
}

esp_err_t getKbSide(int8_t *side)
{
    ESP_LOGI(TAG_CFG, "Opening Non-Volatile Storage (NVS)... ");
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_CFG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_i8(handle, CFG_KB_SIDE, side);
    switch (err)
    {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG_CFG, "The value is not initialized yet!");
            err = nvs_set_i8(handle, CFG_KB_SIDE, 0);
            if (err)
            {
                ESP_LOGW(TAG_CFG, "Cannot set initial value!");
            }

            err = nvs_commit(handle);
            if (err)
            {
                ESP_LOGW(TAG_CFG, "Cannot commit value!");
            }
            break;
        default:
            ESP_LOGE(TAG_CFG, "Error (%s) reading kb side!", esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

esp_err_t setKbSide(int8_t side)
{
    ESP_LOGI(TAG_CFG, "Opening Non-Volatile Storage (NVS)... ");
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_CFG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_i8(handle, CFG_KB_SIDE, side ? 1 : 0);
    if (err)
    {
        ESP_LOGW(TAG_CFG, "Cannot set kb side value!");
    }

    err = nvs_commit(handle);
    if (err)
    {
        ESP_LOGW(TAG_CFG, "Cannot commit value!");
    }

    nvs_close(handle);
    return err;
}