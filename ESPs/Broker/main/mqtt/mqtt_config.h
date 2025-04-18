#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

// MQTT Configuration
#define MQTT_BROKER_URI           "mqtt://192.168.43.150" //"mqtt://192.168.43.91" 
#define MQTT_BROKER_PORT          (1883)

#define MQTT_QOS_0_TOPIC          "topic/qos0"

#define MQTT_NODE_1_SENDSOR_TOPIC          "wot/sensors/1"
#define MQTT_NODE_2_SENDSOR_TOPIC          "wot/sensors/2"

#define MQTT_NODE_1_CONTROL_TOPIC          "wot/control/1"
#define MQTT_NODE_2_CONTROL_TOPIC          "wot/control/2"

// WiFi Configuration
#define WIFI_SSID             "Moh"
#define WIFI_PASSWORD         "123456789"

// Task Configuration
#define MQTT_TASK_STACK_SIZE  4096
#define MQTT_TASK_PRIORITY    5
#define MQTT_QUEUE_SIZE       10
#define MQTT_CORE_ID           0

#endif /* MQTT_CONFIG_H */