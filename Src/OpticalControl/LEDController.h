/*
 * LEDController.h
 *
 *  Created on: 2016年9月2日
 *      Author: LIANG
 */

#ifndef SRC_OPTICALCONTROL_LEDCONTROLLER_H_
#define SRC_OPTICALCONTROL_LEDCONTROLLER_H_

#include "Common/Types.h"

typedef struct
{
    float proportion;
    float integration;
    float differential;
}LEDControllerParam;

void LEDController_Init(void);
void LEDController_Restore(void);
void LEDController_TurnOnLed();
Bool LEDController_Start(void);
void LEDController_Stop(void);
void LEDController_SendEventOpen(void);
Uint32 LEDController_GetTarget(void);
void LEDController_SetTarget(Uint32 target);
LEDControllerParam LEDController_GetParam(void);
void LEDController_SetParam(LEDControllerParam param);
Bool LEDController_AdjustToValue(Uint32 targetAD, Uint32 tolerance, Uint32 timeout);
void LEDController_AdjustStop(void);
void LEDController_SendEventOpen(void);
void LEDController_SendEventClose(void);

#endif /* SRC_OPTICALCONTROL_LEDCONTROLLER_H_ */
