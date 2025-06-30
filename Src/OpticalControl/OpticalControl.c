/**
 #include <OpticalADCollect.h>
 * @file
 * @brief 光学采集控制接口实现
 * @details
 * @version 1.0.0
 * @author lemon.xiaoxun
 * @date 2016-5-27
 */

#include <OpticalDriver/OpticalChannel.h>
#include <OpticalDriver/OpticalLed.h>

#include "Common/Types.h"
#include "DncpStack/DncpStack.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "Tracer/Trace.h"
#include "System.h"
#include "DNCP/Lai/LaiRS485Handler.h"
#include "Driver/OpticalDriver/OpticalXonen.h"
#include "Driver/OpticalDriver/ADS1146Collect.h"
#include "Driver/OpticalDriver/AD7791Collect.h"
#include "Driver/OpticalDriver/OpticalChannel.h"
#include "OpticalControl.h"
#include "SystemConfig.h"
#include <string.h>

typedef enum
{
    COLLECT_IDLE, COLLECT_BUSY
} CollectStatus;

#define MODE_SWITCH_TIMEOUT    1000     //双ADC模式切换时间

#define SAMPLE_DATA_QUEUE_SIZE           (5)

#define SIGNAL_SAMPLE_TIME       (45)                 // 信号采样时间  单位:ms
#define SIGNAL_SAMPLE_COUNT      (4)                  // 信号采样个数
#define DATA_FILTER_NUM(x)       (x/4)                // 数据过滤比例
#define DATA_MAX_LENGHT          (25)               // 数据最大长度

#define ADS1146_DATA_MAX_LEN  1024
#define ADS1146_DATA_FILTER_NUM(x)       (x/15)                // 数据过滤比例

typedef struct
{
    AcquiredResult ret;
    OpticalSignalAD ad;
}OpticalAcquireResult;

typedef struct
{
    Bool setModeEvent;
    OpticalWorkMode workMode;
    OpticalControlStatus status;
    Bool isRequestStop;
    Bool isSendEvent;
    QueueHandle_t ad7791DataQueue;//采集处理任务运行时，用于接收采集任务发送的数据
    QueueHandle_t ads1146DataQueue;//采集处理任务运行时，用于接收采集任务发送的数据
    OpticalSignalAD curAD7791Data;    //当前采样数据
    OpticalSignalAD curADS1146Data;    //当前采样数据
    Bool isNewSampleData;
    Bool isNewAD7791Data;
    Bool isNewADS1146Data;
    ADCType currentAcquireType;
    OpticalAcquireResult result;
    TaskHandle_t adCollectHandle;
    TaskHandle_t opticalCollectHandle;
    float acquireADTime;
    float acquireADStartTime;
    TimerHandle_t signalADNotifyPeriodTimer;
    Bool isOpenTestMode;
    Uint8 testChannel;
}OpticalControl;

static OpticalControl s_opticalControl;

static void OpticalControl_ADHandleTask(void *argument);
static void OpticalControl_ADCollectTask(void *argument);
static void OpticalControl_SignalADPeriodHandle(TimerHandle_t argument);

/**
 * @brief 光学信号采集模块初始化
 */
