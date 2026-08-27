/**
 * @file mqtt_handler.h
 * @brief MQTT 客户端接口
 * @version 1.0
 */

#ifndef __MQTT_HANDLER_H
#define __MQTT_HANDLER_H

/**
 * @brief 初始化 MQTT 客户端
 */
void mqtt_client_init(void);

/**
 * @brief 启动 MQTT 连接
 * @return 0 成功，-1 失败
 */
int mqtt_client_start(void);

/**
 * @brief 发布消息
 * @param topic 主题
 * @param payload 消息内容
 * @return 0 成功，-1 失败
 */
int mqtt_client_publish(const char *topic, const char *payload);

/**
 * @brief 停止 MQTT 连接
 */
void mqtt_client_stop(void);

#endif /* __MQTT_HANDLER_H */
