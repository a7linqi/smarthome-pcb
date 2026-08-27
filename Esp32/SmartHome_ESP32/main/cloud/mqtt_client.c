/**
 * @file mqtt_client.c
 * @brief MQTT 客户端实现（巴法云）
 * @version 1.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"  // ESP-IDF MQTT 头文件
#include "mqtt_handler.h"
#include "mqtt_config.h"
#include "uart_bridge.h"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;

/**
 * @brief MQTT 事件处理函数
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT 连接成功");
        /* 连接成功后订阅主题 */
        esp_mqtt_client_subscribe(client, MQTT_TOPIC, 0);
        ESP_LOGI(TAG, "订阅主题: %s", MQTT_TOPIC);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT 断开连接");
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "收到消息 [%.*s]: %.*s",
                 event->topic_len, event->topic,
                 event->data_len, event->data);

        /* 转发指令给 STM32 */
        {
            char cmd[64];
            int len = event->data_len;
            if (len >= sizeof(cmd)) len = sizeof(cmd) - 1;
            memcpy(cmd, event->data, len);
            cmd[len] = '\0';
            uart_bridge_send_cmd(cmd);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT 错误");
        break;

    default:
        break;
    }
}

/**
 * @brief 初始化 MQTT 客户端
 */
void mqtt_client_init(void)
{
    ESP_LOGI(TAG, "初始化 MQTT 客户端");

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
        .credentials.client_id = MQTT_CLIENT_ID,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
}

/**
 * @brief 启动 MQTT 连接
 */
int mqtt_client_start(void)
{
    if (client == NULL)
    {
        ESP_LOGE(TAG, "MQTT 客户端未初始化");
        return -1;
    }

    ESP_LOGI(TAG, "连接 MQTT 服务器: %s", MQTT_BROKER_URL);
    esp_err_t ret = esp_mqtt_client_start(client);

    return (ret == ESP_OK) ? 0 : -1;
}

/**
 * @brief 发布消息
 */
int mqtt_client_publish(const char *topic, const char *payload)
{
    if (client == NULL)
    {
        return -1;
    }

    int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 0, 0);
    ESP_LOGI(TAG, "发布消息 [%s]: %s (msg_id=%d)", topic, payload, msg_id);

    return (msg_id >= 0) ? 0 : -1;
}

/**
 * @brief 停止 MQTT 连接
 */
void mqtt_client_stop(void)
{
    if (client != NULL)
    {
        esp_mqtt_client_stop(client);
        ESP_LOGI(TAG, "MQTT 已停止");
    }
}
