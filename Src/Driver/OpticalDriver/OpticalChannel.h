/**
 * @file
 * @brief 光学信号AD通道选择驱动。
 * @details 提供光学信号AD通道选择接口。
 * @version 1.1.0
 * @author kim.xiejinqiang
 * @date 2015-06-04
 */


#ifndef SRC_DRIVER_OPTICALDRIVER_OPTICALCHANNEL_H_
#define SRC_DRIVER_OPTICALDRIVER_OPTICALCHANNEL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "Common/Types.h"

typedef enum
{
    AD7791 = 0,
    ADS1146 = 1,
}ADCType;

#define PD_REF_CHANNEL          1
#define PD_MEA_CHANNEL          2

void OpticalChannel_Init(void);
bool OpticalChannel_Select(ADCType type, Uint8 index);


#ifdef __cplusplus
}
#endif

#endif /* SRC_DRIVER_OPTICALDRIVER_OPTICALCHANNEL_H_ */
