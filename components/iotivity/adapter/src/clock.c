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
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "port/oc_clock.h"
#include "port/oc_log.h"
#include "lwip/timers.h"
#include <time.h>

void
oc_clock_init(void)
{
}

oc_clock_time_t
oc_clock_time(void)
{
    oc_clock_time_t time = 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    time = now.tv_sec * portTICK_RATE_MS + now.tv_usec / (1000 * 1000 / portTICK_RATE_MS);
    return time;
}

unsigned long
oc_clock_seconds(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec;
}

void
oc_clock_wait(oc_clock_time_t t)
{
  vTaskDelay(t);
}
