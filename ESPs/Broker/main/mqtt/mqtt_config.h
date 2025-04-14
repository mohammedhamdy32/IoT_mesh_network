#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

// MQTT Configuration
#define MQTT_BROKER_URI       "mqtt://192.168.43.91"
#define MQTT_BROKER_PORT      (1883)
#define MQTT_DEFAULT_TOPIC    "test/topic"
#define MQTT_QOS_0_TOPIC      "topic/qos0"

// WiFi Configuration
#define WIFI_SSID             "Moh"
#define WIFI_PASSWORD         "123456789"

// Task Configuration
#define MQTT_TASK_STACK_SIZE  4096
#define MQTT_TASK_PRIORITY    5
#define MQTT_QUEUE_SIZE       10

#endif /* MQTT_CONFIG_H */