void OpticalControl_Init(void)
{
    s_opticalControl.setModeEvent = FALSE;
    s_opticalControl.workMode = ONLY_ADS1146;
    s_opticalControl.status = OPTICALCONTROL_IDLE;
    s_opticalControl.isRequestStop = FALSE;
    s_opticalControl.isSendEvent = FALSE;
    s_opticalControl.ad7791DataQueue = xQueueCreate(SAMPLE_DATA_QUEUE_SIZE, sizeof(OpticalSignalAD));
    s_opticalControl.ads1146DataQueue = xQueueCreate(SAMPLE_DATA_QUEUE_SIZE, sizeof(OpticalSignalAD));
    s_opticalControl.curAD7791Data.reference = 0;
    s_opticalControl.curAD7791Data.measure = 0;
    s_opticalControl.curADS1146Data.reference = 0;
    s_opticalControl.curADS1146Data.measure = 0;
    s_opticalControl.acquireADTime = 0;
    s_opticalControl.acquireADStartTime = 0;
    s_opticalControl.currentAcquireType = ADS1146;

    OpticalChannel_Init();
    OpticalLed_Init();
    OpticalXonen_Init();
    OpticalControl_WorkModeRestore();

    xTaskCreate(OpticalControl_ADCollectTask, "OpticalADCollect",
            OPTICALCONTROL_ADCOLLECT_STK_SIZE, (void *)&s_opticalControl,
            OPTICALCONTROL_ADCOLLECT_TASK_PRIO, &s_opticalControl.adCollectHandle);

    xTaskCreate(OpticalControl_ADHandleTask, "OpticalADHandle",
            OPTICALCONTROL_ADHANDLE_STK_SIZE, (void *)&s_opticalControl,
            OPTICALCONTROL_ADHANDLE_TASK_PRIO, &s_opticalControl.opticalCollectHandle);

    s_opticalControl.signalADNotifyPeriodTimer = xTimerCreate("signalADPeriod",
            (uint32_t) (1000 / portTICK_RATE_MS), pdTRUE, (void *) OPTICAL_AD_NOTIFY_PERIOD_TIMER_REIO,
            OpticalControl_SignalADPeriodHandle);

}

void OpticalControl_ADCChangeInit(ADCType type)
{
    s_opticalControl.currentAcquireType = type;
    switch(type)
    {
        case AD7791:
            AD7791Collect_Init();
            break;

        case ADS1146:
            ADS1146Collect_Init();
            break;

        default:
            break;
    }
}

/**
 * @brief 重设信号采集模块工作模式
 */
void OpticalControl_ResetWorkMode(OpticalWorkMode mode)
{
    TRACE_INFO("\n Optical control mode reset %d", (Uint8)mode);

    taskENTER_CRITICAL();

    s_opticalControl.workMode = mode;
    s_opticalControl.setModeEvent = TRUE;  //重设模式

    taskEXIT_CRITICAL();

//    switch(mode)
//    {
//        case ONLY_AD7791:
//            OpticalControl_ADCChangeInit(AD7791);
//            break;
//
//        case ONLY_ADS1146:
//            OpticalControl_ADCChangeInit(ADS1146);
//            break;
//
//        case BOTH_AD7791_ADS1146:
//            OpticalControl_ADCChangeInit(AD7791);
//            break;
//
//        default:
//            break;
//    }
}

/**
 * @brief 光学采集恢复初始化
 */
void OpticalControl_Restore(void)
{
    OpticalControl_SendEventClose();
    OpticalControl_StopAcquirer();
    OpticalLed_TurnOff();
    OpticalXonen_TurnOff();
}

/**
 * @brief 光学采集模式初始化
 */
void OpticalControl_WorkModeRestore(void)
{
    OpticalControl_ResetWorkMode(s_opticalControl.workMode);
}

/**
 * @brief 获取工作模式
 */
OpticalWorkMode OpticalControl_GetWorkMode(void)
{
    return s_opticalControl.workMode;
}

/**
 * @brief 更新AD7791缓冲数据
 */
static void OpticalControl_SetAD7791Data(OpticalSignalAD sampleData)
{
    s_opticalControl.curAD7791Data = sampleData;
    s_opticalControl.isNewAD7791Data = TRUE;
}

/**
 * @brief 读取ADS1146缓冲数据
 */
Bool OpticalControl_GetAD7791Data(OpticalSignalAD *sampleData)
{
    Bool ret = TRUE;

    if (NULL == sampleData)
    {
        ret = FALSE;
    }

    if (TRUE == s_opticalControl.isNewAD7791Data)
    {
        *sampleData = s_opticalControl.curAD7791Data;
        s_opticalControl.isNewAD7791Data = FALSE;
    }
    else
    {
        *sampleData = s_opticalControl.curAD7791Data;
        ret = FALSE;
    }

    return ret;
}

