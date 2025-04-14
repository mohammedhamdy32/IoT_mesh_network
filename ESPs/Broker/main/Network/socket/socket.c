/*
 * Network_program.h
 *
 *  Created on: May 9, 2024
 *  Author: mohammedhamdy32
 */

#include "esp_wifi.h"
#include "lwip/netdb.h"
#include "socket.h"
#include "esp_log.h"

/**************************************************************************
 * ESP-IDF uses the open source lwIP lightweight TCP/IP stack, which is a 
 * small independent implementation of the TCP/IP protocols designed
 * specifically for embedded systems, eveloped by Adam Dunkels and focuses
 * on minimizing RAM usage while providing essential TCP/IP functionality.
 * 
 * ESP-IDF supports the following lwIP TCP/IP stack functions:
 *   1) BSD Sockets API : The BSD Sockets API is a common cross-platform 
 *      TCP/IP sockets API that originated in the Berkeley Standard 
 *      Distribution of UNIX but is now standardized in a section of the 
 *      POSIX specification. BSD Sockets are sometimes called POSIX Sockets
 *      or Berkeley Sockets.
 * 
 *   2) Netconn API is enabled but not officially supported for ESP-IDF applications
 * 
*/






/**
 * Socket_start_TCP_connect
 * This function starts a TCP client connection to the server through using socket programing
 * @param a_sockHandler a pointer to the socket handler
 * @param a_sockNum the port number to be connected to
 * @return SOCKET_ERROR_t : negative num if it fails, or SOCKET_OK positive number if it success
*/
SOCKET_ERROR_e Socket_start_TCP_connect( int *a_sockHandler , uint16_t a_sockNum  )
{
    if( g_station_connected == 0 ) /* Not connected yet to a station mode */
	{
		return SOCKET_NO_STATION;
	}

	/* The return from socket() is an integer handle that is used to refer to the socket, not the socket number. */
	/* For both the client and the server applications, the task of creating a socket is the same.
	 * It is an API call to the socket() function. */	
	/*
	 * AF_INET     : use IPv4 wifi network
	 * AF_INET6    : use IPv6 wifi network
	 *
	 * SOCK_STREAM : For TCP
	 * SOCK_DGRAM  : For datagram, UDP
	 * SOCK_RAW    : Raw 
	 * 
	 * IPPROTO_TCP : For TCP protocol
	 * IPPROTO_UDP : For UPD protocol
	 * IPPROTO_IP  : Default protocol according to the socket type
	 */
	int l_sockhandler = socket( AF_INET , SOCK_STREAM , IPPROTO_TCP );
	*a_sockHandler = l_sockhandler; /* Put the socket handler */

	// struct timeval timeout;
	// timeout.tv_sec = 1;   // 5 seconds timeout
	// timeout.tv_usec = 0;  // 0 microseconds

	// lwip_setsockopt(l_sockhandler, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

	/* Unable to open a socket */
	if( l_sockhandler < 0 )
	{
		// close(l_sockhandler);
		return SOCKET_CREATE_ERROR;
		
	}else
	{
	}

    struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
    /* Pass the server address */
    inet_pton(AF_INET, WIFI_STA_SERVER_SOCKET_ADDRESS , &serverAddress.sin_addr.s_addr);   
    /* htonl function does is convert the number into what is called "network byte order". This is the
       byte order that has been chosen by convention to be that used for transmitting unsigned
       multi byte binary data over the internet. */
    serverAddress.sin_port = htons(a_sockNum); /* Pass the server port */
    /* Make client to connect with server */
	// printf( "Socket : Connecting to the server of address %s and port %d ...\n" , WIFI_STA_SERVER_SOCKET_ADDRESS , a_sockNum );
	int l_connectionStatus = connect( l_sockhandler , (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));
    
	if( l_connectionStatus == -1 ) /* Connection fail */
	{
	   return SOCKET_CONNECTION_ERROR;
	}else /* Connection success */
	{
	   return SOCKET_OK;
	}

}

/**
 * Socket_close_TCP_socket
 * This function closes the socket using socket handler
 * @param a_sockHandler : is the socket handler
 * @return SOCKET_ERROR_t
 */
SOCKET_ERROR_e Socket_close_socket( int a_sockHandler )
{
	// ESP_LOGI( "Socket" , "Closing socket handler %d" , a_sockHandler );

	int l_closeStatus = close(a_sockHandler);

	if( l_closeStatus < 0 ) /* Closeing fails */
	{
        return SOCKET_ERROR_IN_CLOSE;
	}else
	{
		// printf("Socket : socket handler %d is closed\n" , a_sockHandler );
		return SOCKET_OK;
	}
}

