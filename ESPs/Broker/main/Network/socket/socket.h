
/* Socket programming */

#ifndef SOCKET_H_
#define SOCKET_H_

/*** Defines ***/
#define WIFI_STA_SERVER_SOCKET_ADDRESS        "192.168.100.138"

/*** Enum ***/
typedef enum socket_error {
    SOCKET_OK = 1 ,
    SOCKET_NO_STATION=-1,
    SOCKET_CREATE_ERROR=-2,
    SOCKET_CONNECTION_ERROR=-3,
    SOCKET_ERROR_IN_SEND=-4,
    SOCKET_ERROR_IN_RECEIVE=-5,
    SOCKET_ERROR_IN_CLOSE=-6,
    SOCKET_ERROR_OTHER=-7
} SOCKET_ERROR_e;



/*** Function prototypes ***/
SOCKET_ERROR_e Socket_start_TCP_connect( int *a_sockHandler , uint16_t a_sockNum  );
SOCKET_ERROR_e Socket_close_socket( int a_sockHandler );
SOCKET_ERROR_e Socket_send_TCP_data( int a_sockHandler , uint8_t *a_data , uint32_t a_size );
SOCKET_ERROR_e Socket_receive_TCP_data( int a_sockHandler , uint8_t *a_data , uint32_t a_data_size );
SOCKET_ERROR_e Socket_init_UDP_socket( int *a_sockHandler );
SOCKET_ERROR_e Socket_send_UDP_data( uint8_t *a_data , uint32_t a_size , int a_sockHandler , uint16_t a_port_num );

/* A global extern variable to know if the wifi is connected or not to the station */
extern uint8_t g_station_connected;



#endif /* MAIN_WIFI_APP_H_ */
