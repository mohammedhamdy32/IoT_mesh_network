/*
 * Network_program.c
 *
 *  Created on: May 9, 2024
 *  Author: mohammedhamdy32
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/netdb.h"
#include "esp_http_client.h"
#include "esp_netif.h"  /* The purpose of the ESP-NETIF library is twofold:
1)It provides an abstraction layer for the application on top of the TCP/IP stack. This allows applications to choose between IP stacks in the future.
2)The APIs it provides are thread-safe, even if the underlying TCP/IP stack APIs are not. */

#include "Network_config.h"
#include "../error_led/error_led.h"
#include "Network_inteface.h"


/* TAG used for serial message */
static const char TAG[] = "Network app";

/* This golbal variable will indicate if we connected to a station mode or not */
// extern uint8_t  g_station_connected;


/* Receive message task handler */
// extern TaskHandle_t g_receive_message_task_handler;

/* netif object for the station and access point that we extern in wifi_app.h */
esp_netif_t * esp_netif_sta = NULL;
esp_netif_t * esp_netif_ap  = NULL;
uint8_t g_station_connected = 0;

/* Queue handler used to manipulate the main queue of events */
static QueueHandle_t Network_app_queue_handler; /* A build in queue */


/*** Static functions prototypes ***/
static void Network_app_event_handler_init();
static void Network_app_default_wifi_init(void);
static void Network_app_set_static_ip(esp_netif_t *netif);
static void Network_app_station_mode_config(void);
static void Network_app_ap_mode_config(void);
static void Network_app_event_handler( void *arg , esp_event_base_t event_base , uint32_t event_id , void *event_data );
static void Network_app_task(void *pvParameter);

/* Messages handlers */
static void Network_TCP_send_one_byte_data_message_handler(Network_app_queue_message_t *a_msg);
static void Network_TCP_send_two_byte_data_message_handler(Network_app_queue_message_t *a_msg);
static void Network_TCP_send_four_byte_data_message_handler(Network_app_queue_message_t *a_msg);



/*
 *  Initialize wifi application event handler for wifi and IP event
 */
static void Network_app_event_handler_init()
{

	/* Event loop for wifi driver */
	/* The event loop is the bridge between events and event handlers */
	/* This function sets up a system within an ESP32 application to handle events, like Initialization and Event Storage */
	ESP_ERROR_CHECK( esp_event_loop_create_default() ); /* Create a default event loop, we can use esp_event_loop_create but we should pass some parameters to it  */

	/* Event handler for the connection */
	esp_event_handler_instance_t instance_wifi_event;
	esp_event_handler_instance_t instance_ip_event;
	ESP_ERROR_CHECK(esp_event_handler_instance_register( WIFI_EVENT , ESP_EVENT_ANY_ID , &Network_app_event_handler, NULL, &instance_wifi_event));
	ESP_ERROR_CHECK(esp_event_handler_instance_register( IP_EVENT, ESP_EVENT_ANY_ID , &Network_app_event_handler, NULL, &instance_ip_event));
	/* Register an instance مثال of event handler to the default loop.
    This function does the same as esp_event_handler_instance_register_with, except that it registers the handler to the default event loop */

}



/**
 * Initialize TCP stack and default WiFi configuration
 */
static void Network_app_default_wifi_init(void)
{
	/* Initialize network interface (TCP/IP stack)  */
	ESP_ERROR_CHECK( esp_netif_init() );

	/* Default wifi configuration */
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();/*This is a macros for default configuration*/
	/* Always use WIFI_INIT_CONFIG_DEFAULT macro to initialize the configuration to default values, this can
	 *               guarantee all the fields get correct value when more fields are added into wifi_init_config_t
	 *               in future release. If you want to set your own initial values, overwrite the default values
	 *               which are set by WIFI_INIT_CONFIG_DEFAULT. Please be notified that the field 'magic' of
	 *               wifi_init_config_t should always be WIFI_INIT_CONFIG_MAGIC!
	 */

	/* Init WiFi */ 
	ESP_ERROR_CHECK( esp_wifi_init( &wifi_init_config ) );
	ESP_ERROR_CHECK( esp_wifi_set_storage( WIFI_STORAGE_RAM ) ); /* All configuration will store in RAM */
}



/**
 * wifi_app_set_static_ip
 * @param station network interface
 * Give my ESP a static IP address in staion mode
*/
static void Network_app_set_static_ip(esp_netif_t *netif)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(WIFI_STA_STATIC_IP);
    ip.netmask.addr = ipaddr_addr(WIFI_STA_AP_NETMASK);
    ip.gw.addr = ipaddr_addr(WIFI_STA_AP_GATEWAY);
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ip info");
        return;
    }

}


