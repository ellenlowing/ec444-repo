
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "sdkconfig.h"


#define DEFAULT_VREF    3300        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;    
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

/* above is IR definition*/

#define GREEN_LED 26 // green
#define BLUE_LED 25 // blue
#define RED_LED 4 // red
/* LED pinout */


float kp = 0.5;
float ki = 0.5;
float kd = 0.5;

float current_position = 0;
float target_position = 0;
float error = 0;
float integral = 0;
float derivative = 0;
float pwd = 0;
float last_error = 0;
double pid = 0;

/******* IR functoin define *******/
static void check_efuse()
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

float get_dist()
{
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    //Continuously sample ADC1
    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (unit == ADC_UNIT_1) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        } else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
            adc_reading += raw;
        }
    }
    adc_reading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    int voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    int distance = 146060 * (pow(voltage, -1.126));
    printf("distance: %dcm\n", distance);
    vTaskDelay(pdMS_TO_TICKS(100));
    return distance;
}

/******* LED *******/

void led_init(){
  gpio_pad_select_gpio(GREEN_LED);
  gpio_pad_select_gpio(BLUE_LED);
  gpio_pad_select_gpio(RED_LED);
  gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
  gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
  gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);

}

void  setRed() {
  gpio_set_level(RED_LED, 1);
  gpio_set_level(GREEN_LED, 0);
  gpio_set_level(BLUE_LED, 0);

}

void  setGreen() {
  gpio_set_level(GREEN_LED, 1);
  gpio_set_level(BLUE_LED, 0);
  gpio_set_level(RED_LED, 0);
}

void  setBlue() {
  gpio_set_level(BLUE_LED, 1);
  gpio_set_level(GREEN_LED, 0);
  gpio_set_level(RED_LED, 0);
}


/****** PID *******/

void app_main() {

  led_init();

  while(1){

    current_position = get_dist();

    error = 70 - current_position;

    integral = integral + error;

    derivative = error - last_error;

    pid = (kp * error) + (ki * integral) + (kd * derivative);

    printf("This is pid: %f\n", pid);

    if (pid < 0){
      setRed();
    } else if (pid > 0) {
      setBlue();
    } else {
      setGreen();
    }

    last_error = error;
  }

}
