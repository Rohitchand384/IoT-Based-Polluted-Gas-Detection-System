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


#include <AppHardwareApi.h>

#include "AppQueueApi.h"
#include "dio.h"
#include "config.h"
#include "wifiControl.h"

#include "node.h"
#include "sleep.h"

bool dioIntEn=FALSE;

bool pirSleepEvent=FALSE;

void dioInterruptHandler(uint32 deviceId, uint32 itemBitmap);

extern uint32 bitmap;
extern uint8 wakeSource;
extern bool eventSleepFlag;
extern bool timerSleepFlag;

/***********************************************
Initialize dio
***********************************************/
void dioInit()
{
	vAHI_DioSetDirection(0x00000000,
			 			 0x00000004);
	
	vAHI_DioSetOutput(0x00000000,
					  0x00000004);
}


/**********************************************
 Function to Toggle Dio Pin
 Return Nothing
*********************************************/
void toggleDio (uint8 dioVal)
{
 	uint32 checkDio = u32AHI_DioReadInput ();
	checkDio &= (1<<dioVal);
	if (!checkDio)
		setPin (dioVal);
	else
		clearPin (dioVal);
}

/***********************************************
  Switch on orange led on board
***********************************************/
void ledOn()
{
	setPin(2);
}

/***********************************************
  Switch off orange led on board
***********************************************/
void ledOff()
{
	clearPin(2);
}

/**********************************************
 Sets a pin as input
**********************************************/
void setInput(uint8 dioVal)
{
	vAHI_DioSetDirection(0x00000001<<dioVal,
						 0x00000000);
}


/**********************************************
 Sets a pin as output
**********************************************/
void setoutput(uint8 dioVal)
{
	vAHI_DioSetDirection(0x00000000,
				 		 0x00000000<<dioVal);
}

/***********************************************
Sets a particular dioVal
***********************************************/
void setPin(uint8 dioVal)
{
	vAHI_DioSetDirection(0x00000000,
		 				 0x00000001<<dioVal);

	vAHI_DioSetOutput(0x00000001<<dioVal,
					  0x00000000);
}


/***********************************************
  Clears a particular dioVal
************************************************/
void clearPin(uint8 dioVal)
{
	vAHI_DioSetDirection(0x00000000,
			 			 0x00000001<<dioVal);
	
	vAHI_DioSetOutput(0x00000000,
					  0x00000001<<dioVal);
}

void disablePullUp(uint8 dioVal)
{
	vAHI_DioSetPullup(0x00000000 , 0x00000001<<dioVal );
}

/***********************************************
set specified Digital output
************************************************/
void setDigOut(uint8 doVal)
{
	//vAHI_DoSetDataOut(0x01 << doVal, 0);
	vAHI_DoSetDataOut(0x03, 0);
}

/***********************************************
Clears specified Digital output
************************************************/
void clearDigOut(uint8 doVal)
{
//	vAHI_DoSetDataOut(0, 0x01 << doVal);
	vAHI_DoSetDataOut(0, 0x03);
}

/*******************************************************************************
Configures DIO for waking up from Sleep
*******************************************************************************/
void dioWake(uint8 dioVal, interruptEdge edge)
{
	
	// setting specified DIO as input
	vAHI_DioSetDirection(0x00000001<<dioVal,
						 0x00000000);
	// enable wake interrupt from the specified dio
	vAHI_DioWakeEnable(0x00000001<<dioVal,
					   0x00000000);
					   
	if (edge == RISING)
	{
		vAHI_DioWakeEdge(0x00000001 << dioVal,
					 0x00000000);

	}
	else if (edge == FALLING)
	{
		vAHI_DioWakeEdge(0x00000000,
					 0x00000001<<dioVal);
	}
	eventWakeSleepTime();
    //Register call back function to handle wake interrupt
   
	vAHI_SysCtrlRegisterCallback(dioInterruptHandler);
}

