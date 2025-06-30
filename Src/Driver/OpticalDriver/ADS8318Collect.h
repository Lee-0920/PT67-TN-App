/*
 * ADS8318Collect.h
 *
 *  Created on: 2020年9月2日
 *      Author: Administrator
 */

#ifndef SRC_DRIVER_OPTICALDRIVER_ADS8318COLLECT_H_
#define SRC_DRIVER_OPTICALDRIVER_ADS8318COLLECT_H_

#include "stm32f4xx.h"
#include "Common/Types.h"
#include "Tracer/Trace.h"
#ifdef __cplusplus
extern "C" {
#endif


void ADS8318Collect_Init(void);
Uint16 ADS8318Collect_GetAD(void);


#ifdef __cplusplus
}
#endif

#endif /* SRC_DRIVER_OPTICALDRIVER_ADS8318COLLECT_H_ */





