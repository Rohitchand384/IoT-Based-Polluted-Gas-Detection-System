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
#include "jendefs.h"
#include "i2c.h"
#include "tempPressure.h"
#include "dio.h"
#include "pcInterface.h"
#include "config.h"

#ifdef HTP_ENABLE
uint8 pressureAddress=0x5d; 					/******address = 01011101b*********************/


extern bool i2cOnFlag;
extern bool humidOnFlag;


bool pressureOnFlag=0;
void pressureReadReady();

#ifdef HTP_V1
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define THS_P_L 0x30
#define THS_P_H 0x31
#define INTERRUPT_CFG 0x24

#elif defined HTP_V2
#define CTRL_REG1 0x10
#define CTRL_REG2 0x11
#define CTRL_REG3 0x12
#define THS_P_L 0x0C
#define THS_P_H 0x0D
#define RPDS_L 0x18
#define RPDS_H 0x19
#define INTERRUPT_CFG 0x0B
#endif

#define STATUS_REG 0x27
#define PRESS_OUT_XL 0x28
#define PRESS_OUT_L 0x29
#define PRESS_OUT_H 0x2A
#define TEMP_OUT_L 0x2B
#define TEMP_OUT_H 0x2C

bool intFlag=0;	//flag to check if interrupt request from sensor is on or off

extern uint8 hdcAddress;
/**************************************************************************
Function to initialize pressure sensor. This checks if i2c is enabled or 
not and accordingly initializes it if required.
**************************************************************************/
void pressureInit()
{

	#ifdef HTP_V1
		setPin(1);
	#elif defined HTP_V2
		clearPin(1);
	#endif
	if (humidOnFlag==0)
	{
		#ifdef HTP_V1
		setPin(1);
		#elif defined HTP_V2
		clearPin(1);
		#endif
	}
	pressureOnFlag=1;
	if (i2cOnFlag==0)		//check if i2c is not enabled, if enabled then do nothing
	{
		i2cInit();
		i2cOnFlag=1;
	}
	/******by default the device is off, so turning it on*********/
}

#ifdef HTP_V1
/************************************************************
Read pressure and temperature from sensor.
************************************************************/
void readTempPressure(uint32* pressure, uint16* temperature)
{
	uint8 pressureMsb;
	uint8 pressureLsb;
	uint8 pressureXlsb;

	uint8 measureCheck;

	uint8 temperatureMsb;
	uint8 temperatureLsb;

	i2cWrite(pressureAddress, CTRL_REG1);	//selecting CTRL_REG1 to turn device on
	i2cWriteAndStop(0x84);					//0x84 turns on the device and blocks the update of data until it is read

	i2cWrite(pressureAddress, CTRL_REG2);	//setting the one shot mode in CTRL_REG2
	i2cWriteAndStop(0x01);
	
	do
	{
		i2cWrite(pressureAddress, CTRL_REG2);
		i2cRequestData(pressureAddress);
		measureCheck=i2cReadStop();
	}while (measureCheck!=0);	//keep checking till the data is ready to be read


	
	i2cWrite(pressureAddress, PRESS_OUT_XL | (1<<7)); //selecting the LSB with incrementing option for automatic increment of address

	i2cRequestData(pressureAddress);	//send the read command
	
	pressureXlsb=i2cRead(); //read 1st byte
	
	pressureLsb=i2cRead();	//Read the middle byte of pressure

	pressureMsb=i2cReadStop();	//Read the highest byte of pressure
	
	*pressure=0x00000000 | ((0x00000000|pressureMsb)<<16) | ((0x00000000|pressureLsb)<<8) | pressureXlsb; //| (pressureLsb<<8) | (pressureXlsb);

	i2cWrite(pressureAddress, TEMP_OUT_L | 1<<7 ); //| (1<<7)selecting the LSB with incrementing option for automatic increment of address
	i2cRequestData(pressureAddress);	//send the read command
	temperatureLsb=i2cRead(); //read 1st byte
	temperatureMsb=i2cReadStop();	//Read the last byte of temperature

	*temperature=((0x0000|temperatureMsb)<<8) | temperatureLsb;

	i2cWrite(pressureAddress, CTRL_REG1);	//selecting Control Register 1 and turning it off for fresh reading the next time
	i2cWriteAndStop(0x00);				//0x00 turns thhe device off

}

