#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_system.h"

#define BTN_GPIO 34
#define LED0 12
#define LED1 14
#define LED2 15
#define LED3 32

int led = 0;

int leds [4] = {LED0, LED1, LED2, LED3};

void IRAM_ATTR button_intr(void* arg) {
    if (!gpio_get_level(BTN_GPIO)) {
      if(led == 3) led = 0;
      else led++;
    }
}

void interrupt_task(void *pvParameter){
  gpio_set_intr_type(BTN_GPIO, GPIO_INTR_NEGEDGE);
  gpio_intr_enable(BTN_GPIO);

  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL2);

  gpio_isr_handler_add(
      BTN_GPIO,
      button_intr,
      NULL
  );

  while(1) {
    for(int i = 0; i < 4; i++){
      if(i == led)  gpio_set_level(leds[i], 1);
      else gpio_set_level(leds[i], 0);
    }
    vTaskDelay(10);
  }
}

void poll_task(void *pvParameter) {
  while(1) {
    if (!gpio_get_level(BTN_GPIO)) {
      if(led == 3) led = 0;
      else led++;
    }

    for(int i = 0; i < 4; i++){
      if(i == led)  gpio_set_level(leds[i], 1);
      else gpio_set_level(leds[i], 0);
    }
    vTaskDelay(10);
  }
}

void app_main(void) {
  for(int i = 0; i < 4; i++){
    gpio_pad_select_gpio(leds[i]);
    gpio_set_direction(leds[i], GPIO_MODE_OUTPUT);
  }
  gpio_pad_select_gpio(BTN_GPIO);
  gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);


  // part a: poll task
  xTaskCreate(&poll_task, "poll_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);

  // part b: interrupt task
  //xTaskCreate(&interrupt_task, "interrupt_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}
