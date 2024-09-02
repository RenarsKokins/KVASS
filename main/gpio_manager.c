#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "class/hid/hid.h"

#include "gpio_manager.h"
#include "common_utils.h"
#include "config_manager.h"

#define JOYSTICK_SAMPLE_COUNT 16

const char *TAG_GPIO = "GPIO";

const uint8_t keyboard_layouts[][KB_ROWS][KB_COLS] = {
    // Layout 1 (left side)
    {
        {HID_KEY_ESCAPE, HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_BACKSPACE},
        {HID_KEY_TAB, HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R, HID_KEY_T, HID_KEY_ENTER},
        {HID_KEY_CAPS_LOCK, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G, HID_KEY_PAGE_UP},
        {HID_KEY_SHIFT_LEFT, HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B, HID_KEY_PAGE_DOWN},
        {HID_KEY_CONTROL_LEFT, HID_KEY_NONE, HID_KEY_NONE, HID_KEY_ARROW_UP, HID_KEY_ARROW_DOWN, HID_KEY_GUI_LEFT, HID_KEY_NONE},
    },

    // Layout 2 (right side)
    {
        {HID_KEY_MINUS, HID_KEY_0, HID_KEY_9, HID_KEY_8, HID_KEY_7, HID_KEY_6, HID_KEY_SPACE},
        {HID_KEY_EQUAL, HID_KEY_P, HID_KEY_O, HID_KEY_I, HID_KEY_U, HID_KEY_Y, HID_KEY_BACKSPACE},
        {HID_KEY_APOSTROPHE, HID_KEY_SEMICOLON, HID_KEY_L, HID_KEY_K, HID_KEY_J, HID_KEY_H, HID_KEY_DELETE},
        {HID_KEY_BACKSLASH, HID_KEY_SLASH, HID_KEY_PERIOD, HID_KEY_COMMA, HID_KEY_M, HID_KEY_N, HID_KEY_PRINT_SCREEN},
        {HID_KEY_ALT_RIGHT, HID_KEY_BRACKET_RIGHT, HID_KEY_BRACKET_LEFT, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_LEFT, HID_KEY_CONTROL_RIGHT, HID_KEY_NONE},
    }
};

enum
{
    LAYOUT_LEFT_1 = 0,
    LAYOUT_RIGHT_1,
    LAYOUT_COUNT,
};

const uint8_t cols[KB_COLS] = {
    KB_COL_0_GPIO,
    KB_COL_1_GPIO,
    KB_COL_2_GPIO,
    KB_COL_3_GPIO,
    KB_COL_4_GPIO,
    KB_COL_5_GPIO,
    KB_COL_6_GPIO,
};

const uint8_t rows[KB_ROWS] = {
    KB_ROW_0_GPIO,
    KB_ROW_1_GPIO,
    KB_ROW_2_GPIO,
    KB_ROW_3_GPIO,
    KB_ROW_4_GPIO,
};

void populateModifiers(uint8_t *modifier, bool press_matrix[KB_ROWS][KB_COLS], int layout)
{
    // Clear modifiers
    *modifier = 0;

    // Add currently pressed modifers
    for (int i = 0; i < KB_COLS; i++)
    {
        for (int j = 0; j < KB_ROWS; j++)
        {
            if (!press_matrix[j][i])
            {
                continue;
            }

            switch (keyboard_layouts[layout][j][i])
            {
            case HID_KEY_CONTROL_LEFT:
                *modifier = *modifier | 1;
                break;
            case HID_KEY_SHIFT_LEFT:
                *modifier = *modifier | (1 << 1);
                break;
            case HID_KEY_ALT_LEFT:
                *modifier = *modifier | (1 << 2);
                break;
            case HID_KEY_GUI_LEFT:
                *modifier = *modifier | (1 << 3);
                break;
            case HID_KEY_CONTROL_RIGHT:
                *modifier = *modifier | (1 << 4);
                break;
            case HID_KEY_SHIFT_RIGHT:
                *modifier = *modifier | (1 << 5);
                break;
            case HID_KEY_ALT_RIGHT:
                *modifier = *modifier | (1 << 6);
                break;
            case HID_KEY_GUI_RIGHT:
                *modifier = *modifier | (1 << 7);
                break;
            }
        }
    }
}

int getCurrentLayout()
{
    static bool initialized = false;
    static int8_t side = 0;

    if (!initialized)
    {
        esp_err_t err = getKbSide(&side);
        initialized = true;
    }

    return side;
}

