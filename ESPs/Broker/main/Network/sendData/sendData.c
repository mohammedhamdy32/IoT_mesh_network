#include "esp_wifi.h"
#include "sendData.h"
#include "esp_log.h"
#include "../../device_info.h"

/***
 * Helper_fun_int_to_string
 * This is a static helper funccation that changes an interger number to a string, to send it to a server
 * @param a_num : The number that I want to convert
 * @param a_str : A string that our string will be saved in
 * @return      : The size of the string
*/
static 
uint16_t Helper_fun_int_to_string(uint32_t a_num , uint8_t *a_str ) 
{

    /* Convert integer to string */
    uint16_t l_counter = 0;
    do 
    {
        a_str[l_counter++] = (a_num % 10) + '0';
        a_num /= 10;
    } while (a_num > 0);

    // Reverse the string
    uint16_t j = l_counter - 1;
    for (uint16_t k = 0; k < j; k++, j--) 
    {
        uint8_t temp = a_str[k];
        a_str[k] = a_str[j];
        a_str[j] = temp;
    }

    // Add null terminator
    a_str[l_counter] = '\0';

	return l_counter+1 ;
}



/***
 * Handle_header
 * This function add the information the header
 * @param  a_header      : pointer for my header
 * @param  a_msg_type    : message type
 * @param  a_conn_type   : connection type
 * @param  a_data_size   : size of data
 * @return               : 0 if there is any error, 1 if no error
*/
static 
uint8_t Handle_header( uint8_t *a_header , Network_app_message_e a_msg_type , Network_connection_type_e a_conn_type , uint32_t a_data_size )
{
   /*** First add device ID to the header ***/
   uint16_t l_itos_len = Helper_fun_int_to_string( DEVICE_ID , a_header ); /* Change from int to string */
   /* Check if the ID is bigger that the max header lenght */
   if( l_itos_len > NETWORK_HEADER_LINE_SIZE )
   {
     return 0;
   }
   /* Add padding to the other remaining space */
   for( uint32_t i=l_itos_len-1 ; i<NETWORK_HEADER_LINE_SIZE-1 ; i++ )
      a_header[i] = NETWORK_HEADER_PADDING;
   /* Add \n at the end of line */
   a_header[NETWORK_HEADER_LINE_SIZE-1] = NETWORK_HEADER_END_CHAR;


   /*** Second add message type to the header ***/
   l_itos_len = Helper_fun_int_to_string( a_msg_type , a_header+NETWORK_HEADER_LINE_SIZE ); /* Change from int to string */
   /* Check if the ID is begger that the max header lenght */
   if( l_itos_len > NETWORK_HEADER_LINE_SIZE )
   {
     return 0;
   }
   /* Add padding to the other remaining space */
   for( uint32_t i=l_itos_len-1 ; i<NETWORK_HEADER_LINE_SIZE-1 ; i++ )
      a_header[NETWORK_HEADER_LINE_SIZE + i] = NETWORK_HEADER_PADDING;
   /* Add \n at the end of line */
   a_header[NETWORK_HEADER_LINE_SIZE*2-1] = NETWORK_HEADER_END_CHAR;


   /*** Third add connection type to the header ***/
   l_itos_len = Helper_fun_int_to_string( a_conn_type , a_header+NETWORK_HEADER_LINE_SIZE*2 ); /* Change from int to string */
   /* Check if the ID is begger that the max header lenght */
   if( l_itos_len > NETWORK_HEADER_LINE_SIZE )
   {
     return 0;
   }
   /* Add padding to the other remaining space */
   for( uint32_t i=l_itos_len-1 ; i<NETWORK_HEADER_LINE_SIZE-1 ; i++ )
      a_header[NETWORK_HEADER_LINE_SIZE*2 + i] = NETWORK_HEADER_PADDING;
   /* Add \n at the end of line */
   a_header[NETWORK_HEADER_LINE_SIZE*3-1] = NETWORK_HEADER_END_CHAR;


   /*** Fourth add data size to the header ***/
   l_itos_len = Helper_fun_int_to_string( a_data_size , a_header+NETWORK_HEADER_LINE_SIZE*3 ); /* Change from int to string */
   /* Check if the ID is begger that the max header lenght */
   if( l_itos_len > NETWORK_HEADER_LINE_SIZE )
   {
     return 0;
   }
   /* Add padding to the other remaining space */
   for( uint32_t i=l_itos_len-1 ; i<NETWORK_HEADER_LINE_SIZE-1 ; i++ )
      a_header[NETWORK_HEADER_LINE_SIZE*3 + i] = NETWORK_HEADER_PADDING ;
   /* Add \n at the end of line */
   a_header[NETWORK_HEADER_LINE_SIZE*4-1] = NETWORK_HEADER_END_CHAR;

   return 1;
}




