/*
 * PumpMap.c
 *
 *  Created on: 2016年5月30日
 *      Author: Administrator
 */
#include <LiquidDriver/PumpDriver.h>
#include <LiquidDriver/PumpMap.h>
#include "stm32f4xx.h"

void PumpMap_Init(Pump *pump)
{

    pump[0].driver.pinClock = GPIO_Pin_11;
    pump[0].driver.portClock = GPIOA;
    pump[0].driver.rccClock = RCC_AHB1Periph_GPIOA;

    pump[0].driver.pinDir = GPIO_Pin_12;
    pump[0].driver.portDir = GPIOA;
    pump[0].driver.rccDir = RCC_AHB1Periph_GPIOA;

    pump[0].driver.pinEnable = GPIO_Pin_10;
    pump[0].driver.portEnable = GPIOC;
    pump[0].driver.rccEnable = RCC_AHB1Periph_GPIOC;

    PumpDriver_Init(&pump[0].driver);
    PumpDriver_PullLow(&pump[0].driver);
    PumpDriver_Disable(&pump[0].driver);


    pump[1].driver.pinClock = GPIO_Pin_8;
    pump[1].driver.portClock = GPIOC;
    pump[1].driver.rccClock = RCC_AHB1Periph_GPIOC;

    pump[1].driver.pinDir = GPIO_Pin_11;
    pump[1].driver.portDir = GPIOC;
    pump[1].driver.rccDir = RCC_AHB1Periph_GPIOC;

    pump[1].driver.pinEnable = GPIO_Pin_9;
    pump[1].driver.portEnable = GPIOC;
    pump[1].driver.rccEnable = RCC_AHB1Periph_GPIOC;

    PumpDriver_Init(&pump[1].driver);
    PumpDriver_PullLow(&pump[1].driver);
    PumpDriver_Disable(&pump[1].driver);

}

