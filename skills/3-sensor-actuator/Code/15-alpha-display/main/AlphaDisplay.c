#include "stdio.h"
#include "driver/i2c.h"
#include "alpha.h"

#define I2C_SLAVE_PORT I2C_NUM_0
#define I2C_MASTER_PORT I2C_NUM_1
#define I2C_EXAMPLE_SLAVE_TX_BUF_LEN       (2*DATA_LENGTH)  /*!<I2C slave tx buffer size */
#define I2C_EXAMPLE_SLAVE_RX_BUF_LEN       (2*DATA_LENGTH)  /*!<I2C slave rx buffer size */
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_FREQ_HZ         100000           /*!< I2C master clock frequency */

#define DATA_LENGTH                        512              /*!<Data buffer length for test buffer*/
#define RW_TEST_LENGTH                     129              /*!<Data length for r/w test, any value from 0-DATA_LENGTH*/
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
#define ESP_SLAVE_ADDR 0x70

/******** FUNCTION DECLARATIONS */
void init();
void writeDigit(uint8_t nth, uint8_t num);
static esp_err_t blinkRate(uint8_t b);
static esp_err_t setBrightness(uint8_t b);
static esp_err_t begin();
static esp_err_t writeDisplay(uint8_t* data_wr, size_t size);
static void i2c_master_init();
static void i2c_slave_init();
static void setDigit(uint8_t nth, uint8_t num);

/******** GLOBAL VARIABLES */
int master_sda = 23;
int master_scl = 22;
int slave_sda = 27;
int slave_scl = 26;
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
static const uint16_t alphafonttable[]=  {

  0b0000000000000001,
  0b0000000000000010,
  0b0000000000000100,
  0b0000000000001000,
  0b0000000000010000,
  0b0000000000100000,
  0b0000000001000000,
  0b0000000010000000,
  0b0000000100000000,
  0b0000001000000000,
  0b0000010000000000,
  0b0000100000000000,
  0b0001000000000000,
  0b0010000000000000,
  0b0100000000000000,
  0b1000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0001001011001001,
  0b0001010111000000,
  0b0001001011111001,
  0b0000000011100011,
  0b0000010100110000,
  0b0001001011001000,
  0b0011101000000000,
  0b0001011100000000,
  0b0000000000000000, //
  0b0000000000000110, // !
  0b0000001000100000, // "
  0b0001001011001110, // #
  0b0001001011101101, // $
  0b0000110000100100, // %
  0b0010001101011101, // &
  0b0000010000000000, // '
  0b0010010000000000, // (
  0b0000100100000000, // )
  0b0011111111000000, // *
  0b0001001011000000, // +
  0b0000100000000000, // ,
  0b0000000011000000, // -
  0b0000000000000000, // .
  0b0000110000000000, // /
  0b0000110000111111, // 0
  0b0000000000000110, // 1
  0b0000000011011011, // 2
  0b0000000010001111, // 3
  0b0000000011100110, // 4
  0b0010000001101001, // 5
  0b0000000011111101, // 6
  0b0000000000000111, // 7
  0b0000000011111111, // 8
  0b0000000011101111, // 9
  0b0001001000000000, // :
  0b0000101000000000, // ;
  0b0010010000000000, // <
  0b0000000011001000, // =
  0b0000100100000000, // >
  0b0001000010000011, // ?
  0b0000001010111011, // @
  0b0000000011110111, // A
  0b0001001010001111, // B
  0b0000000000111001, // C
  0b0001001000001111, // D
  0b0000000011111001, // E
  0b0000000001110001, // F
  0b0000000010111101, // G
  0b0000000011110110, // H
  0b0001001000000000, // I
  0b0000000000011110, // J
  0b0010010001110000, // K
  0b0000000000111000, // L
  0b0000010100110110, // M
  0b0010000100110110, // N
  0b0000000000111111, // O
  0b0000000011110011, // P
  0b0010000000111111, // Q
  0b0010000011110011, // R
  0b0000000011101101, // S
  0b0001001000000001, // T
  0b0000000000111110, // U
  0b0000110000110000, // V
  0b0010100000110110, // W
  0b0010110100000000, // X
  0b0001010100000000, // Y
  0b0000110000001001, // Z
  0b0000000000111001, // [
  0b0010000100000000, //
  0b0000000000001111, // ]
  0b0000110000000011, // ^
  0b0000000000001000, // _
  0b0000000100000000, // `
  0b0001000001011000, // a
  0b0010000001111000, // b
  0b0000000011011000, // c
  0b0000100010001110, // d
  0b0000100001011000, // e
  0b0000000001110001, // f
  0b0000010010001110, // g
  0b0001000001110000, // h
  0b0001000000000000, // i
  0b0000000000001110, // j
  0b0011011000000000, // k
  0b0000000000110000, // l
  0b0001000011010100, // m
  0b0001000001010000, // n
  0b0000000011011100, // o
  0b0000000101110000, // p
  0b0000010010000110, // q
  0b0000000001010000, // r
  0b0010000010001000, // s
  0b0000000001111000, // t
  0b0000000000011100, // u
  0b0010000000000100, // v
  0b0010100000010100, // w
  0b0010100011000000, // x
  0b0010000000001100, // y
  0b0000100001001000, // z
  0b0000100101001001, // {
  0b0001001000000000, // |
  0b0010010010001001, // }
  0b0000010100100000, // ~
  0b0011111111111111,

};