/**
 * @brief 更新ADS1146缓冲数据
 */
static void OpticalControl_SetADS1146Data(OpticalSignalAD sampleData)
{
    s_opticalControl.curADS1146Data = sampleData;
    s_opticalControl.isNewADS1146Data = TRUE;
}

/**
 * @brief 读取ADS1146缓冲数据
 */
Bool OpticalControl_GetADS1146Data(OpticalSignalAD *sampleData)
{
    Bool ret = TRUE;

    if (NULL == sampleData)
    {
        ret = FALSE;
    }

    if (TRUE == s_opticalControl.isNewADS1146Data)
    {
        *sampleData = s_opticalControl.curADS1146Data;
        s_opticalControl.isNewADS1146Data = FALSE;
    }
    else
    {
        *sampleData = s_opticalControl.curADS1146Data;
        ret = FALSE;
    }

    return ret;
}

/**
 * @brief 启动AD采集过程
 * @param type ADCType, ADC类型
 * @param acquireADTime float, 采集时间
 */
Bool OpticalControl_StartAcquirer(Uint8 index, float acquireADTime)
{
    ADCType type = (ADCType)index;
    Bool ret = TRUE;
    OpticalControl *opticalControl = &s_opticalControl;
    if (OPTICALCONTROL_IDLE == opticalControl->status)             // 采集处于空闲
    {
            opticalControl->status = OPTICALCONTROL_BUSY;
            opticalControl->isRequestStop = FALSE;
            opticalControl->acquireADStartTime = xTaskGetTickCount();
            opticalControl->acquireADTime = acquireADTime;

            if(type != opticalControl->currentAcquireType) //切换至目标ADC模式
            {
                OpticalControl_ADCChangeInit(type);
            }

            TRACE_INFO("\nOptical Control start acquirer type = %d, acquireADTime = %f", index, acquireADTime);

            // 清空数据队列
            OpticalSignalAD sampleData;
            while(TRUE == xQueueReceive(opticalControl->ad7791DataQueue, &sampleData, 0));
            while(TRUE == xQueueReceive(opticalControl->ads1146DataQueue, &sampleData, 0));

            // 清空数据队列
            //xQueueReset(opticalControl->ad7791DataQueue);
            //xQueueReset(opticalControl->ads1146DataQueue);

            vTaskResume(opticalControl->opticalCollectHandle);
    }
    else                                    // 处于采集过程
    {
        TRACE_ERROR("\n collect is busy");
        ret = FALSE;
    }
    return ret;
}

/**
 * @brief 停止AD采集过程
 */
Bool OpticalControl_StopAcquirer()
{
    Bool retValue = TRUE;
    if (OPTICALCONTROL_BUSY == s_opticalControl.status)              // 处于采集过程
    {
        OpticalControl_WorkModeRestore();
        s_opticalControl.status = OPTICALCONTROL_IDLE;
        s_opticalControl.isRequestStop = TRUE;
        TRACE_INFO("\n Request stop acquirer.");
    }
    else                                    // 采集处于空闲
    {
        TRACE_ERROR("\n collect is idle");
        retValue =  FALSE;
    }
    return retValue;
}

void OpticalControl_SendEventOpen(void)
{
    s_opticalControl.isSendEvent = TRUE;
}

void OpticalControl_SendEventClose(void)
{
    s_opticalControl.isSendEvent = FALSE;
}

/**
 * @brief 实时获取AD7791数据-本地任务采集时使用
 */