/**
 * Network_app_send_data
 * 
 * This functions sends a data to a socket then close the connection
 * First open the socket TCP connection
 * then, send a data to through socket then close the connection quicklly
 * @param a_data : sending data
 * @param a_size : data size
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_send_TCP_data( uint8_t *a_data , uint32_t a_size , uint16_t a_portNumber )
{
   /*** Init TCP connection ***/
   int l_socketHandler;
   SOCKET_ERROR_e l_conectionStatus = Socket_start_TCP_connect( &l_socketHandler , a_portNumber );
   if( l_conectionStatus != SOCKET_OK )
   {
     return l_conectionStatus;
   }

   /*** Send the data ***/
   SOCKET_ERROR_e l_sendStatus = Socket_send_TCP_data( l_socketHandler , a_data , a_size );
   if( l_sendStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_sendStatus;
   } 

   /* After sending data, close the connection  */
   SOCKET_ERROR_e l_closeStatus = Socket_close_socket(l_socketHandler);
   return l_closeStatus;

}


/**
 * Network_app_receive_data
 * 
 * This function receives a data from a server
 * First open the socket connection
 * Receive data from a socket then close the connection quicklly
 * @param a_data : saves the received data
 * @param a_size : received data size
 * @param a_portNumber : port number
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_receive_TCP_data( uint8_t *a_data , uint32_t a_size , uint16_t a_portNumber )
{
   /*** Init connection ***/
   int l_socketHandler;
   SOCKET_ERROR_e l_conectionStatus = Socket_start_TCP_connect( &l_socketHandler , a_portNumber );
   if( l_conectionStatus != SOCKET_OK )
   {
      return l_conectionStatus;
   }


   /*** Receive data ***/
   SOCKET_ERROR_e l_receiveStatus = Socket_receive_TCP_data( l_socketHandler , a_data , a_size );
   if( l_receiveStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_receiveStatus;
   } 


   /*** Close connection ***/
   SOCKET_ERROR_e l_closeStatus = Socket_close_socket(l_socketHandler);
   return l_closeStatus;

}



/**
 * Network_app_send_receive_TCP_data
 * 
 * This function Send then receive TCP data
 * First open the socket connection
 * send and receive data from a socket then close the connection quicklly
 * @param a_sendData    : the send data
 * @param a_receiveData : saves the received data
 * @param a_sendSize    : send data size
 * @param a_receiveSize : received data size
 * @param a_portNumber  : port number 
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_send_receive_TCP_data( uint8_t *a_sendData , uint8_t *a_receiveData , uint32_t a_sendSize , uint32_t a_receiveSize , uint16_t a_portNumber )
{
   /* Init connection */
   int l_socketHandler;
   SOCKET_ERROR_e l_conectionStatus = Socket_start_TCP_connect( &l_socketHandler , a_portNumber );
   if( l_conectionStatus != SOCKET_OK )
   {
      return l_conectionStatus;
   }

    /* Send data */
    SOCKET_ERROR_e l_sendStatus = Socket_send_TCP_data( l_socketHandler , a_sendData , a_sendSize );
    if( l_sendStatus != SOCKET_OK )
    {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_sendStatus;
    } 

    /* Receive data */
   SOCKET_ERROR_e l_receiveStatus = Socket_receive_TCP_data( l_socketHandler , a_receiveData , a_receiveSize );
   if( l_receiveStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_receiveStatus;
   } 

    /* Close connection */
    SOCKET_ERROR_e l_closeStatus = Socket_close_socket(l_socketHandler);
    return l_closeStatus;

}



