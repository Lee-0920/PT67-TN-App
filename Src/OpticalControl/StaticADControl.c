/*
 * StaticADControl.c
 *
 *  Created on: 2018年11月21日
 *      Author: Administrator
 */
#include "FreeRTOS.h"
#include "task.h"
#include "SystemConfig.h"
#include "Driver/McuFlash.h"
#include "DncpStack/DncpStack.h"
#include "Tracer/Trace.h"
#include "StaticADControl.h"
#include "OpticalMeter/Meter.h"
#include "OpticalControl.h"
#include <math.h>
#include <OpticalDriver/AD5175Driver.h>
#include <OpticalDriver/OpticalLed.h>
#include "Driver/OpticalDriver/OpticalXonen.h"

typedef enum
{
    ADCTL_IDLE,
    ADCTL_BUSY
} ADCtrlStatus;

typedef enum
{
    POSITIVE_CORRELATION,    //正相关-电阻越大 AD越大
    NEGATIVE_CORRELATION    //负相关-电阻越大 AD越小
}ADCtrlCorrelation;

typedef struct
{
    AD5175Driver* ad5175;
    Uint16  defaultValue;
    Uint8   addr;
}ADController;

static StaticADControlResult s_adCtrlResult;
static Bool s_adCtrlSendEvent;
static ADCtrlStatus s_adCtrlStatus;
static Uint8 s_currentIndex;
static Uint32 s_targetAD;
static Uint32 s_currentAD;
static Uint32 s_lastAD;
static Uint16 s_currentValue;
static Uint16 s_lastValue;
static Uint16 s_minValue;
static Uint16 s_maxValue;
static Bool s_adCtrlOver;
static AD5175Driver ad5175_iic1;
static AD5175Driver ad5175_iic2;
static ADController g_adController[AD_CONTROLLER_NUM];
static Bool s_isValid = FALSE;
static ADCtrlCorrelation s_correlation = NEGATIVE_CORRELATION;

static void StaticADControl_ADHandleTask(void *argument);
static Uint32 StaticADControl_GetCurrentAD(Uint8 index);

static xTaskHandle s_staticADControlHandle;

void StaticADControl_Init(void)
{
    StaticADControl_InitDriver();
    StaticADControl_InitParam();
    StaticADControl_InitSetting();

    xTaskCreate(StaticADControl_ADHandleTask, "StaticADControlHandle",
            STATIC_AD_CONTROL_STK_SIZE, NULL,
            STATIC_AD_CONTROL_TASK_PRIO, &s_staticADControlHandle);
}

void StaticADControl_InitDriver(void)
{
    ad5175_iic1.pinSCL = GPIO_Pin_0;
    ad5175_iic1.portSCL = GPIOC;
    ad5175_iic1.rccSCL  = RCC_AHB1Periph_GPIOC;
    ad5175_iic1.pinSDA = GPIO_Pin_1;
    ad5175_iic1.portSDA = GPIOC;
    ad5175_iic1.rccSDA  = RCC_AHB1Periph_GPIOC;
    AD5175_Init(&ad5175_iic1);

    g_adController[TN_LED_MEA].ad5175 = &ad5175_iic1;
    g_adController[TN_LED_MEA].addr = AD5175_ADDR_0;

    g_adController[TN_LED_REF].ad5175 = &ad5175_iic1;
    g_adController[TN_LED_REF].addr = AD5175_ADDR_1;

    g_adController[TP_LED_MEA].ad5175 = &ad5175_iic1;
    g_adController[TP_LED_MEA].addr = AD5175_ADDR_2;

    ad5175_iic2.pinSCL = GPIO_Pin_0;
    ad5175_iic2.portSCL = GPIOA;
    ad5175_iic2.rccSCL  = RCC_AHB1Periph_GPIOA;
    ad5175_iic2.pinSDA = GPIO_Pin_1;
    ad5175_iic2.portSDA = GPIOA;
    ad5175_iic2.rccSDA  = RCC_AHB1Periph_GPIOA;
    AD5175_Init(&ad5175_iic2);

    g_adController[LED_METER1].ad5175 = &ad5175_iic2;
    g_adController[LED_METER1].addr = AD5175_ADDR_0;

    g_adController[LED_METER2].ad5175 = &ad5175_iic2;
    g_adController[LED_METER2].addr = AD5175_ADDR_1;

    g_adController[TP_LED_REF].ad5175 = &ad5175_iic2;
    g_adController[TP_LED_REF].addr = AD5175_ADDR_2;
}

