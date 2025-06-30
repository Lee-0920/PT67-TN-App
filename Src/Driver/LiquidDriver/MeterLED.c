/*
 * MeterLedDriver.c
 *
 *  Created on: 2016年6月14日
 *      Author: Administrator
 */
#include <LiquidDriver/MeterLED.h>
#include "Tracer/Trace.h"
#include "SystemConfig.h"
#include "McuFlash.h"

#define METERLED_LED0_PIN       GPIO_Pin_4
#define METERLED_LED0_PORT      GPIOA
#define METERLED_LED0_RCC       RCC_AHB1Periph_GPIOA

#define METERLED_LED0_CTL0_PIN       GPIO_Pin_14
#define METERLED_LED0_CTL0_PORT      GPIOC
#define METERLED_LED0_CTL0_RCC       RCC_AHB1Periph_GPIOC
#define METERLED_LED0_CTL0_LOW     GPIO_ResetBits(METERLED_LED0_CTL0_PORT, METERLED_LED0_CTL0_PIN);
#define METERLED_LED0_CTL0_HIGH     GPIO_SetBits(METERLED_LED0_CTL0_PORT, METERLED_LED0_CTL0_PIN);

#define METERLED_LED0_CTL1_PIN       GPIO_Pin_15
#define METERLED_LED0_CTL1_PORT      GPIOC
#define METERLED_LED0_CTL1_RCC       RCC_AHB1Periph_GPIOC
#define METERLED_LED0_CTL1_LOW     GPIO_ResetBits(METERLED_LED0_CTL1_PORT, METERLED_LED0_CTL1_PIN);
#define METERLED_LED0_CTL1_HIGH     GPIO_SetBits(METERLED_LED0_CTL1_PORT, METERLED_LED0_CTL1_PIN);

#define METERLED_LED1_PIN       GPIO_Pin_3
#define METERLED_LED1_PORT      GPIOA
#define METERLED_LED1_RCC       RCC_AHB1Periph_GPIOA

//不能使用
#define METERLED_LED1_CTL0_PIN       GPIO_Pin_6
#define METERLED_LED1_CTL0_PORT      GPIOE
#define METERLED_LED1_CTL0_RCC       RCC_AHB1Periph_GPIOE
#define METERLED_LED1_CTL0_LOW     GPIO_ResetBits(METERLED_LED1_CTL0_PORT, METERLED_LED1_CTL0_PIN);
#define METERLED_LED1_CTL0_HIGH     GPIO_SetBits(METERLED_LED1_CTL0_PORT, METERLED_LED1_CTL0_PIN);

#define METERLED_LED1_CTL1_PIN       GPIO_Pin_13
#define METERLED_LED1_CTL1_PORT      GPIOD
#define METERLED_LED1_CTL1_RCC       RCC_AHB1Periph_GPIOD
#define METERLED_LED1_CTL1_LOW     GPIO_ResetBits(METERLED_LED1_CTL1_PORT, METERLED_LED1_CTL1_PIN);
#define METERLED_LED1_CTL1_HIGH     GPIO_SetBits(METERLED_LED1_CTL1_PORT, METERLED_LED1_CTL1_PIN);

static Uint16 s_meterLedLevel[METERLED_LEVEL_NUM] = {1, 1};

void MeterLED_Init(void)
{
    MeterLED_InitParam();
    MeterLED_InitDriver();
}

