/* iotivity server example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "oc_api.h"
#include "port/oc_clock.h"
#include "freertos_mutex.h"
#include "exception_handling.h"
#include "lightbulb.h"

#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

static EventGroupHandle_t wifi_event_group;
static const int CONNECTED_BIT = BIT0;

typedef xSemaphoreHandle osi_mutex_t;
static osi_mutex_t m_ev;
static osi_mutex_t m_cv;

static struct timespec ts;
static int quit = 0;
static bool light_state = false;
static const char *TAG = "iotivity server";

static void set_device_custom_property(void *data)
{
    (void)data;
    oc_set_custom_device_property(purpose, "desk lamp");
}

static int app_init(void)
{
    int ret = oc_init_platform("Intel", NULL, NULL);
    ret |= oc_add_device("/oic/d", "oic.d.light", "Kishen's light", "1.0", "1.0",
                         set_device_custom_property, NULL);
    return ret;
}

static void get_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
    (void)user_data;
    PRINT("GET_light:\n");
    oc_rep_start_root_object();

    switch (interface) {
    case OC_IF_BASELINE:
        oc_process_baseline_interface(request->resource);

    case OC_IF_RW:
        oc_rep_set_boolean(root, state, light_state);
        break;

    default:
        break;
    }

    oc_rep_end_root_object();
    oc_send_response(request, OC_STATUS_OK);
    PRINT("Light state %d\n", light_state);
}

// receive data from client
static void post_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
    (void)user_data;
    (void)interface;
    PRINT("POST_light:\n");
    bool state = false;
    oc_rep_t *rep = request->request_payload;

    while (rep != NULL) {
        PRINT("key: %s ", oc_string(rep->name));

        switch (rep->type) {
        case BOOL:
            state = rep->value.boolean;
            PRINT("value: %d\n", state);
            break;

        case INT:
            PRINT("value: %d\n", rep->value.integer);
            // process light info...
            notify_lightbulb_state(rep->value.integer, 0);
            vTaskDelay(50);
            break;

        // ...
        default:
            oc_send_response(request, OC_STATUS_BAD_REQUEST);
            return;
            break;
        }

        rep = rep->next;
    }

    oc_send_response(request, OC_STATUS_CHANGED);
    light_state = state;
}

static void put_light(oc_request_t *request, oc_interface_mask_t interface,
                      void *user_data)
{
    post_light(request, interface, user_data);
}

static void register_resources(void)
{
    oc_resource_t *res = oc_new_resource("/light/1", 1, 0);
    oc_resource_bind_resource_type(res, "oic.r.light");
    oc_resource_bind_resource_interface(res, OC_IF_RW);
    oc_resource_set_default_interface(res, OC_IF_RW);
    oc_resource_set_discoverable(res, true);
    oc_resource_set_periodic_observable(res, 1);
    oc_resource_set_request_handler(res, OC_GET, get_light, NULL);
    oc_resource_set_request_handler(res, OC_POST, post_light, NULL);
    oc_resource_set_request_handler(res, OC_PUT, put_light, NULL);
    oc_add_resource(res);
}

static void signal_event_loop(void)
{
    mutex_lock(&m_ev);
    mutex_unlock(&m_cv);
    mutex_unlock(&m_ev);
}

static void handle_signal(int signal)
{
    (void)signal;
    signal_event_loop();
    quit = 1;
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "ESP32 get an IPV4 Address!");

        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;

    case SYSTEM_EVENT_STA_CONNECTED:
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;

    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        ESP_LOGI(TAG, "ESP32 get an IPV6 Address!");
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
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void iotivity_server_task(void *pvParameter)
{
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "OIC server task started,had Connected to AP with IPv6 address");

    if (mutex_new(&m_cv) || mutex_new(&m_ev)) {
        ESP_LOGE(TAG, "new mutex failed!");
        task_fatal_error();
    }

    oc_clock_time_t currenttick = oc_clock_time();
    int tick_per_ms = portTICK_PERIOD_MS;
    ESP_LOGI(TAG, "tick_per_ms:%d & %llu\n", tick_per_ms, currenttick / portTICK_PERIOD_MS);

    time(&now);
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);

    int init_value = 0;
    static const oc_handler_t handler = {.init = app_init,
                                         .signal_event_loop = signal_event_loop,
                                         .register_resources = register_resources
                                        };

    oc_clock_time_t next_event;

#ifdef OC_SECURITY
    oc_storage_config("./creds");
#endif /* OC_SECURITY */

    init_value = oc_main_init(&handler);

    if (init_value < 0) {
        return task_fatal_error();
    }

    while (quit != 1) {
        next_event = oc_main_poll();
        mutex_lock(&m_ev);

        if (next_event == 0) {
            mutex_unlock(&m_ev);
            mutex_lock(&m_cv);
            mutex_lock(&m_ev);
        } else {
            ts.tv_sec = (next_event / OC_CLOCK_SECOND);
            ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
            mutex_unlock(&m_ev);
            uint32_t timeout = 100;
            xSemaphoreTake(m_cv, timeout / portTICK_PERIOD_MS);
            mutex_lock(&m_ev);
        }

        mutex_unlock(&m_ev);
    }

    oc_main_shutdown();
    vTaskDelete(NULL);
    return;
}

void app_main(void)
{
    nvs_flash_init();
    initialise_wifi();
    xTaskCreate(&iotivity_server_task, "iotivity_server_task", 8192, NULL, 5, NULL);
    xTaskCreate(&lightbulb_damon_task, "lightbulb_damon_task", 8192, NULL, 5, NULL);
}
