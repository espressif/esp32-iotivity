/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "port/oc_clock.h"
#include "port/oc_log.h"
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_deep_sleep.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "apps/sntp/sntp.h"


#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD
static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;
static const char* TAG = "clocktime";
static void obtain_time(void);
static void initialize_sntp(void);
static void initialise_wifi(void);
static esp_err_t event_handler(void *ctx, system_event_t *event);

void
oc_clock_init(void)
{

}
//
//
//
//static void obtain_time(void)
//{
//    initialize_sntp();
//
//    // wait for time to be set
//    time_t now = 0;
//    struct tm timeinfo = { 0 };
//    int retry = 0;
//    const int retry_count = 10;
//    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
//        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//        time(&now);
//        localtime_r(&now, &timeinfo);
//    }
//    ESP_ERROR_CHECK( esp_wifi_stop() );
//}
//
//static void initialize_sntp(void)
//{
//    ESP_LOGI(TAG, "Initializing SNTP");
//    sntp_setoperatingmode(SNTP_OPMODE_POLL);
//    sntp_setservername(0, "pool.ntp.org");
//    sntp_init();
//}

// get clock time from 1970.01.01
oc_clock_time_t
oc_clock_time(void)
{
	oc_clock_time_t tick_time = 0;
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "error:Time is not set yet. Connecting to WiFi and getting time over NTP.");
        //obtain_time();	// update 'now' variable with current time
    }
    time(&now);	// update 'now' variable with current time
    tick_time = (oc_clock_time_t)now * portTICK_PERIOD_MS;
    return tick_time;
}

unsigned long
oc_clock_seconds(void)
{
  time_t now;
  time(&now);
  return now;
}

void
oc_clock_wait(oc_clock_time_t t)
{
	vTaskDelay(t);
}
