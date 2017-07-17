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
//
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "lwip/netdb.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include <sys/socket.h>

#include "oc_api.h"
#include "port/oc_clock.h"
#include "apps/sntp/sntp.h"


#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD



static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;

typedef xSemaphoreHandle osi_mutex_t;

static osi_mutex_t mutex;

static osi_mutex_t cv;

static struct timespec ts;
static int quit = 0;
static const char * TAG = "iotivityclient";
static void
set_device_custom_property(void *data)
{
  (void)data;
  oc_set_custom_device_property(purpose, "operate lamp");
}

static int
app_init(void)
{
  int ret = oc_init_platform("Apple", NULL, NULL);
  ret |= oc_add_device("/oic/d", "oic.d.phone", "Kishen's IPhone", "1.0", "1.0",
                       set_device_custom_property, NULL);
  return ret;
}

#define MAX_URI_LENGTH (30)
static char light_1[MAX_URI_LENGTH];
static oc_server_handle_t light_server;
static bool light_state = false;

static oc_event_callback_retval_t
stop_observe(void *data)
{
  (void)data;
  PRINT("Stopping OBSERVE\n");
  oc_stop_observe(light_1, &light_server);
  return DONE;
}

static void
post_light(oc_client_response_t *data)
{
  PRINT("POST_light:\n");
  if (data->code == OC_STATUS_CHANGED)
    PRINT("POST response OK\n");
  else
    PRINT("POST response code %d\n", data->code);
}

static void
observe_light(oc_client_response_t *data)
{
  PRINT("OBSERVE_light:\n");
  oc_rep_t *rep = data->payload;
  while (rep != NULL) {
    PRINT("key %s, value ", oc_string(rep->name));
    switch (rep->type) {
    case BOOL:
      PRINT("%d\n", rep->value.boolean);
      light_state = rep->value.boolean;
      break;
    default:
      break;
    }
    rep = rep->next;
  }

  if (oc_init_post(light_1, &light_server, NULL, &post_light, LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, !light_state);
    oc_rep_end_root_object();
    if (oc_do_post())
      PRINT("Sent POST request\n");
    else
      PRINT("Could not send POST\n");
  } else
    PRINT("Could not init POST\n");
}

static oc_discovery_flags_t
discovery(const char *di, const char *uri, oc_string_array_t types,
          oc_interface_mask_t interfaces, oc_server_handle_t *server,
          void *user_data)
{
  (void)di;
  (void)interfaces;
  (void)user_data;
  int i;
  int uri_len = strlen(uri);
  uri_len = (uri_len >= MAX_URI_LENGTH) ? MAX_URI_LENGTH - 1 : uri_len;

  for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
    char *t = oc_string_array_get_item(types, i);
    if (strlen(t) == 11 && strncmp(t, "oic.r.light", 11) == 0) {
      memcpy(&light_server, server, sizeof(oc_server_handle_t));

      strncpy(light_1, uri, uri_len);
      light_1[uri_len] = '\0';

      oc_do_observe(light_1, &light_server, NULL, &observe_light, LOW_QOS,
                    NULL);
      oc_set_delayed_callback(NULL, &stop_observe, 30);
      return OC_STOP_DISCOVERY;
    }
  }
  return OC_CONTINUE_DISCOVERY;
}

static void
issue_requests(void)
{
  oc_do_ip_discovery("oic.r.light", &discovery, NULL);
}


int
mutex_cv_new()
{
    int xReturn = -1;

    mutex = xSemaphoreCreateMutex();
    cv = xSemaphoreCreateMutex();
    if (mutex != NULL && cv!= NULL) {
        xReturn = 0;
    }

    return xReturn;
}

void
mutex_lock()
{
	while (xSemaphoreTake(mutex, portMAX_DELAY) != pdPASS);
}

void
mutex_unlock()
{
	 xSemaphoreGive(mutex);
}

void
cv_lock()
{
	while (xSemaphoreTake(cv, portMAX_DELAY) != pdPASS);
}

void
cv_unlock()
{
	 xSemaphoreGive(cv);
}

static void
signal_event_loop(void)
{

  mutex_lock();
  //pthread_cond_signal(&cv);

  cv_unlock();

  mutex_unlock();
}

static void
handle_signal(int signal)
{
  (void)signal;
  signal_event_loop();
  quit = 1;
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}


static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

}

// get clock time from 1970.01.01
oc_clock_time_t
client_oc_clock_time(void)
{
	oc_clock_time_t tick_time = 0;
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();	// update 'now' variable with current time
    }
    time(&now);	// update 'now' variable with current time
    tick_time = (oc_clock_time_t)now * portTICK_PERIOD_MS;
    return tick_time;
}

static int iotivity_client_task(void* pvParameter){
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "iotivity_client task had Connected to AP");


    if(!mutex_cv_new())
    	printf("MUTEX init success!\n");
	oc_clock_time_t currenttick = client_oc_clock_time();
	int clockpersecond = portTICK_PERIOD_MS;
    printf("OC_CLOCK_SECOND:%d & %llu\n",clockpersecond, currenttick/portTICK_PERIOD_MS);

	  int init_value = 0;

#ifdef OC_CLIENT
	  printf("oc client defined!\n");
#else
	  printf("oc client not defined!\n");
#endif

	  static const oc_handler_t handler = {.init = app_init,
	                                       .signal_event_loop = signal_event_loop,
	                                       .requests_entry = issue_requests };


	  oc_clock_time_t next_event;


	#ifdef OC_SECURITY
	  oc_storage_config("./creds");
	#endif /* OC_SECURITY */

	  init_value = oc_main_init(&handler);

//	  printf("file:%s function:%s line:%d init_value:%d\n",__FILE__,__FUNCTION__,__LINE__,init_value);
	  if (init_value < 0)
	    return init_value;

	//  printf("file:%s function:%s line:%d\n",__FILE__,__FUNCTION__,__LINE__);
	  while (quit != 1) {

	    next_event = oc_main_poll();

	    mutex_lock();
	    if (next_event == 0) {
	     // pthread_cond_wait(&cv, &mutex);
	        mutex_unlock();
	        cv_lock();
	        mutex_lock();

	    } else {
	      ts.tv_sec = (next_event / OC_CLOCK_SECOND);
	      ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
	      //pthread_cond_timedwait(&cv, &mutex, &ts);
	      mutex_unlock();
	      //mutex_lock(&cv);
	      uint32_t timeout = 100;
	      xSemaphoreTake(cv, timeout / portTICK_PERIOD_MS);

	      mutex_lock();
	    }
	    mutex_unlock();
	  }
	  printf("file:%s function:%s line:%d\n",__FILE__,__FUNCTION__,__LINE__);
	  oc_main_shutdown();
	  vTaskDelete(NULL);
	  return 0;
}


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG,"ESP32 get an IPV4 Address!");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:	// ipv4 had connected!
    	tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
    	break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
    	xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    	//tcpip_adapter_get_ip6_linklocal();
		break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void app_main(void)
{
	nvs_flash_init();
	initialise_wifi();

	xTaskCreate(&iotivity_client_task, "iotivity_client_task", 8192*4, NULL, 5, NULL);
}
