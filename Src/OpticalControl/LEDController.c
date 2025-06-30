/*
 * LEDController.c
 *
 *  Created on: 2016年9月2日
 *      Author: LIANG
 */

#include <OpticalDriver/OpticalLed.h>
#include "Tracer/Trace.h"
#include "FreeRTOS.h"
#include "DncpStack/DncpStack.h"
#include "task.h"
#include "System.h"
#include "OpticalControl.h"
#include "LEDController.h"
#include "SystemConfig.h"
#include "String.h"
#include "Driver/McuFlash.h"
#include "math.h"

#define LED_DAC_DEFAULT_VALVE  1
#define PID_TARGET_DEFAULT_VALVE 11000000

typedef enum
{
    LEDCTL_IDLE,
    LEDCTL_BUSY
} LEDCTLStatus;

typedef enum
{
    LED_CONTROL_AUTO,
    LED_CONTROL_ONCE,
}LEDControlMode;

static LEDControlMode s_ledctlMode;
static LEDCTLStatus s_ledctlStatus;
static int s_ledctlTimeout;
static Uint32 s_ledctlTolerance;
static AdjustResult s_ledctlResult;
static Bool s_ledctlAdjustOver;
static Bool s_ledctlSendEvent;

static float s_LEDDACOut = LED_DAC_DEFAULT_VALVE;
static Uint32 s_PIDTarget;
static float s_lastError = 0;   //上次误差值
static float s_lastDifferential = 0;   //上次的微分值
const static LEDControllerParam kDefaultLEDControllerParamm =
{ .proportion = 0.003, .integration = 0.01, .differential = 0.006, };
static LEDControllerParam s_LEDControllerParam;

static void LEDController_LEDHandle(void *argument);
static xTaskHandle s_LEDHandle;

void LEDController_Init(void)
{
    Uint8 buffer[LEDCONTROLLER_SIGN_FLASH_LEN] =
    { 0 };
    Uint32 flashFactorySign = 0;

    McuFlash_Read(LEDCONTROLLER_SIGN_FLASH_BASE_ADDR,
    LEDCONTROLLER_SIGN_FLASH_LEN, buffer); //读取出厂标志位

    memcpy(&flashFactorySign, buffer, LEDCONTROLLER_SIGN_FLASH_LEN);

    if (FLASH_FACTORY_SIGN == flashFactorySign) //表示已经过出厂设置
    {
        s_PIDTarget = LEDController_GetTarget();
        s_LEDControllerParam = LEDController_GetParam();
    }
    else
    {
        LEDController_SetTarget(PID_TARGET_DEFAULT_VALVE);
        LEDController_SetParam(kDefaultLEDControllerParamm);

        flashFactorySign = FLASH_FACTORY_SIGN;
        memcpy(buffer, &flashFactorySign, LEDCONTROLLER_SIGN_FLASH_LEN);
        McuFlash_Write(
        LEDCONTROLLER_SIGN_FLASH_BASE_ADDR,
        LEDCONTROLLER_SIGN_FLASH_LEN, buffer);
    }

    s_ledctlMode = LED_CONTROL_AUTO;
    s_ledctlStatus = LEDCTL_IDLE;
    OpticalLed_Init();

    xTaskCreate(LEDController_LEDHandle, "MeaLEDPID", MEASURE_LED_PID_HANDLE_STK_SIZE, NULL, MEASURE_LED_PID_HANDLE_TASK_PRIO,
            &s_LEDHandle);
}

void LEDController_Restore(void)
{
    LEDController_Stop();
}

/**
 * @brief 打开LED灯，其控制电压为默认值
 */
void LEDController_TurnOnLed()
{
    OpticalLed_TurnOn();
    TRACE_INFO("\n LED turn on");
}

Uint32 LEDController_GetTarget(void)
{
    Uint8 readData[LEDCONTROLLER_TARGET_FLASH_LEN] =
    { 0 };
    Uint32 target;

    McuFlash_Read(LEDCONTROLLER_TARGET_ADDRESS,
    LEDCONTROLLER_TARGET_FLASH_LEN, readData);
    memcpy(&target, readData, sizeof(Uint32));

    return target;
}

void LEDController_SetTarget(Uint32 target)
{
    Uint8 writeData[LEDCONTROLLER_TARGET_FLASH_LEN] =
    { 0 };

    memcpy(writeData, &target, sizeof(Uint32));
    McuFlash_Write(LEDCONTROLLER_TARGET_ADDRESS,
    LEDCONTROLLER_TARGET_FLASH_LEN, writeData);
    s_PIDTarget = target;
    TRACE_INFO("\n LEDController Target %d:", s_PIDTarget);
}