OpticalSignalAD OpticalControl_GetFilterAD7791AD(OpticalControl *opticalControl)
{
    OpticalSignalAD resultAD = { 0 };
    uint64_t sumRefData = 0;
    uint64_t sumMeaData = 0;
    uint16_t count = 0;

    OpticalSignalAD sampleData;
    TickType_t timeout = (TickType_t) (opticalControl->acquireADTime * 1000) + xTaskGetTickCount();

    opticalControl->acquireADStartTime = xTaskGetTickCount();

    // 采集
    while (1)
    {
        do
        {
            System_Delay(2);
            if (TRUE == opticalControl->isRequestStop)
            {
                return resultAD;
            }
        } while (FALSE == xQueueReceive(opticalControl->ad7791DataQueue, &sampleData, 0));

        if ((sumRefData + sampleData.reference) >= 0xFFFFFFFFFFFFFFFF
                || (sumMeaData + sampleData.measure) >= 0xFFFFFFFFFFFFFFFF)
        {

            TRACE_ERROR(
                    "The intermediate variable of the measuring end or the reference end is out of range.");
            break;
        }

        sumRefData += sampleData.reference; //获取Ref端AD值
        sumMeaData += sampleData.measure; //获取Mea端AD值
        count++;
        TRACE_DEBUG("\n ad7791 ref = %d, mea = %d, count = %d", sampleData.reference, sampleData.measure, count);


        if (TRUE == opticalControl->isRequestStop)
        {
            resultAD.reference = 0;
            resultAD.measure = 0;
            return resultAD;
        }

        if (timeout <  (xTaskGetTickCount() + 100))       // 采样超时检查，剩余时间不足以再进行一次采样
        {
            break;
        }
    }

    resultAD.reference = sumRefData / count;
    resultAD.measure = sumMeaData / count;
    TRACE_MARK("\n count %d", count);
    return resultAD;
}

/**
 * @brief 实时获取ADS1146数据-本地任务采集时使用
 */
OpticalSignalAD OpticalControl_GetFilterADS1146AD(OpticalControl *opticalControl)
{
    OpticalSignalAD resultAD = { 0 };

    uint16_t count = 0;
    static Uint16 refData[ADS1146_DATA_MAX_LEN] = {0};               //275
    static Uint16 meaData[ADS1146_DATA_MAX_LEN] = {0};           //220

    memset(refData, 0, sizeof(refData));
    memset(meaData, 0, sizeof(meaData));


    OpticalSignalAD sampleData;
    TickType_t timeout = (TickType_t) (opticalControl->acquireADTime * 1000) + xTaskGetTickCount();

    opticalControl->acquireADStartTime = xTaskGetTickCount();

    // 采集
    while (1)
    {
        do
        {
            System_Delay(2);
            if (TRUE == opticalControl->isRequestStop)
            {
                return resultAD;
            }
        } while (FALSE == xQueueReceive(opticalControl->ads1146DataQueue, &sampleData, 0));

        refData[count] = sampleData.reference;
        meaData[count] = sampleData.measure;
        count++;
        TRACE_DEBUG("\n ads1146 ref %d, mea %d, count = %d", sampleData.reference, sampleData.measure, count);

        if (TRUE == opticalControl->isRequestStop)
        {
            resultAD.reference = 0;
            resultAD.measure = 0;
            return resultAD;
        }

        if (timeout <  (xTaskGetTickCount() + 100))       // 采样超时检查，剩余时间不足以再进行一次采样
        {
            break;
        }
    }
    TRACE_INFO("\n ads1146 data collect count %d", count);

    if ((0 != count) && (FALSE == opticalControl->isRequestStop))
    {
        // 对数据进行滤波处理
        resultAD.reference = OpticalControl_FilterData(refData, count, ADS1146_DATA_FILTER_NUM(count), ADS1146_DATA_FILTER_NUM(count));
        resultAD.measure   = OpticalControl_FilterData(meaData, count, ADS1146_DATA_FILTER_NUM(count), ADS1146_DATA_FILTER_NUM(count));
        TRACE_INFO("\n ads1146 collect data ref: %d  mea: %d", sampleData.reference, sampleData.measure);
    }
    else
    {
        resultAD.reference = 0;
        resultAD.measure   = 0;
        TRACE_INFO("\n ads1146 collect data 0");
    }

    return resultAD;
}

/**
 * @brief 信号采集命令处理任务
 */
