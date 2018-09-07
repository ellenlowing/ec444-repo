/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "freertos/event_groups.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"

#define BLINK_GPIO_1 12
#define BLINK_GPIO_2 27
#define BLINK_GPIO_3 33
#define BLINK_GPIO_4 15
#define BLINK_NUM 4

#if CONFIG_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem()
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_STORE_HISTORY

static void initialize_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void initialize_console()
{
    /* Disable buffering on stdin and stdout */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

int gpios [4] = {BLINK_GPIO_1, BLINK_GPIO_2, BLINK_GPIO_3, BLINK_GPIO_4};
int gpio_values[4] = {1, 1, 1, 1};
int i;

static struct {
    struct arg_int *fourbitval;
    struct arg_end *end;
} join_args;

static int leds_read(int argc, char** argv){
  int nerrors = arg_parse(argc, argv, (void**) &join_args);
  if(nerrors!=0){
    arg_print_errors(stderr, join_args.end, argv[0]);
    return 1;
  }
  // read 4 bit value and print
  //printf("%d", join_args.fourbitval->ival[0]);
  int led_val = join_args.fourbitval->ival[0];
  gpio_values[0] = led_val/1000;
  gpio_values[1] = (led_val - gpio_values[0]*1000)/100;
  gpio_values[2] = (led_val - gpio_values[0]*1000 - gpio_values[1]*100)/10;
  gpio_values[3] = led_val - gpio_values[0]*1000 - gpio_values[1]*100 - gpio_values[2]*10;
  for(i = 0; i < BLINK_NUM; i++){
    gpio_set_level(gpios[i], gpio_values[i]);
  }
  return 0;
}

void register_read()
{
    join_args.fourbitval = arg_int0(NULL, NULL, "<4bit>", "4 bit boolean");
    join_args.end = arg_end(1);

    const esp_console_cmd_t join_cmd = {
        .command = "leds_read",
        .help = "Reads 4 bit value to LEDs",
        .hint = NULL,
        .func = &leds_read,
        .argtable = &join_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );
}

void app_main()
{

  for(i = 0; i < BLINK_NUM; i++){
    gpio_pad_select_gpio(gpios[i]);
    gpio_set_direction(gpios[i], GPIO_MODE_OUTPUT);
  }

  initialize_nvs();

  #if CONFIG_STORE_HISTORY
    initialize_filesystem();
  #endif

  initialize_console();

  /* Register commands */
  esp_console_register_help_command();
  register_read();
  const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

  printf("\n"
           "This is an example of ESP-IDF console component.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "esp32> ";
#endif //CONFIG_LOG_COLORS
    }

  while(1){
    /* Get a line using linenoise.
      * The line is returned when ENTER is pressed.
      */
    char* line = linenoise(prompt);
    if (line == NULL) { /* Ignore empty lines */
      continue;
    }
    /* Add the command to the history */
    linenoiseHistoryAdd(line);
    #if CONFIG_STORE_HISTORY
      /* Save command history to filesystem */
      linenoiseHistorySave(HISTORY_PATH);
    #endif

    /* Try to run the command */
    int ret;
    esp_err_t err = esp_console_run(line, &ret);
    if (err == ESP_ERR_NOT_FOUND) {
      printf("Unrecognized command\n");
    } else if (err == ESP_ERR_INVALID_ARG) {
                // command was empty
    } else if (err == ESP_OK && ret != ESP_OK) {
      printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
    } else if (err != ESP_OK) {
      printf("Internal error: %s\n", esp_err_to_name(err));
    }
    linenoiseFree(line);

  }
}