/**
 * Network_app_start_TCP_connection
 * 
 * This function open the socket
 * opem the socket connection
 * 
 * @param a_portNumber    : the socket/port number
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_start_TCP_connection( uint16_t a_portNumber , int *a_socketHandler )
{
   /* Init connection */
   SOCKET_ERROR_e l_conectionStatus = Socket_start_TCP_connect( a_socketHandler , a_portNumber );

   return l_conectionStatus;

}

/**
 * Network_app_close_socket
 * 
 * This function close the socket
 * close the socket connection
 * used for both TCP and UDP
 * 
 * @param a_socketHandler : the socket handler
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_close_socket( int a_socketHandler )
{
   /* Close connection */
   SOCKET_ERROR_e l_closeStatus = Socket_close_socket(a_socketHandler);
   return l_closeStatus;
}



/**
 * Network_app_send_TCP_data_only_without_start_TCP_connection
 * 
 * This functions sends a data to a socket only
 * @param a_data : sending data
 * @param a_size : data size
 * @param a_socketHandler : Handler for the socket
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_send_TCP_data_only_without_start_TCP_connection( uint8_t *a_data , uint32_t a_size , int a_socketHandler )
{
   /* Send data */
   SOCKET_ERROR_e l_sendStatus = Socket_send_TCP_data( a_socketHandler , a_data , a_size );
   return l_sendStatus;
}




/**
 * Network_app_send_TCP_data_only_without_start_TCP_connection_with_header
 * 
 * This function sends a data to a server with a header contains the lenght of message
 * @param a_meg_type      : The type of the message
 * @param a_conn_type     : The type of connection, keep alife or close after send
 * @param a_data          : send data
 * @param a_size          : the size of send array
 * @param a_socketHandler : Handler for the socket
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_send_TCP_data_only_without_start_TCP_connection_with_header( Network_app_message_e a_msg_type , Network_connection_type_e a_conn_type , uint8_t *a_data , uint32_t a_size , int a_socketHandler )
{
   /*** Handle header array ***/
   uint8_t l_header[NETWORK_TOTAL_HEADER_SIZE];
   uint8_t l_status = Handle_header( l_header , a_msg_type ,  a_conn_type , a_size );
   /* There an error in handling header */
   if( l_status != 1 )
   {

   }

   /*** Send the header first ***/
   SOCKET_ERROR_e l_socketStatus = Socket_send_TCP_data( a_socketHandler , (uint8_t *)l_header , NETWORK_TOTAL_HEADER_SIZE );
   if( l_socketStatus != SOCKET_OK )
   {
      return l_socketStatus;
   }

   /*** Send the message ***/
   l_socketStatus = Socket_send_TCP_data( a_socketHandler , (uint8_t *)a_data , a_size );
   if( l_socketStatus != SOCKET_OK )
   {
      return l_socketStatus;
   }


   /*** Finally, close the connection if it's not keep alive ***/
   if( a_conn_type == CLOSE_AFTER_SEND )
   {
      l_socketStatus = Socket_close_socket(a_socketHandler);
   }
   
   return l_socketStatus;

}




/**
 * Network_app_receive_TCP_data_only_without_start_TCP_connection
 * 
 * This function receives a data from a server
 * First open the socket connection
 * Receive data from a socket then close the connection quicklly
 * @param a_socket_handler : socket handler
 * @param a_data           : saves the received data
 * @param a_size           : received data size
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_receive_TCP_data_only_without_start_TCP_connection( int a_socket_handler , uint8_t *a_data , uint32_t a_size )
{
   /* Receive data */
   SOCKET_ERROR_e l_receiveStatus = Socket_receive_TCP_data( a_socket_handler , a_data , a_size );
   return l_receiveStatus;
}