void populateKeys(uint8_t keycode[KB_BUFFER_SIZE], bool press_matrix[KB_ROWS][KB_COLS], int layout)
{
    static List *list = NULL;
    if (list == NULL)
    {
        list = makeLinkedList();
    }

    for (int i = 0; i < KB_COLS; i++)
    {
        for (int j = 0; j < KB_ROWS; j++)
        {
            if (press_matrix[j][i])
            {
                // Key pressed
                if (!existsInLL(keyboard_layouts[layout][j][i], list))
                {
                    addLLElement(keyboard_layouts[layout][j][i], list);
                }
                ESP_LOGW(TAG_GPIO, "Key [%d,%d] is pressed", j, i);
            }
            else
            {
                // Key released
                deleteLLElement(keyboard_layouts[layout][j][i], list);
            }
        }
    }

    Node *element = list->head;
    memset(keycode, 0, KB_BUFFER_SIZE);
    for (int i = 0; i < KB_BUFFER_SIZE; i++)
    {
        if (element == NULL)
        {
            break;
        }
        keycode[i] = element->data;
        element = element->next;
    }
}

bool scanKeys(struct GpioParameters *params)
{
    int level = false;
    bool changed = false;
    static bool press_matrix[KB_ROWS][KB_COLS] = {0};

    for (int i = 0; i < KB_COLS; i++)
    {
        gpio_set_level(cols[i], 1);
        for (int j = 0; j < KB_ROWS; j++)
        {
            level = gpio_get_level(rows[j]);

            if (press_matrix[j][i] != level)
            {
                changed = true;
            }
            press_matrix[j][i] = level;
        }
        gpio_set_level(cols[i], 0);
    }

    if (changed)
    {
        populateModifiers(&params->commsParameters->commsData.modifier, press_matrix, getCurrentLayout());
        populateKeys(params->commsParameters->commsData.keycode, press_matrix, getCurrentLayout());
        xSemaphoreGive(params->commsParameters->hidChanged);
        ESP_LOGI(TAG_GPIO, "Key changed!");
    }

    return changed;
}

void scanJoystick(struct GpioParameters *params)
{
    int btn_pressed = false;
    int raw_value_adc1 = 0, raw_value_adc2 = 0, temp_raw = 0;

    for (int i = 0; i < JOYSTICK_SAMPLE_COUNT; i++)
    {
        raw_value_adc1 = raw_value_adc1 + (adc1_get_raw(JOYSTICK_LR_ADC) / JOYSTICK_SAMPLE_COUNT);
        adc2_get_raw(JOYSTICK_UD_ADC, ADC_WIDTH_BIT_DEFAULT, &temp_raw);
        raw_value_adc2 = raw_value_adc2 + (temp_raw / JOYSTICK_SAMPLE_COUNT);
    }

    btn_pressed = gpio_get_level(JOYSTICK_BTN_GPIO);

    ESP_LOGI(TAG_GPIO, "LR: %d, UD: %d, pressed: %d", raw_value_adc1, raw_value_adc2, btn_pressed);
}

void vGpioTask(void *gpioParameters)
{
    struct GpioParameters *params = (struct GpioParameters *)(gpioParameters);

    ESP_LOGI(TAG_GPIO, "Initializing GPIO task...");
    gpio_set_direction(KB_COL_0_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(KB_COL_1_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(KB_COL_2_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(KB_COL_3_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(KB_COL_4_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(KB_COL_5_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(KB_COL_6_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(KB_ROW_0_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(KB_ROW_1_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(KB_ROW_2_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(KB_ROW_3_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(KB_ROW_4_GPIO, GPIO_MODE_INPUT);

    gpio_set_pull_mode(KB_ROW_0_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(KB_ROW_1_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(KB_ROW_2_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(KB_ROW_3_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(KB_ROW_4_GPIO, GPIO_PULLDOWN_ONLY);

    gpio_set_direction(JOYSTICK_BTN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(JOYSTICK_BTN_GPIO, GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG_GPIO, "GPIOs digital pins configured!");

    adc1_config_channel_atten(JOYSTICK_LR_ADC, ADC_ATTEN_DB_12);
    adc2_config_channel_atten(JOYSTICK_UD_ADC, ADC_ATTEN_DB_12);

    adc1_config_width(ADC_WIDTH_BIT_DEFAULT);

    ESP_LOGI(TAG_GPIO, "GPIOs analog pins configured!");

    while (1)
    {
        scanKeys(params);
        scanJoystick(params);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
