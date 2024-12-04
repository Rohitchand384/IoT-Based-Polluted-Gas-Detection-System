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

#include "i2c.h"
#include "tmpSensor.h"
#include "dio.h"

uint8 tmpAddresss=0x48; 					/******address = 1001000*********************/

extern bool i2cOnFlag;

void tmpReadReady();

/**************************************************************************
Function to initialize temperature sensor. This checks if i2c is enabled or 
not and accordingly initializes it if required.
**************************************************************************/
void tmpInit()
{
	if (i2cOnFlag==0)		//check if i2c is not enabled, if enabled then do nothing
	{
		i2cInit();
		i2cOnFlag=1;
	}
}


/********************************************************************************
Function to set threshold temperature high and lowin temperature sensor. The 
sensor will generate an interrupt,if temperature goes above Thigh from a lower 
temperature,it generates an inrrupt. Similarly if temperature goes below Tlow
from higher temperature, then it generates an interrupt.

User will be required to read temperature in critical task handler if this 
interrupt is recieved. 

Threshold high and low should be set such that only one of high or low 
temperature is detected.
For eg. if temperature above 60 degree Celcius is to be detected, something in the 
line of
Thigh = 60
Tlow = 59
should be selected.
*********************************************************************************/
void setTmpThreshold(uint8 high, uint8 low)
{
	/********configuring Thermostat mode with 2 consecutive
	  ********faults to generate interrupts*****************/
	
	initDioInterrupt(4,FALLING);
	vAHI_SiMasterWriteSlaveAddr(tmpAddresss,
				 FALSE );
	
	/*********setting the operation to be start and write***/
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                     //start before write
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());
	
	/************writing to pointer register of sensor to
	************* select config register of sensor*******/
	
	vAHI_SiMasterWriteData8(1);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/************setting the config register as mentioned***/
	
	vAHI_SiMasterWriteData8(0x6a);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0xa0);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,            //stopping once configuration is done
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteSlaveAddr(tmpAddresss,
					 FALSE );
		
	/*********setting the operation to be start and write***/
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                     //start before write
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(3);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());		//wait till transmission is complete
	
	/************setting the default high threshold to be tempC
	************ set by user****************/
	
	vAHI_SiMasterWriteData8(high);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());		//wait till transmission is complete
	
	vAHI_SiMasterWriteData8(0);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,            //send stop after write
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteSlaveAddr(tmpAddresss,
					 FALSE );
		
	/*********setting the operation to be start and write***/
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                     //start before write
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());
	
	/************writing to pointer register of sensor to
	************* select Tlow register of sensor*******/
	
	vAHI_SiMasterWriteData8(2);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());		//wait till transmission is complete
	
	/************setting the default low threshold to be tempC
	************ set by user********************************/
	
	vAHI_SiMasterWriteData8(low);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());    //wait till transmission is complete
	
	vAHI_SiMasterWriteData8(0);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,            //send stop after write
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/************writing to pointer register of sensor to
	************* select Temperature register of sensor*******/
	
	vAHI_SiMasterWriteSlaveAddr(tmpAddresss,
					 FALSE );
		
	/*********setting the operation to be start and write***/
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                     //start before write
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,	//stopping after temperature register is selected
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
}


/************************************************************
Read temperature from temperature sensor.
************************************************************/
float readTmpFloat()
{
	uint8 u8High,u8Low;
	float value;
	
	tmpReadReady();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8High=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,	//stopping after temperature register is selected
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	u8Low=u8AHI_SiMasterReadData8();
	
	if ((u8High & 0x80)==0)
	{
		value=0.0625*(u8Low>>4);
		value=value+u8High;
	}
	else
	{
		uint16 temp=u8High;
		temp= (temp<<8)|u8Low;
		temp=~temp;
		temp= temp>>4;
		temp=temp+1;
		value=temp*0.0625*(-1);
	}
	return value;
}


/************************************************************
Read temperature from temperature sensor.
************************************************************/
int8 readTmp()
{
	int8 u8High;

	tmpReadReady();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8High=u8AHI_SiMasterReadData8();
	
	return u8High;
}


/*********************************************************
Internal call. not for users.
*********************************************************/
void tmpReadReady()
{
	vAHI_SiMasterWriteSlaveAddr(tmpAddresss ,
							   TRUE );
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
}
