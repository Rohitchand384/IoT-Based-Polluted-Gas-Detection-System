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
#include <AppQueueApi.h>
#include <jendefs.h>
	
#include "config.h"
#ifdef ENABLE_EC
#include "i2c.h"
#include "dio.h"
#include "node.h"
#include "string.h"
#include "pcInterface.h"

uint8 ecAddress=0x64;

extern bool i2cOnFlag;

#define DEVICE_TYPE_REG 0
#define DEVICE_VERSION_REG 1
#define ADDRESS_LOCK_REG 2
#define ADDRESS_REG 3
#define INTERRUPT_CONTROL_REG 4
#define LED_CONTROL_REG 5
#define HIBERNATE_REG 6
#define NEW_READ_AVAILABLE_REG 7
#define PROBE_TYPE_MSB 8
#define PROBE_TYPE_LSB 9
#define CAL_MSB_REG 10
#define CAL_HIGH_REG 11
#define CAL_LOW_REG 12
#define CAL_LSB_REG 13
#define CAL_REQUEST_REG 14
#define CAL_CONFIRM_REG 15
#define TEMP_COMP_MSB_REG 16
#define TEMP_COMP_HIGH_REG 17
#define TEMP_COMP_LOW_REG 18
#define TEMP_COMP_LSB_REG 19
#define TEMP_CONF_MSB_REG 20
#define TEMP_CONF_HIGH_REG 21
#define TEMP_CONF_LOW_REG 22
#define TEMP_CONF_LSB_REG 23
#define EC_MSB_REG 24
#define EC_HIGH_REG 25
#define EC_LOW_REG 26
#define EC_LSB_REG 27
#define TDS_MSB_REG 28
#define TDS_HIGH_REG 29
#define TDS_LOW_REG 30
#define TDS_LSB_REG 31
#define PSS_MSB_REG 32
#define PSS_HIGH_REG 33
#define PSS_LOW_REG 34
#define PSS_LSB_REG 35


// internal functions

void ecReadReady();
void tdsReadReady();
void pssReadReady();
void ecWrite(uint8 address, uint8 data);
void probeReadReady();
/**************************************************************************
Function to initialize humidity sensor. This checks if i2c is enabled or 
not and accordingly initializes it if required.
**************************************************************************/
void ecInit()
{
	if (i2cOnFlag==0)		//check if i2c is not enabled, if enabled then do nothing
		{
			vAHI_SiMasterConfigure(FALSE ,
								   FALSE ,
					               31);
			i2cOnFlag=1;
		}
}

void ecPowerOn()
{
	setPin(10);
}

void ecPowerOff()
{
	clearPin(10);
}

void setEcProbe(float probeConstant)
{
	uint16 pc = (uint16)(probeConstant * 100);
	uint8 msb = pc >> 8;
	uint8 lsb = pc;
  	i2cWrite(ecAddress, PROBE_TYPE_MSB);
  	i2cWriteContinue(msb);
  	i2cWriteAndStop(lsb);
}

uint8 ecRead()
{
	uint8 i2cData;
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
							E_AHI_SI_STOP_BIT,
							E_AHI_SI_SLAVE_READ,
							E_AHI_SI_NO_SLAVE_WRITE,
							E_AHI_SI_SEND_NACK,
							E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());

	i2cData=u8AHI_SiMasterReadData8();

	return i2cData;
}
void ecActive()
{
    i2cWrite(ecAddress, HIBERNATE_REG);
    i2cWriteAndStop(0x01);
}
void ecLightOff()
{
    i2cWrite(ecAddress, LED_CONTROL_REG);
    i2cWriteAndStop(0x00);
}
void ecHibernate()
{
    i2cWrite(ecAddress, HIBERNATE_REG);
    i2cWriteAndStop(0x00);    
}
void ecIntEnable()
{
    i2cWrite(ecAddress, INTERRUPT_CONTROL_REG);
    i2cWriteAndStop(0x08);
}

uint8 ecAvailable()
{
    uint8 value;
    ecWrite(ecAddress, NEW_READ_AVAILABLE_REG);
    i2cRequestData(ecAddress);
    value = i2cReadStop();
    return value;
}

void resetEcAvail()
{
    i2cWrite(ecAddress, NEW_READ_AVAILABLE_REG);
    i2cWriteAndStop(0x00);	
}

/************************************************************
Read PH from the sensor.
************************************************************/
uint32 readEc()
{
	uint8 msb = 0,high = 0,low = 0,lsb = 0;
	uint8 packet[4];
	//float phVal=0;
	uint32 value = 0;
		
	ecReadReady();							// Start device for Read
	// msdelay(100);
	msb = i2cRead();
	//	    uartSend(E_AHI_UART_0,&msb,1);

	//debug (&msb,1);
	high = i2cRead();
	//	    uartSend(E_AHI_UART_0,&high,1);

	//debug (&high,1);
	low = i2cRead();
	//	    uartSend(E_AHI_UART_0,&low,1);

	//debug (&low,1);	
	lsb = i2cReadStop();
	//	    uartSend(E_AHI_UART_0,&lsb,1);
	//    uartSend(E_AHI_UART_0,"\r\n",2);

	//debug (&lsb,1);
	

	 memcpy(packet,&msb,1);
	 memcpy(packet+1,&high,1);
	 memcpy(packet+2,&low,1);
	 memcpy(packet+3,&lsb,1);
	 memcpy(&value,packet,4);
	/*********The below formula shall be used at the destination to 
	************convert the value to original PH value***********/
	return value;
}

uint32 readTds()
{
	uint8 msb = 0,high = 0,low = 0,lsb = 0;
	uint8 packet[4];
	//float phVal=0;
	uint32 value = 0;
		
	tdsReadReady();							// Start device for Read
	
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
	
	return value;
}

/*********************************************************
Internal call. not for users.
*********************************************************/
void ecReadReady()
{
	ecWrite(ecAddress, EC_MSB_REG);
	
	i2cRequestData(ecAddress);	
}

void tdsReadReady()
{
	ecWrite(ecAddress, TDS_MSB_REG);
	
	i2cRequestData(ecAddress);	
}

void probeReadReady()
{
    ecWrite(ecAddress, PROBE_TYPE_MSB);
	i2cRequestData(ecAddress);	

}


void ecWrite(uint8 address, uint8 data)
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

#endif    /*ENABLE_EC*/