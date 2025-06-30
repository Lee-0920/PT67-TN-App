/**
 * @file
 * @brief 光学信号AD通道选择驱动头文件。
 * @details 提供光学信号AD通道选择口。
 * @version 1.1.0
 * @author kim.xiejinqiang
 * @date 2015-06-04
 */

#include <OpticalDriver/OpticalChannel.h>
#include "stm32f4xx.h"
#include "Common/Types.h"

// **************** 引脚定义 ****************
//REFC
#define AD7791_REF_CTL_PIN      GPIO_Pin_7
#define AD7791_REF_CTL_PORT    GPIOE
#define AD7791_REF_CTL_RCC      RCC_AHB1Periph_GPIOE
#define AD7791_REF_CTL_HIGH()   GPIO_SetBits(AD7791_REF_CTL_PORT, AD7791_REF_CTL_PIN)
#define AD7791_REF_CTL_LOW()    GPIO_ResetBits(AD7791_REF_CTL_PORT, AD7791_REF_CTL_PIN)

//MEAC
#define AD7791_MEA_CTL_PIN        GPIO_Pin_6
#define AD7791_MEA_CTL_PORT     GPIOA
#define AD7791_MEA_CTL_RCC        RCC_AHB1Periph_GPIOA
#define AD7791_MEA_CTL_HIGH()  GPIO_SetBits(AD7791_MEA_CTL_PORT, AD7791_MEA_CTL_PIN)
#define AD7791_MEA_CTL_LOW()   GPIO_ResetBits(AD7791_MEA_CTL_PORT, AD7791_MEA_CTL_PIN)

//TN275 //REF
#define ADS1146_REF_CTL_PIN      GPIO_Pin_14
#define ADS1146_REF_CTL_PORT     GPIOE
#define ADS1146_REF_CTL_RCC      RCC_AHB1Periph_GPIOE
#define ADS1146_REF_CTL_HIGH()   GPIO_SetBits(ADS1146_REF_CTL_PORT, ADS1146_REF_CTL_PIN)
#define ADS1146_REF_CTL_LOW()    GPIO_ResetBits(ADS1146_REF_CTL_PORT, ADS1146_REF_CTL_PIN)

//TN220 //MEA
#define ADS1146_MEA_CTL_PIN     GPIO_Pin_15
#define ADS1146_MEA_CTL_PORT    GPIOE
#define ADS1146_MEA_CTL_RCC     RCC_AHB1Periph_GPIOE
#define ADS1146_MEA_CTL_HIGH()  GPIO_SetBits(ADS1146_MEA_CTL_PORT, ADS1146_MEA_CTL_PIN)
#define ADS1146_MEA_CTL_LOW()   GPIO_ResetBits(ADS1146_MEA_CTL_PORT, ADS1146_MEA_CTL_PIN)

//TP_REFC       //暂时不用
#define ADS1146_EXT_REF_CTL_PIN      GPIO_Pin_11
#define ADS1146_EXT_REF_CTL_PORT     GPIOE
#define ADS1146_EXT_REF_CTL_RCC      RCC_AHB1Periph_GPIOE
#define ADS1146_EXT_REF_CTL_HIGH()   GPIO_SetBits(ADS1146_EXT_REF_CTL_PORT, ADS1146_EXT_REF_CTL_PIN)
#define ADS1146_EXT_REF_CTL_LOW()    GPIO_ResetBits(ADS1146_EXT_REF_CTL_PORT, ADS1146_EXT_REF_CTL_PIN)

//TP_MEAC    //暂时不用
#define ADS1146_EXT_MEA_CTL_PIN     GPIO_Pin_10
#define ADS1146_EXT_MEA_CTL_PORT    GPIOE
#define ADS1146_EXT_MEA_CTL_RCC     RCC_AHB1Periph_GPIOE
#define ADS1146_EXT_MEA_CTL_HIGH()  GPIO_SetBits(ADS1146_EXT_MEA_CTL_PORT, ADS1146_EXT_MEA_CTL_PIN)
#define ADS1146_EXT_MEA_CTL_LOW()   GPIO_ResetBits(ADS1146_EXT_MEA_CTL_PORT, ADS1146_EXT_MEA_CTL_PIN)

/**
 * @brief 光学采集通道初始化
 * @param
 */
void OpticalChannel_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd( AD7791_REF_CTL_RCC | AD7791_MEA_CTL_RCC |
                                                    ADS1146_REF_CTL_RCC | ADS1146_MEA_CTL_RCC |
                                                   ADS1146_EXT_REF_CTL_RCC | ADS1146_EXT_MEA_CTL_RCC, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_InitStructure.GPIO_Pin = AD7791_REF_CTL_PIN;
    GPIO_Init(AD7791_REF_CTL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = AD7791_MEA_CTL_PIN;
    GPIO_Init(AD7791_MEA_CTL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ADS1146_REF_CTL_PIN;
    GPIO_Init(ADS1146_REF_CTL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ADS1146_MEA_CTL_PIN;
    GPIO_Init(ADS1146_MEA_CTL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ADS1146_EXT_REF_CTL_PIN;
    GPIO_Init(ADS1146_EXT_REF_CTL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ADS1146_EXT_MEA_CTL_PIN;
    GPIO_Init(ADS1146_EXT_MEA_CTL_PORT, &GPIO_InitStructure);


    // 初始化
    AD7791_REF_CTL_LOW();
    AD7791_MEA_CTL_LOW();
    ADS1146_REF_CTL_LOW();
    ADS1146_MEA_CTL_LOW();

    ADS1146_EXT_REF_CTL_LOW();
    ADS1146_EXT_MEA_CTL_LOW();
}

/**
 * @brief 光学采集通道选择
 * @param index PD_REF_CHANNEL = 1 PD_MEA_CHANNEL = 2
 */
bool OpticalChannel_Select(ADCType type, Uint8 index)
{
    if(type == AD7791)
    {
        switch(index)
        {
            case 1:
                AD7791_REF_CTL_HIGH();
                AD7791_MEA_CTL_LOW();
                ADS1146_REF_CTL_LOW();
                ADS1146_MEA_CTL_LOW();
                break;

            case 2:
                AD7791_REF_CTL_LOW();
                AD7791_MEA_CTL_HIGH();
                ADS1146_REF_CTL_LOW();
                ADS1146_MEA_CTL_LOW();
                break;

            default:
                return FALSE;
        }
    }
    else if(type == ADS1146)
    {
        switch(index)
        {
            case 1:
                ADS1146_REF_CTL_HIGH();
                ADS1146_MEA_CTL_LOW();
                AD7791_REF_CTL_LOW();
                AD7791_MEA_CTL_LOW();
                break;

            case 2:
                ADS1146_REF_CTL_LOW();
                ADS1146_MEA_CTL_HIGH();
                AD7791_REF_CTL_LOW();
                AD7791_MEA_CTL_LOW();
                break;

            default:
                return FALSE;
        }
    }

    return TRUE;
}

