
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "ui.h"
#include "sd_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_timer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"

// Defines
#define LV_TICK_PERIOD_MS 10
#define MONITOR_HEAP 1
#define GUI_TASK_STACK_SIZE 8 * 1024
#define GUI_TASK_PRIORITY 3
#define GUI_TASK_CORE 1
#define USE_DOUBLE_BUFFER 0

// global vars

// take this semaphore to call lvgl related func from different core
SemaphoreHandle_t xGuiSemaphore;

// Static Prototypes
static void lv_tick_task(void *arg);
static void ui_task(void *pvParameter);
static void heapCalc(void *pvParameter);
static void my_log_cb(const char *buf);

void app_main()
{

// monitor heap size
#if MONITOR_HEAP
  xTaskCreatePinnedToCore(heapCalc, "printFreeHeap", 2 * 1024, NULL, 2, NULL, 0);
#endif

  // initialize sd card
  initSD();

  // start ui task
  xTaskCreatePinnedToCore(ui_task, "gui", GUI_TASK_STACK_SIZE, NULL, GUI_TASK_PRIORITY, NULL, GUI_TASK_CORE);

  // register print log from lvgl
  lv_log_register_print_cb(my_log_cb);
}

uint16_t buf[LV_HOR_RES_MAX * 40];
uint16_t buf2[LV_HOR_RES_MAX * 40];

static void ui_task(void *pvParameter)
{
  // (void)pvParameter;
  xGuiSemaphore = xSemaphoreCreateMutex();

  // initialize lvgl
  lv_init();

  // Initialize SPI or I2C bus used by the drivers
  lvgl_driver_init();

  lv_display_t *display = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);

  lv_display_set_buffers(display, buf, buf2, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_set_flush_cb(display, disp_driver_flush);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_NATIVE);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);

  lv_indev_set_read_cb(indev, touch_driver_read);

  // Create and start a periodic timer interrupt to call lv_tick_inc
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &lv_tick_task, .name = "periodic_gui"};
  esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

  // initialize ui and set ui initializes bit to safely access ui related stuff
  ui_init();

  // lv_demo_widgets();

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(10));
    // take this semaphore to call lvgl related function on success
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
    {
      lv_task_handler();

      xSemaphoreGive(xGuiSemaphore);
    }
  }
  vTaskDelete(NULL);
}

// lvgl timer task
static void lv_tick_task(void *arg)
{
  lv_tick_inc(LV_TICK_PERIOD_MS);
}

// print lvgl related log
static void my_log_cb(const char *buf)
{
  printf(buf, strlen(buf));
  fflush(stdout);
}

static void heapCalc(void *pvParameter)
{
  while (1)
  {
    printf("Free heap size: %d Kb\n", esp_get_free_heap_size() / 1024);
    fflush(stdout);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}
