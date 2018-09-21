/*
  First, set up console input for rate of alarm.
    Re-initialize timer when there is valid input.
    Perhaps 2 timers to switch ? ----- skip!
  Second, control LEDs via GPIO to show alarm going off. ----- OK!
  Third, connect servos to count down through alarming period. --- second hand OK! minute hand left!

  Question: do I need to give user access to change the rate of alarming period?


*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "driver/i2c.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (10) // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC   (1)
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

#define BLINK_GPIO 12

#define SECONDHAND_GPIO 15
#define MINUTEHAND_GPIO 33
#define START_SECOND 59
#define START_MINUTE 59
#define START_HOUR 11
#define SECONDHAND_SERVO MCPWM_OPR_A
#define MINUTEHAND_SERVO MCPWM_OPR_B
#define SERVO_MIN_PULSEWIDTH 500 //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 2400 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 90 //Maximum angle in degree upto which servo can rotate

#define BTN_GPIO 34

#define DATA_LENGTH                        512              /*!<Data buffer length for test buffer*/
#define RW_TEST_LENGTH                     129              /*!<Data length for r/w test, any value from 0-DATA_LENGTH*/
#define DELAY_TIME_BETWEEN_ITEMS_MS        1000             /*!< delay time between different test items */

#define I2C_EXAMPLE_SLAVE_SCL_IO           26               /*!<gpio number for i2c slave clock  */
#define I2C_EXAMPLE_SLAVE_SDA_IO           27               /*!<gpio number for i2c slave data */
#define I2C_EXAMPLE_SLAVE_NUM              I2C_NUM_0        /*!<I2C port number for slave dev */
#define I2C_EXAMPLE_SLAVE_TX_BUF_LEN       (2*DATA_LENGTH)  /*!<I2C slave tx buffer size */
#define I2C_EXAMPLE_SLAVE_RX_BUF_LEN       (2*DATA_LENGTH)  /*!<I2C slave rx buffer size */

#define I2C_EXAMPLE_MASTER_SCL_IO          22               /*!< gpio number for I2C master clock */
#define I2C_EXAMPLE_MASTER_SDA_IO          23               /*!< gpio number for I2C master data  */
#define I2C_EXAMPLE_MASTER_NUM             I2C_NUM_1        /*!< I2C port number for master dev */
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_FREQ_HZ         100000           /*!< I2C master clock frequency */

#define ESP_SLAVE_ADDR                     0x70             /*!< ESP32 slave address, you can set any 7bit value */
#define WRITE_BIT                          I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                           I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */

#define LED_ON 1
#define LED_OFF 0

#define LED_RED 1
#define LED_YELLOW 2
#define LED_GREEN 3

#define HT16K33_BLINK_CMD 0x80
#define HT16K33_BLINK_DISPLAYON 0x01
#define HT16K33_BLINK_OFF 0
#define HT16K33_BLINK_2HZ  1
#define HT16K33_BLINK_1HZ  2
#define HT16K33_BLINK_HALFHZ  3

#define HT16K33_CMD_BRIGHTNESS 0xE0

static const uint8_t numbertable[] = {
	0x3F, /* 0 */
	0x06, /* 1 */
	0xDB, /* 2 */
	0xCF, /* 3 */
	0xE6, /* 4 */
	0xED, /* 5 */
	0xFD, /* 6 */
	0x07, /* 7 */
	0xFF, /* 8 */
	0xEF, /* 9 */
	0xF7, /* a */
	0xFC, /* b */
	0x39, /* C */
	0xDE, /* d */
	0xF9, /* E */
	0xF1, /* F */
};

SemaphoreHandle_t print_mux = NULL;
uint8_t* data_wr;

volatile static bool reset = 0;
const int CONNECTED_BIT = BIT0;

