#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "linenoise/linenoise.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_vfs_fat.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

char * outputStr;

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

}

static struct {
    struct arg_str *cmdstr;
    struct arg_end *end;
} join_args;

static int read_string(int argc, char** argv){
  printf("before running read_string func\n");
  int nerrors = arg_parse(argc, argv, (void**) &join_args);
  if(nerrors!=0){
    arg_print_errors(stderr, join_args.end, argv[0]);
    return 1;
  }
  int n = sprintf(outputStr, "%s", join_args.cmdstr->sval[0]);
  printf("%s\n", outputStr);
  return 0;
}

void register_read()
{
    join_args.cmdstr = arg_str0(NULL, NULL, "<string>", "10 characters");
    join_args.end = arg_end(1);

    const esp_console_cmd_t join_cmd = {
        .command = "read",
        .help = "Read string of 10 characters",
        .hint = NULL,
        .func = &read_string,
        .argtable = &join_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );
}

void app_main()
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Initialize console
    initialize_console();
    const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

    // Register command
    outputStr = (char *)malloc(sizeof(char)*10);
    esp_console_register_help_command();
    register_read();


    // while(1) {
    //   // Get string on console
    //   char* line = linenoise(prompt);
    //   /* Add the command to the history */
    //   linenoiseHistoryAdd(line);
    //
    //   /* Try to run the command */
    //   int ret;
    //   esp_err_t err = esp_console_run(line, &ret);
    //   if (err == ESP_ERR_NOT_FOUND) {
    //     printf("Unrecognized command\n");
    //   } else if (err == ESP_ERR_INVALID_ARG) {
    //               // command was empty
    //   } else if (err == ESP_OK && ret != ESP_OK) {
    //     printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
    //   } else if (err != ESP_OK) {
    //     printf("Internal error: %s\n", esp_err_to_name(err));
    //   }
    //   linenoiseFree(line);
    // }

    // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        // Read
        size_t required_size;
        err = nvs_get_str(my_handle, "str", NULL, &required_size);
        char* inputStr = malloc(required_size);
        err = nvs_get_str(my_handle, "str", inputStr, &required_size);

        switch (err) {
            case ESP_OK:
                printf("Done\n");
                // printf("Restart counter = %d\n", restart_counter);
                printf("Previous string input = %s\n", inputStr);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }


        // Get string on console
        char* line = linenoise(prompt);
        /* Add the command to the history */
        linenoiseHistoryAdd(line);

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



        // Write
        // printf("Updating restart counter in NVS ... ");
        // restart_counter++;
        // err = nvs_set_i32(my_handle, "restart_counter", restart_counter);
        printf("Writing to next loop. . .\n");
        err = nvs_set_str(my_handle, "str", outputStr);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);

    }

    printf("\n");

    // Restart module
    for (int i = 3; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();

}