/**
 * Network_app_send_image
 * 
 * This function sends an image to a server
 * First open the socket connection
 * then, send the image frames  through socket 1024 bytes after other 
 * then close the connection quicklly
 * @param a_image_frames : image array
 * @param a_image_size   : image size
 * @return SOCKET_ERROR_e
 **/
#define  MAX_PACKET_SIZE   1024
SOCKET_ERROR_e Network_app_send_TCP_image( uint8_t *a_image_frames , uint32_t a_image_size )
{
   int l_socketHandler;
   SOCKET_ERROR_e l_socketStatus ;
   
   /*** First open TCP connection ***/
	l_socketStatus = Network_app_start_TCP_connection(NETWORK_STA_IMAGE_TCP_PORT_NUM , &l_socketHandler);
   if( l_socketStatus != SOCKET_OK )
   {
      return l_socketStatus;
   }

   /*** Split massage into smaller packets, and send them ***/
	uint32_t image_bytes_counter=0;
   for(  ; (image_bytes_counter)<(a_image_size-MAX_PACKET_SIZE)
      ;(image_bytes_counter)+=MAX_PACKET_SIZE )
   {
     /* Try 4 times if it gives an error */
     for( int i=0 ; i<4 ; i++ )
     { 
        l_socketStatus = Network_app_send_TCP_data_only_without_start_TCP_connection( (uint8_t *)(a_image_frames+(image_bytes_counter)) , MAX_PACKET_SIZE , l_socketHandler );

        if( l_socketStatus == SOCKET_OK )
        {	
            break;
        }
        else
        {
           printf("&");
           vTaskDelay(10/portMAX_DELAY);
        }
     }
     
     if( l_socketStatus == SOCKET_OK )
        printf("+");
     else
        printf("-");
  }

  	/*** Send remain image ***/
	l_socketStatus = Network_app_send_TCP_data_only_without_start_TCP_connection( (uint8_t *)(a_image_frames+(image_bytes_counter)) , MAX_PACKET_SIZE , l_socketHandler );
	if( l_socketStatus == SOCKET_OK )
		printf("+");
	else
		printf("-");

   /*** Send image complete message ***/
	uint8_t l_endMesg[3] = "END";
	l_socketStatus = Network_app_send_TCP_data_only_without_start_TCP_connection( l_endMesg , 3 , l_socketHandler );

    /*** Close connection  ***/
	l_socketStatus = Network_app_close_socket(l_socketHandler);

   return l_socketStatus;

}


/**
 * Network_app_send_TCP_data_with_header
 * 
 * This function sends a data to a server with a header contains the lenght of message
 * @param a_meg_type      : The type of the message
 * @param a_conn_type     : The type of connection, keep alife or close after send
 * @param a_data          : send data
 * @param a_size          : the size of send array
 * @param a_socketHandler : (out) socekt handler used if it's is keep alive
 * @param a_portNumber    : port number
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_send_TCP_data_with_header( Network_app_message_e a_msg_type , Network_connection_type_e a_conn_type , uint8_t *a_data , uint32_t a_size , int *a_socketHandler , uint16_t a_portNumber )
{
   /*** Init TCP connection ***/
   int l_socketHandler;
   SOCKET_ERROR_e l_socketStatus = Socket_start_TCP_connect( &l_socketHandler , a_portNumber );
   if( l_socketStatus != SOCKET_OK )
   {
     return l_socketStatus;
   }


   /*** Handle header array ***/
   uint8_t l_header[NETWORK_TOTAL_HEADER_SIZE];
   uint8_t l_status = Handle_header( l_header , a_msg_type ,  a_conn_type , a_size );
   /* There an error in handling header */
   if( l_status != 1 )
   {

   }

   
   /*** Send the header first ***/
   l_socketStatus = Socket_send_TCP_data( l_socketHandler , (uint8_t *)l_header , NETWORK_TOTAL_HEADER_SIZE );
   if( l_socketStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_socketStatus;
   }


   /*** Send the message ***/
   l_socketStatus = Socket_send_TCP_data( l_socketHandler , (uint8_t *)a_data , a_size );
   if( l_socketStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_socketStatus;
   }


   /*** Finally, close the connection if it's not keep alive ***/
   if( a_conn_type == CLOSE_AFTER_SEND )
   {
      l_socketStatus = Socket_close_socket(l_socketHandler);
   }else
   {
      *a_socketHandler = l_socketStatus;
   }
   return l_socketStatus;

}