typedef struct {
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

xQueueHandle timer_queue_0;
xQueueHandle timer_queue_1;

void IRAM_ATTR button_intr(void* arg) {
    if (!gpio_get_level(BTN_GPIO)) {
      reset = 1;
    }
}

void blink_task()
{
    printf("alarm going off\n");
    int count = 0;
    while(1) {
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (count < 2) {
          count ++;
        }
        else {
          gpio_set_level(BLINK_GPIO, 0);
          break;
        }
    }
}

static void mcpwm_example_gpio_initialize()
{
    printf("initializing mcpwm servo control gpio......\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SECONDHAND_GPIO);    //Set GPIO 15 as PWM0A, to which servo is connected
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, MINUTEHAND_GPIO);
}

static uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}

void mcpwm_example_servo_control(uint32_t count,  mcpwm_operator_t operator)
{
    uint32_t angle;
    uint32_t mapped_count = count * SERVO_MAX_DEGREE / 60;
    angle = servo_per_degree_init(mapped_count);
    //printf("pulse width: %dus\n", angle);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, operator, angle);
    //vTaskDelay(10);
}

static void inline print_timer_counter(uint64_t counter_value)
{
    printf("Counter: 0x%08x%08x\n", (uint32_t) (counter_value >> 32),
                                    (uint32_t) (counter_value));
    printf("Time   : %.8f s\n", (double) counter_value / TIMER_SCALE);
}

void IRAM_ATTR timer_group0_isr(void *para)
{
    int timer_idx = (int) para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[timer_idx].update = 1;
    uint64_t timer_counter_value =
        ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
        | TIMERG0.hw_timer[timer_idx].cnt_low;

    /* Prepare basic event data
       that will be then sent back to the main program task */
    timer_event_t evt;
    evt.timer_group = 0;
    evt.timer_idx = timer_idx;
    evt.timer_counter_value = timer_counter_value;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        evt.type = TEST_WITHOUT_RELOAD;
        TIMERG0.int_clr_timers.t0 = 1;
        timer_counter_value += (uint64_t) (TIMER_INTERVAL0_SEC * TIMER_SCALE);
        TIMERG0.hw_timer[timer_idx].alarm_high = (uint32_t) (timer_counter_value >> 32);
        TIMERG0.hw_timer[timer_idx].alarm_low = (uint32_t) timer_counter_value;
    } else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1) {
        evt.type = TEST_WITH_RELOAD;
        TIMERG0.int_clr_timers.t1 = 1;
    } else {
        evt.type = -1; // not supported even type
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

    /* Now just send the event data back to the main program task */
    xQueueSendFromISR(timer_queue_0, &evt, NULL);
}

void IRAM_ATTR timer_group1_isr(void *para)
{
    int timer_idx = (int) para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG1.int_st_timers.val;
    TIMERG1.hw_timer[timer_idx].update = 1;
    uint64_t timer_counter_value =
        ((uint64_t) TIMERG1.hw_timer[timer_idx].cnt_high) << 32
        | TIMERG1.hw_timer[timer_idx].cnt_low;

    /* Prepare basic event data
       that will be then sent back to the main program task */
    timer_event_t evt;
    evt.timer_group = 0;
    evt.timer_idx = timer_idx;
    evt.timer_counter_value = timer_counter_value;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        evt.type = TEST_WITHOUT_RELOAD;
        TIMERG1.int_clr_timers.t0 = 1;
        timer_counter_value += (uint64_t) (TIMER_INTERVAL1_SEC * TIMER_SCALE);
        TIMERG1.hw_timer[timer_idx].alarm_high = (uint32_t) (timer_counter_value >> 32);
        TIMERG1.hw_timer[timer_idx].alarm_low = (uint32_t) timer_counter_value;
    } else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1) {
        evt.type = TEST_WITH_RELOAD;
        TIMERG1.int_clr_timers.t1 = 1;
    } else {
        evt.type = -1; // not supported even type
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    TIMERG1.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

    /* Now just send the event data back to the main program task */
    xQueueSendFromISR(timer_queue_1, &evt, NULL);
}

static void example_tg0_timer_init(int timer_idx,
    bool auto_reload, double timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

}

