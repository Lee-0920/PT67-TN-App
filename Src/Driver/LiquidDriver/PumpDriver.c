/*
 * PumpDriver.c
 *
 *  Created on: 2016年5月30日
 *      Author: Administrator
 */
#include <LiquidDriver/PumpDriver.h>

void PumpDriver_Init(PumpDriver *pumpDriver)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(
            pumpDriver->rccClock | pumpDriver->rccDir | pumpDriver->rccEnable,
            ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_InitStructure.GPIO_Pin = pumpDriver->pinEnable;
    GPIO_Init(pumpDriver->portEnable, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = pumpDriver->pinClock;
    GPIO_Init(pumpDriver->portClock, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = pumpDriver->pinDir;
    GPIO_Init(pumpDriver->portDir, &GPIO_InitStructure);
}

void PumpDriver_Enable(PumpDriver *pumpDriver)
{
    GPIO_ResetBits(pumpDriver->portEnable, pumpDriver->pinEnable);
}

void PumpDriver_Disable(PumpDriver *pumpDriver)
{
    GPIO_SetBits(pumpDriver->portEnable, pumpDriver->pinEnable);
}

void PumpDriver_SetDirection(PumpDriver *pumpDriver, Direction dir)
{
    if (CLOCKWISE == dir)
    {
        GPIO_SetBits(pumpDriver->portDir, pumpDriver->pinDir);
    }
    else
    {
        GPIO_ResetBits(pumpDriver->portDir, pumpDriver->pinDir);
    }
}

void PumpDriver_PullHigh(PumpDriver *pumpDriver)
{
    GPIO_SetBits(pumpDriver->portClock, pumpDriver->pinClock);
}

void PumpDriver_PullLow(PumpDriver *pumpDriver)
{
    GPIO_ResetBits(pumpDriver->portClock, pumpDriver->pinClock);
}

