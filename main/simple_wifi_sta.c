#include "simple_wifi_sta.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/ip4_addr.h"
#include "onenet_mqtt.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/semphr.h"

//需要把这两个修改成你家WIFI，测试是否连接成功
#define DEFAULT_WIFI_SSID           "OnePlus"
#define DEFAULT_WIFI_PASSWORD       "czj123123"

#define DEFAULT_AP_SSID           "ESP32-AP"
#define DEFAULT_AP_PASSWORD       "12345678"
static const char *TAG = "wifi";
static esp_netif_t* netid_ap;
static SemaphoreHandle_t scan_sem;

/** 事件回调函数
 * @param arg   用户传递的参数
 * @param event_base    事件类别
 * @param event_id      事件ID
 * @param event_data    事件携带的数据
 * @return 无
*/
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{   
    if(event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:      //WIFI以STA模式启动后触发此事件
            esp_wifi_connect();         //启动WIFI连接
            break;
        case WIFI_EVENT_STA_CONNECTED:  //WIFI连上路由器后，触发此事件
            ESP_LOGI(TAG, "connected to AP");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:   //WIFI从路由器断开连接后触发此事件
            //esp_wifi_connect();             //继续重连
            //ESP_LOGI(TAG,"connect to the AP fail,retry now");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG,"AP connect");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG,"AP disconnect");
            break;
        default:
            break;
        }
    }
    if(event_base == IP_EVENT)                  //IP相关事件
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:           //只有获取到路由器分配的IP，才认为是连上了路由器
                ESP_LOGI(TAG,"get ip address");
                onenet_start();
                break;
        }
    }
}


//WIFI STA初始化
esp_err_t wifi_sta_init(void)
{   
    ESP_ERROR_CHECK(esp_netif_init());  //用于初始化tcpip协议栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());       //创建一个默认系统事件调度循环，之后可以注册回调函数来处理系统的一些事件
    esp_netif_create_default_wifi_sta();    //使用默认配置创建STA对象
    netid_ap=esp_netif_create_default_wifi_ap();    //使用默认配置创建AP对象
    //初始化WIFI
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    //注册事件
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,NULL));

    scan_sem=xSemaphoreCreateBinary();
    xSemaphoreGive(scan_sem);
    //WIFI配置
 /*  wifi_config_t wifi_config = 
    { 
        .sta = 
        { 
            .ssid = DEFAULT_WIFI_SSID,              //WIFI的SSID
            .password = DEFAULT_WIFI_PASSWORD,      //WIFI密码
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,   //加密方式
            
            .pmf_cfg = 
            {
                .capable = true,
                .required = false
            },
        },
    };
*/ 
    //启动WIFI
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );         //设置工作模式为STA
 //   ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );   //设置wifi配置
    ESP_ERROR_CHECK(esp_wifi_start()); 
    
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    return ESP_OK;
}


//进入AP+STA模式
esp_err_t wifi_manage_ap(void)
{
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if(mode==WIFI_MODE_APSTA)
    {
        return ESP_OK;
    }
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_APSTA);

 wifi_config_t wifi_config =
    {
        .ap =
        {
            .authmode=WIFI_AUTH_WPA2_PSK,
            .channel=5,
            .max_connection=3,
            .ssid=DEFAULT_AP_SSID,
            .password=DEFAULT_AP_PASSWORD
        }
    };
    esp_wifi_set_config(WIFI_IF_AP,&wifi_config);

    esp_netif_ip_info_t info;
    IP4_ADDR(&info.ip,192,168,100,1);
    IP4_ADDR(&info.gw,192,168,100,1);
    IP4_ADDR(&info.netmask,255,255,255,0);

    esp_netif_dhcps_stop(netid_ap);
    esp_netif_set_ip_info(netid_ap,&info);
    esp_netif_dhcps_start(netid_ap);

    return esp_wifi_start();
}

static void scan_task(void* param)
{
    uint16_t ap_count=0;
    uint16_t ap_num=10;
    wifi_ap_record_t *aplist =(wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t)*ap_num);
    p_wifi_scan_cb callback =(p_wifi_scan_cb)param;
    esp_wifi_scan_start(NULL,true);
    esp_wifi_scan_get_ap_num(&ap_count);
    esp_wifi_scan_get_ap_records(&ap_num,aplist);
    ESP_LOGI(TAG,"ap count:%d, ap num:%d",ap_count,ap_num);
    if(callback)
    {
        callback(ap_num,aplist);
    }
    free(aplist);
    xSemaphoreGive(scan_sem);
    vTaskDelete(NULL);

}
esp_err_t wifi_scan(p_wifi_scan_cb f)
{
    if(!scan_sem)
    {
        scan_sem = xSemaphoreCreateBinary();
        xSemaphoreGive(scan_sem);
    }
    if(pdTRUE==xSemaphoreTake(scan_sem,0))
    {
        esp_wifi_clear_ap_list();
        return xTaskCreatePinnedToCore(scan_task,"scan",8192,f,3,NULL,0);
    }

    return ESP_OK;
}

esp_err_t wifi_manager_connect(const char* ssid,const char* password)
{
    wifi_config_t wifi_config = 
    {
        .sta = 
        {
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,   //加密方式
        },
    };
    snprintf((char*)wifi_config.sta.ssid,31,"%s",ssid);
    snprintf((char*)wifi_config.sta.password,63,"%s",password);
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if(mode != WIFI_MODE_STA)
    {
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_start();
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
    }
    return ESP_OK;
}