#include <stdio.h>
#include "driver/i2c.h"
#include <time.h>


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


SemaphoreHandle_t print_mux = NULL;
uint8_t* data_wr;

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

static esp_err_t blinkRate(i2c_port_t i2c_num, uint8_t b) {
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

static esp_err_t setBrightness(i2c_port_t i2c_num, uint8_t b) {
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
static esp_err_t begin(i2c_port_t i2c_num) {
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
void setDigit(uint8_t nth, uint8_t num) {
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

static esp_err_t writeDisplay(i2c_port_t i2c_num, uint8_t* data_wr, size_t size) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) , ACK_CHECK_EN);
  i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static void i2c_test_task(void* arg)
{
    int i = 0;
    int ret;
    uint32_t task_idx = (uint32_t) arg;
    uint8_t* data = (uint8_t*) malloc(DATA_LENGTH);
    data_wr = (uint8_t*) malloc(DATA_LENGTH);
    // initialize data_wr array that stores current display digit
    for(i = 0; i < DATA_LENGTH; i++) {
      // only write starting address 71 - 78
      if(i == 113 || i == 115 || i == 117 || i == 119) data_wr[i] = numbertable[0];
      else data_wr[i] = 0x0;
    }
    int cnt = 0;
    ret = begin(I2C_EXAMPLE_MASTER_NUM);
    if(ret == ESP_OK) printf("SETUP OKAY!\n");
    ret = blinkRate(I2C_EXAMPLE_MASTER_NUM, HT16K33_BLINK_OFF);
    if(ret == ESP_OK) printf("BLINK RATE SETUP OKAY!\n");
    ret = setBrightness(I2C_EXAMPLE_MASTER_NUM, 15);
    if(ret == ESP_OK) printf("BRIGHTNESS SETUP OKAY!\n");

    while (1) {
        int size;
        xSemaphoreTake(print_mux, portMAX_DELAY);
        setDigit(0, cnt % 16);
        setDigit(1, 10);
        setDigit(2, 15);
        setDigit(3, 2);
        cnt ++;
        ret = writeDisplay( I2C_EXAMPLE_MASTER_NUM, data_wr, RW_TEST_LENGTH);
        if (ret == ESP_OK) {
            size = i2c_slave_read_buffer( I2C_EXAMPLE_SLAVE_NUM, data, RW_TEST_LENGTH, 1000 / portTICK_RATE_MS);
        }
        if (ret == ESP_ERR_TIMEOUT) {
            printf("I2C timeout\n");
        } else if (ret == ESP_OK) {
            printf("*******************\n");
            printf("TASK[%d]  MASTER WRITE TO SLAVE\n", task_idx);
            printf("*******************\n");
            printf("----TASK[%d] Master write ----\n", task_idx);
            disp_buf(data_wr, RW_TEST_LENGTH);
        } else {
            printf("TASK[%d] %s: Master write slave error, IO not connected....\n", task_idx, esp_err_to_name(ret));
        }
        xSemaphoreGive(print_mux);
        vTaskDelay(( DELAY_TIME_BETWEEN_ITEMS_MS * ( task_idx + 1 ) ) / portTICK_RATE_MS);
    }
}

void app_main()
{
    print_mux = xSemaphoreCreateMutex();
    i2c_example_slave_init();
    i2c_example_master_init();
    xTaskCreate(i2c_test_task, "i2c_test_task_0", 1024 * 2, (void* ) 0, 10, NULL);
    //xTaskCreate(i2c_test_task, "i2c_test_task_1", 1024 * 2, (void* ) 1, 10, NULL);

}
