/*
   This code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _EXCEPTION_HANDLING_H_
#define _EXCEPTION_HANDLING_H_

/**
 * @brief  task fatal error handle
 *
 * call task_fatal_error() if a fatal error happened
 *
 * @param[in] no param input
 *
 * @return noreturn
 *
 * */
void __attribute__((noreturn)) task_fatal_error();

#endif