LEDControllerParam LEDController_GetParam(void)
{
    Uint8 readData[LEDCONTROLLER_PARAM_FLASH_LEN] =
    { 0 };
    LEDControllerParam ledControllerParam;

    McuFlash_Read(LEDCONTROLLER_PARAM_ADDRESS,
    LEDCONTROLLER_PARAM_FLASH_LEN, readData);
    memcpy(&ledControllerParam, readData, sizeof(LEDControllerParam));

    return ledControllerParam;
}

void LEDController_SetParam(LEDControllerParam param)
{
    Uint8 writeData[LEDCONTROLLER_PARAM_FLASH_LEN] =
    { 0 };

    memcpy(writeData, &param, sizeof(LEDControllerParam));
    McuFlash_Write(LEDCONTROLLER_PARAM_ADDRESS,
    LEDCONTROLLER_PARAM_FLASH_LEN, writeData);

    s_LEDControllerParam = param;
    TRACE_INFO("\n LEDControllerParam:");
    TRACE_INFO("\n KP: ");
    System_PrintfFloat(TRACE_LEVEL_INFO, s_LEDControllerParam.proportion, 6);
    TRACE_INFO("\n KI: ");
    System_PrintfFloat(TRACE_LEVEL_INFO, s_LEDControllerParam.integration, 6);
    TRACE_INFO("\n KD: ");
    System_PrintfFloat(TRACE_LEVEL_INFO, s_LEDControllerParam.differential, 6);
}

Bool LEDController_Start(void)
{
    if (LEDCTL_IDLE == s_ledctlStatus)
    {
        s_PIDTarget = LEDController_GetTarget();
        s_lastError = 0;
        s_lastDifferential = 0;
        s_LEDDACOut = LED_DAC_DEFAULT_VALVE;
        s_ledctlStatus = LEDCTL_BUSY;
        s_ledctlMode = LED_CONTROL_AUTO;
        OpticalLed_TurnOn();
        vTaskResume(s_LEDHandle);
        TRACE_INFO("\n LEDController start");
        return TRUE;
    }
    else
    {
        TRACE_ERROR("\n LEDController failed to start because it is running.");
        return FALSE;
    }
}

Bool LEDController_AdjustToValue(Uint32 targetAD, Uint32 tolerance, Uint32 timeout)
{
    if(targetAD <= 16777215)
    {
        if (LEDCTL_IDLE == s_ledctlStatus)
        {
            s_PIDTarget = targetAD;
            s_lastError = 0;
            s_lastDifferential = 0;
            s_LEDDACOut = LED_DAC_DEFAULT_VALVE;
            s_ledctlStatus = LEDCTL_BUSY;
            s_ledctlMode = LED_CONTROL_ONCE;
            s_ledctlResult = ADJUST_RESULT_FAILED;
            s_ledctlAdjustOver = FALSE;
            s_ledctlTimeout = timeout;
            s_ledctlTolerance = tolerance;
            OpticalLed_TurnOn();
            vTaskResume(s_LEDHandle);
            TRACE_INFO("\n LEDController adjust start: targetAD =  %d,  tolerance = %d, timeout = %d", targetAD, tolerance, timeout);
            return TRUE;
        }
        else
        {
            TRACE_ERROR("\n LEDController adjust failed to start because it is running.");
            return FALSE;
        }
    }
    else
    {
        TRACE_ERROR("\n LEDController adjust failed to start because target AD out of range");
        return FALSE;
    }
}

void LEDController_AdjustStop(void)
{
    OpticalLed_TurnOff();
    TRACE_INFO("\n led close.");
    if (LEDCTL_BUSY == s_ledctlStatus)
    {
        s_ledctlStatus = LEDCTL_IDLE;
        TRACE_INFO("\n LEDController Adjust stop.");

        if(s_ledctlAdjustOver == FALSE)
        {
            s_ledctlResult = ADJUST_RESULT_STOPPED;
        }

        if(s_ledctlSendEvent == TRUE)
        {
            // 发送结果事件
            DncpStack_SendEvent(DSCP_EVENT_OAI_AD_ADJUST_RESULT,
                    (void*)&s_ledctlResult, 1);
            DncpStack_BufferEvent(DSCP_EVENT_OAI_AD_ADJUST_RESULT,
                    (void*)&s_ledctlResult, 1);
            TRACE_INFO("\n LEDController Adjust Stopped.  Result = %d",(int)s_ledctlResult);
            LEDController_SendEventClose();  //关闭事件发送
        }

        s_ledctlAdjustOver = FALSE;
        s_ledctlResult = ADJUST_RESULT_FAILED;
        s_ledctlTimeout = 10000;
        s_ledctlTolerance = 500000;
    }
    else
    {
        TRACE_INFO("\n LEDController Adjust is not running.");
    }
}


void LEDController_Stop(void)
{
    OpticalLed_TurnOff();
    TRACE_INFO("\n led close.");
    if (LEDCTL_BUSY == s_ledctlStatus)
    {
        s_ledctlStatus = LEDCTL_IDLE;
        TRACE_INFO("\n LEDController stop.");
    }
    else
    {
        TRACE_INFO("\n LEDController is not running.");
    }
}

