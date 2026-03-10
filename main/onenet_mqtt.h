#ifndef _ONENET_MQTT_H_
#define _ONENET_MQTT_H_
#include "esp_err.h"

//产品ID
#define  ONENET_PRODUCT_ID  "vxHw9hX8Pr"

//产品秘钥
#define  ONENET_ACCESS_KEY  "OdVFGfVV8RO1u4GJj2MNBzOlWc+lhbMPOe8nVznLues="

//设备名称
#define ONENET_DEVICE_NAME  "esp32_01"

//启动连接云平台
esp_err_t onenet_start(void);
#endif