/**
 * Configure the wifi sation mode setting
 */
static void Network_app_station_mode_config(void)
{
    esp_netif_sta = esp_netif_create_default_wifi_sta();   /* Create a WiFi station mode */
    /* STA - wifi staion mode configuration */
	/* It takes ap or sta according to you want station or access point  */
	wifi_config_t sta_config =
	{
			.sta = {
					.ssid        = WIFI_STA_SSID ,
					.password    = WIFI_STA_PASS ,
					.channel     = WIFI_STA_CHANNEL,
					.scan_method = WIFI_FAST_SCAN ,
			},
	};

	
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );  /* setting the mode to station/access point */
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_STA , &sta_config) );  /* Set our configuration */
    ESP_ERROR_CHECK( esp_wifi_set_ps( WIFI_PS_NONE ) ) ; /* No power save */
    
    /* Give   */
    // Network_app_set_static_ip(esp_netif_sta);
	
}


/**
 * Configure the wifi access point setting and assign tthe static IP to the softAP
 */
static void Network_app_ap_mode_config(void)
{

	esp_netif_ap = esp_netif_create_default_wifi_ap();   /* Create a WiFi access point mode */
	/* SoftAP ifi access point configuration */
	/* It takes ap or sta according to you want station or access point  */
	wifi_config_t ap_config =
	{
			.ap = {
					.ssid      =  WIFI_AP_SSID ,
					.ssid_len  =  strlen(WIFI_AP_SSID) ,
					.password  =  WIFI_AP_PASS ,
					.max_connection = WIFI_APP_MAX_CONNECTIONS ,        /* Max number of stations allowed to connect in */
					.channel = WIFI_AP_CHANNEL,
					.ssid_hidden = WIFI_APP_SSID_HIDDEN ,      /* Make it hidden or not */
					.authmode = WIFI_AUTH_WPA2_PSK ,
					.beacon_interval = WIFI_BEACON_INTERVAL ,
					.pmf_cfg = {
                                .required = true,
                               }

			},
	};

	/* Configure DHCP for AP */
	/* DHCP : The Dynamic Host Configuration Protocol is a network management protocol used
	 * on Internet Protocol networks for automatically assigning IP addresses and other communication
	 * parameters to devices connected to the network using a client–server architecture
	 **/

	esp_netif_ip_info_t ap_ip_info;
	memset( &ap_ip_info , 0x00 , sizeof(ap_ip_info) ); /* Fill all ap_ip_info with 0x00  */

	esp_netif_dhcps_stop(esp_netif_ap); /* Stop DHCPS first */
    inet_pton( AF_INET , WIFI_AP_IP , &ap_ip_info.ip );  /* This function changes the ip in string form to numeric binary form and put it in ap_ip_info.ip */
    inet_pton( AF_INET , WIFI_AP_GATEWAY , &ap_ip_info.gw );
    inet_pton( AF_INET , WIFI_AP_NETMASK , &ap_ip_info.netmask );
    ESP_ERROR_CHECK( esp_netif_set_ip_info(esp_netif_ap , &ap_ip_info) ); /* Configure network interface */
    ESP_ERROR_CHECK( esp_netif_dhcps_start(esp_netif_ap) );              /* Start DHCP server  */

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );  /* setting the mode to station/access point */
    ESP_ERROR_CHECK( esp_wifi_set_config( ESP_IF_WIFI_AP , &ap_config) );  /* Set our configuration */
    ESP_ERROR_CHECK( esp_wifi_set_bandwidth( WIFI_IF_AP , WIFI_AP_BANDWITH) );  /* Set our AP Bandwidth */
    // ESP_ERROR_CHECK( esp_wifi_set_ps( WIFI_STA_POWER_SAVE ) ) ;

}


/**
 * Configure the wifi station and access point setting and assign the static IP to the softAP
 */
static void Network_app_station_and_acess_point_modes_config(void)
{
    Network_app_station_mode_config();
    Network_app_ap_mode_config();
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_APSTA) );  /* setting the mode to station/access point */
}

/*
 *   Network_handling_no_station_connection_handler
 *   If the station is disconnected or not connected to the station, This
 *   function will handle it
 */
static void Network_handling_no_station_connection_handler(void)
{
	/* A variable indicat that the it is connected or not  */
	g_station_connected = 0;
	/* Turn on error led */
	error_led_on();
	/* Delay 10ms, then try to connect again */
	vTaskDelay( 10 / portTICK_PERIOD_MS ); 
	ESP_LOGI(TAG, "Retry to connect again...");
	/* Try to connect again */	
	esp_wifi_connect(); 
}

