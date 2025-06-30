/*
 * Relay.c
 *
 *  Created on: 2019年3月20日
 *      Author: Administrator
 */
#include <TempDriver/Relay.h>
#include "Tracer/Trace.h"

static Relay g_relay[RELAY_NUM];

void Relay_Init(void)
{
    g_relay[0].port = GPIOC;
    g_relay[0].pin =  GPIO_Pin_13;
    g_relay[0].rcc = RCC_AHB1Periph_GPIOC;
    Relay_DriverInit(&g_relay[0]);
}

void Relay_DriverInit(Relay* relay)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(relay->rcc, ENABLE);

    GPIO_InitStructure.GPIO_Pin = relay->pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(relay->port, &GPIO_InitStructure);

    GPIO_SetBits(relay->port, relay->pin);  //默认关闭
}

void Relay_TurnOnAll(void)
{
    TRACE_INFO("\n Relay Turn On All");
    for(int index = 0; index < RELAY_NUM; index++)
    {
        GPIO_ResetBits(g_relay[index].port, g_relay[index].pin);
    }
}

void Relay_TurnOffAll(void)
{
    TRACE_INFO("\n Relay Turn Off All");
    for(int index = 0; index < RELAY_NUM; index++)
    {
        GPIO_SetBits(g_relay[index].port, g_relay[index].pin);
    }
}

void Relay_TurnOn(int index)
{
    if(index >= 0 && index < RELAY_NUM)
    {
        GPIO_ResetBits(g_relay[index].port, g_relay[index].pin);
        TRACE_INFO("\n Relay %d on", index);
    }
    else
    {
        TRACE_WARN("\n Relay %d invalid", index);
    }
}

void Relay_TurnOff(int index)
{
    if(index >= 0 && index < RELAY_NUM)
    {
        GPIO_SetBits(g_relay[index].port, g_relay[index].pin);
        TRACE_INFO("\n Relay %d off", index);
    }
    else
    {
        TRACE_WARN("\n Relay %d invalid", index);
    }
}

Bool Relay_IsOpen(int index)
{
    Bool ret = FALSE;
    if(index >= 0 && index < RELAY_NUM)
    {
        if(!GPIO_ReadOutputDataBit(g_relay[index].port, g_relay[index].pin))
        {
            ret = TRUE;
            TRACE_INFO("\n Relay %d is on", index);
        }
        else
        {
            ret = FALSE;
            TRACE_INFO("\n Relay %d is off", index);
        }
    }
    else
    {
        TRACE_WARN("\n Relay %d invalid", index);
    }
    return ret;
}

Uint8 Relay_TotalNumber(void)
{
    return RELAY_NUM;
}
