idf_component_register(SRCS "sd_fat.c"
                    INCLUDE_DIRS "include"
                    REQUIRES fatfs sdmmc ui lvgl
                    )
