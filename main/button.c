#include "button.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>
static const char* TAG = "button";

typedef enum
{
    BUTTON_RELEASE,             //按键没有按下
    BUTTON_PRESS,               //按键按下了，等待一点延时（消抖），然后触发短按回调事件，进入BUTTON_HOLD
    BUTTON_HOLD,                //按住状态，如果时间长度超过设定的超时计数，将触发长按回调函数，进入BUTTON_LONG_PRESS_HOLD
    BUTTON_LONG_PRESS_HOLD,     //此状态等待电平消失，回到BUTTON_RELEASE状态
}BUTTON_STATE;

typedef struct Button
{
    button_config_t btn_cfg;    //按键配置
    BUTTON_STATE    state;      //当前状态
    int press_cnt;              //按下计数
    struct Button* next;        //下一个按键参数
}button_dev_t;

//按键处理列表
static button_dev_t *s_button_head = NULL;

//消抖过滤时间
#define FILITER_TIMER   20

//定时器释放运行标志
static bool g_is_timer_running = false;

//定时器句柄
static esp_timer_handle_t g_button_timer_handle;

static void button_handle(void *param);

/** 设置按键事件
 * @param cfg   配置结构体
 * @return ESP_OK or ESP_FAIL 
*/
esp_err_t button_event_set(button_config_t *cfg)
{
    gpio_config_t gpio_t = 
    {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ull<<cfg->gpio_num),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_t));
    button_dev_t* btn = (button_dev_t*)malloc(sizeof(button_dev_t));
    if(!btn)
        return ESP_FAIL;
    memset(btn,0,sizeof(button_dev_t));
    if(!s_button_head)
    {
        s_button_head = btn;
        ESP_LOGI(TAG,"first btn add");
    }
    else
    {
        button_dev_t* btn_p = s_button_head;
        while(btn_p->next != NULL)
            btn_p = btn_p->next;
        btn_p->next = btn;
        ESP_LOGI(TAG,"btn add");
    }
    memcpy(&btn->btn_cfg,cfg,sizeof(button_config_t));

    if (false == g_is_timer_running) {
        ESP_LOGI(TAG,"run button timer");
        static int timer_interval = 5;
        esp_timer_create_args_t button_timer;
        button_timer.arg = (void*)timer_interval;
        button_timer.callback = button_handle;
        button_timer.dispatch_method = ESP_TIMER_TASK;
        button_timer.name = "button_handle";
        esp_timer_create(&button_timer, &g_button_timer_handle);
        esp_timer_start_periodic(g_button_timer_handle,  5000);
        g_is_timer_running = true;
    }


    return ESP_OK;
}

/** 定时器回调函数，本例中是5ms执行一次
 * @param cfg   配置结构体
 * @return ESP_OK or ESP_FAIL 
*/
static void button_handle(void *param)
{
    int increase_cnt = (int)param;  //传入的参数是5，表示定时器运行周期是5ms
    button_dev_t* btn_target = s_button_head;
    for(;btn_target;btn_target = btn_target->next)
    {
        switch(btn_target->state)
        {
            case BUTTON_RELEASE:             //按键没有按下
                if(gpio_get_level(btn_target->btn_cfg.gpio_num) == btn_target->btn_cfg.active_level)
                {
                    btn_target->press_cnt += increase_cnt;
                    btn_target->state = BUTTON_PRESS;   //调转到按下状态
                }
                break;
            case BUTTON_PRESS:               //按键按下了，等待一点延时（消抖），然后触发短按回调事件，进入BUTTON_HOLD
                if(gpio_get_level(btn_target->btn_cfg.gpio_num) == btn_target->btn_cfg.active_level)
                {
                    btn_target->press_cnt += increase_cnt;
                    if(btn_target->press_cnt >= FILITER_TIMER)  //过了滤波时间，执行短按回调函数
                    {
                        ESP_LOGI(TAG,"short press detect");
                        if(btn_target->btn_cfg.short_cb)
                            btn_target->btn_cfg.short_cb();
                        btn_target->state = BUTTON_HOLD;    //状态转入按下状态
                    }
                }
                else
                {
                    btn_target->state = BUTTON_RELEASE;
                    btn_target->press_cnt = 0;
                }
                break;
            case BUTTON_HOLD:                //按住状态，如果时间长度超过设定的超时计数，将触发长按回调函数，进入BUTTON_LONG_PRESS_HOLD
                if(gpio_get_level(btn_target->btn_cfg.gpio_num) == btn_target->btn_cfg.active_level)
                {
                    btn_target->press_cnt += increase_cnt;
                    if(btn_target->press_cnt >= btn_target->btn_cfg.long_press_time)  //已经检测到按下大于预设长按时间,执行长按回调函数
                    {
                        ESP_LOGI(TAG,"long press detect");
                        if(btn_target->btn_cfg.long_cb)
                            btn_target->btn_cfg.long_cb();
                        btn_target->state = BUTTON_LONG_PRESS_HOLD;
                    }
                }
                else
                {
                    btn_target->state = BUTTON_RELEASE;
                    btn_target->press_cnt = 0;
                }
                break;
            case BUTTON_LONG_PRESS_HOLD:     //此状态等待电平消失，回到BUTTON_RELEASE状态
                if(gpio_get_level(btn_target->btn_cfg.gpio_num) != btn_target->btn_cfg.active_level)    //检测到释放，就回到初始状态
                {
                    btn_target->state = BUTTON_RELEASE;
                    btn_target->press_cnt = 0;
                }
                break;
            default:break;
        }
    }
}
