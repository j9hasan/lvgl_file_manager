
#ifndef _GLOB_H_
#define _GLOB_H_
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
// #include "lvgl_helpers.h"
extern uint16_t buf[];
extern uint16_t buf2[];

extern SemaphoreHandle_t xGuiSemaphore;

#endif
