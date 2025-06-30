/*
 * ADS8318Collect.c
 *
 *  Created on: 2020年9月2日
 *      Author: Administrator
 */

#include <OpticalDriver/ADS8318Collect.h>
#include <System.h>
#include "Tracer/Trace.h"

static uint32_t ADS8318Collect_Read16Bits(void);

void ADS8318_TestGetAD(void);
void ADS8318_Test(void);


// CONVST
#define ADS8318_CONVST_PIN           GPIO_Pin_1
#define ADS8318_CONVST_GPIO          GPIOB
#define ADS8318_CONVST_RCC           RCC_AHB1Periph_GPIOB
#define ADS8318_CONVST_HIGH()        GPIO_SetBits(ADS8318_CONVST_GPIO, ADS8318_CONVST_PIN)
#define ADS8318_CONVST_LOW()         GPIO_ResetBits(ADS8318_CONVST_GPIO, ADS8318_CONVST_PIN)

// SCLK
#define ADS8318_SCLK_PIN         GPIO_Pin_13
#define ADS8318_SCLK_PINSRC      GPIO_PinSource13
#define ADS8318_SCLK_GPIO        GPIOB
#define ADS8318_SCLK_HIGH()      GPIO_SetBits(ADS8318_SCLK_GPIO, ADS8318_SCLK_PIN)
#define ADS8318_SCLK_LOW()       GPIO_ResetBits(ADS8318_SCLK_GPIO, ADS8318_SCLK_PIN)

// SDI
#define ADS8318_SDI_PIN          GPIO_Pin_15
#define ADS8318_SDI_GPIO         GPIOB
#define ADS8318_SDI_RCC          RCC_AHB1Periph_GPIOB
#define ADS8318_SDI_HIGH()       GPIO_SetBits(ADS8318_SDI_GPIO, ADS8318_SDI_PIN)
#define ADS8318_SDI_LOW()        GPIO_ResetBits(ADS8318_SDI_GPIO, ADS8318_SDI_PIN)

// SDO
#define ADS8318_SDO_PIN          GPIO_Pin_14
#define ADS8318_SDO_GPIO         GPIOB
#define ADS8318_SDO_RCC          RCC_AHB1Periph_GPIOB
#define ADS8318_SDO_READ()       GPIO_ReadInputDataBit(ADS8318_SDO_GPIO, ADS8318_SDO_PIN)


//*********用于测试 用完之后删除*****************

// 氙灯
#define LED_HIGH()      GPIO_SetBits(GPIOA, GPIO_Pin_7)  //开
#define LED_LOW()       GPIO_ResetBits(GPIOA, GPIO_Pin_7) //关

#define IVC_S1_HIGH()      GPIO_SetBits(GPIOE, GPIO_Pin_12)
#define IVC_S1_LOW()       GPIO_ResetBits(GPIOE, GPIO_Pin_12)

#define IVC_S2_HIGH()      GPIO_SetBits(GPIOE, GPIO_Pin_13)
#define IVC_S2_LOW()       GPIO_ResetBits(GPIOE, GPIO_Pin_13)



/**
 * @brief ADS8318引脚初始化
 * @param
 */
void ADS8318Collect_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    //时钟配置
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    //IO配置
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    // CONVST
    GPIO_InitStructure.GPIO_Pin = ADS8318_CONVST_PIN;
    GPIO_Init(ADS8318_CONVST_GPIO, &GPIO_InitStructure);

    // SDI
    GPIO_InitStructure.GPIO_Pin = ADS8318_SDI_PIN;
    GPIO_Init(ADS8318_SDI_GPIO, &GPIO_InitStructure);

    // SCLK
    GPIO_InitStructure.GPIO_Pin = ADS8318_SCLK_PIN;
    GPIO_Init(ADS8318_SCLK_GPIO, &GPIO_InitStructure);


    // SDO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_Pin = ADS8318_SDO_PIN;
    GPIO_Init(ADS8318_SDO_GPIO, &GPIO_InitStructure);

	ADS8318_SDI_HIGH();
}

/**
 * @brief ADS8318读取十六位数据
 * @param
 */
uint32_t ADS8318Collect_Read16Bits(void)
{

	Printf("333\n");
	uint32_t data = 0;
	uint8_t i;
	// 读数据
	data = 0;

	for (i=0;i<16;i++)
	{
		 ADS8318_SCLK_HIGH();
		 data <<= 1;
		 if (ADS8318_SDO_READ())
		 {
			 data |= 0x01;
		 }
		 ADS8318_SCLK_LOW();
		 for (int t = 0; t < 5; t++);

	}

	data &= 0x7FFF;

	Printf("data=%d\n",data);
	return data;
}


Uint16 ADS8318Collect_GetAD(void)
{
	Uint16 adData;

		ADS8318_CONVST_HIGH();
		System_DelayUs(20);
		ADS8318_CONVST_LOW();
		adData=ADS8318Collect_Read16Bits();
		if (0x8000 >= adData)
		{
			TRACE_ERROR("\n Data error  = %d",adData);
			adData=0;
		}
		Printf("get AD = %d/n",adData);

	return adData;
}

//**************下列函数用于测试*****************
void ADS8318_TestGetAD(void)
{
/*
1、信号不采集时，氙灯关，S1低，S2低 ；
2、采集信号时，同时将S1高、S2高，氙灯开，高的时间最好可调(默认10us)，1us为单位调节；
4、ADC采集结束后，氙灯开，S1低，S2低 ；
3、S1高、S2高设定时间到，ADC采集信号；
*/
//
	uint32_t data = 0;
// 1、信号不采集时，氙灯关，S1低，S2低
	LED_LOW();
	IVC_S1_LOW();
	IVC_S2_LOW();

	System_NonOSDelay(1);
	Printf("\n");
// 2、采集信号时，同时将S1高、S2高，氙灯开，高的时间最好可调(默认10us)，1us为单位调节；
	LED_HIGH();
	IVC_S1_HIGH();
	IVC_S2_HIGH();
	System_DelayUs(10);
	Printf("\n");

	ADS8318_CONVST_HIGH();
	System_DelayUs(20);
	ADS8318_CONVST_LOW();

	data=ADS8318Collect_Read16Bits();

	if (0x8000 >= data)
	{
		TRACE_ERROR("\n Data error  = %d",data);
		data=0;
	}
// 4、ADC采集结束后，氙灯开，S1低，S2低 ；
	IVC_S1_LOW();
	IVC_S2_LOW();
	Printf("4\n");
	System_NonOSDelay(1);

	Printf("get AD = %d/n",data);

}

void ADS8318_Test(void)
{

}

