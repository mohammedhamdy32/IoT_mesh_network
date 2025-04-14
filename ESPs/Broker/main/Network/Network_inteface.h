/*
 * Network_intercae.h
 *
 *  Created on: May 9, 2024
 *  Author: mohammedhamdy32
 */

#ifndef NETWORK_INTERFCAE_H_
#define NETWORK_INTERFCAE_H_

#include "sendData/sendData.h"

/*
 * Structure for the message queue
 */
typedef struct Network_app_queue_message
{
	Network_app_message_e msgID;
	uint32_t data;
	uint32_t size;
}Network_app_queue_message_t;



/********** Functions prototypes **********/
void Netwok_app_start(void);
void Network_app_send_message( Network_app_message_e a_msgID , uint32_t a_data , uint32_t a_size , uint32_t xTicksToWait );

/*************** Global variables ******************/

/* This golbal variable will indicate if we connected to a station mode or not */
extern uint8_t  g_station_connected;




#endif /* NETWORK_INTERFCAE_H_ */

