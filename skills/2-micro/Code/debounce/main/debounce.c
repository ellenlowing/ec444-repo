#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_system.h"

#define BTN_GPIO 34
#define LED_GPIO 12

volatile static bool led = 0;

void IRAM_ATTR button_intr(void* arg) {
    if (!gpio_get_level(BTN_GPIO)) {
      led = !led;
    }
}

void btn_task(void *pvParameter){
  gpio_pad_select_gpio(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_pad_select_gpio(BTN_GPIO);
  gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);

  gpio_set_intr_type(BTN_GPIO, GPIO_INTR_NEGEDGE);
  gpio_intr_enable(BTN_GPIO);

  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL2);

  gpio_isr_handler_add(
      BTN_GPIO,
      button_intr,
      NULL
  );

  while(1) {
    gpio_set_level(LED_GPIO, led);
    vTaskDelay(10);
  }
}

void app_main(void) {
  xTaskCreate(&btn_task, "btn_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}
