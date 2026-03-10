#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_log.h"
#include "st7789_driver.h"
#include "cst816t_driver.h"
#include "driver/gpio.h"
#include "esp_timer.h"

/*
1.初始化和注册LVGL显示驱动
2.初始化和注册LVGL触摸驱动
3.初始化ST7789硬件接口
4.初始化CST816T硬件接口
5.提供一个定时器给LVGL使用
*/

#define TAG     "lv_port"

#define LCD_WIDTH       240
#define LCD_HEIGHT      280

static lv_disp_drv_t    disp_drv;

void lv_flush_done_cb(void* param)
{
    lv_disp_flush_ready(&disp_drv);
}

void disp_flush(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    st7789_flush(area->x1,area->x2 + 1,area->y1+20,area->y2+20 + 1,color_p);
}

void lv_disp_init(void)
{
    static lv_disp_draw_buf_t   disp_buf;
    const size_t disp_buf_size = LCD_WIDTH*(LCD_HEIGHT/7);

    lv_color_t *disp1 = heap_caps_malloc(disp_buf_size*sizeof(lv_color_t),MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA);
    lv_color_t *disp2 = heap_caps_malloc(disp_buf_size*sizeof(lv_color_t),MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA);
    if(!disp1 || !disp2)
    {
        ESP_LOGE(TAG,"disp buff malloc fail!");
        return;
    }
    lv_disp_draw_buf_init(&disp_buf,disp1,disp2,disp_buf_size);

    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = disp_flush;
    lv_disp_drv_register(&disp_drv);
}

void IRAM_ATTR indev_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    int16_t x,y;
    int state;
    cst816t_read(&x,&y,&state);
    data->point.x = x;
    data->point.y = y;
    data->state = state;
}

void lv_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = indev_read;
    lv_indev_drv_register(&indev_drv);
}

void st7789_hw_init(void)
{
    st7789_cfg_t st7789_config = {
        .bl = GPIO_NUM_26,
        .clk = GPIO_NUM_18,
        .cs = GPIO_NUM_5,
        .dc = GPIO_NUM_17,
        .mosi = GPIO_NUM_19,
        .rst = GPIO_NUM_21,
        .spi_fre = 40*1000*1000,
        .height = LCD_HEIGHT,
        .width = LCD_WIDTH,
        .spin = 0,
        .done_cb = lv_flush_done_cb,
        .cb_param = &disp_drv,
    };

    st7789_driver_hw_init(&st7789_config);
}

void cst816t_hw_init(void)
{
    cst816t_cfg_t cst816t_config = 
    {
        .scl = GPIO_NUM_22,
        .sda = GPIO_NUM_23,
        .fre = 300*1000,
        .x_limit = LCD_WIDTH,
        .y_limit = LCD_HEIGHT,
    };
    cst816t_init(&cst816t_config);
}

void lv_timer_cb(void* arg)
{
    uint32_t tick_interval = *((uint32_t*)arg);
    lv_tick_inc(tick_interval);
}

void lv_tick_init(void)
{
    static uint32_t tick_interval = 5;
    const esp_timer_create_args_t arg = 
    {
        .arg = &tick_interval,
        .callback = lv_timer_cb,
        .name = "",
        .dispatch_method = ESP_TIMER_TASK,
        .skip_unhandled_events = true,
    };

    esp_timer_handle_t timer_handle;
    esp_timer_create(&arg,&timer_handle);
    esp_timer_start_periodic(timer_handle,tick_interval*1000);
}

void lv_port_init(void)
{
    lv_init();
    st7789_hw_init();
    cst816t_hw_init();
    lv_disp_init();
    lv_indev_init();
    lv_tick_init();
}