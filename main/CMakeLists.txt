idf_component_register(SRCS  "main.c" "ws_server.c" "ap_wifi.c" "simple_wifi_sta.c" "button.c" "onenet_token.c" "onenet_mqtt.c" "onenet_dm.c" "led_ws2812.c"
                    INCLUDE_DIRS "." )

spiffs_create_partition_image(html ../html FLASH_IN_PROJECT)
