/*

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos_mutex.h"

int mutex_new(osi_mutex_t *p_mutex)
{
    int xReturn = -1;

    *p_mutex = xSemaphoreCreateMutex();
    if (*p_mutex != NULL) {
        xReturn = 0;
    }

    return xReturn;
}

void mutex_lock(osi_mutex_t *p_mutex)
{
    while (xSemaphoreTake(*p_mutex, portMAX_DELAY) != pdPASS);
}

void mutex_unlock(osi_mutex_t *p_mutex)
{
    xSemaphoreGive(*p_mutex);
}

