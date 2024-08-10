#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"

#include "comms_manager.h"


#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)
#define DISTANCE_MAX        250
#define DELTA_SCALAR        1

const char *TAG_COMMS = "comms";

const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE))
};

const char* hid_string_descriptor[6] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: supported language is English (0x0409)
    "RKBoard",             // 1: Manufacturer
    "RKBoard v1.0",        // 2: Product
    "esp32s3_FIXME",       // 3: Serials, should use chip ID
    CONFIG_TINYUSB_DESC_CDC_STRING, // 4: CDC
    "Split keyboard with mouse",  // 5: HID
};

// CDC callback buffer
static uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE + 1];

static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 3, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 200),

    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, HID_ITF_PROTOCOL_NONE, sizeof(hid_report_descriptor), 0x83, 16, 5),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(1, 5, 0x81, 8, 0x02, 0x82, 64),
};


uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
}

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    size_t rx_size = 0;
    esp_err_t ret = tinyusb_cdcacm_read(itf, buf, CFG_TUD_CDC_RX_BUFSIZE, &rx_size);
    if (ret == ESP_OK)
    {
        // ESP_LOGI(TAG_COMMS, "Data from channel %d:", itf);
        // ESP_LOG_BUFFER_HEXDUMP(TAG_COMMS, buf, rx_size, ESP_LOG_DEBUG);
    }
    tinyusb_cdcacm_write_queue(itf, buf, rx_size);
    tinyusb_cdcacm_write_flush(itf, 0);
}

void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    int dtr = event->line_state_changed_data.dtr;
    int rts = event->line_state_changed_data.rts;
    ESP_LOGI(TAG_COMMS, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}

typedef enum {
    MOUSE_DIR_RIGHT,
    MOUSE_DIR_DOWN,
    MOUSE_DIR_LEFT,
    MOUSE_DIR_UP,
    MOUSE_DIR_MAX,
} mouse_dir_t;

static void mouse_draw_square_next_delta(int8_t *delta_x_ret, int8_t *delta_y_ret)
{
    static mouse_dir_t cur_dir = MOUSE_DIR_RIGHT;
    static uint32_t distance = 0;

    // Calculate next delta
    if (cur_dir == MOUSE_DIR_RIGHT) {
        *delta_x_ret = DELTA_SCALAR;
        *delta_y_ret = 0;
    } else if (cur_dir == MOUSE_DIR_DOWN) {
        *delta_x_ret = 0;
        *delta_y_ret = DELTA_SCALAR;
    } else if (cur_dir == MOUSE_DIR_LEFT) {
        *delta_x_ret = -DELTA_SCALAR;
        *delta_y_ret = 0;
    } else if (cur_dir == MOUSE_DIR_UP) {
        *delta_x_ret = 0;
        *delta_y_ret = -DELTA_SCALAR;
    }

    // Update cumulative distance for current direction
    distance += DELTA_SCALAR;
    // Check if we need to change direction
    if (distance >= DISTANCE_MAX) {
        distance = 0;
        cur_dir++;
        if (cur_dir == MOUSE_DIR_MAX) {
            cur_dir = 0;
        }
    }
}

static void app_send_hid_demo(void)
{
    uint8_t keycode[6] = {HID_KEY_A};
    keycode[0] = HID_KEY_SHIFT_LEFT;

    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, keycode);
    vTaskDelay(pdMS_TO_TICKS(50));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);

    int8_t delta_x;
    int8_t delta_y;
    for (int i = 0; i < (DISTANCE_MAX / DELTA_SCALAR) * 4; i++) {
        // Get the next x and y delta in the draw square pattern
        mouse_draw_square_next_delta(&delta_x, &delta_y);
        tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, 0x00, delta_x, delta_y, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void doUSBCommunication(struct CommsParameters *CommsParameters)
{
    ESP_LOGI(TAG_COMMS, "USB communication selected");

    // Setup HID config
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif // TUD_OPT_HIGH_SPEED
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Setup CDC config
    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_cdc_rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL,
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    // add CDC callback
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
        TINYUSB_CDC_ACM_0,
        CDC_EVENT_LINE_STATE_CHANGED,
        &tinyusb_cdc_line_state_changed_callback));

    // Init CDC console
    esp_tusb_init_console(TINYUSB_CDC_ACM_0);

    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_COMMS, "USB initialization DONE");

    // Send data to host device
    while (1) {
        if (tud_mounted()) {
            static bool send_hid_data = true;
            if (send_hid_data) {
                app_send_hid_demo();
            }
            send_hid_data = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void doBluetoothCommunication(struct CommsParameters *CommsParameters)
{
    ESP_LOGI(TAG_COMMS, "Bluetooth communication selected");
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void doESPNOWCommunication(struct CommsParameters *CommsParameters)
{
    ESP_LOGI(TAG_COMMS, "ESPNOW communication selected");
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vCommsTask(void *commsParameters)
{
    struct CommsParameters *params = (struct CommsParameters*)(commsParameters);
    ESP_LOGI(TAG_COMMS, "Initializing communications task...");
    while(1)
    {
        switch (params->protocol)
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
        }
    }
}