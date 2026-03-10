#include "ap_wifi.h"
#include "simple_wifi_sta.h"
#include "esp_spiffs.h"
#include "sys/stat.h"
#include <string.h>
#include "ws_server.h"
#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define SPIFFS_MOUNT "/spiffs"
#define HTML_PATH "/spiffs/apcfg.html"
#define TAG "ap_wifi"
#define APCFGBIT (BIT0)

static char current_ssid[32];
static char current_password[64];
static char *html_code = NULL;
static EventGroupHandle_t apcfg_ev=NULL;

static char* init_web_page_buf(void)
{
    esp_vfs_spiffs_conf_t conf =
    {
        .base_path = SPIFFS_MOUNT,
        .format_if_mount_failed = false,
        .max_files = 3,
        .partition_label = NULL
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    struct stat st;
    if(stat(HTML_PATH,&st))
    {
        ESP_LOGE(TAG, "apcfg.html not found");
        return NULL;
    }
    char *buf = (char *)malloc(st.st_size+1);
    memset(buf,0,st.st_size+1);
    FILE *fp =fopen(HTML_PATH,"r");
    if(fp)
    {
        if(0 == fread(buf,st.st_size,1,fp))
        {
            free(buf);
            buf=NULL;
            ESP_LOGE(TAG, "fread failed");
        }
        fclose(fp);
    }
    else
    {
        free(buf);
        buf=NULL;
    }
    return buf;
}

static void ap_wifi_task(void *param)
{
    EventBits_t ev;
    while(1)
    {
        ev=xEventGroupWaitBits(apcfg_ev,APCFGBIT,pdTRUE,pdFALSE,pdMS_TO_TICKS(10000));
        if(ev&APCFGBIT)
        {
            web_ws_stop();
            wifi_manager_connect(current_ssid,current_password);
        }
    }
}

void ap_wifi_init(void)
{
    wifi_sta_init();
    html_code=init_web_page_buf();
    apcfg_ev=xEventGroupCreate();
    xTaskCreatePinnedToCore(ap_wifi_task,"wifi_task",4096,NULL,3,NULL,1);
}



void wifi_scan_handle(int num,wifi_ap_record_t *ap_records)
{
    cJSON *root=cJSON_CreateObject();
    cJSON *wifilist_js=cJSON_AddArrayToObject(root,"wifi_list");
    for(int i=0;i<num;i++)
    {
        cJSON *wifi_js=cJSON_CreateObject();
        cJSON_AddStringToObject(wifi_js,"ssid",(char *)ap_records[i].ssid);
        cJSON_AddNumberToObject(wifi_js,"rssi",ap_records[i].rssi);
        if(ap_records[i].authmode==WIFI_AUTH_OPEN)
            cJSON_AddBoolToObject(wifi_js,"encrypted",0);
        else
            cJSON_AddBoolToObject(wifi_js,"encrypted",1);
        cJSON_AddItemToArray(wifilist_js,wifi_js);
    }
    char *data = cJSON_Print(root);
    ESP_LOGI(TAG,"WS:send%s",data);

    web_ws_send((uint8_t*)data,strlen(data));
    cJSON_free(data);
    cJSON_Delete(root);
}


static void ws_receive_handle(uint8_t *payload,int len)
{
    cJSON* root=cJSON_Parse((char *)payload);
    if(root)
    {
        cJSON *scan_js=cJSON_GetObjectItem(root,"scan");
        cJSON *ssid_js=cJSON_GetObjectItem(root,"ssid");
        cJSON *password_js=cJSON_GetObjectItem(root,"password");
        if(scan_js)
        {
            char *scan_val=cJSON_GetStringValue(scan_js);
            if(strcmp(scan_val,"start")==0)
            {
                wifi_scan(wifi_scan_handle);
            }
        }
        if(ssid_js&&password_js)
        {
            char *ssid_val = cJSON_GetStringValue(ssid_js);
            char *password_val = cJSON_GetStringValue(password_js);
            snprintf(current_ssid,sizeof(current_ssid),"%s",ssid_val);
            snprintf(current_password,sizeof(current_password),"%s",password_val);
            ESP_LOGI(TAG,"Receive ssid:%s,password:%s,now stop http server",current_ssid,current_password);
            xEventGroupSetBits(apcfg_ev,APCFGBIT);
        }
    }
}
//进入配网模式
void ap_wifi_apcfg(void)
{
    wifi_manage_ap();
    ws_cfg_t ws_cfg =
    {
        .html_code=html_code,
        .receive_fn=ws_receive_handle,
    };
    web_ws_start(&ws_cfg);
    
}