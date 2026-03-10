#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "simple_wifi_sta.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "button.h"
#include "ap_wifi.h"
#include "onenet_dm.h"

#define BTN_GPIO    GPIO_NUM_39
#define LED_GPIO    GPIO_NUM_27
#define EV_WIFI_CONNECTED_BIT (BIT0)
//按键事件组
static EventGroupHandle_t s_pressEvent;
static EventGroupHandle_t s_wifi_ev = NULL;
#define LONG_EV     BIT1    //长按

/** 长按按键回调函数
 * @param 无
 * @return 无
*/
void long_press_handle(void)
{
    //xEventGroupSetBits(s_pressEvent,LONG_EV);
    ap_wifi_apcfg();
}


/** 完整的按键+LED演示程序
 * @param 无
 * @return 无
*/
void complete_btn_test(void* arg)
{
    s_pressEvent = xEventGroupCreate();
    button_config_t btn_cfg = 
    {
        .gpio_num = BTN_GPIO,       //gpio号
        .active_level = 0,          //按下的电平
        .long_press_time = 3000,    //长按时间
        .long_cb = long_press_handle             //长按回调函数
    };
    button_event_set(&btn_cfg);     //添加按键响应事件处理
    EventBits_t ev;
    while(1)
    {
        //等待按键按下事件
        ev = xEventGroupWaitBits(s_pressEvent,LONG_EV,pdTRUE,pdFALSE,portMAX_DELAY);
        if(ev & LONG_EV)
        {
            ap_wifi_apcfg();
        }
        xEventGroupClearBits(s_pressEvent,LONG_EV);
    }
}

void app_main(void)
{
    //NVS初始化（WIFI底层驱动有用到NVS，所以这里要初始化）
    nvs_flash_init();
    onenet_dm_init();
    s_wifi_ev = xEventGroupCreate();
    //wifi STA工作模式初始化
    ap_wifi_init();
    
    button_config_t btn_cfg = 
    {
        .gpio_num = BTN_GPIO,       //gpio号
        .active_level = 0,          //按下的电平
        .long_press_time = 3000,    //长按时间
        .long_cb = long_press_handle             //长按回调函数
    };
    button_event_set(&btn_cfg);     //添加按键响应事件处理

    /*EventBits_t ev;
    while(1)
    {
        ev=xEventGroupWaitBits(s_wifi_ev,EV_WIFI_CONNECTED_BIT,pdTRUE,pdFALSE,portMAX_DELAY);
        if(ev&EV_WIFI_CONNECTED_BIT)
        {

        }

    }*/
}