/*
-30 dBm to -50 dBm: Very strong signal
-50 dBm to -60 dBm: Good signal strength
-60 dBm to -70 dBm: Fair signal strength (acceptable for basic tasks but may struggle with high bandwidth applications like video streaming)
-70 dBm to -80 dBm: Weak signal (might experience some performance issues, particularly with speed and reliability)
Below -80 dBm: Very poor signal strength (likely to have connectivity issues and slow speeds)
*/
void print_wifi_rssi() 
{
    // Structure to hold the AP information
    wifi_ap_record_t ap_info;

    // Get information about the connected AP
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        // Print the RSSI value
        ESP_LOGI("WiFi", "RSSI: %d dBm", ap_info.rssi);
    } else {
        ESP_LOGE("WiFi", "Failed to get AP information");
    }
}

/**
 * Network application event handler
 * @param data, data that passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for - WiFi or IP
 * @param event _id the id for the event to rigister the handler for
 * @param event_data is the event data
 */
static void Network_app_event_handler( void *arg , esp_event_base_t event_base , uint32_t event_id , void *event_data )
{
	if( WIFI_EVENT == event_base )
	{
		switch(event_id)
		{
        /*** Station events ***/

		/* The station starts */
		case WIFI_EVENT_STA_START:
			ESP_LOGI( TAG , "WIFI_EVENT_STA_START" );
			esp_wifi_connect();
			break;
        
		/* The station stops */
		case WIFI_EVENT_STA_STOP:
			ESP_LOGI( TAG , "WIFI_EVENT_STA_STOP" );
	        Network_handling_no_station_connection_handler();
			break;

		/* Connected to the station */    
		case WIFI_EVENT_STA_CONNECTED:
		    ESP_LOGI( TAG , "WIFI_EVENT_STA_CONNECTED" );
			g_station_connected = 1;
			error_led_off();
			print_wifi_rssi();
			break;

        /* Disconnected from the station */
		case WIFI_EVENT_STA_DISCONNECTED:
			ESP_LOGI( TAG , "WIFI_EVENT_STA_DISCONNECTED" );
			Network_handling_no_station_connection_handler();
			break;


        /*** Acess point events ***/

        case WIFI_EVENT_AP_START:
			ESP_LOGI( TAG , "WIFI_EVENT_AP_START" );
			break;
		case WIFI_EVENT_AP_STOP:
			ESP_LOGI( TAG , "WIFI_EVENT_AP_STOP" );
			break;
		case WIFI_EVENT_AP_STACONNECTED:
			ESP_LOGI( TAG , "WIFI_EVENT_AP_STACONNECTED" );  
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
			ESP_LOGI( TAG , "WIFI_EVENT_AP_STADISCONNECTED" );  
			break;    


		}
	}else if( IP_EVENT == event_base )
	{
		switch(event_id)
		{
		case IP_EVENT_STA_GOT_IP:
			ESP_LOGI( TAG , "IP_EVENT_STA_GOT_IP" );
			break;
		}
	}
}

/*****************************************************************************
 *                          Message Handlers                                 *
 *****************************************************************************/

/**
 * Network_TCP_send_one_byte_data_message_handler 
 * 
 * @param received_msg : received message
 */
static void Network_TCP_send_one_byte_data_message_handler(Network_app_queue_message_t *a_msg)
{
    ESP_LOGI( TAG , "SEND_TCP_ONE_BYTE_DATA" );
	/* Save the data in an array to send it */
	uint8_t l_1byte_data[1];
	*(uint8_t *)l_1byte_data = (uint8_t)a_msg->data ;

	SOCKET_ERROR_e l_connectionStatus = Network_app_send_TCP_data_with_header( SEND_TCP_ONE_BYTE_DATA , CLOSE_AFTER_SEND , l_1byte_data , 1 , NULL , NETWORK_STA_GENERAL_TCP_PORT_NUM );

	
	if( l_connectionStatus != SOCKET_OK )
	{
		ESP_LOGE( TAG , "SEND_TCP_ONE_BYTE : Error of number is %d " , l_connectionStatus );
	}
}

/**
 * Network_TCP_send_two_byte_data_message_handler 
 *
 * @param received_msg : received message
 */
