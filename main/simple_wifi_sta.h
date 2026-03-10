#ifndef _WIFI_MANAGER_H_
#define _WIFI_MANAGER_H_
#include "esp_err.h"
#include "esp_wifi.h"

typedef void(*p_wifi_scan_cb)(int num,wifi_ap_record_t *ap_records);
//WIFI STA初始化
esp_err_t wifi_sta_init(void);

//WIFI AP初始化
esp_err_t wifi_manage_ap(void);

//WIFI 扫描
esp_err_t wifi_scan(p_wifi_scan_cb f);

//WIFI 连接
esp_err_t wifi_manager_connect(const char* ssid,const char* password);


#endif
