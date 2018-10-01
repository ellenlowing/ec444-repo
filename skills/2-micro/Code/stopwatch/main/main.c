#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_types.h"
#include "freertos/queue.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "alpha.h"


#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (5) // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC   (1)
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

#define BTN_GPIO 34

typedef struct {
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;
xQueueHandle timer_queue_1;


uint8_t btn_state = 2;
uint32_t second_count = 0;
bool timerStarted = false;
bool resetTime = false;

// btn_states
// 0: start timer
// 1: stop timer
// 2: reset timer
void IRAM_ATTR button_intr(void* arg) {
    int i;
    for(i = 0; i < 10000; i++){
      //debounce
    }
    if (!gpio_get_level(BTN_GPIO)) {
      btn_state = (btn_state + 1) % 3;
    }
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

void setTime() {
  writeDigit(0, second_count / 1000 % 10);
  writeDigit(1, second_count / 100 % 10);
  writeDigit(2, second_count / 10 % 10);
  writeDigit(3, second_count % 10);
}

void ticking () {
    timer_event_t evt_1;
    xQueueReceive(timer_queue_1, &evt_1, portMAX_DELAY);
    second_count = (second_count + 1) % 10000;
    setTime();
    printf("%d\n", second_count);
}

void app_main()
{
  // initialize alphanumeric display
  init();
  setTime();

  // initialize pushbutton
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

  // initialize timer and alarm
  timer_queue_1 = xQueueCreate(10, sizeof(timer_event_t));
  example_tg1_timer_init(TIMER_0, TEST_WITH_RELOAD, TIMER_INTERVAL1_SEC);


  while(1) {
    if(btn_state == 0) {
      if(!timerStarted) {
        timer_start(TIMER_GROUP_1, TIMER_0);
        timerStarted = true;
        resetTime = false;
      } else {
        ticking();
      }
    }
    else if (btn_state == 1) {
      timer_pause(TIMER_GROUP_1, TIMER_0);
      timerStarted = false;
    }
    else if (btn_state == 2) {
      second_count = 0;
      if(!resetTime) setTime();
      resetTime = true;
    }
    vTaskDelay(10);
  }

}
