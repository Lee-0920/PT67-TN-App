/**
 * @file
 * @brief 光学信号LED驱动头文件。
 * @details 提供光学信号LED控制功能接口。
 * @version 1.1.0
 * @author kim.xiejinqiang
 * @date 2015-06-04
 */

#ifndef SRC_DRIVER_OPTICALDRIVER_OPTICALLED_H_
#define SRC_DRIVER_OPTICALDRIVER_OPTICALLED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "Common/Types.h"

void OpticalLed_Init(void);
void OpticalLed_TurnOn();
void OpticalLed_TurnOff();
void OpticalLed_ChangeDACOut(float valve);
void OpticalLed_InitParam(void);
float OpticalLed_GetDefaultValue(void);
Bool OpticalLed_SetDefaultValue(float value);
#ifdef __cplusplus
}
#endif

#endif /* SRC_DRIVER_OPTICALDRIVER_OPTICALLED_H_ */
