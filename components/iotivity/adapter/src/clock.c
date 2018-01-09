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

#include "esp_log.h"
#include "apps/sntp/sntp.h"

#include "port/oc_clock.h"

//all of the function declarations are under the iotivity-constrained/port/oc_clock.h

static const char *TAG = "clock";

void oc_clock_init(void)
{
    // do nothing
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "init SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    // set sntp server after got ip address, you had better to adjust the sntp server to your area
    sntp_setservername(0, "pool.ntp.org");
    //    for set more sntp server, plz modify macro SNTP_MAX_SERVERS in sntp_opts.h file
    //    sntp_setservername(1, "202.112.29.82");
    //    sntp_setservername(2, "ntp.sjtu.edu.cn");
    //    sntp_setservername(3, "0.nettime.pool.ntp.org");
    //    sntp_setservername(4, "time-b.nist.gov");
    //    sntp_setservername(5, "time-a.timefreq.bldrdoc.gov");
    //    sntp_setservername(6, "time-b.timefreq.bldrdoc.gov");
    //    sntp_setservername(7, "time-c.timefreq.bldrdoc.gov");
    //    sntp_setservername(8, "utcnist.colorado.edu");
    //    sntp_setservername(9, "time.nist.gov");

    sntp_init();
}

static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry) {
        ESP_LOGW(TAG, "waiting for system time to be set... (retry times:%d)", retry);
        vTaskDelay(2000 / portTICK_RATE_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

// get total tick counts from 1970.01.01
// actual time(i.e. https://unixtime.51240.com) = ret / portTICK_RATE_MS
oc_clock_time_t oc_clock_time(void)
{
    oc_clock_time_t tick_time = 0;
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGW(TAG, "time does not be set yet. please connecting to WiFi and getting time over NTP");
    }

    // update 'now' variable with current time
    time(&now);
    tick_time = (oc_clock_time_t)now * portTICK_PERIOD_MS;
    return tick_time;
}

unsigned long oc_clock_seconds(void)
{
    time_t now;
    time(&now);
    return now;
}

void oc_clock_wait(oc_clock_time_t t)
{
    vTaskDelay(t);
}


void start_sntp()
{
    obtain_time();
}
