/* NEC remote infrared RMT example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
#include <sys/time.h>
#include "driver/gpio.h"

static const char* NEC_TAG = "NEC";

//CHOOSE SELF TEST OR NORMAL TEST
#define RMT_RX_SELF_TEST   1

/******************************************************/
/*****                SELF TEST:                  *****/
/*Connect RMT_TX_GPIO_NUM with RMT_RX_GPIO_NUM        */
/*TX task will send NEC data with carrier disabled    */
/*RX task will print NEC data it receives.            */
/******************************************************/
//#if RMT_RX_SELF_TEST
//#define RMT_RX_ACTIVE_LEVEL  1   /*!< Data bit is active high for self test mode */
//#define RMT_TX_CARRIER_EN    0   /*!< Disable carrier for self test mode  */
//#else
//Test with infrared LED, we have to enable carrier for transmitter
//When testing via IR led, the receiver waveform is usually active-low.
#define RMT_TX_CHANNEL          1           /* RMT channel for transmitter */
#define RMT_TX_GPIO_NUM         PIN_TRIGGER /* GPIO number for transmitter signal */
#define RMT_RX_CHANNEL          0           /* RMT channel for receiver */
#define RMT_RX_GPIO_NUM         PIN_ECHO    /* GPIO number for receiver */
#define RMT_CLK_DIV             100         /* RMT counter clock divider */
#define RMT_TX_CARRIER_EN       0           /* Disable carrier  */
#define rmt_item32_tIMEOUT_US   9500        /*!< RMT receiver timeout value(us) */

#define RMT_TICK_10_US          (80000000/RMT_CLK_DIV/100000)   /* RMT counter value for 10 us.(Source clock is APB clock) */
#define ITEM_DURATION(d)  ((d & 0x7fff)*10/RMT_TICK_10_US)

#define PIN_TRIGGER 25 //A1
#define PIN_ECHO 26 // A0
//#define NEC_HEADER_HIGH_US    9000                         /*!< NEC protocol header: positive 9ms */
//#define NEC_HEADER_LOW_US     4500                         /*!< NEC protocol header: negative 4.5ms*/
//#define NEC_BIT_ONE_HIGH_US    560                         /*!< NEC protocol data bit 1: positive 0.56ms */
//#define NEC_BIT_ONE_LOW_US    (2250-NEC_BIT_ONE_HIGH_US)   /*!< NEC protocol data bit 1: negative 1.69ms */
//#define NEC_BIT_ZERO_HIGH_US   560                         /*!< NEC protocol data bit 0: positive 0.56ms */
//#define NEC_BIT_ZERO_LOW_US   (1120-NEC_BIT_ZERO_HIGH_US)  /*!< NEC protocol data bit 0: negative 0.56ms */
//#define NEC_BIT_END            560                         /*!< NEC protocol end: positive 0.56ms */
//#define NEC_BIT_MARGIN         20                          /*!< NEC parse margin time */


//#define NEC_DATA_ITEM_NUM   34  /*!< NEC code item number: header + 32bit data + end */
//#define RMT_TX_DATA_NUM  100    /*!< NEC tx test data number */
//#define rmt_item32_tIMEOUT_US  9500   /*!< RMT receiver timeout value(us) */


// transmitter

static void ultrasonic_tx_init() //configure the transmitter
{
    rmt_config_t rmt_tx;
    rmt_tx.channel = 0;
    rmt_tx.gpio_num = RMT_TX_GPIO_NUM;
    rmt_tx.mem_block_num = 1;
    rmt_tx.clk_div = RMT_CLK_DIV;
    rmt_tx.tx_config.loop_en = false;
    rmt_tx.tx_config.carrier_duty_percent = 50;
    rmt_tx.tx_config.carrier_freq_hz = 3000;
    rmt_tx.tx_config.carrier_level = 1;
    rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_EN;
    rmt_tx.tx_config.idle_level = 0;
    rmt_tx.tx_config.idle_output_en = true;
    rmt_tx.rmt_mode = 0;
    rmt_config(&rmt_tx);
    rmt_driver_install(rmt_tx.channel, 0, 0);
}

//receiver

static void ultrasonic_rx_init() //configure the receiver
{
    rmt_config_t rmt_rx;
    rmt_rx.channel = 0;
    rmt_rx.gpio_num = RMT_RX_GPIO_NUM;
    rmt_rx.clk_div = RMT_CLK_DIV;
    rmt_rx.mem_block_num = 1;
    rmt_rx.rmt_mode = RMT_MODE_RX;
    rmt_rx.rx_config.filter_en = true;
    rmt_rx.rx_config.filter_ticks_thresh = 100;
    rmt_rx.rx_config.idle_threshold = rmt_item32_tIMEOUT_US / 10 * (RMT_TICK_10_US);
    rmt_config(&rmt_rx);
    rmt_driver_install(rmt_rx.channel, 1000, 0);
}

/**
 * @brief RMT receiver demo, this task will print each received NEC data.
 *
 */
 void app_main()
 {
     ultrasonic_tx_init();
     ultrasonic_rx_init();

     rmt_item32_t item;
     item.level0 = 1;
     item.duration0 = RMT_TICK_10_US;
     item.level1 = 0;
     item.duration1 = RMT_TICK_10_US; // for one pulse this doesn't matter

     size_t rx_size = 0;
     RingbufHandle_t rb = NULL;
     rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rb);
     rmt_rx_start(RMT_RX_CHANNEL, 1);

     double distance = 0;

     while(1)
     {
         rmt_write_items(RMT_TX_CHANNEL, &item, 1, true);
         rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY);

         rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(rb, &rx_size, 10000);
         distance = 340.29 * ITEM_DURATION(item->duration0) / (1000 * 1000 * 2); // distance in meters
         printf("Distance is %f cm\n", distance * 100); // distance in centimeters

         vRingbufferReturnItem(rb, (void*) item);
         vTaskDelay(100 / portTICK_PERIOD_MS);
     }

 }
/**
 * @brief RMT transmitter demo, this task will periodically send NEC data. (100 * 32 bits each time.)
 *
 */
