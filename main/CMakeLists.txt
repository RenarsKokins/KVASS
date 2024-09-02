idf_component_register(SRCS "kb_interconnect_manager.c" "config_manager.c" "common_utils.c" "main.c"
                            "comms_manager.c"
                            "gpio_manager.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_driver_gpio
                       PRIV_REQUIRES nvs_flash
                       )