/**
 * Socket_send_TCP_data
 * This function sends a data throug socket
 * @param a_sockHandler : socket handler number
 * @param a_data : data array
 * @param a_size : data size
 * @return SOCKET_ERROR_t 
*/
SOCKET_ERROR_e Socket_send_TCP_data( int a_sockHandler , uint8_t *a_data , uint32_t a_size )
{
	
	/* send function takes sock handler number, sending data, it's lenght and flag
	 * and it returns -1 if error, or data lenght if sucess.
	 * It uses with connection-oriented protocols like TCP
	 */
	//   Flages
	// MSG_PEEK (recv only)       TCP/UDP    used to inspect message, read message from a socket without removing it from the receive buffer
	// MSG_WAITALL (recv only)    TCP        Unimplemented: Requests that the function block until the full amount of data (len) is received
	// MSG_DONTWAIT (send recv)   TCP/UDP    non-blocking. If the operation cannot be completed immediately, the function returns an error instead of waiting
	// MSG_OOB (send recv)        TCP        (Out-of-Band Data) Unimplemented: send or receive high-priority data 
	// MSG_MORE (send only)       TCP        Delay sending data over a TCP socket because Sender will send more, in esp-idf MAX is 1440 byte
	// MSG_NOSIGNAL               TCP        Uninmplemented: prevents sending a SIGPIPE signal when a send() or write() operation fails due to a broken pipe 

    int l_sendingStatus = send( a_sockHandler , a_data , (uint32_t)a_size , 0 ); /* 0 is flag */
	if( l_sendingStatus < 0 )
	{
	//    printf("Socket : Sending data fails at sock handler %d \n" , a_sockHandler );
	   return SOCKET_ERROR_IN_SEND;
	}else
	{
	   return SOCKET_OK;
	}

}



/**
 * Socket_receive_TCP_data
 * This function recives a data from a socket
 * @param a_sockHandler : socket handler
 * @param a_data : Received data array
 * @param a_data_size : size of received data
 * @return status : -1 if it fails, data size if it sucess
*/
SOCKET_ERROR_e Socket_receive_TCP_data( int a_sockHandler , uint8_t *a_data , uint32_t a_data_size )
{
	int ReceiveStatus = recv( a_sockHandler , a_data , a_data_size , 0 );
	if( ReceiveStatus < 0 ) /* Fails */
	{
	    // printf("Socket : Reciving data fails at sock handler %d \n" , a_sockHandler );
		return SOCKET_ERROR_IN_RECEIVE;
	}else
	{
        // printf("Socket : reciving data sucess at sock handler %d \n" , a_sockHandler );
		return SOCKET_OK;
	}
}




/**
 * Socket_init_UDP_socket
 * This function init UDP socket
 * @param a_sockHandler : (out) socket handler number
 * @return SOCKET_ERROR_t 
*/
SOCKET_ERROR_e Socket_init_UDP_socket( int *a_sockHandler )
{
    /* The return from socket() is an integer handle that is used to refer to the socket, not the socket number. */
	/* For both the client and the server applications, the task of creating a socket is the same.
	 * It is an API call to the socket() function. */	
	/*
	 * AF_INET     : use IPv4 wifi network
	 * AF_INET6    : use IPv6 wifi network
	 *
	 * SOCK_STREAM : For TCP
	 * SOCK_DGRAM  : For datagram, UDP
	 * SOCK_RAW    : Raw 
	 * 
	 * IPPROTO_TCP : For TCP protocol
	 * IPPROTO_UDP : For UPD protocol
	 */
	int l_sockHandler = socket( AF_INET , SOCK_DGRAM , IPPROTO_UDP );

	/* Unable to open a socket */
	if( l_sockHandler < 0 )
	{
		return SOCKET_CREATE_ERROR;
	}

	*a_sockHandler = l_sockHandler;
    return SOCKET_OK;

}


/**
 * Socket_send_UDP_data
 * This function sends a data throug socket using UDP
 * @param a_data 	 : data array
 * @param a_size 	 : data size
 * @param a_sockHandler : socket Handler
 * @param a_port_num : port number
 * @return SOCKET_ERROR_t 
*/
SOCKET_ERROR_e Socket_send_UDP_data( uint8_t *a_data , uint32_t a_size , int a_sockHandler , uint16_t a_port_num )
{

    struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
    /* htonl function does is convert the number into what is called "network byte order". This is the
       byte order that has been chosen by convention to be that used for transmitting unsigned
       multi byte binary data over the internet. */
    serverAddress.sin_port = htons(a_port_num); /* Pass the server port */
    /* Pass the server address */
    inet_pton( AF_INET, WIFI_STA_SERVER_SOCKET_ADDRESS , &serverAddress.sin_addr.s_addr);   


	/* sendto function takes sock handler number, sending data, it's lenght and flag
	 * and it returns -1 if error, or data lenght if sucess
	 * sendto is used with connectionless prtocols like UDP
	 */
    int l_sendingStatus = sendto( a_sockHandler , a_data , a_size , 0 , (const struct sockaddr *) &serverAddress , sizeof(serverAddress) ); /* 0 is flag */
	if( l_sendingStatus < 0 )
	{
	//    printf("Socket : Sending data fails at sock handler %d \n" , a_sockHandler );
	   return SOCKET_ERROR_IN_SEND;
	}else
	{
	   return SOCKET_OK;
	}

}



