/*
 * Network_config.h
 *
 *  Created on: May 9, 2024
 *  Author: mohammedhamdy32
 */

#ifndef NETWORK_CONFIG_H_
#define NETWORK_CONFIG_H_

/******************** Network application tasks for Freertos ******************/
#define  NETWORK_APP_TASK_STACK_SIZE               (40*1024) 
#define  NETWORK_APP_TASK_PRIORITY                    5
#define  NETWORK_APP_TASK_CORE_ID                     0


/****************** Task configration ****************/
#define STATION                           0
#define ACESS_POINT                       1
#define BOTH_STA_ACESS                    2

#define STATION_ACESS_POINT_BOTH        STATION

/* Staion mode */
#define WIFI_STA_SSID                      "Moh"//"Hamdy Halim" //"WE_B66CE9"
#define WIFI_STA_PASS                      "123456789"// "{A7med_H@mdy}"//"13b09ba4" /* The password should be more than 8 char */
#define WIFI_STA_CHANNEL                         0  /* Set to 1~13 to scan starting from the specified channel before connecting to AP. If the channel of AP is unknown, set it to 0 */     
#define WIFI_STA_SCAN_MODE                 WIFI_FAST_SCAN
#define WIFI_STA_MAX_CONN_TRIES                  5
#define WIFI_STA_STATIC_IP                 "192.168.100.55"  /* Static IP for staion mode */
#define WIFI_STA_AP_GATEWAY 	      	 "192.168.100.1"  /* Gateway for acesses point */
#define WIFI_STA_AP_NETMASK                "255.255.255.0"  /* Netmask for acesses point */


/* Acess point */
#define  WIFI_AP_SSID                "ESP32_AP"
#define  WIFI_AP_PASS                "123456789"  /* Password shoud be larg, or it will gives an error */
#define  WIFI_AP_CHANNEL              1    /* WiFi channel - for ESP32 we can choose from 1-14 channel */
#define  WIFI_APP_SSID_HIDDEN         0    /* Choose to make the SSID hidden or not */
#define  WIFI_APP_MAX_CONNECTIONS     5
#define  WIFI_BEACON_INTERVAL         100  /* beacon is the time lag between each of the beacons sendby AP 
                                              In esp32 from 100ms - 60000ms
                                              In the context of Wi-Fi networks, a "beacon" refers to
											  a type of frame that is periodically broadcasted
											  by wireless access points (APs) or routers. 
											  These beacon frames serve several important functions in Wi-Fi communicatio
											  like Network Advertisement, Synchronization, Power Management and Roaming Assistance	  
										   */
#define  WIFI_AP_IP                   "192.168.0.1"
#define  WIFI_AP_GATEWAY 			  "192.168.0.1"
#define  WIFI_AP_NETMASK              "255.255.255.0"
#define  WIFI_AP_BANDWITH             WIFI_BW_HT20  /* 20MHZ Bandwidth */
#define  WIFI_STA_POWER_SAVE          WIFI_PS_NONE  /* Power save form power consumption */
#define  MAX_SSID_LENGTH              32
#define  MAX_PASSWORD_LENGTH          64
#define  MAX_CONNECTION_RETRIES       5  /* Retry number on disconnect */


/* network interface object for station and acess point */
extern esp_netif_t* esp_netif_sta;
extern esp_netif_t * esp_netif_ap;


#endif /* MAIN_TASKS_COMMAN_H_ */