/**
 * Network_app_send_big_TCP_data_with_header
 * 
 * This function sends a data to a server with a header contains the lenght of message
 * It is used to send big data like an image
 * @param a_meg_type      : The type of the message
 * @param a_conn_type     : The type of connection, keep alife or close after send
 * @param a_data          : send data
 * @param a_size          : the size of send array
 * @param a_socketHandler : (out) socekt handler used if it's is keep alive
 * @param a_portNumber    : port number
 * @return SOCKET_ERROR_e
 **/
#define MAX_PACKET_SIZE    1024
SOCKET_ERROR_e Network_app_send_big_TCP_data_with_header( Network_app_message_e a_msg_type , Network_connection_type_e a_conn_type , uint8_t *a_data , uint32_t a_size , int *a_socketHandler , uint16_t a_portNumber , uint32_t *image_bytes_counter)
{
   /*** Init connection ***/
   *image_bytes_counter = 0;
   printf("Msg size = %ld, and stat size = %ld\n" , a_size , (*image_bytes_counter) );
   int l_socketHandler;
   SOCKET_ERROR_e l_socketStatus = Socket_start_TCP_connect( &l_socketHandler , a_portNumber );
   if( l_socketStatus != SOCKET_OK )
   {
     return l_socketStatus;
   }
   
   /*** Handle header array ***/
   uint8_t l_header[NETWORK_TOTAL_HEADER_SIZE];
   uint8_t l_status = Handle_header( l_header , a_msg_type ,  a_conn_type , a_size );
   /* There an error in handling header */
   if( l_status != 1 )
   {
      return SOCKET_ERROR_OTHER;
   }
   // printf("Num:%c%c%c%c%c%c",l_header[30],l_header[31],l_header[32],l_header[33],l_header[34],l_header[35]);


   /*** Send the header ***/
   l_socketStatus = Socket_send_TCP_data( l_socketHandler , (uint8_t *)l_header , NETWORK_TOTAL_HEADER_SIZE );
   if( l_socketStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_socketStatus;
   }


   /* Split massage into smaller packets, and send them */
   // uint32_t image_bytes_counter ;
   for(  ; (*image_bytes_counter)<(a_size-MAX_PACKET_SIZE)
       ;(*image_bytes_counter)+=MAX_PACKET_SIZE )
   {
      printf("%ld %ld\n" , (*image_bytes_counter) , a_size );
      for( int k=0 ; k<4 ; k++ )
      {
         l_socketStatus = Socket_send_TCP_data( l_socketHandler , (uint8_t *)(a_data+(*image_bytes_counter)) , MAX_PACKET_SIZE );
         if( l_socketStatus == SOCKET_OK )
         {
            break;
         }
      }

      if( l_socketStatus != SOCKET_OK )
      {
         printf("-");
         vTaskDelay(100/portMAX_DELAY);
         Socket_close_socket(l_socketHandler);  /* Close connection */
         return l_socketStatus;
      }else
      {
         printf("+"); //131286
      }  
   }

   /* Send remain image */
   l_socketStatus = Socket_send_TCP_data( l_socketHandler , (uint8_t *)(a_data+(*image_bytes_counter)) , a_size - *image_bytes_counter );
   if( l_socketStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_socketStatus;
   }

   /* Send Finish */
   uint8_t finish = 'F';
   l_socketStatus = Socket_send_TCP_data( l_socketHandler , (uint8_t *)&finish , 1 );
   if( l_socketStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_socketStatus;
   }

   /* Finally, close the connection if it's not keep alive */
   if( a_conn_type == CLOSE_AFTER_SEND )
   {
      l_socketStatus = Socket_close_socket(l_socketHandler);
   }else
   {
      *a_socketHandler = l_socketStatus;
   }

   printf("\nFinish\n");
   return l_socketStatus;

}


