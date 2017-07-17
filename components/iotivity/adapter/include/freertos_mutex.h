/*

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef _FREERTOS_MUTEX_H_
#define _FREERTOS_MUTEX_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef xSemaphoreHandle osi_mutex_t;

int mutex_new(osi_mutex_t *p_mutex);

void mutex_lock(osi_mutex_t *p_mutex);

void mutex_unlock(osi_mutex_t *p_mutex);

#endif
