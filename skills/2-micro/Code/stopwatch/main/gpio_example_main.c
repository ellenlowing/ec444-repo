#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BTN_GPIO 14

void button_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BTN_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
    while(1) {
        // /* Blink off (output low) */
        // gpio_set_level(BLINK_GPIO, 0);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // /* Blink on (output high) */
        // gpio_set_level(BLINK_GPIO, 1);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        int btn_level = gpio_get_level(BTN_GPIO);
        printf("%d\n", btn_level);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    xTaskCreate(&button_task, "button_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}