void StaticADControl_InitParam(void)
{
    Uint8 buffer[AD5175_CONTROL_SIGN_FLASH_LEN] = { 0 };
    Uint32 flashFactorySign = 0;
    Uint8 param[AD_CONTROLLER_NUM*2] = {0};
    Uint8 i;

    McuFlash_Read(AD5175_CONTROL_SIGN_FLASH_BASE_ADDR, AD5175_CONTROL_SIGN_FLASH_LEN, buffer);                //读取出厂标志位
    memcpy(&flashFactorySign, buffer, AD5175_CONTROL_SIGN_FLASH_LEN);

    if (FLASH_FACTORY_SIGN == flashFactorySign)       //表示已经过出厂设置
    {
        for(i = 0; i < AD_CONTROLLER_NUM; i++)
        {
            g_adController[i].defaultValue = StaticADControl_GetDefaultValue(i);
        }
    }
    else
    {
        //未出厂,使用设备默认值
        for(i = 0; i < AD_CONTROLLER_NUM; i++)
        {
            g_adController[i].defaultValue = AD5175_ReadRDAC(g_adController[i].ad5175, g_adController[i].addr);
            memcpy(&param[i*2], &g_adController[i].defaultValue, sizeof(Uint16));
        }
        //保存设备默认值至Flash
        McuFlash_Write(AD5175_CONTROL_PARAM_FLASH_BASE_ADDR, AD5175_CONTROL_PARAM_FLASH_LEN, param);

        //写入出厂标志
        flashFactorySign = FLASH_FACTORY_SIGN;
        memcpy(buffer, &flashFactorySign, AD5175_CONTROL_SIGN_FLASH_LEN);

        McuFlash_Write(AD5175_CONTROL_SIGN_FLASH_BASE_ADDR, AD5175_CONTROL_SIGN_FLASH_LEN, buffer);
    }
}

void StaticADControl_InitSetting(void)
{
    s_isValid = FALSE;
    for(Uint8 i = 0; i < AD_CONTROLLER_NUM; i++)
    {
        if(TRUE == AD5175_WriteRDAC(g_adController[i].ad5175, g_adController[i].addr, g_adController[i].defaultValue))     //使用默认值设置AD5175
        {
            s_isValid = TRUE;
            TRACE_INFO("\n static ad control is valid.");
        }
    }
}

Bool StaticADControl_Start(Uint8 index, Uint32 targetAD)
{
    if(index < AD_CONTROLLER_NUM)
    {
        if (ADCTL_IDLE == s_adCtrlStatus)
        {
            if(index == TN_LED_REF || index == TN_LED_MEA)
            {
                OpticalXonen_TurnOn();
                OpticalControl_EnterCollectStatus();
                OpticalControl_ADCChangeInit(ADS1146);
                s_correlation = POSITIVE_CORRELATION;
            }
            else if(index == TP_LED_REF || index == TP_LED_MEA)
            {
                OpticalLed_TurnOn();
                OpticalControl_EnterCollectStatus();
                OpticalControl_ADCChangeInit(AD7791);
                s_correlation = NEGATIVE_CORRELATION;
            }
            else if(index == LED_METER1)
            {
                MeterLED_TurnOn(0);
                s_correlation = NEGATIVE_CORRELATION;
            }
            else if(index == LED_METER2)
            {
                MeterLED_TurnOn(1);
                s_correlation = NEGATIVE_CORRELATION;
            }
            s_currentIndex = index;
            s_targetAD = targetAD;
            s_currentAD = 0xFFFFFFFF;
            s_lastAD = s_currentAD;
            s_currentValue =  AD5175_ReadRDAC(g_adController[index].ad5175, g_adController[index].addr);
            s_lastValue = 0xFFFF;
            s_maxValue = AD5175_MAX_VALUE;
            s_minValue = AD5175_MIN_VALUE;
            s_adCtrlOver = FALSE;
            s_adCtrlResult = STATIC_AD_CONTROL_RESULT_UNFINISHED;
            s_adCtrlStatus = ADCTL_BUSY;
            TRACE_INFO("\n static AD control start. index = %d, targetAD = %d", index, targetAD);
            vTaskResume(s_staticADControlHandle);
            return TRUE;
        }
        else
        {
            TRACE_ERROR("\n static AD control failed to start because it is running.");
            return FALSE;
        }
    }
    else
    {
        TRACE_ERROR("\n static AD control failed to start because index must between 0 - %d.", AD_CONTROLLER_NUM);
        return FALSE;
    }
}

