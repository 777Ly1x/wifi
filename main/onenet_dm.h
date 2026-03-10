#ifndef _ONENET_DM_H_
#define _ONENET_DM_H_
#include "cJSON.h"

/**
 * 物模型数据初始化
 * @param 无
 * @return 无
 */
void onenet_dm_init(void);

/**
 * 处理onenet下行的数据
 * @param property_js 包含下行数据的json
 * @return 无
 */
void onenet_property_handle(cJSON* property_js);

/**
 * 生成上报所有数据的cJSON对象
 * @param 无
 * @return cJSON对象，包含所有属性值
 */
cJSON* onenet_property_upload_dm(void);

#endif