static void example_tg1_timer_init(int timer_idx,
    bool auto_reload, double timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_1, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_1, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_1, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_1, timer_idx);
    timer_isr_register(TIMER_GROUP_1, timer_idx, timer_group1_isr,
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    //timer_start(TIMER_GROUP_0, timer_idx);
}


static void i2c_example_master_init()
{
    int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                       I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
}

static void i2c_example_slave_init()
{
    int i2c_slave_port = I2C_EXAMPLE_SLAVE_NUM;
    i2c_config_t conf_slave;
    conf_slave.sda_io_num = I2C_EXAMPLE_SLAVE_SDA_IO;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_io_num = I2C_EXAMPLE_SLAVE_SCL_IO;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.slave.slave_addr = ESP_SLAVE_ADDR;
    i2c_param_config(i2c_slave_port, &conf_slave);
    i2c_driver_install(i2c_slave_port, conf_slave.mode,
                       I2C_EXAMPLE_SLAVE_RX_BUF_LEN,
                       I2C_EXAMPLE_SLAVE_TX_BUF_LEN, 0);
}

static void disp_buf(uint8_t* buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if (( i + 1 ) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

static esp_err_t blinkRate(i2c_port_t i2c_num, uint8_t b)
{
  if(b > 3) b = 0;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, (HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (b << 1)), ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static esp_err_t setBrightness(i2c_port_t i2c_num, uint8_t b)
{
  if (b > 15) b = 15;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, (HT16K33_CMD_BRIGHTNESS | b), ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

// equivalent to Adafruit_LEDBackpack::begin
static esp_err_t begin(i2c_port_t i2c_num)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, 0x21, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

// nth: 0-3
// 0 --- 113
// 1 --- 115
// 2 --- 117
// 3 --- 119
void setDigit(uint8_t nth, uint8_t num)
{
  int index = 0;
  switch(nth) {
    case 0:
      index = 113;
      break;
    case 1:
      index = 115;
      break;
    case 2:
      index = 117;
      break;
    case 3:
      index = 119;
      break;
  }
  data_wr[index] = numbertable[(int)num];
}

void setTime(uint32_t hour, uint32_t minute) {
  setDigit(0, hour / 10);
  setDigit(1, hour % 10);
  setDigit(2, minute / 10);
  setDigit(3, minute % 10);
}

static esp_err_t writeDisplay(i2c_port_t i2c_num, uint8_t* data_wr, size_t size)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) , ACK_CHECK_EN);
  i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}


void app_main()
{
    uint32_t second_count = START_SECOND;
    uint32_t minute_count = START_MINUTE;
    uint32_t hour_count = START_HOUR;

    // initialize i2c communication and alpha display
    print_mux = xSemaphoreCreateMutex();
    i2c_example_slave_init();
    i2c_example_master_init();
    int ret, size;
    uint32_t task_idx = 0;
    uint8_t* data = (uint8_t*) malloc(DATA_LENGTH);
    data_wr = (uint8_t*) malloc(DATA_LENGTH);
    for(int i = 0; i < DATA_LENGTH; i++) {
      data_wr[i] = 0x0;
    }
    ret = begin(I2C_EXAMPLE_MASTER_NUM);
    if(ret == ESP_OK) printf("SETUP OKAY!\n");
    ret = blinkRate(I2C_EXAMPLE_MASTER_NUM, HT16K33_BLINK_OFF);
    if(ret == ESP_OK) printf("BLINK RATE SETUP OKAY!\n");
    ret = setBrightness(I2C_EXAMPLE_MASTER_NUM, 15);
    if(ret == ESP_OK) printf("BRIGHTNESS SETUP OKAY!\n");
    xSemaphoreTake(print_mux, portMAX_DELAY);
    setTime(hour_count, minute_count);
    ret = writeDisplay( I2C_EXAMPLE_MASTER_NUM, data_wr, RW_TEST_LENGTH);
    if (ret == ESP_OK) {
        size = i2c_slave_read_buffer( I2C_EXAMPLE_SLAVE_NUM, data, RW_TEST_LENGTH, 1000 / portTICK_RATE_MS);
    }
    xSemaphoreGive(print_mux);

    // initialize button gpio input and interrupt handler
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

    // initialize led gpio and set output
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    // initialize servo motor
    mcpwm_example_gpio_initialize();
    printf("Configuring Initial Parameters of mcpwm......\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
    mcpwm_example_servo_control(START_MINUTE, MINUTEHAND_SERVO);
    mcpwm_example_servo_control(START_SECOND, SECONDHAND_SERVO);

    // initialize timer and alarm
    timer_queue_0 = xQueueCreate(10, sizeof(timer_event_t));
    timer_queue_1 = xQueueCreate(10, sizeof(timer_event_t));
    example_tg0_timer_init(TIMER_0, TEST_WITH_RELOAD, TIMER_INTERVAL0_SEC);
    example_tg1_timer_init(TIMER_0, TEST_WITH_RELOAD, TIMER_INTERVAL1_SEC);
    timer_start(TIMER_GROUP_1, TIMER_0);
    uint32_t current_second = minute_count * 60 + second_count;
    timer_event_t evt_1;

    while (1) {
      if(reset){
        timer_start(TIMER_GROUP_0, TIMER_0);
        current_second = minute_count * 60 + second_count;
        while((minute_count * 60 + second_count) <= (current_second + TIMER_INTERVAL0_SEC)){
          xQueueReceive(timer_queue_1, &evt_1, portMAX_DELAY);


          if(second_count > 58) {
            second_count = 0;
            if (minute_count > 58) {
              minute_count = 0;
              if (hour_count > 23) hour_count = 0;
              else hour_count ++;
            } else {
              minute_count ++;
            }
            mcpwm_example_servo_control(0, SECONDHAND_SERVO);
            mcpwm_example_servo_control(minute_count, MINUTEHAND_SERVO);
            xSemaphoreTake(print_mux, portMAX_DELAY);
            setTime(hour_count, minute_count);
            ret = writeDisplay( I2C_EXAMPLE_MASTER_NUM, data_wr, RW_TEST_LENGTH);
            if (ret == ESP_OK) {
                size = i2c_slave_read_buffer( I2C_EXAMPLE_SLAVE_NUM, data, RW_TEST_LENGTH, 1000 / portTICK_RATE_MS);
            }
            xSemaphoreGive(print_mux);
          } else {
            mcpwm_example_servo_control(second_count, SECONDHAND_SERVO);
            second_count ++;
          }
          printf("triggered: %d min : %d s \n", minute_count, second_count);
        }
        timer_event_t evt_0;
        xQueueReceive(timer_queue_0, &evt_0, portMAX_DELAY);
        reset = 0;
        blink_task(); //when alarm goes off
        timer_pause(TIMER_GROUP_0, TIMER_0);
      } else {
        xQueueReceive(timer_queue_1, &evt_1, portMAX_DELAY);

        if(second_count > 58) {
          second_count = 0;
          if (minute_count > 58) {
            minute_count = 0;
            if (hour_count > 23) hour_count = 0;
            else hour_count ++;
          } else {
            minute_count ++;
          }
          mcpwm_example_servo_control(0, SECONDHAND_SERVO);
          mcpwm_example_servo_control(minute_count, MINUTEHAND_SERVO);
          xSemaphoreTake(print_mux, portMAX_DELAY);
          setTime(hour_count, minute_count);
          ret = writeDisplay( I2C_EXAMPLE_MASTER_NUM, data_wr, RW_TEST_LENGTH);
          if (ret == ESP_OK) {
              size = i2c_slave_read_buffer( I2C_EXAMPLE_SLAVE_NUM, data, RW_TEST_LENGTH, 1000 / portTICK_RATE_MS);
          }
          xSemaphoreGive(print_mux);

        } else {
          mcpwm_example_servo_control(second_count, SECONDHAND_SERVO);
          second_count ++;
        }
      }
      printf("%d min : %d s \n", minute_count, second_count);
      vTaskDelay(10);
    }
}
