/**
 * @file
 * @brief 光学信号AD采集驱动头文件。
 * @details 提供光学信号AD采集功能接口。
 * @version 1.1.0
 * @author kim.xiejinqiang
 * @date 2015-06-04
 */
#ifndef SRC_DRIVER_OPTICALDRIVER_AD7791COLLECT_H_
#define SRC_DRIVER_OPTICALDRIVER_AD7791COLLECT_H_


#include "stm32f4xx.h"
#include "Common/Types.h"
#ifdef __cplusplus
extern "C" {
#endif

// 极性
#define AD7791_MODE_UNIPOLAR   (1 << 2)     // 单极性
#define AD7791_MODE_BIPOLAR    0x00         // 双极性

void AD7791Collect_Init(void);
uint32_t AD7791Collect_GetAD(Uint8 channel);
void AD7791Collect_ChangePolar(unsigned char polar);
#ifdef __cplusplus
}
#endif


#endif /* SRC_DRIVER_OPTICALDRIVER_AD7791COLLECT_H_ */
