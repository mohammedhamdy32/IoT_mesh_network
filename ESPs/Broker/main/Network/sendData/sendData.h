/*
 *  sendData.h
 *
 *  Created on: May 9, 2024
 *  Author: mohammedhamdy32
 */

#ifndef SEND_DATA_H_
#define SEND_DATA_H_

#include "../socket/socket.h"

#define NETWORK_STA_GENERAL_TCP_PORT_NUM                9999
#define NETWORK_STA_IMAGE_TCP_PORT_NUM                  1111
#define NETWORK_STA_IMAGE_UDP_PORT_NUM                  4444
#define NETWORK_STA_VOICE_STREAM_TCP_PORT_NUM           2222
#define NETWORK_STA_VOICE_STREAM_UDP_PORT_NUM           3333

#define NETWORK_STA_RECEIVE_DATA_UDP_PORT_NUM           8888


#define NETWORK_MAX_SEND_HEADER_LENGHT               20
#define NETWORK_MAX_RECV_HEADER_LENGHT               20
#define NETWORK_MAX_VOICE_STREAM_HEADER_LENGHT       20


/***************************************
 *            Header Formate
 *  Each line is 10 bytes size and ends with \n
 * 
 *  The first line is the device ID 
 *  The second line is the type of message
 *  The third line is the connection type (keep alive or close connection)
 *  The fourth line is the size of message
 */
#define  NETWORK_HEADER_LINE_SIZE                    (10)
#define  NETWORK_NUM_OF_LINES_IN_HEADER              (4)
#define  NETWORK_TOTAL_HEADER_SIZE                   ((NETWORK_HEADER_LINE_SIZE)*(NETWORK_NUM_OF_LINES_IN_HEADER))
#define  NETWORK_HEADER_PADDING                      ('-')
#define  NETWORK_HEADER_END_CHAR                     ('\n')


/* Enum for connection type */
typedef enum Network_connection_type{
     KEEP_ALIVE,
     CLOSE_AFTER_SEND,
}Network_connection_type_e;


/**
 *  Message type 
 *  @note Expand this based on your application
 */
typedef enum Network_app_message
{
	SEND_TCP_ONE_BYTE_DATA=0,
	SEND_TCP_TWO_BYTES_DATA,
	SEND_TCP_FOUR_BYTES_DATA,

	SEND_TCP_IMAGE,

	RECEIVE_TCP_DATA,
	SEND_RECEIVE_TCP_DATA,

	SEND_UDP_IMAGE,
	SEND_IMAGE_COMPLETE,

}Network_app_message_e; /* e : for enum */




SOCKET_ERROR_e Network_app_send_TCP_data( uint8_t *a_data , uint32_t a_size , uint16_t a_portNumber );
SOCKET_ERROR_e Network_app_receive_TCP_data( uint8_t *a_data , uint32_t a_size , uint16_t a_portNumber );
SOCKET_ERROR_e Network_app_send_receive_TCP_data( uint8_t *a_sendData , uint8_t *a_reciData , uint32_t a_sendSize , uint32_t a_reciSize , uint16_t a_portNumber );
SOCKET_ERROR_e Network_app_start_TCP_connection( uint16_t a_portNumber , int *a_socketHandler );
SOCKET_ERROR_e Network_app_close_socket( int a_socketHandler );
SOCKET_ERROR_e Network_app_send_TCP_data_only_without_start_TCP_connection( uint8_t *a_data , uint32_t a_size , int a_socketHandler );
SOCKET_ERROR_e Network_app_send_TCP_data_only_without_start_TCP_connection_with_header( Network_app_message_e a_msg_type , Network_connection_type_e a_conn_type , uint8_t *a_data , uint32_t a_size , int a_socketHandler );
SOCKET_ERROR_e Network_app_receive_TCP_data_only_without_start_TCP_connection( int a_socket_handler , uint8_t *a_data , uint32_t a_size );

SOCKET_ERROR_e Network_app_send_TCP_image( uint8_t *a_image_frames , uint32_t a_image_size );
SOCKET_ERROR_e Network_app_send_TCP_data_with_header( Network_app_message_e a_msg_type , Network_connection_type_e a_conn_type , uint8_t *a_data , uint32_t a_size , int *a_socketHandler , uint16_t a_portNumber );
SOCKET_ERROR_e Network_app_send_receive_TCP_data_with_header( uint8_t *a_receiveData , uint32_t a_receiveSize , uint16_t a_portNumber );
SOCKET_ERROR_e Network_app_send_big_TCP_data_with_header( Network_app_message_e a_msg_type , Network_connection_type_e a_conn_type , uint8_t *a_data , uint32_t a_size , int *a_socketHandler , uint16_t a_portNumber , uint32_t *image_bytes_counter );

SOCKET_ERROR_e Network_app_init_UDP(  int * a_socketHandler );
SOCKET_ERROR_e Network_app_send_UDP_data( uint8_t *a_data , uint32_t a_size , int a_socketHandler , uint16_t a_portNumber );
SOCKET_ERROR_e Network_app_send_big_data_over_UDP( uint8_t *a_data , uint32_t a_size , int a_socketHandler , uint16_t a_portNumber );

#endif