void StaticADControl_Stop(void)
{
    if (ADCTL_BUSY == s_adCtrlStatus)
    {
        if(s_adCtrlOver == TRUE)
        {
            s_adCtrlResult =  STATIC_AD_CONTROL_RESULT_FINISHED;
        }
        else
        {
            s_adCtrlResult =  STATIC_AD_CONTROL_RESULT_UNFINISHED;
        }

        if(s_currentIndex == TN_LED_REF || s_currentIndex == TN_LED_MEA)
        {
            OpticalXonen_TurnOff();
            OpticalControl_EnterIdleStatus();
            OpticalControl_WorkModeRestore();
        }
        else if(s_currentIndex == TP_LED_REF || s_currentIndex == TP_LED_MEA)
        {
            OpticalLed_TurnOff();
            OpticalControl_EnterIdleStatus();
            OpticalControl_WorkModeRestore();
        }
        else if(s_currentIndex == LED_METER1)
        {
            MeterLED_TurnOff(0);
        }
        else if(s_currentIndex == LED_METER2)
        {
            MeterLED_TurnOff(1);
        }

        if(s_adCtrlSendEvent == TRUE)
        {
            // 发送结果事件
            DncpStack_SendEvent(DSCP_EVENT_OAI_STATIC_AD_CONTROL_RESULT, &s_adCtrlResult, sizeof(StaticADControlResult));
            DncpStack_BufferEvent(DSCP_EVENT_OAI_STATIC_AD_CONTROL_RESULT, &s_adCtrlResult, sizeof(StaticADControlResult));
            StaticADControl_SendEventClose();  //关闭事件发送
            TRACE_INFO("\n static ad control send result = %d addr = %0x", (Uint8)s_adCtrlResult, &s_adCtrlResult);
            System_Delay(50);
        }

        if(s_adCtrlOver == TRUE)
        {
            if(s_currentValue != g_adController[s_currentIndex].defaultValue)
            {
                StaticADControl_SetDefaultValue(s_currentIndex, s_currentValue);
            }
            else
            {
                TRACE_INFO("\n control result value is equal to default value");
            }
        }

        s_adCtrlOver = FALSE;
        s_adCtrlStatus = ADCTL_IDLE;
        s_correlation = NEGATIVE_CORRELATION;
        TRACE_INFO("\n static ad control stop.");
    }
    else
    {
        TRACE_INFO("\n static ad controller is not running.");
    }
}

Uint16 StaticADControl_GetDefaultValue(Uint8 index)
{
    Uint8 buffer[2] = {0};
    Uint16 offset = 0;
    Uint16 value = 0xFFFF;

    if(index < AD_CONTROLLER_NUM)
    {
        offset = index*(sizeof(Uint16));

        McuFlash_Read(AD5175_CONTROL_PARAM_FLASH_BASE_ADDR + offset, sizeof(Uint16), buffer);

        memcpy(&value, buffer, sizeof(Uint16));

        TRACE_DEBUG("\n static ad ctrl get  index %d default value %d.", index, value);
    }
    else
    {
        TRACE_ERROR("\n invalid index %d.", index);
    }

    return value;
}

