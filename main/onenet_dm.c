#include "onenet_dm.h"
#include "led_ws2812.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <string.h>

#define WS2812_GPIO_NUM     GPIO_NUM_26
#define WS2812_LED_NUM      12

static int lightness =0;
static bool led_status =0;
static int ws2812_red =0;
static int ws2812_blue =0;
static int ws2812_green =0;
static ws2812_strip_handle_t ws2812_handle;
/**
 * 物模型数据初始化
 * @param 无
 * @return 无
 */
void onenet_dm_init(void)
{
    ws2812_init(WS2812_GPIO_NUM,WS2812_LED_NUM,&ws2812_handle);
    ledc_timer_config_t led_timer = 
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_TIMER_12_BIT,   //2^12-1
        .freq_hz = 5000,
        .timer_num = LEDC_TIMER_0,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&led_timer));
    //PWM通道初始化
    ledc_channel_config_t led_channel = 
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = GPIO_NUM_27,
        .timer_sel = LEDC_TIMER_0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&led_channel));
    //启用渐变功能，如果要调节占空比，必须启用这句
    ledc_fade_func_install(0);

}

/**
 * 处理onenet下行的数据
 * @param property_js 包含下行数据的json
 * @return 无
 */
void onenet_property_handle(cJSON* property_js)
{
    /*
        {
            "id": "123",
            "version": "1.0",
            "params": {
                "brightness":50,
                "lightswitch":true,
                "rgb":{
                    "red":50,
                    "blue":50,
                    "green":50
                }
            }
        }
    */
   cJSON *params_js=cJSON_GetObjectItem(property_js,"params");
   if(params_js)
   {
        cJSON *name_js=params_js->child;
        while(name_js)
        {
            if(strcmp(name_js->string,"brightness")==0)
            {
                lightness=cJSON_GetNumberValue(name_js);
                int duty=lightness*4095/100;
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,duty,0);
            }
            else if(strcmp(name_js->string,"lightswitch")==0)
            {
                if(cJSON_IsTrue(name_js))
                {
                    lightness=50;
                    led_status=1;
                    int duty=lightness*4095/100;
                    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,duty,0);
                }
                else
                {
                    led_status=0;
                    lightness=0;
                    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,0,0);
                }
            }
            else if (strcmp(name_js->string,"rgb")==0)
            {
                ws2812_red=cJSON_GetNumberValue(cJSON_GetObjectItem(name_js,"red"));
                ws2812_blue=cJSON_GetNumberValue(cJSON_GetObjectItem(name_js,"blue"));
                ws2812_green=cJSON_GetNumberValue(cJSON_GetObjectItem(name_js,"green"));
                for(int i=0;i<WS2812_LED_NUM;i++)
                {
                    ws2812_write(ws2812_handle,i,ws2812_red,ws2812_green,ws2812_blue);
                }
                
            }
            name_js=name_js->next;
        }
    }

}

/**
 * 生成上报所有数据的cJSON对象
 * @param 无
 * @return cJSON对象，包含所有属性值
 */
cJSON* onenet_property_upload_dm(void)
{
    /*
        {
            "id": "123",
            "version": "1.0",
            "params": {
                "brightness":{
                    "value":50
                },
                "lightswitch":{
                    "value":true
                },
                "rgb":{
                    "value":{
                        "red":50,
                        "blue":50,
                        "green":50
                    }
                }
            }
        }
    */
    cJSON *root=cJSON_CreateObject();
    cJSON_AddStringToObject(root,"id","123");
    cJSON_AddStringToObject(root,"version","1.0");
    cJSON *params_js=cJSON_AddObjectToObject(root,"params");
    cJSON *brightness_js=cJSON_AddObjectToObject(params_js,"brightness");
    cJSON_AddNumberToObject(brightness_js,"value",lightness);
    cJSON *lightswitch_js=cJSON_AddObjectToObject(params_js,"lightswitch");
    cJSON_AddBoolToObject(lightswitch_js,"value",led_status);
    cJSON *rgb_js=cJSON_AddObjectToObject(params_js,"rgb");
    cJSON *value_js=cJSON_AddObjectToObject(rgb_js,"value");
    cJSON_AddNumberToObject(value_js,"red",ws2812_red);
    cJSON_AddNumberToObject(value_js,"blue",ws2812_blue);
    cJSON_AddNumberToObject(value_js,"green",ws2812_green);
    return root;
}