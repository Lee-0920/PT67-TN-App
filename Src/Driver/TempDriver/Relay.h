/*
 * Relay.h
 *
 *  Created on: 2019年3月20日
 *      Author: Administrator
 */

#ifndef SRC_DRIVER_TEMPDRIVER_RELAY_H_
#define SRC_DRIVER_TEMPDRIVER_RELAY_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f4xx.h"
#include "Common/Types.h"

#define RELAY_NUM 1

typedef struct
{
    GPIO_TypeDef *port;
    Uint16 pin;
    uint32_t rcc;
} Relay;

void Relay_Init(void);
void Relay_DriverInit(Relay* relay);
void Relay_TurnOnAll(void);
void Relay_TurnOffAll(void);
void Relay_TurnOn(int index);
void Relay_TurnOff(int index);
Uint8 Relay_TotalNumber(void);
Bool Relay_IsOpen(int index);

#ifdef __cplusplus
}
#endif

#endif /* SRC_DRIVER_TEMPDRIVER_RELAY_H_ */
