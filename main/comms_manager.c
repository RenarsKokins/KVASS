#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"

#include "common_kvass.h"
#include "comms_manager.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(HID, 1) | _PID_MAP(VENDOR, 2))
#define USB_VID 0x303a
#define USB_BCD 0x200

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)

const char *TAG_COMMS = "comms";

const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))};

tusb_desc_device_t const desc_device =
    {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = USB_BCD,
        .bDeviceClass = 0x00,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor = USB_VID,
        .idProduct = USB_PID,
        .bcdDevice = 0x0100,

        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,

        .bNumConfigurations = 0x01};

const char *hid_string_descriptor[6] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},        // 0: supported language is English (0x0409)
    "RKBoard",                   // 1: Manufacturer
    "RKBoard v1.0",              // 2: Product
    "C0FFEE",                    // 3: Serials, should use chip ID
    "Split keyboard with mouse", // 4: HID
    "RKBoard debug"              // 5: CDC debug channel
};

enum
{
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_HID,
    ITF_TOTAL,
};

static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 200),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_HID, 4, HID_ITF_PROTOCOL_NONE, sizeof(hid_report_descriptor), 0x81, 64, 5),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 5, 0x83, 8, 0x04, 0x84, 64),
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    ESP_LOGI(TAG_COMMS, "Set report: instance: %d, report_id: %d, report_type: %d, buffer: %d, bufsize: %d", instance, report_id, report_type, buffer[0], bufsize);
}

void doUSBCommunication(struct GodParameters *godParameters)
{
    BaseType_t xResult;
    uint32_t notifyValue = 0;
    struct MouseData mouseData = {0};
    struct KeyboardData kbData = {0};
    struct CommsParameters *commsParams = (struct CommsParameters *)godParameters->commsParameters;
    ESP_LOGI(TAG_COMMS, "USB communication selected");

    // Setup HID config
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &desc_device,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Setup CDC config
    tinyusb_config_cdcacm_t acm_cfg = {0};
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    // Init CDC console
    esp_tusb_init_console(TINYUSB_CDC_ACM_0);

    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_COMMS, "USB initialization DONE");

    // Send data to host device
    while (1)
    {
        xResult = xTaskNotifyWait(pdFALSE,      /* Don't clear bits on entry. */
                                  UINT32_MAX,   /* Clear bits on exit. */
                                  &notifyValue, /* Stores the notified value. */
                                  portMAX_DELAY);
        // If protocol changed, break out
        if (xResult == pdTRUE && (notifyValue & NOTIF_PROTOCOL_CHANGED) != 0)
        {
            break;
        }

        // Check if we can communicate
        if (tud_mounted())
        {
            // block unitl HID has data
            if (xResult == pdTRUE && (notifyValue & NOTIF_KEYB_CHANGED) != 0)
            {
                xResult = xQueueReceive(commsParams->commsData.keyboardQueue, &kbData, 10);
                if (xResult == pdTRUE)
                {
                    tud_hid_keyboard_report(
                        HID_ITF_PROTOCOL_KEYBOARD,
                        kbData.modifier,
                        kbData.keycode);

                    // ESP_LOGI(TAG_COMMS, "Sent keyboard report!");
                    // ESP_LOGI(TAG_COMMS, "Keyboard mod: %d, buffer: ", kbData.modifier);
                    // for (int i = 0; i < KB_BUFFER_SIZE; i++)
                    // {
                    //     printf("%u", kbData.keycode[i]);
                    // }
                    // printf("\n");
                }
            }
            else if (xResult == pdTRUE && (notifyValue & NOTIF_MOUSE_CHANGED) != 0)
            {
                xResult = xQueueReceive(commsParams->commsData.mouseQueue, &mouseData, 10);
                if (xResult == pdTRUE)
                {
                    tud_hid_mouse_report(
                        HID_ITF_PROTOCOL_MOUSE,
                        mouseData.button,
                        mouseData.delta_x,
                        mouseData.delta_y,
                        mouseData.scroll_vertical,
                        mouseData.scroll_horizontal);

                    // ESP_LOGI(TAG_COMMS, "Sent mouse report!");
                    // ESP_LOGI(TAG_COMMS, "Mouse button: %d, x: %d, y:, %d",
                    //          mouseData.button,
                    //          mouseData.delta_x,
                    //          mouseData.delta_y);
                }
            }
        }
    }

    // Close USB
    ESP_ERROR_CHECK(tinyusb_driver_uninstall());
}

