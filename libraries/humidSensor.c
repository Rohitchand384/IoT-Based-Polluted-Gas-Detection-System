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

#include "AppHardwareApi.h"
#include "jendefs.h"
#include "i2c.h"
#include "humidSensor.h"
#include "dio.h"
#include "math.h"
#include "pcInterface.h"
#include "stdlib.h"
#include "config.h"

#ifdef HTP_ENABLE

uint8 hdcAddress =0x40;
uint8 humAddresss=0x27; 					/******address = 100111*********************/

extern bool i2cOnFlag;
bool humidOnFlag=0;
extern bool pressureOnFlag;

void humidReadReady();

/**************************************************************************
Function to initialize humidity sensor. This checks if i2c is enabled or 
not and accordingly initializes it if required.
**************************************************************************/
void humInit()
{
	if (i2cOnFlag==0)		//check if i2c is not enabled, if enabled then do nothing
		{
			i2cInit();
			i2cOnFlag=1;
		}

	if(pressureOnFlag==0)
	{
		#ifdef HTP_V1
		setPin(1);
		#elif defined HTP_V2
		clearPin(1);
		#endif
	}
	humidOnFlag=1;
}


/********************************************************************************
Function to Enable the Digital humidity sensor 
 in the configuration described below:-
 	`
		 	  Alarm_High_On = 80% humidity 
		 	 Alarm_Low_On = 20% humidity   
			Alarm_High_Off = 75% humidity
		  Alarm_Low_Off = 25% humidity 			
	    I2C Address = 0x27
		 Command Window = 3 ms 	
	      Alarm_High = Active_High 
		   Alarm_Low = Active_High 
			 Alarm_High = Full_Push-Pull Output
			   Alarm_Low = Full_Push-Pull Output
	 
*********************************************************************************/


void setUpperHumidThreshold(uint8 thresholdOn,uint8 thresholdOff)
{

	initDioInterrupt(16,FALLING);
	volatile uint32 wait;


	float countf;
	uint16 upperThresholdCount;
	uint16 lowerThresholdCount;

	countf=(16382.0/100.0)*thresholdOn;
	upperThresholdCount=(uint16)countf;

	countf=163.82*thresholdOff;
	lowerThresholdCount=(uint16)countf;

	clearPin(1);
	wait=500;
	while (wait--);
	setPin(1);
	wait=250;
	while (wait--);


	/*Sending the command to enter command mode*/

	i2cWrite(humAddresss, 0xa0);	//0xa0 is the command to be sent within 10 ms
	i2cWriteContinue(0);
	i2cWriteAndStop(0);
	wait=60000;
	while(wait--);


	//Now read
	// uint8 tempHigh;
	// uint8 tempLow;
	// uint8 tempXlow;


	/*Setting High Alarm On threshold value***********/

	i2cWrite(humAddresss, 0x58);	//0x58 is the command to write to EEPROM location for higher threshold
	i2cWriteContinue(upperThresholdCount>>8);
	i2cWriteAndStop(upperThresholdCount);
	wait=100000;
	while(wait--);

	/*Setting High alarm off threshold value*/

	i2cWrite(humAddresss, 0x59);	//0x58 is the command to write to EEPROM location for higher threshold
	i2cWriteContinue(lowerThresholdCount>>8);
	i2cWriteAndStop(lowerThresholdCount);
	wait=100000;
	while(wait--);

	/**********Reading from the above location*******/

	i2cWrite(humAddresss, 0x19);	//0x58 is the command to write to EEPROM location for higher threshold
	i2cWriteContinue(0x00);
	i2cWriteAndStop(0x00);

	/**********  Select Cust. Config. Reg. *********/

	i2cWrite(humAddresss, 0x5c);
	i2cWriteContinue(0x02);	//setting the higher and lower alarms to be active low and push pull and leaving
	i2cWriteAndStop(0xA7);	//leaving the i2c address as 0x27 itself
	wait=100000;
	while(wait--);


	clearPin(1);
	wait=500;
	while (wait--);
	setPin(1);
	wait=200;
	while (wait--);
	

}



