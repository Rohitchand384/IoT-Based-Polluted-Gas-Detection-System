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
#include<stdio.h>
#include<string.h>

#include "pcInterface.h"
#include "util.h"
#include "task.h"
#include "dio.h"
#include "uart.h"
#include "config.h"

//function to read a byte coming from pc
uint8 readByteFromPc()
{
	return readByteFromUart(E_AHI_UART_0);
}

//initialize uart0 as pc connecting uart
void sendToPcInit()
{    
	uartInit(E_AHI_UART_0,E_AHI_UART_RATE_115200);   
}

void sendToPcInit_9600()
{    
	uartInit(E_AHI_UART_0,E_AHI_UART_RATE_9600);   
}

//initialize uart0 interrupt (this will help us receive data from pc)
void receiveFrmPcInit()
{
	enUartInterrupt(E_AHI_UART_0);
}

//this function can be called if the data is to be received in any  windows other than sensenuts gui
void sendDataToPc(uint8* baseAddress, uint16 noBytes)
{
	uartSend(E_AHI_UART_0,baseAddress,noBytes);
}
//table for smoke detection
void smokeData(uint16 nodeId,uint16 smokeValue)
{
	uint8 packetToPC[5];
	packetToPC[0]=0x75;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	packetToPC[3]=smokeValue>>8;
	packetToPC[4]=smokeValue;
	

	txCmd(E_AHI_UART_0,packetToPC,5);
}


//update database with humidity and temperature fron DHT11 info
void showOnTable(uint16 nodeId, uint16 u16Humidity, uint16 u16Temperature)
{
	uint8 packetToPC[7];
	packetToPC[0]=0x70;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	packetToPC[3]=u16Humidity>>8;
	packetToPC[4]=u16Humidity;
	packetToPC[5]=u16Temperature>>8;
	packetToPC[6]=u16Temperature;

	txCmd(E_AHI_UART_0,packetToPC,7);	
}


//update database with light and temperature info
void updateSTLdb(uint16 nodeId,uint16 light,uint8 temperature)
{
	uint8 packetToPC[6];
	packetToPC[0]=0x45;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	packetToPC[3]=light>>8;
	packetToPC[4]=light;
	packetToPC[5]=temperature;

	txCmd(E_AHI_UART_0,packetToPC,6);
}

//update database with light and temperature info
void updateSTLdbf(uint16 nodeId,uint16 light,uint16 temperature)
{
	uint8 packetToPC[7];
	packetToPC[0]=0x44;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	packetToPC[3]=light>>8;
	packetToPC[4]=light;
	packetToPC[5]=temperature>>8;
	packetToPC[6]=temperature;

	txCmd(E_AHI_UART_0,packetToPC,7);	
}


//function to send a string to gui to be displayed in output window
void debug(uint8* string,uint8 length)
{
	uint8 packetToPC[MAX_STRING_LENGTH];
	packetToPC[0]=0x40;
	memcpy(&packetToPC[1],string,length);

	txCmd(E_AHI_UART_0,packetToPC,length+1);	
}

//function to send a string to gui to be displayed in output window
void returnMsg(uint8* string,uint8 length)
{
	uint8 packetToPC[MAX_STRING_LENGTH];
	packetToPC[0]=0x50;
	memcpy(&packetToPC[1],string,length);

	txCmd(E_AHI_UART_0,packetToPC,length+1);
}
//function to send a string to gui to be displayed in output window
void sendToSpW(uint8* string,uint8 length)
{
	uint8 packetToPC[MAX_STRING_LENGTH];
	packetToPC[0]=0x51;
	memcpy(&packetToPC[1],string,length);

	txCmd(E_AHI_UART_0,packetToPC,length+1);
}

void updateGendb(uint8* packet,int size)
{
	txCmd(E_AHI_UART_0,packet,size);	
}

#ifdef HTP_V1
/*********update humidity, pressure and temperature Database******************/
void updateMetdb(uint16 nodeId,uint16 humidity,uint32 pressure, uint16 temperature)
{
	uint8 packetToPC[11];
	packetToPC[0]=0x46;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	packetToPC[3]=humidity>>8;
	packetToPC[4]=humidity;
	packetToPC[5]=pressure>>24;
	packetToPC[6]=pressure>>16;
	packetToPC[7]=pressure>>8;
	packetToPC[8]=pressure;
	packetToPC[9]=temperature>>8;
	packetToPC[10]=temperature;
	txCmd(E_AHI_UART_0,packetToPC,11);
}
#elif defined HTP_V2
void updateMetdb(uint16 nodeId,float humidity, float pressure, float temperature)
{
	uint8 packetToPC[11];
	packetToPC[0]=0x52;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	memcpy(&packetToPC[3],&humidity,4);
	memcpy(&packetToPC[7],&temperature,4);
	memcpy(&packetToPC[11],&pressure,4);
	
	txCmd(E_AHI_UART_0,packetToPC,15);
}
#endif

