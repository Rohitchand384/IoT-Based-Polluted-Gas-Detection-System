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
#include "lightSensor.h"
#include "dio.h"

uint8 lightAddress=0x44;
extern bool i2cOnFlag;

/**Name:     lightSensorInit
   Purpose:  enables the light sensor with default values which***
   **************are yet to be defined***************************/
void lightSensorInit()
{
	if (i2cOnFlag==0)		//check if i2c is not enabled, if enabled then do nothing
	{
		i2cInit();
		i2cOnFlag=1;
	}
	
	/***Enabling continuous ALS mode, with persistence level of 4*****
	***********sending 10100001 i.e. 0xA1 to command register 1******/
	
	vAHI_SiMasterWriteSlaveAddr(lightAddress ,
				 FALSE );	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                     //start before write
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/*selecting command register 1 for write operation***************/
	
	vAHI_SiMasterWriteData8(0);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/***** Writing the data, i.e. 0xA1 to command register***********/
	/**interrupt generated when there are 4 consecutive faults*******/
	/**measures Ambient Light Intensity continuously*****************/
	
	vAHI_SiMasterWriteData8(0xA1);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/*** Writing to Command 2 register for range to be 64k lux, and **
	*********16 bit resolution hence sending 00000011 i.e 0x03*******/
	
	vAHI_SiMasterWriteData8(0x03);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/****generating stop condition*********/
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteSlaveAddr(lightAddress ,
							 FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                     //start before write
					E_AHI_SI_NO_STOP_BIT,
					E_AHI_SI_NO_SLAVE_READ,
					E_AHI_SI_SLAVE_WRITE,
					E_AHI_SI_SEND_ACK,
					E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(4);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
					E_AHI_SI_NO_STOP_BIT,
					E_AHI_SI_NO_SLAVE_READ,
					E_AHI_SI_SLAVE_WRITE,
					E_AHI_SI_SEND_ACK,
					E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	///////////setting MSB of low threshold in burst mode operation//////
	vAHI_SiMasterWriteData8(0);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/////////setting LSB of high threshold in burst mode operation/////////
	
	vAHI_SiMasterWriteData8(0xff);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0xff);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
}


/*Name:   setLightThreshold()
* Purpose: setting the higher and lower luminiscence threshold
* Arguments: uint16 high and low luminescence value
* Returns: None
*/
void setLightThreshold(uint16 high, uint16 low)
{
	uint16 hValue,lValue;
	hValue=(high*1.024);			//1.024 is the value given in data sheet of light sensor
	lValue=(low*1.024);
	
	//initialize interrupt at dio5
	initDioInterrupt(5,FALLING);

	//////////setting low threshold////////////////////////
		
	vAHI_SiMasterWriteSlaveAddr(lightAddress ,
							 FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                     //start before write
					E_AHI_SI_NO_STOP_BIT,
					E_AHI_SI_NO_SLAVE_READ,
					E_AHI_SI_SLAVE_WRITE,
					E_AHI_SI_SEND_ACK,
					E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0x04);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
					E_AHI_SI_NO_STOP_BIT,
					E_AHI_SI_NO_SLAVE_READ,
					E_AHI_SI_SLAVE_WRITE,
					E_AHI_SI_SEND_ACK,
					E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	///////////setting LSB of low threshold in burst mode operation//////
	vAHI_SiMasterWriteData8(lValue);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	///////////setting MSB of low threshold in burst mode operation//////
	vAHI_SiMasterWriteData8(lValue>>8);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/////////setting LSB of high threshold in burst mode operation/////////
	
	vAHI_SiMasterWriteData8(hValue);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	///////////setting MSB of high threshold in burst mode operation//////
	vAHI_SiMasterWriteData8(hValue>>8);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
 }


/** Name: readLux*******************************
 ** Purpose: Read the lux value from light sensor********/
uint16 readLux(void)
{
	uint8 u8High=0,u8Low=0;
	uint16 temp=0;	
	
	/**********Reading Low Byte of LUX***********************/
	vAHI_SiMasterWriteSlaveAddr(lightAddress ,
							  FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0x02);                 //setting lower register for lux
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	
	
	vAHI_SiMasterWriteSlaveAddr(lightAddress ,
							  TRUE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());

	///////////burst read, so no stop bit is sent after lower byte is read///////
	
	/*this command may not be required, testing needed*******/
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8Low=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8High=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	temp|=u8High;
	temp=temp<<8;
	temp|=u8Low;

	//temp= temp/1.024;		//though this line gives precise result but it also increases code size by 8 kb.
							//more testing needed
	return (temp);
}


//clear light interrupt after reading the light info when an aert by ic is generated
void clearLightInterrupt()
{
	// uint8 readConfigReg;
	
	vAHI_SiMasterWriteSlaveAddr(lightAddress ,
							 FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0x00);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	
	vAHI_SiMasterWriteSlaveAddr(lightAddress ,
							 TRUE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	/*this command may not be required, testing needed*******/
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	//readConfigReg=
	u8AHI_SiMasterReadData8();
}




