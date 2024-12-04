/*All rights reserved.
 *
 * Eigen Technologies Pvt. Ltd.
 * New Delhi
 * www.eigen.in
 *
 * This source code is provided to SensenutsTM  users for
 * research purpose only. No portion of this source code
 * may be used as the basis of development of any similar
 * kind of product.
 *
 */
 
#include "config.h"
#include <AppHardwareApi.h>
#include <jendefs.h>
#include "AppApi.h"
#include "node.h"
#include "config.h"
#include "AppQueueApi.h"
#include "task.h"
#include "dio.h"
#include "sleep.h"

extern uint8 curPQSize;
extern int8 readPtr;
extern AppQApiTaskInd_s getPTask();
extern AppQApiTaskInd_s passiveTaskQueue[APP_MAX_TEMP_TASK_IND];
extern Time currentTime;
extern bool pirSleepEvent;


bool eventSleepFlag = FALSE;
bool timerSleepFlag = FALSE;
extern void dioInterruptHandler(uint32 deviceId, uint32 itemBitmap);


// Settings for wake timer based on sleep type from user
float N;
void wakeTimerSettings(uint64 timeDuration)
{
	//Enable wake timer interrupts
	vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_1 ,
			     						TRUE );
	uint32 timerCalVal = u32AHI_WakeTimerCalibrate();
	N=(10000.0/(float)timerCalVal);
		
	
	uint64 clocks = timeDuration*32*N;
	//memcpy (&devicePramInfo.timeCountForSleep, &clocks, 8);
	// Start the wake timer
	vAHI_WakeTimerStartLarge(E_AHI_WAKE_TIMER_1,
								clocks);					
	//Register call back function to handle wake interrupt
    vAHI_SysCtrlRegisterCallback(dioInterruptHandler);	
}

void eventWakeSleepTime()
{	
	vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0 ,
			     			FALSE );
			     
	// Clear the previous status
	u8AHI_WakeTimerFiredStatus();
	
	// Start the wake timer
	vAHI_WakeTimerStartLarge(E_AHI_WAKE_TIMER_0,
								0x1FFFFFFFFFF);	
}

extern uint64 timeQ;
void timerSleepSettings(uint64 sleepTime)
{
	Time postSleepTime;
	wakeTimerSettings(sleepTime);
	
	// clock adjustment for sleep period
	//addTimeInCurrent(&postSleepTime,sleepTime*1000);
	addTimeInCurrent(&postSleepTime,sleepTime);
	timeQ += sleepTime; 
	currentTime.clockLow = postSleepTime.clockLow;
	currentTime.clockHigh = postSleepTime.clockHigh;
	timerSleepFlag = TRUE;
}

// sets the DIO pin edge of the signal
void eventSleepSettings(uint8 dioVal, interruptEdge edge)
{
	if (dioVal == 12)
    {
        pirSleepEvent = TRUE;
    }
	dioWake(dioVal, edge);	
	eventSleepFlag = TRUE;
}


// Put device to sleep
void enterSleep(sleep_mode type)
{
    vAppApiSaveMacSettings();
	switch (type)
	{
		case TIMEROFF_RAMON:
		{			
			vAHI_Sleep(E_AHI_SLEEP_OSCOFF_RAMON);
			break;
		}
		case TIMERON_RAMON:	
		{	
			vAHI_Sleep(E_AHI_SLEEP_OSCON_RAMON);
			break;
		}
		case TIMEROFF_RAMOFF:
		{
			vAHI_Sleep(E_AHI_SLEEP_OSCOFF_RAMOFF);
			break;
		}
		case TIMERON_RAMOFF:
		{
			vAHI_Sleep(E_AHI_SLEEP_OSCON_RAMOFF);
			break;
		}
		case DEEP_SLEEP:		
		{	
			vAHI_Sleep(E_AHI_SLEEP_DEEP);
			break;
		}
	}		
}


//Receiver on command for after Sleep
void receiverOnCommand (uint32 recOnTimeInUs)
{
	////////////////////////////////////////////////////////////////
	#define RX_ON_TIME 0x00
	#define RX_ON_DURATION 0x200000
	/* Structures used to hold data for MLME request and response */
	MAC_MlmeReqRsp_s sMlmeReqRsp;
	MAC_MlmeSyncCfm_s sMlmeSyncCfm;
	/* Post receiver enable request */
	sMlmeReqRsp.u8Type = MAC_MLME_REQ_RX_ENABLE;
	sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqRxEnable_s);
	sMlmeReqRsp.uParam.sReqRxEnable.u8DeferPermit = FALSE;
	sMlmeReqRsp.uParam.sReqRxEnable.u32RxOnTime = RX_ON_TIME;
	sMlmeReqRsp.uParam.sReqRxEnable.u32RxOnDuration = RX_ON_DURATION;
	vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
	/* Handle response */

	//debug(&sMlmeSyncCfm.u8Status,1);
	if (sMlmeSyncCfm.u8Status != MAC_ENUM_SUCCESS)
	{
	/* Receiver not enabled */
	}
	////////////////////////////////////////////////////////////////
}


//Receiver off command for after Sleep
void receiverOffCommand (uint32 recOnTimeInUs)
{
	////////////////////////////////////////////////////////////////
	#define RX_ON_TIME 0x00
	#define RX_ON_DURATION 0x200000
	/* Structures used to hold data for MLME request and response */
	MAC_MlmeReqRsp_s sMlmeReqRsp;
	MAC_MlmeSyncCfm_s sMlmeSyncCfm;
	/* Post receiver enable request */
	sMlmeReqRsp.u8Type = MAC_MLME_REQ_RX_ENABLE;
	sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqRxEnable_s);
	sMlmeReqRsp.uParam.sReqRxEnable.u8DeferPermit = TRUE;
	sMlmeReqRsp.uParam.sReqRxEnable.u32RxOnTime = 0;
	sMlmeReqRsp.uParam.sReqRxEnable.u32RxOnDuration = 0;
	vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
	/* Handle response */

	//debug(&sMlmeSyncCfm.u8Status,1);
	if (sMlmeSyncCfm.u8Status != MAC_ENUM_SUCCESS)
	{
		/* Receiver not enabled */
		//debug ("888",3);
	}
	////////////////////////////////////////////////////////////////
}


