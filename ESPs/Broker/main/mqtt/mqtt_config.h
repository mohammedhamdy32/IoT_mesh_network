#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

// MQTT Configuration
#define MQTT_BROKER_URI           "mqtt://192.168.43.150" //"mqtt://192.168.43.91" 
#define MQTT_BROKER_PORT          (1883)
#define MQTT_DATA_TOPIC           "home/data"
#define MQTT_ESP_CONTROL_TOPIC    "home/esp_control"
#define MQTT_RASS_CONTROL_TOPIC   "home/rass_control"
#define MQTT_QOS_0_TOPIC          "topic/qos0"

// WiFi Configuration
#define WIFI_SSID             "Moh"
#define WIFI_PASSWORD         "123456789"

// Task Configuration
#define MQTT_TASK_STACK_SIZE  4096
#define MQTT_TASK_PRIORITY    5
#define MQTT_QUEUE_SIZE       10
#define MQTT_CORE_ID           0

#endif /* MQTT_CONFIG_H */