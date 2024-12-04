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
#include <jendefs.h>
#include "i2c.h"
#include "RGBSensor.h"
#include "dio.h"
#include "config.h"

#ifdef RGB_ENABLE


uint8 RgbAddress=0x29;
extern bool i2cOnFlag;

/**Name:     RGBSensorInit
   Purpose:  enables the rgb sensor with default values which***
   **************are yet to be defined***************************/
void RGBSensorInit()
{
	if (i2cOnFlag==0)		//check if i2c is not enabled, if enabled then do nothing
	{
		i2cInit();
		i2cOnFlag=1;
	}
}



void setRGBThreshold(uint16 high, uint16 low)
{
	uint16 hValue,lValue;
	hValue=(high*1.024);			
	lValue=(low*1.024);
	
	//initialize interrupt at dio5
	initDioInterrupt(5,FALLING);

	//////////setting low threshold////////////////////////
		
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							 FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                    
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


uint8* readRGBvalue()
{
	uint16 clear = 0;
	uint16 red = 0;
	uint16 green = 0;
	uint16 blue = 0;
	static uint8 RGBdata[10];
	
	clear = readClear();
	red = readRed();
	blue = readBlue();
	green = readGreen();
	memcpy(&RRGBdata[0],&clear,2);
	memcpy(&RGBdata[2],&red,2);
	memcpy(&RGBdata[4],&green,2);
	memcpy(&RGBdata[6],&blue,2);
	return RGBdata;
}

uint16 readClear()
{
	uint8 u8cHigh=0,u8cLow=0;
	uint16 tempc=0;	
	
	/**********Reading Low Byte of clear***********************/
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
											
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0x14);                 //setting lower register for clear
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	
	
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  TRUE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8cLow=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8cHigh=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	tempc|=u8cHigh;
	tempc=tempc<<8;
	tempc|=u8cLow;
	return (tempc);
}


uint16 readRed()
{
	uint8 u8rHigh=0,u8rLow=0;
	uint16 tempr=0;
	
	/**********Reading Low Byte of red***********************/
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0x16);                 //setting lower register for red
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	
	
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  TRUE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8rLow=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8rHigh=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	tempr|=u8rHigh;
	tempr=tempr<<8;
	tempr|=u8rLow;
	return (tempr);
}	
	
	
uint16 readGreen()
{
	uint8 u8gHigh=0,u8gLow=0;
	uint16 tempg=0;
	/**********Reading Low Byte of green***********************/
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	while (bAHI_SiMasterPollTransferInProgress());					
	vAHI_SiMasterWriteData8(0x18);                 //setting lower register for blue
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	
	
	vAHI_SiMasterWriteSlaveAddr(RgblightAddress ,
							  TRUE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());

	
	u8gLow=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8gHigh=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	tempg|=u8gHigh;
	tempg=temp<<8;
	tempg|=u8gLow;
	return (tempg);
}
	
	
uint16 readBlue()
{
	uint8 u8bHigh=0,u8bLow=0;
	uint16 tempb=0;
	
	/**********Reading Low Byte of clear***********************/
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							  FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
						
	while (bAHI_SiMasterPollTransferInProgress());
						
	vAHI_SiMasterWriteData8(0x1A);                 //setting lower register for blue
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	
	
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
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
	
	u8bLow=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	u8bHigh=u8AHI_SiMasterReadData8();
	
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_SLAVE_READ,
						E_AHI_SI_NO_SLAVE_WRITE,
						E_AHI_SI_SEND_NACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	tempb|=u8bHigh;
	tempb=tempb<<8;
	tempb|=u8bLow;

	
	return (tempb);}



//clear rgb interrupt after reading the rgb info when an aert by ic is generated
void clearRGBInterrupt()
{
	// uint8 readConfigReg;
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							 FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
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
	
	
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
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
	u8AHI_SiMasterReadData8();
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
							 FALSE );
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	vAHI_SiMasterWriteData8(0x06);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);
	
	while (bAHI_SiMasterPollTransferInProgress());
	
	
	vAHI_SiMasterWriteSlaveAddr(RgbAddress ,
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
	u8AHI_SiMasterReadData8();
}

#endif
