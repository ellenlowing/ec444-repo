#include "stdio.h"
#include <string.h>
#include "driver/i2c.h"
#include "alpha.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"


static const char* TAG = "example";

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

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .use_ref_tick = true
    };
    ESP_ERROR_CHECK( uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config) );

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

static struct {
    struct arg_str *zeroth;
    struct arg_str *first;
    struct arg_str *second;
    struct arg_str *third;
    struct arg_end *end;
} join_args;

static int set_digits(int argc, char** argv){
  int nerrors = arg_parse(argc, argv, (void**) &join_args);
  if(nerrors!=0){
    arg_print_errors(stderr, join_args.end, argv[0]);
    return 1;
  }
  const char * zero = join_args.zeroth->sval[0];
  const char * first = join_args.first->sval[0];
  const char * second = join_args.second->sval[0];
  const char * third = join_args.third->sval[0];
  
  writeDigit(0, (int)zero[0]);
  writeDigit(1, (int)first[0]);
  writeDigit(2, (int)second[0]);
  writeDigit(3, (int)third[0]);
  return 0;
}

static void register_read()
{
    join_args.zeroth = arg_str0(NULL, NULL, "1st", "char/int");
    join_args.first = arg_str0(NULL, NULL, "2nd", "char/int");
    join_args.second = arg_str0(NULL, NULL, "3rd", "char/int");
    join_args.third = arg_str0(NULL, NULL, "4th", "char/int");
    join_args.end = arg_end(1);

    const esp_console_cmd_t join_cmd = {
        .command = "set",
        .help = "Enter 4 digits to display on alphanumeric display",
        .hint = NULL,
        .func = &set_digits,
        .argtable = &join_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );
}


void app_main() {
  init();
  initialize_nvs();

  #if CONFIG_STORE_HISTORY
    initialize_filesystem();
  #endif

  initialize_console();

  esp_console_register_help_command();

  register_read();

  const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

  printf("\n"
         "This is an example of ESP-IDF console component.\n"
         "Type 'help' to get the list of commands.\n"
         "Use UP/DOWN arrows to navigate through command history.\n"
         "Press TAB when typing command name to auto-complete.\n");

  int probe_status = linenoiseProbe();
  if (probe_status) {
      printf("\n"
             "Your terminal application does not support escape sequences.\n"
             "Line editing and history features are disabled.\n"
             "On Windows, try using Putty instead.\n");
      linenoiseSetDumbMode(1);
  #if CONFIG_LOG_COLORS
        prompt = "esp32> ";
  #endif
  }

  while(true) {
      char* line = linenoise(prompt);
      if (line == NULL) {
          continue;
      }
      linenoiseHistoryAdd(line);
      #if CONFIG_STORE_HISTORY
            linenoiseHistorySave(HISTORY_PATH);
      #endif

      int ret;
      esp_err_t err = esp_console_run(line, &ret);
      if (err == ESP_ERR_NOT_FOUND) {
          printf("Unrecognized command\n");
      } else if (err == ESP_ERR_INVALID_ARG) {
      } else if (err == ESP_OK && ret != ESP_OK) {
          printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
      } else if (err != ESP_OK) {
          printf("Internal error: %s\n", esp_err_to_name(err));
      }
      linenoiseFree(line);
  }
}
