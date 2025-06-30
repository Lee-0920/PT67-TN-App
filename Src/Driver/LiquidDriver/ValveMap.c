/*
 * ValveMap.c
 *
 *  Created on: 2016年6月7日
 *      Author: Administrator
 */

#include <LiquidDriver/ValveMap.h>
#include "stm32f4xx.h"
#include "SolenoidValve/ValveManager.h"

void ValveMap_Init(Valve *valve)
{
    Uint8 i;

    valve[0].pin = GPIO_Pin_12;
    valve[0].port = GPIOC;
    valve[0].rcc = RCC_AHB1Periph_GPIOC;
    ValveDriver_Init(&valve[0]);

    valve[1].pin = GPIO_Pin_0;
    valve[1].port = GPIOD;
    valve[1].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[1]);

    valve[2].pin = GPIO_Pin_1;
    valve[2].port = GPIOD;
    valve[2].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[2]);

    valve[3].pin = GPIO_Pin_2;
    valve[3].port= GPIOD;
    valve[3].rcc =  RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[3]);

    valve[4].pin = GPIO_Pin_3;
    valve[4].port = GPIOD;
    valve[4].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[4]);

    valve[5].pin = GPIO_Pin_4;
    valve[5].port = GPIOD;
    valve[5].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[5]);

    valve[6].pin = GPIO_Pin_5;
    valve[6].port = GPIOD;
    valve[6].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[6]);

    valve[7].pin = GPIO_Pin_6;
    valve[7].port = GPIOD;
    valve[7].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[7]);

    valve[8].pin = GPIO_Pin_7;
    valve[8].port = GPIOD;
    valve[8].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[8]);

    valve[9].pin = GPIO_Pin_5;
    valve[9].port = GPIOB;
    valve[9].rcc = RCC_AHB1Periph_GPIOB;
    ValveDriver_Init(&valve[9]);

    valve[10].pin = GPIO_Pin_6;
    valve[10].port= GPIOB;
    valve[10].rcc = RCC_AHB1Periph_GPIOB;
    ValveDriver_Init(&valve[10]);

    valve[11].pin = GPIO_Pin_7;
    valve[11].port = GPIOB;
    valve[11].rcc = RCC_AHB1Periph_GPIOB;
    ValveDriver_Init(&valve[11]);

    valve[12].pin = GPIO_Pin_0;
    valve[12].port= GPIOE;
    valve[12].rcc = RCC_AHB1Periph_GPIOE;
    ValveDriver_Init(&valve[12]);

    valve[13].pin = GPIO_Pin_8;
    valve[13].port = GPIOD;
    valve[13].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[13]);

    valve[14].pin = GPIO_Pin_9;
    valve[14].port= GPIOD;
    valve[14].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[14]);

    valve[15].pin = GPIO_Pin_10;
    valve[15].port = GPIOD;
    valve[15].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[15]);

    valve[16].pin = GPIO_Pin_11;
    valve[16].port= GPIOD;
    valve[16].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[16]);

    valve[17].pin = GPIO_Pin_12;
    valve[17].port = GPIOD;
    valve[17].rcc = RCC_AHB1Periph_GPIOD;
    ValveDriver_Init(&valve[17]);

    for(i = 0; i < SOLENOIDVALVECONF_TOTALVAlVES; i++)
    {
        ValveDriver_Control(&valve[i], VAlVE_CLOSE);
    }

}


