SET(SOURCES
    lvgl_file_manager.cpp
    )
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS .
                       REQUIRES lvgl sd_card global
                       )