void StaticADControl_SetDefaultValue(Uint8 index, Uint16 value)
{
    Uint8 buffer[2] = {0};
    Uint16 offset = 0;
    Uint16 useValue = value&AD5175_MAX_VALUE;

    if(index < AD_CONTROLLER_NUM)
    {
        offset = index*(sizeof(Uint16));

        memcpy(buffer, &useValue, sizeof(Uint16));

        McuFlash_Write(AD5175_CONTROL_PARAM_FLASH_BASE_ADDR + offset, sizeof(Uint16), buffer);

        g_adController[index].defaultValue = useValue;

        TRACE_INFO("\n static ad ctrl set  index %d default value %d.", index, value);
    }
    else
    {
        TRACE_ERROR("\n invalid index %d.", index);
    }
}

Uint16 StaticADControl_GetRealValue(Uint8 index)
{
    Uint16 value = 0xFFFF;

    if(index < AD_CONTROLLER_NUM)
    {
        value = AD5175_ReadRDAC(g_adController[index].ad5175, g_adController[index].addr);
        TRACE_INFO("\n static ad ctrl get  index %d current value %d.", index, value);
    }
    else
    {
        TRACE_ERROR("\n invalid index %d  return %d.", index, value);
    }

    return value;
}

Bool StaticADControl_SetRealValue(Uint8 index, Uint16 value)
{
    if(AD5175_WriteRDAC(g_adController[index].ad5175, g_adController[index].addr, value))
    {
        TRACE_INFO("\n static ad ctrl set  index %d real value %d.", index, value);
        return TRUE;
    }
    else
    {
        TRACE_ERROR("\n static ad ctrl set  real value fail.");
        return FALSE;
    }
}

static Uint32 StaticADControl_GetCurrentAD(Uint8 index)
{
    Uint8 filterNum = 10;
    Uint32 ad = 0;
    OpticalSignalAD optAD;
    OpticalMeterAD meterAD;

    switch(index)
    {
        case TN_LED_REF:
            for(int i = 0; i < filterNum; i++)
            {
                while(FALSE == OpticalControl_GetADS1146Data(&optAD))
                {
                    System_Delay(2);
                }
                ad = ad + optAD.reference;
                TRACE_CODE("\n current TN_LED_REF ad[%d] = %d", i, optAD.reference);
                System_Delay(300);
            }
            ad = ad/filterNum;
            TRACE_DEBUG("\n current TN_LED_REF ad average  = %d", ad);
            break;

        case TN_LED_MEA:
            for(int i = 0; i < filterNum; i++)
            {
                while(FALSE == OpticalControl_GetADS1146Data(&optAD))
                {
                    System_Delay(2);
                }
                ad = ad + optAD.measure;
                TRACE_CODE("\n current TN_LED_MEA ad[%d] = %d", i, optAD.measure);
                System_Delay(300);
            }
            ad = ad/filterNum;
            TRACE_DEBUG("\n current TN_LED_MEA ad average  = %d", ad);
            break;

        case TP_LED_REF:
            for(int i = 0; i < filterNum; i++)
            {
                while(FALSE == OpticalControl_GetAD7791Data(&optAD))
                {
                    System_Delay(2);
                }
                ad = ad + optAD.reference;
                TRACE_CODE("\n current LED_REF ad[%d] = %d", i, optAD.reference);
                System_Delay(300);
            }
            ad = ad/filterNum;
            TRACE_DEBUG("\n current LED_REF ad average  = %d", ad);
            break;

        case TP_LED_MEA:
            for(int i = 0; i < filterNum; i++)
            {
                while(FALSE == OpticalControl_GetAD7791Data(&optAD))
                {
                    System_Delay(2);
                }
                ad = ad + optAD.measure;
                TRACE_CODE("\n current TP_LED_MEA ad[%d] = %d", i, optAD.measure);
                System_Delay(300);
            }
            ad = ad/filterNum;
            TRACE_DEBUG("\n current TP_LED_MEA ad average  = %d", ad);
            break;

        case LED_METER1:
            for(int i = 0; i < filterNum; i++)
            {
                meterAD = Meter_GetOpticalAD();
                ad = ad + meterAD.adValue[0];
                TRACE_CODE("\n current LED_METER1 ad[%d] = %d", i, meterAD.adValue[0]);
                System_Delay(100);
            }
            ad = ad/filterNum;
            TRACE_DEBUG("\n current LED_METER1 ad average  = %d", ad);
            break;

        case LED_METER2:
            for(int i = 0; i < filterNum; i++)
            {
                meterAD = Meter_GetOpticalAD();
                ad = ad + meterAD.adValue[1];
                TRACE_CODE("\n current LED_METER2 ad[%d] = %d", i, meterAD.adValue[1]);
                System_Delay(100);
            }
            ad = ad/filterNum;
            TRACE_DEBUG("\n current LED_METER2 ad average  = %d", ad);
            break;

        default:
            break;
    }

    return ad;
}

