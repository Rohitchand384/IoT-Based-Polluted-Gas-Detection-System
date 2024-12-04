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


#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include "config.h"
#ifdef ENABLE_PH

#include "i2c.h"
#include "pH.h"
#include "dio.h"
#include "ec.h"
#include "node.h"

uint8 phAddress=0x65; 					

extern bool i2cOnFlag;

#define DEVICE_TYPE_REG 0
#define DEVICE_VERSION_REG 1
#define ADDRESS_LOCK_REG 2
#define ADDRESS_REG 3
#define INTERRUPT_CONTROL_REG 4
#define LED_CONTROL_REG 5
#define HIBERNATE_REG 6
#define NEW_READ_AVAILABLE_REG 7
#define CAL_MSB_REG 8
#define CAL_HIGH_REG 9
#define CAL_LOW_REG 10
#define CAL_LSB_REG 11
#define CAL_REQUEST_REG 12
#define CAL_CONFIRM_REG 13
#define TEMP_COMP_MSB_REG 14
#define TEMP_COMP_HIGH_REG 15
#define TEMP_COMP_LOW_REG 16
#define TEMP_COMP_LSB_REG 17
#define TEMP_CONF_MSB_REG 18
#define TEMP_CONF_HIGH_REG 19
#define TEMP_CONF_LOW_REG 20
#define TEMP_CONF_LSB_REG 21
#define PH_MSB_REG 22
#define PH_HIGH_REG 23
#define PH_LOW_REG 24
#define PH_LSB_REG 25



void phReadReady();

/**************************************************************************
Function to initialize humidity sensor. This checks if i2c is enabled or 
not and accordingly initializes it if required.
**************************************************************************/
void phInit()
{
	if (i2cOnFlag==0)		//check if i2c is not enabled, if enabled then do nothing
		{
			vAHI_SiMasterConfigure(FALSE ,
								   FALSE ,
					               47);
			i2cOnFlag=1;
		}
        /******** enabling interrupt for low output *******/
    
    //initDioInterrupt(4, FALLING);
}

void phPowerOn()
{
	setPin(8);
}

void phPowerOff()
{
	clearPin(8);
}

void phActive()
{
    i2cWrite(phAddress, HIBERNATE_REG);
    i2cWriteAndStop(0x01);    
}

void phHibernate()
{
    i2cWrite(phAddress, HIBERNATE_REG);
    i2cWriteAndStop(0x00);    
}
void phIntEnable()
{
    i2cWrite(phAddress, INTERRUPT_CONTROL_REG);
    i2cWriteAndStop(0x08);	
}

uint8 phAvailable()
{
    uint8 value;
    phWrite(phAddress, 7);
    i2cRequestData(phAddress);
    value = 	i2cReadStop();
    return value;
    
}

void resetPhAvail()
{
    i2cWrite(phAddress, NEW_READ_AVAILABLE_REG);
    i2cWriteAndStop(0x00);	
}

/************************************************************
Read PH from the sensor.
************************************************************/
uint32 readPh()
{
	uint8 msb = 0,high = 0,low = 0,lsb = 0;
	uint8 packet[4];
	//float phVal=0;
	uint32 value = 0;
		
	phReadReady();							// Start device for Read
	
	msb = i2cRead();
	//debug (&msb,1);
	high = i2cRead();
	//debug (&high,1);
	low = i2cRead();
	//debug (&low,1);	
	lsb = i2cReadStop();
	//debug (&lsb,1);
	
	memcpy(packet,&msb,1);
	memcpy(packet+1,&high,1);
	memcpy(packet+2,&low,1);
	memcpy(packet+3,&lsb,1);
	memcpy(&value,packet,4);
	/*********The below formula shall be used at the destination to 
	************convert the value to original PH value***********/
//	phVal = (value * 1.0) / 1000;
	
	return value;
}


/*********************************************************
Internal call. not for users.
*********************************************************/
void phReadReady()
{
	phWrite(phAddress, PH_MSB_REG);
	
	i2cRequestData(phAddress);	
}


void phWrite(uint8 address, uint8 data)
{
	vAHI_SiMasterWriteSlaveAddr(address,
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

	vAHI_SiMasterWriteData8(data);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());

}

#endif    //ENABLE_PH//