/*******************************************************************************
Initialize the Dio Interrupts on a particular dio pin with expected edge
*******************************************************************************/
void initDioInterrupt(uint8 dioVal, interruptEdge edge)
{
	vAHI_DioSetDirection(0x00000001<<dioVal,
						 0x00000000);
		
	if (edge == RISING)
	{
		vAHI_DioInterruptEdge(0x00000001 << dioVal,
					 0x00000000);

	}
	else if (edge == FALLING)
	{
		vAHI_DioInterruptEdge(0x00000000,
					 0x00000001<<dioVal);
	}
	
	vAHI_DioInterruptEnable(0x00000001<<dioVal,
							0x00000000);
	
	vAHI_SysCtrlRegisterCallback(dioInterruptHandler);
}

/*******************************************************************************
Enable interrupts from PIR sensor
*******************************************************************************/
void enablePir(void)
{
	vAHI_DioSetDirection(0x00000001<<12,
						 0x00000000);
	vAHI_DioSetPullup(0x00000000, 0x00000001<<12);

	vAHI_DioInterruptEdge(0x00000001<<12,
						0x00000000);

	vAHI_DioInterruptEnable(0x00000001<<12,
							0x00000000);

	vAHI_SysCtrlRegisterCallback(dioInterruptHandler);
}

/*******************************************************************************
Disables interrupts from PIR sensor
*******************************************************************************/
void disablePirInterrupt()
{
    vAHI_DioInterruptEnable(0x00000000,
							0x00000001<<12);
}

/*******************************************************************************
Enables interrupts from PIR sensor
*******************************************************************************/
void enablePirInterrupt()
{
    vAHI_DioInterruptEnable(0x00000001<<12,
							0x00000000);
}

/**************************************************************************
Put the device to sleep which goes off when PIR event is detected
**************************************************************************/
void pirWakeSleep()
{
    disablePirInterrupt();
    disablePullUp(12);
    eventSleepSettings(12,RISING);
    enterSleep(TIMERON_RAMON);
}