void StaticADControl_ADHandleTask(void *argument)
{
    vTaskSuspend(NULL);
    while (1)
    {
        switch (s_adCtrlStatus)
        {
            case ADCTL_IDLE:
                vTaskSuspend(NULL);
                break;

            case ADCTL_BUSY:
                s_currentAD = StaticADControl_GetCurrentAD(s_currentIndex);
                TRACE_DEBUG("\n static AD control current value = %d, ad = %d", s_currentValue, s_currentAD);
                if(fabs((double)s_lastValue - (double)s_currentValue)  < 2 || fabs((double)s_currentAD - (double)s_targetAD) <= 0.01*(double)s_targetAD)
                {
                    if(fabs((double)s_lastAD - (double)s_targetAD) <= fabs((double)s_currentAD - (double)s_targetAD))
                    {
                        s_currentValue = s_lastValue;
                    }
                    s_adCtrlOver = TRUE;
                }
                else
                {
                    if(POSITIVE_CORRELATION == s_correlation)  //正相关
                    {
                        if(s_currentAD <= s_targetAD)    //电阻越大 AD越大
                        {
                            s_minValue = s_currentValue;
                            s_lastValue = s_currentValue;
                            s_lastAD = s_currentAD;
                            s_currentValue = (s_currentValue + s_maxValue)/2;
                        }
                        else
                        {
                            s_maxValue = s_currentValue;
                            s_lastValue = s_currentValue;
                            s_lastAD = s_currentAD;
                            s_currentValue = (s_currentValue + s_minValue)/2;
                        }
                    }
                    else if(NEGATIVE_CORRELATION == s_correlation)  //负相关
                    {
                        if(s_currentAD <= s_targetAD)    //电阻越小 AD越大
                        {
                            s_maxValue = s_currentValue;
                            s_lastValue = s_currentValue;
                            s_lastAD = s_currentAD;
                            s_currentValue = (s_currentValue + s_minValue)/2;
                        }
                        else
                        {
                            s_minValue = s_currentValue;
                            s_lastValue = s_currentValue;
                            s_lastAD = s_currentAD;
                            s_currentValue = (s_currentValue + s_maxValue)/2;
                        }
                    }

                    s_adCtrlOver = FALSE;
                }
                TRACE_DEBUG("\n calculate new value = %d", s_currentValue);
                if(FALSE == AD5175_WriteRDAC(g_adController[s_currentIndex].ad5175, g_adController[s_currentIndex].addr, s_currentValue)) //写失败后停止
                {
                    StaticADControl_Stop();
                }

                if(s_adCtrlOver == TRUE)
                {
                    TRACE_INFO("\n static AD control over. value = %d", s_currentValue);
//                    if(s_currentValue != g_adController[s_currentIndex].defaultValue)
//                    {
//                        StaticADControl_SetDefaultValue(s_currentIndex, s_currentValue);
//                    }
//                    else
//                    {
//                        TRACE_INFO("\n control result value is equal to default value");
//                    }

                    StaticADControl_Stop();
                }
                System_Delay(1000);
                break;
        }
    }
}

void StaticADControl_SendEventOpen(void)
{
    s_adCtrlSendEvent = TRUE;
}

void StaticADControl_SendEventClose(void)
{
    s_adCtrlSendEvent = FALSE;
}

Bool StaticADControl_IsValid(void)
{
    return s_isValid;
}

