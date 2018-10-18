#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "string.h"

#define RX (16)
#define TX (17)

static const int RX_BUF_SIZE = 256;

void app_main() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);

    while(1) {
        uint32_t distance = 0;
        int argc = 0;
        uint32_t temp_dist = 0;
        uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
        int length = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        uart_flush(UART_NUM_1);

        for (int i = 0; i < length; i++) {
          temp_dist = 0;
          if (data[i] == 0x59 && data[i+1] == 0x59) {
            temp_dist = data[i+3];
            temp_dist <<= 8;
            temp_dist += data[i+2];
            distance += temp_dist;
            argc++;
          }
        }
        if (argc > 0) {
          distance /= argc;
        } else {
          distance = 0;
        }
        printf("Distance: %dcm\n", distance);
    }
}