static void OpticalControl_ADHandleTask(void *argument)
{
    OpticalSignalAD resultAD = { 0 };
    OpticalControl *opticalControl;
    opticalControl = (OpticalControl *)argument;
    vTaskSuspend(NULL);
    while (1)
    {
        switch (opticalControl->status)
        {
            case OPTICALCONTROL_IDLE:
                if (TRUE == LaiRS485_GetHostStatus() && TRUE == opticalControl->isSendEvent &&  TRUE == opticalControl->isRequestStop)
                {
                    DncpStack_SendEvent(DSCP_EVENT_OAI_AD_ACQUIRED, &opticalControl->result, sizeof(OpticalAcquireResult));
                    DncpStack_ClearBufferedEvent();
                    DncpStack_BufferEvent(DSCP_EVENT_OAI_AD_ACQUIRED, &opticalControl->result, sizeof(OpticalAcquireResult));
                }
                vTaskSuspend(NULL);
                break;
            case OPTICALCONTROL_BUSY:
                if( AD7791 == opticalControl->currentAcquireType)
                {
                    resultAD = OpticalControl_GetFilterAD7791AD(opticalControl);
                    TRACE_INFO("\n ad7791 result: ref %d, mea %d", resultAD.reference, resultAD.measure);
                }
                else if(ADS1146 == opticalControl->currentAcquireType)
                {
                    resultAD = OpticalControl_GetFilterADS1146AD(opticalControl);
                    TRACE_INFO("\n ads1146 result: ref %d, mea %d", resultAD.reference, resultAD.measure);
                }

                TRACE_INFO("\n Stop acquirer acquireADUseTime: %f", (xTaskGetTickCount() - opticalControl->acquireADStartTime)/1000);
                if (TRUE == opticalControl->isSendEvent)
                {
                    AcquiredResult result;
                    if (FALSE == opticalControl->isRequestStop)
                    {
                        result = ACQUIRED_RESULT_FINISHED;
                    }
                    else
                    {
                        result = ACQUIRED_RESULT_STOPPED;
                    }
                    Uint8 data[9] = {0};
                    memcpy(data, &resultAD, sizeof(resultAD));
                    data[8] = result;
                    DncpStack_SendEvent(DSCP_EVENT_OAI_AD_ACQUIRED, (void *)data, sizeof(data));
                    DncpStack_BufferEvent(DSCP_EVENT_OAI_AD_ACQUIRED, (void *)data, sizeof(data));
                }
                opticalControl->isRequestStop = FALSE;
                opticalControl->status = OPTICALCONTROL_IDLE;
                break;
            case OPTICALCONTROL_COLLECT:
                vTaskSuspend(NULL);
                break;
        }
    }
}

/**
 * @brief AD信号采集任务
 */
