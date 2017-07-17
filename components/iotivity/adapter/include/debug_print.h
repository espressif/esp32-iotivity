/*

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef _DEBUG_PRINT_H_
#define _DEBUG_PRINT_H_

#include "oc_network_events.h"

void print_macro_info();

void print_message_info(oc_message_t *message);

void print_debug(const char* data, const int len, const char* note);

#endif
