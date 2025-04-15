idf_component_register(SRCS "main.c"
"Network/Network_program.c" "Network/socket/socket.c" "Network/sendData/sendData.c" 
"error_led/error_led.c" 
"mqtt/mqtt_program.c"
"uart/uart_program.c"

"KWS/other/micro_features_generator.cc"
"KWS/other/recognize_commands.cc"
"KWS/other/command_responder.cc"
"KWS/other/feature_provider.cc"
"KWS/other/micro_features_generator.cc"
"KWS/other/audio_provider.cc"
"KWS/other/ringbuf.c"

    
"KWS/keyword_spotting_model.cc" 
"KWS/keyword_spotting_program.cc"

                    INCLUDE_DIRS ".")