static void OpticalControl_ADCollectTask(void *argument)
{

    OpticalControl *opticalControl;
    opticalControl = (OpticalControl *)argument;
    // 数据缓冲区
    Uint16 meaDataBuff[SIGNAL_SAMPLE_COUNT] = {0};
    Uint16 refDataBuff[SIGNAL_SAMPLE_COUNT] = {0};

    TickType_t modeSwitchTimeout = xTaskGetTickCount();
    TickType_t modeSwitchBase = xTaskGetTickCount();

   OpticalSignalAD sampleData;
   ///vTaskSuspend(NULL);
    while (1)
    {
        if (TRUE == opticalControl->isOpenTestMode)
        {
            switch(opticalControl->currentAcquireType)
            {
                case AD7791:
                    if(PD_REF_CHANNEL == opticalControl->testChannel)
                    {
                        sampleData.reference = AD7791Collect_GetAD(PD_REF_CHANNEL);
                        sampleData.measure = 0;
                    }
                    else if(PD_MEA_CHANNEL == opticalControl->testChannel)
                    {
                        sampleData.reference = 0;
                        sampleData.measure = AD7791Collect_GetAD(PD_MEA_CHANNEL);
                    }
                    OpticalControl_SetAD7791Data(sampleData);
                    vTaskDelay(2);
                    break;

                case ADS1146:
                    if(PD_REF_CHANNEL == opticalControl->testChannel)
                    {
                        sampleData.reference = ADS1146Collect_GetADWithSync(PD_REF_CHANNEL);
                        sampleData.measure = 0;
                    }
                    else if(PD_MEA_CHANNEL == opticalControl->testChannel)
                    {
                        sampleData.reference = 0;
                        sampleData.measure = ADS1146Collect_GetADWithSync(PD_MEA_CHANNEL);
                    }
                    OpticalControl_SetADS1146Data(sampleData);
                    vTaskDelay(2);
                    break;
            }
        }
        else
        {
            switch(opticalControl->currentAcquireType)
            {
                case AD7791:
                    sampleData.reference = AD7791Collect_GetAD(PD_REF_CHANNEL);
                    sampleData.measure = AD7791Collect_GetAD(PD_MEA_CHANNEL);

                    if (OPTICALCONTROL_BUSY == opticalControl->status && AD7791 == opticalControl->currentAcquireType)
                    {
                        xQueueSend(opticalControl->ad7791DataQueue, &sampleData, 0);
                    }
                    OpticalControl_SetAD7791Data(sampleData);
                    vTaskDelay(2);
                    break;

                case ADS1146:
                    if (TRUE == ADS1146Collect_GetDoubleChannelADWithSync(meaDataBuff, PD_MEA_CHANNEL,
                            refDataBuff, PD_REF_CHANNEL, SIGNAL_SAMPLE_COUNT, SIGNAL_SAMPLE_TIME))
                    {
                        sampleData.measure = OpticalControl_FilterData(meaDataBuff, SIGNAL_SAMPLE_COUNT,
                                DATA_FILTER_NUM(SIGNAL_SAMPLE_COUNT), DATA_FILTER_NUM(SIGNAL_SAMPLE_COUNT));
                        sampleData.reference = OpticalControl_FilterData(refDataBuff, SIGNAL_SAMPLE_COUNT,
                                DATA_FILTER_NUM(SIGNAL_SAMPLE_COUNT), DATA_FILTER_NUM(SIGNAL_SAMPLE_COUNT));
                    }
                    else
                    {
                        sampleData.measure   = 0;
                        sampleData.reference = 0;
                    }

                    if (OPTICALCONTROL_BUSY == opticalControl->status  && ADS1146 == opticalControl->currentAcquireType)
                    {
                        xQueueSend(opticalControl->ads1146DataQueue, &sampleData, 0);
                    }
                    OpticalControl_SetADS1146Data(sampleData);
                    vTaskDelay(2);
                    break;

                default:
                    break;
            }

            if(BOTH_AD7791_ADS1146 == opticalControl->workMode)
            {
                modeSwitchTimeout = xTaskGetTickCount();
                if((modeSwitchTimeout - modeSwitchBase) > MODE_SWITCH_TIMEOUT && OPTICALCONTROL_IDLE == opticalControl->status)
                {
                    modeSwitchBase = modeSwitchTimeout;
                    if(AD7791 == opticalControl->currentAcquireType)
                    {
                        OpticalControl_ADCChangeInit(ADS1146);
                    }
                    else if(ADS1146 == opticalControl->currentAcquireType)
                    {
                        OpticalControl_ADCChangeInit(AD7791);
                    }
                    vTaskDelay(50);
                }
            }

            //由任务重设模式
            if(opticalControl->setModeEvent == TRUE)
            {
                switch(opticalControl->workMode)
                {
                    case ONLY_AD7791:
                        OpticalControl_ADCChangeInit(AD7791);
                        break;

                    case ONLY_ADS1146:
                        OpticalControl_ADCChangeInit(ADS1146);
                        break;

                    case BOTH_AD7791_ADS1146:
                        OpticalControl_ADCChangeInit(AD7791);
                        break;

                    default:
                        break;
                }
                opticalControl->setModeEvent = FALSE;
            }

        }
    }
}

/**
 * @brief 设置信号上报周期
 */
