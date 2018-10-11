/* Console example — WiFi commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"

/** Arguments used by 'leds_read' function */
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
  printf("%d", join_args.fourbitval->ival[0]);
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