static void Network_TCP_send_two_byte_data_message_handler(Network_app_queue_message_t *a_msg)
{
    ESP_LOGI( TAG , "SEND_TCP_TWO_BYTES_DATA" );
	/* Save the data in an array to send it */
	uint8_t l_2bytes_data[2];
	*(uint16_t *)l_2bytes_data = (uint16_t)a_msg->data ;

	SOCKET_ERROR_e l_connectionStatus = Network_app_send_TCP_data_with_header( SEND_TCP_TWO_BYTES_DATA , CLOSE_AFTER_SEND ,l_2bytes_data , 2 , NULL , NETWORK_STA_GENERAL_TCP_PORT_NUM );
	if( l_connectionStatus != SOCKET_OK )
	{
		ESP_LOGE( TAG , "SEND_TCP_TWO_BYTES : Error of number is %d " , l_connectionStatus );
	}
}

/**
 * Network_TCP_send_four_byte_data_message_handler 
 * 
 * @param received_msg : received message
 */
static void Network_TCP_send_four_byte_data_message_handler(Network_app_queue_message_t *a_msg)
{
	ESP_LOGI( TAG , "SEND_TCP_FOUR_BYTES_DATA" );
	/* Save the data in an array to send it */
	uint8_t l_4bytes_data[4];
	*(uint32_t *)l_4bytes_data = (uint32_t)a_msg->data ;

	SOCKET_ERROR_e l_connectionStatus = Network_app_send_TCP_data_with_header( SEND_TCP_FOUR_BYTES_DATA , CLOSE_AFTER_SEND , l_4bytes_data , 4 , NULL , NETWORK_STA_GENERAL_TCP_PORT_NUM );
	if( l_connectionStatus != SOCKET_OK )
	{
		ESP_LOGE( TAG , "SEND_TCP_FOUR_BYTES : Error of number is %d " , l_connectionStatus );
	}
}


/***************************************************************************
 *                         Network app task                                *
 ***************************************************************************/
/**
 * Network app task
 * Main task for wifi application which will be in FerrRTOS
 * @param pvParameter is a parameter which can be passed to the task
*/
static void Network_app_task(void *pvParameter)
{
    /* Initialize the event handler */
	Network_app_event_handler_init();

	/* Initialize the TCP/IP stack and WiFi config */
	Network_app_default_wifi_init();


	/*** Config wifi ***/
    /* chosse between staion, access point or both */
    Network_app_station_mode_config();


	/* start WiFi */
	ESP_ERROR_CHECK( esp_wifi_start() );
	esp_wifi_set_max_tx_power(40);


    /* Wait utill connecting to station */
	while( g_station_connected == 0 )
	{
		vTaskDelay( 5 / portTICK_PERIOD_MS ); 
	}

    Network_app_queue_message_t received_msg ; /* Will contain the receives message */

    /* Loop for the receive incoming messages */
	ESP_LOGI( TAG , "Entring infinity loop" );
    for(;;)
    {
        if( xQueueReceive( Network_app_queue_handler , &received_msg , portMAX_DELAY) )
		{
            switch (received_msg.msgID)
            {
				case SEND_TCP_ONE_BYTE_DATA :
					Network_TCP_send_one_byte_data_message_handler(&received_msg);
					break;

				case SEND_TCP_TWO_BYTES_DATA :
					Network_TCP_send_two_byte_data_message_handler(&received_msg);
					break;

				case SEND_TCP_FOUR_BYTES_DATA :
					Network_TCP_send_four_byte_data_message_handler(&received_msg);
					break;

				default:
					break;
            }
        }
    }

}


/*
 * Sent message to the task queue
 * @param msgID : The message ID
 * @return True if the item was successfully sent to the queue, otherwise false
 */
void Network_app_send_message( Network_app_message_e a_msgID ,  uint32_t a_data , uint32_t a_size , TickType_t xTicksToWait )
{
	Network_app_queue_message_t SendingMessage;
	SendingMessage.msgID    = a_msgID;
	SendingMessage.data     = a_data;
	SendingMessage.size     = a_size;

	xQueueSend( Network_app_queue_handler , &SendingMessage , xTicksToWait );

}

void Netwok_app_start(void)
{
	ESP_LOGI( TAG , "Starting Network Application" );

    /* Disable default wifi logging messages */
	esp_log_level_set("wifi" , ESP_LOG_NONE);  /* This function is used to disable all messages from wifi in serial log */

	/* Create the message queue for this task */
	Network_app_queue_handler = xQueueCreate( 50 , sizeof(Network_app_queue_message_t) );

	/* Start Network task in FreeRTOS */
	BaseType_t TaskStatus = xTaskCreatePinnedToCore( &Network_app_task , "Network task" , NETWORK_APP_TASK_STACK_SIZE , NULL , NETWORK_APP_TASK_PRIORITY , NULL , NETWORK_APP_TASK_CORE_ID );

	configASSERT(TaskStatus == pdPASS); /* Is a MACRO, If the condition is false, It will enter an infinity loop */
}