void OpticalControl_SetSignalADNotifyPeriod(float period)
{
    TRACE_INFO("\n OpticalControl ad period:");
    System_PrintfFloat(TRACE_LEVEL_INFO, period, 3);
    TRACE_INFO(" s");
    if (period > 0)
    {
        xTimerChangePeriod(s_opticalControl.signalADNotifyPeriodTimer, (uint32_t)((period * 1000) / portTICK_RATE_MS),  0);
        xTimerStart(s_opticalControl.signalADNotifyPeriodTimer, 0);
    }
    else
    {
        xTimerStop(s_opticalControl.signalADNotifyPeriodTimer, 0);
    }
}

/**
 * @brief 信号周期上报任务
 */
static void OpticalControl_SignalADPeriodHandle(TimerHandle_t argument)
{
    if (TRUE == LaiRS485_GetHostStatus())
    {
        OpticalSignalAD adData[2];
        memset(adData, 0, sizeof(adData));

        if(ONLY_AD7791 == s_opticalControl.workMode)
        {
            OpticalControl_GetAD7791Data(&adData[0]);
        }
        else if(ONLY_ADS1146 == s_opticalControl.workMode)
        {
            OpticalControl_GetADS1146Data(&adData[1]);
        }
        else if(BOTH_AD7791_ADS1146 == s_opticalControl.workMode)
        {
            OpticalControl_GetAD7791Data(&adData[0]);
            OpticalControl_GetADS1146Data(&adData[1]);
        }

        DncpStack_SendEvent(DSCP_EVENT_OAI_SIGNAL_AD_NOTICE, &adData, sizeof(adData));
    }
}

/**
 * @brief 打印调试信息
 */
void OpticalControl_PrintfInfo(void)
{
    OpticalSignalAD adData;
    Uint32 reference = 0;
    Uint32 measure = 0;

    if(AD7791 == s_opticalControl.currentAcquireType)
    {
        if (TRUE == OpticalControl_GetAD7791Data(&adData))
        {
            //reference = adData.reference;
            //measure = adData.measure;

            Printf("\n ad7791 reference %d, ", adData.reference);
            System_PrintfFloat(1, adData.reference * 2.5 / 0xFFFFFF, 3);
            Printf(" V, measure %d, ", adData.measure);
            System_PrintfFloat(1, adData.measure * 2.5 / 0xFFFFFF, 3);
            Printf(" V");

            /*Printf("\n ad7791 reference: %d, ", reference);
            if(adData.reference >= 0x800000) //80000000
            {
                reference -= 0x800000;//去掉符号位
                System_PrintfFloat(1, reference * 2.5 / 8388607 + 2.5, 3);
            }
            else
            {
                reference = 0x800000 - reference;
                System_PrintfFloat(1, 2.5 - reference * 2.5 / 8388607, 3);
            }

            Printf(" V, measure: %d, ", measure);

            if(measure >= 0x800000)
            {
                measure -= 0x800000;//去掉符号位
                System_PrintfFloat(1, measure * 2.5 / 8388607 + 2.5, 3);
            }
            else
            {
                measure = 0x800000 - measure;
                System_PrintfFloat(1, 2.5 - measure * 2.5 / 8388607, 3);
            }
            Printf(" V");*/
        }
    }
    else if(ADS1146 == s_opticalControl.currentAcquireType)
    {
        if (TRUE == OpticalControl_GetADS1146Data(&adData))
        {
            Printf("\n ads1146 reference %d, ", adData.reference);
            System_PrintfFloat(1, adData.reference * 2.5 / 0x7FFF, 3);
            Printf(" V, measure %d, ", adData.measure);
            System_PrintfFloat(1, adData.measure * 2.5 / 0x7FFF, 3);
            Printf(" V");
        }
    }
}

OpticalSignalAD OpticalControl_GetNowSignalAD(void)
{
    OpticalSignalAD ad = {0};
    return ad;
}

/**
 * 对数据进行排序(冒泡法)
 * @param 数据
 * @param 数据个数
 */
