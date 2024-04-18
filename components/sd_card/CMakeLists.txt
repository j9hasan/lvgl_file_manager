idf_component_register(SRCS "sd_card.c"
                    INCLUDE_DIRS "include"
                    REQUIRES fatfs sdmmc lvgl_file_manager lvgl
                    )