void setLowerHumidThreshold(uint8 thresholdOn,uint8 thresholdOff)
{
	initDioInterrupt(17,FALLING);
	volatile uint32 wait;

	float countf;
	uint16 upperThresholdCount;
	uint16 lowerThresholdCount;

	countf=163.82*thresholdOn;
	lowerThresholdCount=(uint16)countf;

	countf=163.82*thresholdOff;
	upperThresholdCount=(uint16)countf;

	clearPin(1);
	wait=500;
	while (wait--);
	setPin(1);
	wait=250;
	while (wait--);

	/*Sending the command to enter command mode*/

	i2cWrite(humAddresss, 0xa0);	//0xa0 is the command to be sent within 10 ms
	i2cWriteContinue(0);
	i2cWriteAndStop(0);


	uint8 tempHigh;
	uint8 tempLow;
	uint8 tempXlow;

	/********** select location Alarm Low On and set 20% humidity *********/

	i2cWrite(humAddresss, 0x5a);
	i2cWriteContinue(lowerThresholdCount>>8);
	i2cWriteAndStop(lowerThresholdCount);
	wait=100000;
	while(wait--);

	/**********  Set Alarm_Low_Off = 25% Humidity. (Write 0x1000 to EEPROM Location 0x1B) *********/
	i2cWrite(humAddresss, 0x5b);
	i2cWriteContinue(upperThresholdCount>>8);
	i2cWriteAndStop(upperThresholdCount);
	wait=100000;
	while(wait--);
	/**********  Select Cust. Config. Reg. *********/

	i2cWrite(humAddresss, 0x5c);
	i2cWriteContinue(0x02);	//setting the higher and lower alarms to be active low and push pull and leaving
	i2cWriteAndStop(0xA7);	//leaving the i2c address as 0x27 itself
	wait=100000;
	while(wait--);

	/**********Reading from the above location*******/

	i2cWrite(humAddresss, 0x1c);	//0x58 is the command to write to EEPROM location for higher threshold
	i2cWriteContinue(0x00);
	i2cWriteAndStop(0x00);
	/*wait=200;
	while(wait--);*/
	wait=100000;
	while (wait--);

	i2cRequestData(humAddresss);
	tempHigh=i2cRead();
	tempLow=i2cRead();
	tempXlow=i2cReadStop();
	//updateAmbientdb(getNodeId(),0,tempLow);


	clearPin(1);
	wait=500;
	while (wait--);
	setPin(1);
	wait=200;
	while (wait--);
}
	

/************************************************************
Read Humidity from the humidity sensor.
************************************************************/
#ifdef HTP_V1
uint16 readHumid()
{
	uint8 u8High,u8Low;
	uint16 humid=0;
		
	humidReadReady();							// Start device for Read

	
	u8High=i2cRead();
	
	//debug (&u8High,1);

	u8Low=i2cReadStop();

	if ((u8High&0xC0)==0x00)
	{
		humid=u8High;
		humid= (humid<<8)|u8Low;
		humid= 0x3FFF & humid;

	}
	else if ((u8High&0xC0)==0x80)
	{
		debug("humidity sensor in command mode",31);
	}

	//humid= humid>>2;
	//	value=((1.0*humid)/(pow(2,14)-2))*100;					//converting 14	bit	ADC	output for humidity to %RH:
	return humid;
}
#endif

#ifdef HTP_V2

float readHumid()
{
	uint16 humData;
	float humValue;

	i2cWrite(hdcAddress,2);
	i2cRequestData(hdcAddress);
	
	humData = i2cRead();
	humData |= (i2cReadStop())<<8;
	humValue= (((float)humData*100)/65536);


	return humValue;
}
/*******this function made by us *******/
/*
float oldHumid = readHumid();
uint32 fToIHumid(float old){

	

	uint32 newHumid = round(oldHumid);

	return newHumid;
}
*/
void writeToHDC(uint8 commandReg, uint8 data)
{
	i2cWrite(hdcAddress,commandReg);
	i2cWriteAndStop(data);
}

bool isDataReady()
{
	uint8 i=0;
	i2cWrite(hdcAddress,4);
	i2cRequestData(hdcAddress);
	i = i2cReadStop();
	if (( i & 0x80 ) == 0x80)
	{
		return 1;
	}
	return 0;
}

void startMeasurement()
{
	writeToHDC (15,1);
}
#endif
/*********************************************************
Internal call. not for users.
*********************************************************/
void humidReadReady()
{

	i2cWriteCommand(humAddresss);

	volatile uint16 i=65000;
	while (i--);

	i2cRequestData(humAddresss);
	
}
#endif