void MeterLED_InitDriver(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(
            METERLED_LED0_RCC | METERLED_LED1_RCC |
            METERLED_LED0_CTL0_RCC | METERLED_LED0_CTL1_RCC|
            METERLED_LED1_CTL0_RCC | METERLED_LED1_CTL1_RCC , ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_InitStructure.GPIO_Pin = METERLED_LED0_PIN;
    GPIO_Init(METERLED_LED0_PORT, &GPIO_InitStructure);
    GPIO_SetBits(METERLED_LED0_PORT, METERLED_LED0_PIN);

    GPIO_InitStructure.GPIO_Pin = METERLED_LED1_PIN;
    GPIO_Init(METERLED_LED1_PORT, &GPIO_InitStructure);
    GPIO_SetBits(METERLED_LED1_PORT, METERLED_LED1_PIN);

    GPIO_InitStructure.GPIO_Pin = METERLED_LED0_CTL0_PIN;
    GPIO_Init(METERLED_LED0_CTL0_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = METERLED_LED0_CTL1_PIN;
    GPIO_Init(METERLED_LED0_CTL1_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = METERLED_LED1_CTL0_PIN;
    GPIO_Init(METERLED_LED1_CTL0_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = METERLED_LED1_CTL1_PIN;
    GPIO_Init(METERLED_LED1_CTL1_PORT, &GPIO_InitStructure);

    Uint8 index = 0;
    for(index = 0; index < METERLED_LEVEL_NUM; index++)
    {
        MeterLED_SetCurrentLevel(index, s_meterLedLevel[index]);
    }
}

Bool MeterLED_TurnOn(uint8_t index)
{
    Bool ret = TRUE;
    switch (index)
    {
    case 0:
        GPIO_ResetBits(METERLED_LED0_PORT, METERLED_LED0_PIN);
        break;
    case 1:
        GPIO_ResetBits(METERLED_LED1_PORT, METERLED_LED1_PIN);
        break;
    default:
        TRACE_ERROR("\n No. %d LED. Index: 0 - 1", index);
        ret = FALSE;
        break;
    }
    return ret;
}

Bool MeterLED_TurnOff(uint8_t index)
{
    Bool ret = TRUE;
    switch (index)
    {
    case 0:
        GPIO_SetBits(METERLED_LED0_PORT, METERLED_LED0_PIN);
        break;
    case 1:
        GPIO_SetBits(METERLED_LED1_PORT, METERLED_LED1_PIN);
        break;
    default:
        TRACE_ERROR("\n No. %d LED. Index : 0 - 1", index);
        ret = FALSE;
        break;
    }
    return ret;
}

void MeterLED_InitParam(void)
{
    Uint8 buffer[METERLED_LEVEL_SIGN_FLASH_LEN] = { 0 };
    Uint32 flashFactorySign = 0;
    Uint8 index = 0;

    McuFlash_Read(METERLED_LEVEL_SIGN_FLASH_BASE_ADDR, METERLED_LEVEL_SIGN_FLASH_LEN, buffer); //读取出厂标志位

    memcpy(&flashFactorySign, buffer, METERLED_LEVEL_SIGN_FLASH_LEN);

    if (FLASH_FACTORY_SIGN == flashFactorySign) //表示已经过出厂设置
    {
        for(index = 0; index < METERLED_LEVEL_NUM; index++)
        {
            s_meterLedLevel[index] = MeterLED_GetDefaultLevel(index);
        }
    }
    else
    {
        for(index = 0; index < METERLED_LEVEL_NUM; index++)
        {
            MeterLED_SetDefaultLevel(index, s_meterLedLevel[index]);
        }

        flashFactorySign = FLASH_FACTORY_SIGN;
        memcpy(buffer, &flashFactorySign, METERLED_LEVEL_SIGN_FLASH_LEN);

        McuFlash_Write(METERLED_LEVEL_SIGN_FLASH_BASE_ADDR, METERLED_LEVEL_SIGN_FLASH_LEN, buffer);  //写入出厂标志
    }
}

Bool MeterLED_SetDefaultLevel(uint8_t index, uint16_t value)
{
    Uint8 writeData[METERLED_LEVEL_DEFAULT_VALUE_FLASH_LEN] = { 0 };

    if(index < METERLED_LEVEL_NUM)
    {
        if(value < METERLED_LEVEL_MAX_VALUE)
        {
            memcpy(writeData, &value, sizeof(uint16_t));
            McuFlash_Write(METERLED_LEVEL_DEFAULT_VALUE_FLASH_BASE_ADDRESS + index*METERLED_LEVEL_DEFAULT_VALUE_FLASH_LEN,
                    METERLED_LEVEL_DEFAULT_VALUE_FLASH_LEN, writeData);

            TRACE_INFO("\n Meter LED %d set default level value: %d", index, value);
            return TRUE;
        }
        else
        {
            TRACE_ERROR("\n Invalid Meter LED %d level value %d", index, value);
            return FALSE;
        }
    }
    else
    {
        TRACE_ERROR("\n Invalid Meter LED index %d", index);
        return FALSE;
    }
}

uint16_t MeterLED_GetDefaultLevel(uint8_t index)
{
    uint16_t value = 0;
    uint8_t readData[METERLED_LEVEL_DEFAULT_VALUE_FLASH_LEN] = { 0 };

    if(index < METERLED_LEVEL_NUM)
    {
        McuFlash_Read(METERLED_LEVEL_DEFAULT_VALUE_FLASH_BASE_ADDRESS + index*METERLED_LEVEL_DEFAULT_VALUE_FLASH_LEN,
                    METERLED_LEVEL_DEFAULT_VALUE_FLASH_LEN, readData);
        memcpy(&value, readData, sizeof(uint16_t));

        TRACE_DEBUG("\n Meter LED %d get default level value: %d", index, value);
    }
    else
    {
        TRACE_ERROR("\n Invalid Meter LED index %d", index);
    }

    return value;
}

Bool MeterLED_SetCurrentLevel(uint8_t index, uint16_t value)
{
    Bool ret = TRUE;
    if(index == 0)
    {
        switch(value)
        {
            case 0:
                METERLED_LED0_CTL0_LOW;
                METERLED_LED0_CTL1_LOW;
                break;

            case 1:
                METERLED_LED0_CTL0_HIGH;
                METERLED_LED0_CTL1_LOW;
                break;

            case 2:
                METERLED_LED0_CTL0_LOW;
                METERLED_LED0_CTL1_HIGH;
                break;

            case 3:
                METERLED_LED0_CTL0_HIGH;
                METERLED_LED0_CTL1_HIGH;
                break;

            default:
                ret = FALSE;
                TRACE_ERROR("\n Invalid Meter LED %d level value %d", index, value);
                break;
        }
    }
    else if(index == 1)
    {
        switch(value)
        {
            case 0:
                METERLED_LED1_CTL0_LOW;
                METERLED_LED1_CTL1_LOW;
                break;

            case 1:
                METERLED_LED1_CTL0_HIGH;
                METERLED_LED1_CTL1_LOW;
                break;

            case 2:
                METERLED_LED1_CTL0_LOW;
                METERLED_LED1_CTL1_HIGH;
                break;

            case 3:
                METERLED_LED1_CTL0_HIGH;
                METERLED_LED1_CTL1_HIGH;
                break;

            default:
                ret = FALSE;
                TRACE_ERROR("\n Invalid Meter LED %d level value %d", index, value);
                break;
        }
    }
    else
    {
        ret = FALSE;
        TRACE_ERROR("\n Invalid Meter LED index %d", index);
    }

    if(TRUE == ret)
    {
        s_meterLedLevel[index] = value;
        TRACE_DEBUG("\n Meter LED %d set current level : %d", index, value);
    }

    return ret;
}

uint16_t MeterLED_GetCurrentLevel(uint8_t index)
{
    if(index < METERLED_LEVEL_NUM)
    {
        TRACE_DEBUG("\n The Meter LED  level is : %d", s_meterLedLevel[index]);
        return s_meterLedLevel[index];
    }
    else
    {
        TRACE_ERROR("\n Invalid Meter LED index %d", index);
        return 0;
    }
}