/**************************************************************************
Deals and Sets dio to accept interrupt from sensors
**************************************************************************/
void dioInterruptHandler(uint32 deviceId, uint32 itemBitmap)
{
	if (timerSleepFlag == TRUE)
	{
		timerSleepFlag = FALSE;
		if (itemBitmap == E_AHI_SYSCTRL_WK0_MASK)
    		{
        		// wake timer 0 wake
        		bitmap = 0;
		        // wake was caused by a wake timer
		        wakeSource = 0;
		        // disable wake timer 0
		    	//vAHI_WakeTimerStop(E_AHI_WAKE_TIMER_0);       
		}
   		else if (itemBitmap == E_AHI_SYSCTRL_WK1_MASK)
        {
        		// wake timer 1 wake
		        bitmap = 1;
		        // wake was caused by a wake timer
		        wakeSource = 0;
		        // disable wake timer 1
		    	//vAHI_WakeTimerStop(E_AHI_WAKE_TIMER_1);
		}
	}
   	else if (eventSleepFlag == TRUE)
	{
		eventSleepFlag = FALSE;
		wakeSource = 1;
		if ((itemBitmap & (0x00000001<<0))==(0x00000001<<0))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<0);		    
		}
		else if ((itemBitmap & (0x00000001<<1))==(0x00000001<<1))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<1);		    
		}
		else if ((itemBitmap & (0x00000001<<2))==(0x00000001<<2))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<2);		    
		}
		else if ((itemBitmap & (0x00000001<<3))==(0x00000001<<3))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<3);		    
		}
		else if ((itemBitmap & (0x00000001<<4))==(0x00000001<<4))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<4);		    
		}
		else if ((itemBitmap & (0x00000001<<5))==(0x00000001<<5))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<5);		    
		}
		else if ((itemBitmap & (0x00000001<<6))==(0x00000001<<6))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<6);		    
		}
		else if ((itemBitmap & (0x00000001<<7))==(0x00000001<<7))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<7);		    
		}
		else if ((itemBitmap & (0x00000001<<8))==(0x00000001<<8))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<8);		    
		}
		else if ((itemBitmap & (0x00000001<<9))==(0x00000001<<9))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<9);		    
		}
		else if ((itemBitmap & (0x00000001<<10))==(0x00000001<<10))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<10);		    
		}
		else if ((itemBitmap & (0x00000001<<11))==(0x00000001<<11))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<11);		    
		}
		else if ((itemBitmap & (0x00000001<<12))==(0x00000001<<12))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<12);		    
		}
		else if ((itemBitmap & (0x00000001<<13))==(0x00000001<<13))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<13);		    
		}
		else if ((itemBitmap & (0x00000001<<14))==(0x00000001<<14))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<14);		    
		}
		else if ((itemBitmap & (0x00000001<<15))==(0x00000001<<15))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<15);		    
		}
		else if ((itemBitmap & (0x00000001<<16))==(0x00000001<<16))
		{
		    // wake source is DIO 0   
		    bitmap = (0x00000001<<16);		    
		}
	}
	else
	{    
	    //set critical queue to read temperature sensor
	    if ((itemBitmap & 0x00000010)==0x00000010)
	    {
		    vAppQApiPostCritInt(USER,TEMP_CRIT_TASK);
	    }
	    else if ((itemBitmap & 0x00000020)==0x00000020)
	    {
			//set critical queue to read light sensor
			vAppQApiPostCritInt(USER,LIGHT_CRIT_TASK);
		}
	    else if ((itemBitmap & 0x00020000)==0x00020000)
	    {
		    //set critical queue to read lower humidity threshold
		    vAppQApiPostCritInt(USER,HUM_LOW_CRIT_TASK);
	    }
	    else if ((itemBitmap & 0x00010000)==0x00010000)
	    {
		    //set critical queue to read higher humidity threshold
		    vAppQApiPostCritInt(USER,HUM_HIGH_CRIT_TASK);
	    }
	    else if ((itemBitmap & 0x00000800)==0x00000800)
	    {
		    //set critical queue to read pressure sensor
		    vAppQApiPostCritInt(USER,PRESSURE_CRIT_TASK);
	    }
	    else if ((itemBitmap & (0x00000001<<12))==(0x00000001<<12))
	    {
		    //set queue to read PIR event
		    vAppQApiPostCritInt(USER,PIR_CRIT_TASK);		
	    }
	}    
}


//function to enable PWM of required frequency and duty cycle
void enablePWM (uint8 pwmId, uint32 pwmFrequency, uint8 pwmDutyPeroid)
{
    uint32 totalTimePer = 0;
    uint32 actualLowTime = 0;
    uint32 maxPWMFreqBand = 0;

    if (pwmFrequency < 10000)
    {
        maxPWMFreqBand = 1000000;

        vAHI_TimerEnable(pwmId ,
                            4,
                            FALSE,
                            FALSE,
                            TRUE);
    }
    else
    {
        if (pwmFrequency > 16000000)
            pwmFrequency = 16000000;

        maxPWMFreqBand = 16000000;

        vAHI_TimerEnable(pwmId ,
                                0,
                                FALSE,
                                FALSE,
                                TRUE);

    }
    totalTimePer = maxPWMFreqBand/pwmFrequency;
    actualLowTime = (totalTimePer/100)*pwmDutyPeroid;

    if (pwmDutyPeroid == 0)
        actualLowTime = 0;
    else if (pwmDutyPeroid >= 100)
        actualLowTime = totalTimePer;

    vAHI_TimerStartRepeat(pwmId,
                      totalTimePer-actualLowTime,
                      totalTimePer);

}

//function of disable PWM
void disablePWM(uint8 timerId)
{
    vAHI_TimerDisable (timerId);
} 