#elif defined HTP_V2
/************************************************************
Read pressure from sensor hdc2080.
************************************************************/
void readPressure(uint32* pressure)
{
	uint8 pressureMsb;
	uint8 pressureLsb;
	uint8 pressureXlsb;

	uint8 measureCheck;

	i2cWrite(pressureAddress, CTRL_REG1);	//selecting CTRL_REG1 to turn device on
	
	i2cWriteAndStop(0x02);					//0x84 turns on the device and blocks the update of data until it is read
	i2cWrite(pressureAddress, RPDS_L);
	i2cWriteAndStop(0x00);
	i2cWrite(pressureAddress, RPDS_H);
	i2cWriteAndStop(0x00);

	i2cWrite(pressureAddress, CTRL_REG2);	//setting the one shot mode in CTRL_REG2
	i2cWriteAndStop(0x01);
	
	do
	{
		i2cWrite(pressureAddress, STATUS_REG);
		i2cRequestData(pressureAddress);
		msdelay(20);	
		measureCheck=i2cReadStop();
		//debug(&measureCheck,1);
	}while (measureCheck ==0);	//keep checking till the data is ready to be read
	
	i2cWrite(pressureAddress, PRESS_OUT_XL); //selecting the LSB with incrementing option for automatic increment of address

	i2cRequestData(pressureAddress);	//send the read command
	
	pressureXlsb=i2cReadStop(); //read 1st byte
	i2cWrite(pressureAddress, PRESS_OUT_L);
	i2cRequestData(pressureAddress);	//send the read command
	pressureLsb=i2cReadStop();	//Read the middle byte of pressure

	i2cWrite(pressureAddress, PRESS_OUT_H);
	i2cRequestData(pressureAddress);	//send the read command
	pressureMsb=i2cReadStop();	//Read the highest byte of pressure
	
	*pressure= 0x00000000 | ((0x00000000|pressureMsb)<<16) | ((0x00000000|pressureLsb)<<8) | pressureXlsb; //| (pressureLsb<<8) | (pressureXlsb);

	i2cWrite(pressureAddress, CTRL_REG1);	//selecting Control Register 1 and turning it off for fresh reading the next time
	i2cWriteAndStop(0x00);				//0x00 turns thhe device off

}

float readTemp()
{
	uint16 tempData;
	float tempValue;
	i2cWrite(hdcAddress,0);
	i2cRequestData(hdcAddress);
	tempData = i2cRead();
	tempData |= (i2cReadStop())<<8;
	tempValue= ((((float)tempData)*165)/65536)-40;

	return tempValue;
}

#endif

void tempPressureDisable(void)
{
	i2cWrite(pressureAddress, CTRL_REG1);	//selecting Control Register 1 and turning it off for fresh reading the next time
	i2cWriteAndStop(0x00);				//0x00 turns thhe device off
}


/***Set pressure Threshold************
 *
 */

void setPressureThreshold(uint16 pressureInHpa)
{

	uint8 temp=0;
	initDioInterrupt(11,FALLING);
	uint16 pressureVal;
	uint8 pressureHigh;
	uint8 pressureLow;
	pressureVal=pressureInHpa*16;
	pressureLow=pressureVal;
	pressureHigh=pressureVal>>8;


//	i2cWrite(pressureAddress, CTRL_REG1);	//selecting CTRL_REG1 to turn device on
	//i2cWriteAndStop(0x00);					//0x84 turns on the device and blocks the update of data until it is read


	i2cWrite(pressureAddress, THS_P_L | (1<<7) );		//burst mode write starting from Low Pressure Threshold
	i2cWriteContinue(pressureLow);	//write to low threshold register = pressureLow
	i2cWriteAndStop(pressureHigh);	//write to high threshold register = pressureHigh
	//i2cWrite(pressureAddress, THS_P_L);

	i2cWrite(pressureAddress, CTRL_REG3);
	i2cRequestData(pressureAddress);	//send the read command
	temp=i2cReadStop(); //read 1st byte

//	debug(&temp,1);
	i2cWrite(pressureAddress, CTRL_REG3);
	i2cWriteAndStop((temp & 0x3c) | 0x81);	//send 0x83 in ctrl reg 3 @ 22h, which enables active low interrupt in push pull mode, on both high or low threshold cross

	temp=0;
/*
	i2cWrite(pressureAddress, CTRL_REG3);
			i2cRequestData(pressureAddress);	//send the read command
			temp=i2cReadStop(); //read 1st byte
			debug(&temp,1);
*/


	i2cWrite(pressureAddress, INTERRUPT_CFG);
	i2cRequestData(pressureAddress);	//send the read command
	temp=i2cReadStop(); //read 1st byte
	i2cWrite(pressureAddress, INTERRUPT_CFG);
	i2cWriteAndStop((temp & 0xF8) | 0x01);	//send 0x03 to INTERRUPT_CFG @ 24h, enables register to generate interrupt on both high and low pressure
	//interrupt not latched in INT_SOURCE register, to do the same send 7 in the above register


	intFlag=1;

//	i2cWrite(pressureAddress, CTRL_REG1);	//selecting CTRL_REG1 to turn device on
//	i2cWriteAndStop(0x00);					//0x8C turns on the device,enables interrupts and blocks the update of data until it is read


}

/*********************************************************
Internal call. not for users.
*********************************************************/
void pressureReadReady()
{
	i2cRequestData(pressureAddress);
	
}
#endif
