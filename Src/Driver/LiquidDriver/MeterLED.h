/*
 * MeterLEDDriver.h
 *
 *  Created on: 2016年6月14日
 *      Author: Administrator
 */

#ifndef SRC_DRIVER_METERLED_H_
#define SRC_DRIVER_METERLED_H_

#include "Common/Types.h"
#include "stm32f4xx.h"

#define METERLED_LEVEL_NUM  2
#define METERLED_LEVEL_MAX_VALUE 3

void MeterLED_Init(void);
void MeterLED_InitDriver(void);
Bool MeterLED_TurnOn(uint8_t index);
Bool MeterLED_TurnOff(uint8_t index);
void MeterLED_InitParam(void);
Bool MeterLED_SetDefaultLevel(uint8_t index, uint16_t value);
uint16_t MeterLED_GetDefaultLevel(uint8_t index);
Bool MeterLED_SetCurrentLevel(uint8_t index, uint16_t value);
uint16_t MeterLED_GetCurrentLevel(uint8_t index);

#endif /* SRC_DRIVER_METERLED_H_ */