void init() {
  print_mux = xSemaphoreCreateMutex();
  i2c_slave_init();
  i2c_master_init();
  int ret;
  int i;
  data_wr = (uint8_t*) malloc(DATA_LENGTH);
  for(i = 0; i < DATA_LENGTH; i++) {
    data_wr[i] = 0x0;
  }
  ret = begin();
  if(ret == ESP_OK) printf("SETUP OKAY!\n");
  ret = blinkRate(HT16K33_BLINK_OFF);
  if(ret == ESP_OK) printf("BLINK RATE SETUP OKAY!\n");
  ret = setBrightness(15);
  if(ret == ESP_OK) printf("BRIGHTNESS SETUP OKAY!\n");
}

static esp_err_t blinkRate(uint8_t b) {
  if(b > 3) b = 0;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, (HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (b << 1)), ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static esp_err_t setBrightness(uint8_t b)  {
  if (b > 15) b = 15;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, (HT16K33_CMD_BRIGHTNESS | b), ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static esp_err_t begin() {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, 0x21, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static esp_err_t writeDisplay(uint8_t* data_wr, size_t size) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( ESP_SLAVE_ADDR << 1 ) , ACK_CHECK_EN);
  i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static void i2c_master_init()
{
    int i2c_master_port = I2C_MASTER_PORT;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = master_sda;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = master_scl;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                       I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
}

static void i2c_slave_init()
{
    int i2c_slave_port = I2C_SLAVE_PORT;
    i2c_config_t conf_slave;
    conf_slave.sda_io_num = slave_sda;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_io_num = slave_scl;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.slave.slave_addr = ESP_SLAVE_ADDR;
    i2c_param_config(i2c_slave_port, &conf_slave);
    i2c_driver_install(i2c_slave_port, conf_slave.mode,
                       I2C_EXAMPLE_SLAVE_RX_BUF_LEN,
                       I2C_EXAMPLE_SLAVE_TX_BUF_LEN, 0);
}

static void setDigit(uint8_t nth, uint8_t num) {
  uint8_t index0 = 0;
  uint8_t index1 = 0;
  switch(nth) {
    case 0:
      index0 = 0x71;
      index1 = 0x72;
      break;
    case 1:
      index0 = 0x73;
      index1 = 0x74;
      break;
    case 2:
      index0 = 0x75;
      index1 = 0x76;
      break;
    case 3:
      index0 = 0x77;
      index1 = 0x78;
      break;
  }
  data_wr[index0] = alphafonttable[(int)num];
  data_wr[index1] = alphafonttable[(int)num] >> 8;
}

void writeDigit(uint8_t nth, uint8_t num) {
  xSemaphoreTake(print_mux, portMAX_DELAY);
  setDigit(nth, num);
  int ret = writeDisplay(data_wr, RW_TEST_LENGTH);
  if(ret == ESP_OK) printf("Digit %d written!\n", nth);
  xSemaphoreGive(print_mux);
}