void updateBattery(uint16 nodeId, float battery)
{
    uint8 packetToPC[7];
	packetToPC[0]=0x54;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	memcpy(&packetToPC[3], &battery, 4);

	txCmd(E_AHI_UART_0,packetToPC,7);
}

#ifdef ENABLE_CO2

void updateCo2db(uint16 nodeId,uint16 Co2Value)
{
	uint8 packetToPC[6];
	packetToPC[0]=0x54;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	memcpy(&packetToPC[3],&Co2Value,2);
	
	txCmd(E_AHI_UART_0,packetToPC,5);
}
#endif
// update soil moisture and temperature to system
void updateSoildb(uint16 nodeId, uint16 moisture, uint16 temp)
{
	float moisVolt=0.0, tempVolt=0.0, vwc=0.0, temperature=0.0;
	uint32 soilMoist=0, soilTemp=0;
	// converting 16 bit moisture to voltage
	moisVolt = moisture*2.4/1024;
	// converting 16 bit temperature to voltage
	tempVolt = temp*2.4/1024;

	if(moisVolt <= 1.1)
	{
		// converting voltage to VWC
		vwc = 10 * moisVolt - 1;
	}
	else if (moisVolt > 1.1 && moisVolt <= 1.3)
	{
		// converting voltage to VWC
		vwc = 25 * moisVolt -17.5;
	}
	else if (moisVolt > 1.3 && moisVolt <= 1.82)
	{
		// converting voltage to VWC
		vwc = 48.08 * moisVolt - 47.5;
	}
	else if (moisVolt > 1.82 && moisVolt <= 2.2)
	{
		// converting voltage to VWC
		vwc = 26.32 * moisVolt -7.89;
	}
	else if (moisVolt > 2.2)
	{
	  
		// converting voltage to VWC
		vwc = 62.5*moisVolt - 87.5;
	}
	// converting voltage to temperature
	temperature = tempVolt * 41.67 -40;

	// typecasting float moisture and temperature to uint32 to update to GUI
	// soilMoist = *((uint32*)(&vwc));
	// soilTemp = *((uint32*)(&temperature));

	soilMoist = ((uint32)(vwc));
	soilTemp = ((uint32)(temperature));

	/*soilMoist = *((uint32*)(&moisVolt));
	soilTemp = *((uint32*)(&tempVolt));*/

	uint8 packetToPC[11];
	packetToPC[0]=0x53;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	packetToPC[3]=soilMoist>>24;
	packetToPC[4]=soilMoist>>16;
	packetToPC[5]=soilMoist>>8;
	packetToPC[6]=soilMoist;
	packetToPC[7]=soilTemp>>24;
	packetToPC[8]=soilTemp>>16;
	packetToPC[9]=soilTemp>>8;
	packetToPC[10]=soilTemp;
	txCmd(E_AHI_UART_0,packetToPC,11);
}

void updateEc(uint16 id, uint32 ec)
{
    uint8 packetToPC[11];
    //float eConduct = (float)ec 
    packetToPC[0]=0x56;
	packetToPC[1]=id>>8;
	packetToPC[2]=id;
	memcpy(&packetToPC[3], &ec, 4);
	txCmd(E_AHI_UART_0,packetToPC,11);
}

void updateWirelessMeterData (uint16 nodeId, float voltage, float current, float pf, float freq)
{
    uint8 packetToPC[100];
	packetToPC[0]=0x58;
	memcpy (&packetToPC[1], &nodeId, 2);
	memcpy(&packetToPC[3], &voltage, 4);
	memcpy(&packetToPC[7], &current, 4);
	memcpy(&packetToPC[11], &pf, 4);
	memcpy(&packetToPC[15], &freq, 4);
	txCmd(E_AHI_UART_0,packetToPC,19);
}

void sendAccToGUI(uint16 nodeId ,uint8* string,uint8 length)
{
	uint8 packetToPC[9];
	packetToPC[0]=0x57;
	packetToPC[1]=nodeId>>8;
	packetToPC[2]=nodeId;
	memcpy(&packetToPC[3],string,length);
	//debug(packetToPC,9);
	txCmd(E_AHI_UART_0,packetToPC,length+3);
}