static void OpticalControl_BubbleSort(Uint16 *dataBuff, Uint16 count)
{
    Uint16 i    = 0;
    Uint16 j    = 0;
    Uint16 temp = 0;

    for(i = 1; i < count; i++)
    {
        for(j = count - 1; j >= i; j--)
        {
            if(dataBuff[j] < dataBuff[j-1])
            {
                temp = dataBuff[j-1];
                dataBuff[j-1] = dataBuff[j];
                dataBuff[j] = temp;
            }
        }
    }
}

/**
 * @brief 对数据进行滤波处理，去掉最大的和最小的数据
 * @param  要进行滤波处理的数据
 * @param  数据个数
 * @param  去掉高端数据个数
 * @param  去掉低端数据个数
 * @return 滤波之后的数据
 */
Uint16 OpticalControl_FilterData(Uint16 *inputData, Uint16 count, Uint16 filterHigh, Uint16 filterLow)
{
    Uint16 i         = 0;
    uint16_t avgData = 0;
    uint32_t sumData = 0;

    // 冒泡
    OpticalControl_BubbleSort(inputData, count);

    // 过滤
    memcpy(inputData, (inputData + filterLow), (count - filterLow) * sizeof(Uint16));
    count -= (filterHigh + filterLow);

    // 取平均值
    for (i = 0; i < count; i++)
    {
        sumData += inputData[i];
    }

    avgData = sumData / count;

    return avgData;
}

/**
 * @brief LED灯控制
 * @param  0 关闭     1 打开
 */
void OpticalControl_TurnOnLed(Bool status)
{
    Bool needTurnOn = status;

    if (TRUE == needTurnOn)
    {
        OpticalLed_TurnOn();          // 打开LED灯
    }
    else
    {
        OpticalLed_TurnOff();         // 关闭LED灯
    }

    TRACE_INFO("\nLED control = %d ; ", needTurnOn);
}

/**
 * @brief 氙灯控制
 * @param  0 关闭     1 打开
 */
void OpticalControl_TurnOnXonen(Bool status)
{
    Bool needTurnOn = status;

    if (TRUE == needTurnOn)
    {
        OpticalXonen_TurnOn();          // 打开氙灯
    }
    else
    {
        OpticalXonen_TurnOff();         // 关闭氙灯
    }

    TRACE_INFO("\nXonen control = %d ; ", needTurnOn);
}

/**
 * 氙灯定频闪烁
 * @param  0 关闭     1 打开
 */
void OpticalControl_GlitterXonen(Bool status)
{
    Bool needTurnOn = FALSE;

    needTurnOn = status;

    if (TRUE == needTurnOn)
    {
        OpticalXonen_Glitter();          // 氙灯定频闪烁
    }
    else
    {
        OpticalXonen_TurnOff();         // 关闭氙灯
    }

    TRACE_INFO("\nXonen control = %d ; ", needTurnOn);
}


OpticalControlStatus OpticalControl_GetCurrentStatus(void)
{
    return s_opticalControl.status;
}

Bool OpticalControl_EnterCollectStatus(void)
{
    if(s_opticalControl.status != OPTICALCONTROL_BUSY)
    {
        s_opticalControl.status = OPTICALCONTROL_COLLECT;
        return TRUE;
    }
    return FALSE;
}

Bool OpticalControl_EnterIdleStatus(void)
{
    if(s_opticalControl.status != OPTICALCONTROL_BUSY)
    {
        s_opticalControl.status = OPTICALCONTROL_IDLE;
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief 此函数用于测试单独某个通道信号采集，测量端和参考端的采集的相关功能将无效
 * @param isOpenTestMode 是否打开测试功能，TRUE为打开
 * @param channel 需要单独测试的通道
 */
void OpticalControl_CollectTestFun(Bool isOpenTestMode, Uint8 channel)
{
    s_opticalControl.isOpenTestMode = isOpenTestMode;
    s_opticalControl.testChannel = channel;
}

