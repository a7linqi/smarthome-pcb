idf_component_register(SRCS "main.c"
                            "app/task_sensor.c"
                            "app/task_actuator.c"
                            "app/task_cloud.c"
                            "app/task_display.c"
                            "cloud/wifi_manager.c"
                            "cloud/mqtt_client.c"
                            "driver/uart_bridge.c"
                            "driver/dht11.c"
                            "driver/led.c"
                            "driver/buzzer.c"
                            "driver/servo.c"
                    INCLUDE_DIRS "."
                                 "app"
                                 "cloud"
                                 "driver"
                    REQUIRES driver
                             esp_timer
                             esp_wifi
                             uart
                             esp_event
                             esp_netif
                             mqtt
                             log
                             nvs_flash)