#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#define GPIO_PWM0A_OUT 32   //Set GPIO 15 as PWM0A Enable pin Left
#define GPIO_PWM0B_OUT 14   //Set GPIO A0 as PWM0B Enable pin Right

// Left wheele GPIO input
#define GPIO_LEFT_IN_ONE 15 //left 1
#define GPIO_LEFT_IN_TWO 33 //left 2

//Right wheele GPIO input
#define GPIO_RIGHT_IN_ONE 27 //A5
#define GPIO_RIGHT_IN_TWO 4 //

static void mcpwm_example_gpio_initialize()
{
    // Left PWM wave
    printf("initializing mcpwm gpio...\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    gpio_set_level(GPIO_PWM0A_OUT, 0);

    // Right PWM wave
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);
    gpio_set_level(GPIO_PWM0B_OUT, 0);


    // Set left wheele GPIO output
    gpio_pad_select_gpio(GPIO_LEFT_IN_ONE);
    gpio_set_direction(GPIO_LEFT_IN_ONE, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_LEFT_IN_ONE, 0);

    gpio_pad_select_gpio(GPIO_LEFT_IN_TWO);
    gpio_set_direction(GPIO_LEFT_IN_TWO, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_LEFT_IN_TWO, 0);

    //Right wheele GPIO output
    gpio_pad_select_gpio(GPIO_RIGHT_IN_ONE);
    gpio_set_direction(GPIO_RIGHT_IN_ONE, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_RIGHT_IN_ONE, 0);

    gpio_pad_select_gpio(GPIO_RIGHT_IN_TWO);
    gpio_set_direction(GPIO_RIGHT_IN_TWO, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_RIGHT_IN_TWO, 0);
}

/* Forward */
static void brushed_motor_forward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{

    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cycle);

    // Set GPIO One 1 high 1 low
    gpio_set_level(GPIO_LEFT_IN_ONE, 1);
    gpio_set_level(GPIO_LEFT_IN_TWO, 0);

    // Set GPIO Two to high 1 low
    gpio_set_level(GPIO_RIGHT_IN_ONE, 1);
    gpio_set_level(GPIO_RIGHT_IN_TWO, 0);

}

/* Backward */
static void brushed_motor_backward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cycle);

    // Set GPIO One 1 high 1 low
    gpio_set_level(GPIO_LEFT_IN_ONE, 0);
    gpio_set_level(GPIO_LEFT_IN_TWO, 1);

    // Set GPIO Two i high 1 low
    gpio_set_level(GPIO_RIGHT_IN_ONE, 0);
    gpio_set_level(GPIO_RIGHT_IN_TWO, 1);
}

/* Stop */
static void brushed_motor_stop(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float duty_cycle)
{
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cycle);

    // Set GPIO One both low
    gpio_set_level(GPIO_LEFT_IN_ONE, 0);
    gpio_set_level(GPIO_LEFT_IN_TWO, 0);

    // Set GPIO Two both low
    gpio_set_level(GPIO_RIGHT_IN_ONE, 0);
    gpio_set_level(GPIO_RIGHT_IN_TWO, 0);
}

static void brushed_motor_left(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float duty_cycle)
{
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cycle);

    // Set GPIO One both low
    gpio_set_level(GPIO_LEFT_IN_ONE, 0);
    gpio_set_level(GPIO_LEFT_IN_TWO, 0);

    // Set GPIO Two both low
    gpio_set_level(GPIO_RIGHT_IN_ONE, 1);
    gpio_set_level(GPIO_RIGHT_IN_TWO, 0);
}

/* Main function for motor control */
static void mcpwm_example_brushed_motor_control(void *arg)
{
    //1. mcpwm gpio initialization
    mcpwm_example_gpio_initialize();

    //2. initial mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;    //frequency = 500Hz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
    while (1) {
        brushed_motor_forward(MCPWM_UNIT_0, MCPWM_TIMER_0, 100);
        vTaskDelay(2000 / portTICK_RATE_MS);
        brushed_motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_0, 100);
        vTaskDelay(1000 / portTICK_RATE_MS);
        brushed_motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_0, 100);
        vTaskDelay(1000 / portTICK_RATE_MS);

    }
}

void app_main()
{
    printf("Testing brushed motor...\n");
    xTaskCreate(mcpwm_example_brushed_motor_control, "mcpwm_examlpe_brushed_motor_control", 4096, NULL, 5, NULL);
}
