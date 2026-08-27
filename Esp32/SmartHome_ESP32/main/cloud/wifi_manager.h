#ifndef __WIFI_MANAGER_H
#define __WIFI_MANAGER_H

/**
 * @brief 初始化 WiFi 管理器
 */
void wifi_manager_init(void);

/**
 * @brief 连接 WiFi
 * @param ssid WiFi 名称
 * @param password WiFi 密码
 * @return 0 成功，-1 失败
 */
int wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief 断开 WiFi 连接
 */
void wifi_manager_disconnect(void);

/**
 * @brief 检查 WiFi 是否连接
 * @return true 已连接，false 未连接
 */
bool wifi_manager_is_connected(void);

#define DEFAULT_SSID "Xiaomi14"
#define DEFAULT_PWD  "qwer123456"
void wifista_init(void);

#endif /* __WIFI_MANAGER_H */