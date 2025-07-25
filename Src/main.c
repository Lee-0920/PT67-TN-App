/**
 * @file main.c
 * @brief 
 * @details
 *
 * @version 1.0.0
 * @author xingfan
 * @date 2016-4-28
 */



#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "System.h"
#include "console/Console.h"
#include "CmdLine.h"
#include "DncpStack/DncpStack.h"
#include "SolenoidValve/ValveManager.h"
#include "PeristalticPump/PumpManager.h"
#include <OpticalMeter/Meter.h>
#include "DncpStack/DeviceInfo.h"
#include "DeviceIndicatorLED.h"
#include "Watchdog.h"
#include "DncpStack/DeviceStatus.h"
#include "DeviceUpdate/UpdateHandle.h"
#include "OpticalControl/OpticalControl.h"
#include "OpticalControl/LEDController.h"
#include "OpticalControl/StaticADControl.h"
#include "Driver/TempDriver/Relay.h"
#include "Driver/TempDriver/BoxFanDriver.h"
#include "TemperatureControl/ThermostatDeviceManager.h"
#include "TemperatureControl/TempCollecterManager.h"
#include "TemperatureControl/ThermostatManager.h"

int main(void)
{
    System_Init();
    DeviceIndicatorLED_Init();
    Watchdog_Init();
    // 功能模块初始化
    Console_Init();
    CmdLine_Init();

    DncpStack_Init();

    ValveManager_Init();
    PumpManager_Init();
    Meter_Init();

    Relay_Init();
    BoxFanDriver_Init();
    ThermostatDeviceManager_Init();        //恒温设备初始化
    TempCollecterManager_Init();        //温度采集器初始化
    ThermostatManager_Init();                  //恒温器初始化

    LEDController_Init();
    StaticADControl_Init();
    OpticalControl_Init();

    DeviceInfo_Init();
 	UpdateHandle_Init();
    DeviceStatus_ReportResetEvent(DEVICE_RESET_POWER_ON); // 报告复位事件

    vTaskStartScheduler();

    /* Infinite loop */
    while (1)
    {
    }
}
#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

/**
 * @}
 */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
