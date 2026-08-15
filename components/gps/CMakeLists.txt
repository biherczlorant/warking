idf_component_register(SRCS "gps_driver.c" "gps.c"
                    PRIV_REQUIRES esp_driver_uart esp_driver_gpio
                    INCLUDE_DIRS ".")