/**
 * Network_app_send_receive_TCP_data_with_header
 * 
 * This function Send the heder then receive a data
 * First open the socket connection
 * then send device ID then receive data from a socket 
 * then close the connection quicklly
 * @param a_receiveData : saves the received data
 * @param a_receiveSize : received data size
 * @param a_portNumber  : port number 
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_send_receive_TCP_data_with_header( uint8_t *a_receiveData , uint32_t a_receiveSize , uint16_t a_portNumber )
{
   /*** Init connection ***/
   int l_socketHandler;
   SOCKET_ERROR_e l_socketStatus = Socket_start_TCP_connect( &l_socketHandler , a_portNumber );
   if( l_socketStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_socketStatus;
   }


   /*** Handle header array ***/
   uint8_t l_header[NETWORK_TOTAL_HEADER_SIZE];
   uint8_t l_status = Handle_header( l_header , RECEIVE_TCP_DATA , CLOSE_AFTER_SEND , 0 );
   /* There an error in handling header */
   if( l_status != 1 )
   {
      return SOCKET_ERROR_OTHER;
   }


   /*** Send the header ***/
   l_socketStatus = Socket_send_TCP_data( l_socketHandler , (uint8_t *)l_header , NETWORK_TOTAL_HEADER_SIZE );
   if( l_socketStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_socketStatus;
   }


   /* Receive data */
   // struct timeval timeout;
   // timeout.tv_sec =  0 ;   // 5 seconds timeout
   // timeout.tv_usec = 10;  // 0 microseconds
   // lwip_setsockopt(l_socketHandler, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

   SOCKET_ERROR_e l_receiveStatus = Socket_receive_TCP_data( l_socketHandler , a_receiveData , a_receiveSize );
   if( l_receiveStatus != SOCKET_OK )
   {
      Socket_close_socket(l_socketHandler);  /* Close connection */
      return l_receiveStatus;
   } 

    /* Close connection */
    SOCKET_ERROR_e l_closeStatus = Socket_close_socket(l_socketHandler);
    return l_closeStatus;

}


/**
 * Network_app_init_UDP
 * 
 * This functions sends a data over UDP
 * @param a_socketHandler : (out) socket handler
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_init_UDP( int * a_socketHandler  )
{
   /* Send data */
   SOCKET_ERROR_e l_sendStatus = Socket_init_UDP_socket( a_socketHandler );
   return l_sendStatus;
}

/**
 * Network_app_send_data_over_UDP
 * 
 * This functions sends a data over UDP
 * @param a_data          : sending data
 * @param a_size          : data size
 * @param a_socketHandler : data size
 * @param a_portNumber    : port number
 * @return SOCKET_ERROR_e
 **/
SOCKET_ERROR_e Network_app_send_UDP_data( uint8_t *a_data , uint32_t a_size , int a_socketHandler , uint16_t a_portNumber )
{
   /* Send data */
   SOCKET_ERROR_e l_sendStatus = Socket_send_UDP_data( a_data , a_size , a_socketHandler , a_portNumber );
   return l_sendStatus;
}



/**
 * Network_app_send_big_data_over_UDP
 * 
 * This functions sends a big data over UDP, by spliting it to smaller parts
 * @param a_data          : sending data
 * @param a_size          : data size
 * @param a_socketHandler : socket handler
 * @param a_portNumber    : port number
 * 
 * @return SOCKET_ERROR_e
 **/
