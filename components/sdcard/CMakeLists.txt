idf_component_register(SRCS "sdcard.c"
                    REQUIRES vfs fatfs sdmmc wigle
                    INCLUDE_DIRS ".")
