#include "onenet_mqtt.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "esp_log.h"
#include "cJSON.h"
#include "mqtt_client.h"
#include "onenet_token.h"
#include "cJSON.h"
#include "onenet_dm.h"

#define TAG "mqtt"
static void onenet_subscribe(void);
static esp_mqtt_client_handle_t mqtt_handle = NULL;
static void onenet_property_ack(const char* id,int code,const char* message);
esp_err_t onenet_post_property_data(const char* data);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        onenet_subscribe();
        cJSON* property_js = onenet_property_upload_dm();
        char* data = cJSON_PrintUnformatted(property_js);
        onenet_post_property_data(data);
        cJSON_free(data);
        cJSON_Delete(property_js);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        break;
    case MQTT_EVENT_PUBLISHED:
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        if(strstr(event->topic,"/property/set"))
            {
                cJSON *property_js = cJSON_Parse(event->data);
                cJSON *id_js = cJSON_GetObjectItem(property_js,"id");

                onenet_property_handle(property_js);
                onenet_property_ack(cJSON_GetStringValue(id_js),200,"success");
                cJSON_Delete(property_js);
            }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

//启动连接
esp_err_t onenet_start(void)
{
    esp_mqtt_client_config_t mqtt_config;
    memset(&mqtt_config,0,sizeof(esp_mqtt_client_config_t));
    mqtt_config.broker.address.uri = "mqtt://mqtts.heclouds.com";
    mqtt_config.broker.address.port = 1883;

    mqtt_config.credentials.client_id = ONENET_DEVICE_NAME;
    mqtt_config.credentials.username = ONENET_PRODUCT_ID;

    static char token[256];
    dev_token_generate(token,SIG_METHOD_SHA256,2025245642,ONENET_PRODUCT_ID,ONENET_DEVICE_NAME,ONENET_ACCESS_KEY);
    mqtt_config.credentials.authentication.password = token;
    mqtt_handle = esp_mqtt_client_init(&mqtt_config);

    esp_mqtt_client_register_event(mqtt_handle,ESP_EVENT_ANY_ID,mqtt_event_handler,NULL);
    return esp_mqtt_client_start(mqtt_handle);

}

//属性设置应答
static void onenet_property_ack(const char* id,int code,const char* message)
{
    char topic[128];
    snprintf(topic,sizeof(topic),"$sys/%s/%s/thing/property/set_reply",ONENET_PRODUCT_ID,ONENET_DEVICE_NAME);

    cJSON *reply_js = cJSON_CreateObject();
    cJSON_AddStringToObject(reply_js,"id",id);
    cJSON_AddNumberToObject(reply_js,"code",code);
    cJSON_AddStringToObject(reply_js,"message",message);
    char* data = cJSON_PrintUnformatted(reply_js);
    esp_mqtt_client_publish(mqtt_handle,topic,data,strlen(data),1,0); 
    cJSON_free(data);
    cJSON_Delete(reply_js);
}

//订阅主题
static void onenet_subscribe(void)
{
    char topic[128];
    //订阅上报属性回复主题
    snprintf(topic,sizeof(topic),"$sys/%s/%s/thing/property/post/reply",
        ONENET_PRODUCT_ID,ONENET_DEVICE_NAME);
    esp_mqtt_client_subscribe_single(mqtt_handle,topic,1);
    //订阅下行设置属性主题
    snprintf(topic,sizeof(topic),"$sys/%s/%s/thing/property/set",
        ONENET_PRODUCT_ID,ONENET_DEVICE_NAME);
    esp_mqtt_client_subscribe_single(mqtt_handle,topic,1);
}

//上报数据
esp_err_t onenet_post_property_data(const char* data)
{
    char topic[128];
    snprintf(topic,sizeof(topic),"$sys/%s/%s/thing/property/post",
        ONENET_PRODUCT_ID,ONENET_DEVICE_NAME);
    ESP_LOGI(TAG,"Upload topic:%s,payload:%s",topic,data);
    return esp_mqtt_client_publish(mqtt_handle,topic,data,strlen(data),1,0);
}