#define IMG_HEADER_SIZE          10
#define IMG_PACKET_SIZE         1014
#define IMG_PADDING              '-'
#define IMG_TOTAL_PACKET_SIZE   (IMG_HEADER_SIZE+IMG_PACKET_SIZE)
#define NUM_OF_TRIES              3
SOCKET_ERROR_e Network_app_send_big_data_over_UDP( uint8_t *a_data , uint32_t a_size , int a_socketHandler , uint16_t a_portNumber )
{

   /* Split massage into smaller packets, and send them */
   uint32_t image_bytes_counter ;
   SOCKET_ERROR_e l_socketStatus;
   uint8_t temp_send_arr[IMG_TOTAL_PACKET_SIZE]; /* Contains my send message each time */
   for( image_bytes_counter=0 ; image_bytes_counter<(a_size-IMG_TOTAL_PACKET_SIZE)
       ;image_bytes_counter+=IMG_TOTAL_PACKET_SIZE )
   {
      /* Fill temp_send_arr with header and data */
      vTaskDelay(15/portTICK_PERIOD_MS);
      /* Fill header */
      int i;
      for( i=0 ; i<DEVICE_ID_STR_SIZE ; i++ )
      {
         temp_send_arr[i] = DEVICE_ID_STR[i] ;
      }
      /* Add padding to remain space */
      for( ;i<10;i++ )
      {
         temp_send_arr[i] = IMG_PADDING ;
      }
      /* Add the data to the array */
      for(int j=0;j<IMG_TOTAL_PACKET_SIZE;j++,i++)
      {
         temp_send_arr[i] = ((int8_t *)(a_data+image_bytes_counter))[j] ; 
      }

      /* Send packet, and make 3 tries for each packet */
      for( int k=0 ; k<NUM_OF_TRIES ; k++ )
      {
         l_socketStatus = Socket_send_UDP_data( (uint8_t *)(temp_send_arr) , IMG_TOTAL_PACKET_SIZE , a_socketHandler , a_portNumber );
         if( l_socketStatus == SOCKET_OK )
         {
            break;   /* break if it sends correctly */
         }  
         vTaskDelay(10/portTICK_PERIOD_MS);
      }

      if( l_socketStatus != SOCKET_OK )
      {
         printf("-");
      }else
      {
         printf("+");
      }

   }

   /* Send remain image */
   uint16_t remain_data_size = a_size - image_bytes_counter;
   /*Fill header*/
   int i;
   for( i=0 ; i<DEVICE_ID_STR_SIZE ; i++ )
   {
      temp_send_arr[i] = DEVICE_ID_STR[i] ;
   }
   /* Add padding to remain space */
   for( ;i<10;i++ )
   {
      temp_send_arr[i] = IMG_PADDING ;
   }
   /* Add the data to the array */
   for(int j=0;j<remain_data_size;j++,i++)
   {
      temp_send_arr[i] = (uint8_t *)(a_data+image_bytes_counter)[j]; 
   }
   l_socketStatus = Socket_send_UDP_data( (uint8_t *)(temp_send_arr) , remain_data_size , a_socketHandler , a_portNumber );
   if( l_socketStatus != SOCKET_OK )
   {
      // Socket_close_socket(l_socketHandler);  /* Close connection */
      // return l_socketStatus;
   }

   /* Finally, close the connection if it's not keep alive */
   l_socketStatus = Network_app_close_socket(a_socketHandler);

   /* Send a image complete msg to server, to know that sending finshes */
   uint8_t l_complete = 'C';
   Network_app_send_TCP_data_with_header( SEND_IMAGE_COMPLETE , CLOSE_AFTER_SEND , &l_complete , 1 , NULL , NETWORK_STA_GENERAL_TCP_PORT_NUM );

   printf("\nFinish\n");
   return l_socketStatus;

}
