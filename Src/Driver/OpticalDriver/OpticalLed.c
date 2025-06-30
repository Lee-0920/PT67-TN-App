/**
 * @file
 * @brief 光学信号LED驱动。
 * @details 提供光学信号LED控制功能接口。
 * @version 1.1.0
 * @author kim.xiejinqiang
 * @date 2015-06-04
 */

#include <OpticalDriver/OpticalLed.h>
#include "stm32f4xx.h"
#include "SystemConfig.h"
#include "Driver/McuFlash.h"
#include "Driver/System.h"

// **************** 引脚定义 ****************
#define OPT_DAC_PIN          GPIO_Pin_5
#define OPT_DAC_PORT         GPIOA
#define OPT_DAC_RCC          RCC_AHB1Periph_GPIOA

#define DAC_DEFAULT_VALUE  1

static float s_DACDefValue = 1;
static float s_DACOutValue = 1; //当前值

void OpticalLed_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    DAC_InitTypeDef DAC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

    // 变量初始化
    GPIO_InitStructure.GPIO_Pin = OPT_DAC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//配置为模拟输入，连接到DAC时会连接到模拟输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(OPT_DAC_PORT, &GPIO_InitStructure);
    // 变量初始化

    DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;//不使用触发功能
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;//不使用波形发生
    DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;//屏蔽/幅值选择器
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;//输出缓存关闭
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);
    /* Enable ADC */
    DAC_Cmd(DAC_Channel_2, ENABLE);
    DAC_SetChannel2Data(DAC_Align_12b_R, 0);//12位右对齐数据格式设置DAC，输出0V

    OpticalLed_InitParam();
}

void OpticalLed_InitParam(void)
{
    Uint8 buffer[LEDDAC_SIGN_FLASH_LEN] = { 0 };
    Uint32 flashFactorySign = 0;

    McuFlash_Read(LEDDAC_SIGN_FLASH_BASE_ADDR, LEDDAC_SIGN_FLASH_LEN, buffer); //读取出厂标志位

    memcpy(&flashFactorySign, buffer, LEDDAC_SIGN_FLASH_LEN);

    if (FLASH_FACTORY_SIGN == flashFactorySign) //表示已经过出厂设置
    {
        s_DACDefValue = OpticalLed_GetDefaultValue();
    }
    else
    {
        OpticalLed_SetDefaultValue(DAC_DEFAULT_VALUE);

        flashFactorySign = FLASH_FACTORY_SIGN;
        memcpy(buffer, &flashFactorySign, LEDDAC_SIGN_FLASH_LEN);

        McuFlash_Write(LEDDAC_SIGN_FLASH_BASE_ADDR, LEDDAC_SIGN_FLASH_LEN, buffer);  //写入出厂标志
    }
}

float OpticalLed_GetDefaultValue(void)
{
    float value;
    Uint8 readData[LEDDAC_DEFAULT_VALUE_FLASH_LEN] = { 0 };

    McuFlash_Read(LEDDAC_DEFAULT_VALUE_FLASH_BASE_ADDRESS, LEDDAC_DEFAULT_VALUE_FLASH_LEN, readData);
    memcpy(&value, readData, sizeof(float));

    return value;
}

Bool OpticalLed_SetDefaultValue(float value)
{
    Uint8 writeData[LEDDAC_DEFAULT_VALUE_FLASH_LEN] = { 0 };

    if(value  >= 0 && value <= 2.5)
    {
        memcpy(writeData, &value, sizeof(float));
        McuFlash_Write(LEDDAC_DEFAULT_VALUE_FLASH_BASE_ADDRESS, LEDDAC_DEFAULT_VALUE_FLASH_LEN, writeData);
        s_DACDefValue = value;

        TRACE_INFO("\n LED DAC set default value : ", s_DACDefValue);
        System_PrintfFloat(TRACE_LEVEL_INFO, s_DACDefValue, 3);
        TRACE_INFO(" V");
        return TRUE;
    }
    else
    {
        TRACE_INFO("\n Error param :");
        System_PrintfFloat(TRACE_LEVEL_INFO, value, 3);
        TRACE_INFO(" . DAC value between 0 ~ 2.5 V");
        return FALSE;
    }
}


void OpticalLed_TurnOn()
{
    uint16_t vol;
    vol = (uint16_t)(s_DACDefValue * 4095 / 2.5);
    DAC_SetChannel2Data(DAC_Align_12b_R,vol);
}

void OpticalLed_TurnOff()
{
    DAC_SetChannel2Data(DAC_Align_12b_R,0);
}

void OpticalLed_ChangeDACOut(float valve)
{
    uint16_t vol;
    s_DACOutValue = valve;
    vol = (uint16_t)(s_DACOutValue * 4095 / 2.5);
    DAC_SetChannel2Data(DAC_Align_12b_R,vol);
}