static float LEDController_IncPIDCalc(void)
{
    int timeout = 1000;
    Uint32 currentAD;
    float E,DE,Dout;
    int Err;
    OpticalSignalAD opticalSignalAD;
    while((FALSE == OpticalControl_GetAD7791Data(&opticalSignalAD)) && (timeout > 0))
    {
        System_Delay(2);
        timeout -= 2;
    }

    if(timeout < 0)  //超时
    {
        TRACE_DEBUG("\n LED auto control get AD time out");
        return 0;
    }

    currentAD = opticalSignalAD.reference;

    if((s_ledctlMode == LED_CONTROL_ONCE) && ((Uint32)(currentAD - s_PIDTarget) < s_ledctlTolerance || (Uint32)(s_PIDTarget - currentAD) < s_ledctlTolerance))
    {
        s_ledctlResult = ADJUST_RESULT_FINISHED;
        Dout = 0;
        s_ledctlAdjustOver = TRUE;
        TRACE_DEBUG("\n LEDController Adjust reach to %d.", (Uint32)currentAD);
    }
    else
    {
        Err = s_PIDTarget - currentAD;
        E = (float)Err/1000000.0;
        TRACE_DEBUG("\n Err: %d, E: ", Err);
        System_PrintfFloat(TRACE_LEVEL_INFO, E, 6);
        //E = ((float)(s_PIDTarget - currentAD) / 1000000);
        DE = E - s_lastError;

        Dout = s_LEDControllerParam.proportion * DE
                + s_LEDControllerParam.integration * E
                + s_LEDControllerParam.differential * (DE - s_lastDifferential);
        s_lastError = E;
        s_lastDifferential = DE;
        TRACE_DEBUG("\n objref: %d, curref: %d, curmea: %d, pidout:", s_PIDTarget,
                opticalSignalAD.reference, opticalSignalAD.measure);
        System_PrintfFloat(TRACE_LEVEL_INFO, Dout, 6);
        TRACE_DEBUG(" , ");
    }
    return Dout;
}

void LEDController_LEDHandle(void *argument)
{
    vTaskSuspend(NULL);
    while (1)
    {
        switch (s_ledctlStatus)
        {
        case LEDCTL_IDLE:
            OpticalLed_TurnOff();
            vTaskSuspend(NULL);
            break;
        case LEDCTL_BUSY:
            System_Delay(200);
            if(s_ledctlMode == LED_CONTROL_ONCE)
            {
                s_ledctlTimeout -= 200;
                if(s_ledctlTimeout <= 0)
                {
                    s_ledctlResult = ADJUST_RESULT_TIMEOUT;
                    s_ledctlAdjustOver = TRUE;
                    TRACE_INFO("\n LEDController Adjust timeout.");
                }
                s_LEDDACOut += LEDController_IncPIDCalc();
            }
            else
            {
                s_LEDDACOut += LEDController_IncPIDCalc();
            }

            if (s_LEDDACOut > 2.5)
            {
                s_LEDDACOut = 2.5;
            }
            else if (s_LEDDACOut < 0)
            {
                s_LEDDACOut = 0;
            }
            TRACE_DEBUG("LED DACOut: ",);
            System_PrintfFloat(TRACE_LEVEL_INFO, s_LEDDACOut, 6);
            OpticalLed_ChangeDACOut(s_LEDDACOut);

            if((s_ledctlMode == LED_CONTROL_ONCE) && (s_ledctlAdjustOver == TRUE))
            {
                if(s_ledctlSendEvent == TRUE)
                {
                    // 发送结果事件
                    DncpStack_SendEvent(DSCP_EVENT_OAI_AD_ADJUST_RESULT,
                            (void*)&s_ledctlResult, 1);
                    DncpStack_BufferEvent(DSCP_EVENT_OAI_AD_ADJUST_RESULT,
                            (void*)&s_ledctlResult, 1);
                    TRACE_INFO("\n LEDController Adjust Over.  Result = %d",(int)s_ledctlResult);
                    LEDController_SendEventClose();  //关闭事件发送
                }

                s_ledctlAdjustOver = FALSE;
                s_ledctlResult = ADJUST_RESULT_FAILED;
                s_ledctlTimeout = 10000;
                s_ledctlTolerance = 500000;
                s_ledctlStatus = LEDCTL_IDLE;  //停止调节
            }

            break;
        }
    }
}

/**
 * @brief 打开调光停止发送事件功能
 */
void LEDController_SendEventOpen(void)
{
    s_ledctlSendEvent = TRUE;
}

/**
 * @brief 关闭调光停止发送事件功能
 */
void LEDController_SendEventClose(void)
{
    s_ledctlSendEvent = FALSE;
}