void doBluetoothCommunication(struct GodParameters *godParameters)
{
    BaseType_t xResult;
    uint32_t notifyValue = 0;
    struct CommsParameters *commsParams = (struct CommsParameters *)godParameters->commsParameters;

    ESP_LOGI(TAG_COMMS, "Bluetooth communication selected");
    while (1)
    {
        xResult = xTaskNotifyWait(pdFALSE,                /* Don't clear bits on entry. */
                                  NOTIF_PROTOCOL_CHANGED, /* Clear notif bit on exit. */
                                  &notifyValue,           /* Stores the notified value. */
                                  portMAX_DELAY);
        if (xResult == pdPASS && (notifyValue & NOTIF_PROTOCOL_CHANGED) != 0)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void doESPNOWCommunication(struct GodParameters *godParameters)
{
    BaseType_t xResult;
    uint32_t notifyValue = 0;
    struct CommsParameters *commsParams = (struct CommsParameters *)godParameters->commsParameters;

    ESP_LOGI(TAG_COMMS, "ESPNOW communication selected");
    while (1)
    {
        xResult = xTaskNotifyWait(pdFALSE,                /* Don't clear bits on entry. */
                                  NOTIF_PROTOCOL_CHANGED, /* Clear notif bit on exit. */
                                  &notifyValue,           /* Stores the notified value. */
                                  portMAX_DELAY);
        if (xResult == pdPASS && (notifyValue & NOTIF_PROTOCOL_CHANGED) != 0)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void waitForProtocolChange(struct GodParameters *godParameters)
{
    BaseType_t xResult;
    uint32_t notifyValue = 0;
    struct CommsParameters *commsParams = (struct CommsParameters *)godParameters->commsParameters;

    ESP_LOGI(TAG_COMMS, "No communication protocol selected, will wait for protocol change");
    while (1)
    {
        xResult = xTaskNotifyWait(pdFALSE,                /* Don't clear bits on entry. */
                                  NOTIF_PROTOCOL_CHANGED, /* Clear notif bit on exit. */
                                  &notifyValue,           /* Stores the notified value. */
                                  portMAX_DELAY);
        if (xResult == pdPASS && (notifyValue & NOTIF_PROTOCOL_CHANGED) != 0)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vCommsTask(void *godParameters)
{
    ESP_LOGI(TAG_COMMS, "Initializing communications task...");

    struct GodParameters *params = (struct GodParameters *)(godParameters);
    struct CommsParameters *commsParams = (struct CommsParameters *)(params->commsParameters);

    commsParams->commsData.mouseQueue = xQueueCreate(KB_QUEUE_SIZE, sizeof(struct MouseData));
    commsParams->commsData.keyboardQueue = xQueueCreate(MOUSE_QUEUE_SIZE, sizeof(struct KeyboardData));

    if (commsParams->commsData.mouseQueue == 0 || commsParams->commsData.keyboardQueue == 0)
    {
        ESP_LOGE(TAG_COMMS, "Failed to create queues!");
    }

    while (1)
    {
        switch (commsParams->protocol)
        {
        case USB:
            doUSBCommunication(params);
            break;
        case BLUETOOTH:
            doBluetoothCommunication(params);
            break;
        case ESPNOW:
            doESPNOWCommunication(params);
            break;
        case NONE:
            waitForProtocolChange(params);
            break;
        }
    }
}