/*
   This code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef _FREERTOS_MUTEX_H_
#define _FREERTOS_MUTEX_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef xSemaphoreHandle osi_mutex_t;

/**
 * @brief  new a mutex
 *
 * init a mutex
 *
 * @param[in]  p_mutex: an address that points struct osi_mutex_t
 *
 * @return  0:       OK
 *         -1:       failed
 *
 * */
int mutex_new(osi_mutex_t *p_mutex);

/**
 * @brief  lock a mutex
 *
 * lock a mutex once until interval portMAX_DELAY passed
 *
 * @param[in]  p_mutex: an address that points struct osi_mutex_t
 *
 * @return  0:       OK
 *         -1:       failed
 *
 * */
int mutex_lock(osi_mutex_t *p_mutex);

/**
 * @brief  unlock a mutex
 *
 * unlock a mutex
 *
 * @param[in]  p_mutex: an address that points struct osi_mutex_t
 *
 * @return  0:       OK
 *         -1:       failed
 *
 * */
int mutex_unlock(osi_mutex_t *p_mutex);

/**
 * @brief  delete a mutex
 *
 * delete a mutex if a mutex by mutex_new created
 *
 *@param[in]  p_mutex: an address that points struct osi_mutex_t
 *
 * @return  noturn
 *
 * */
void mutex_delete(osi_mutex_t *p_mutex);

#endif
