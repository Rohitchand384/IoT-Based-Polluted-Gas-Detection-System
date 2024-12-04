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

#include "CO2.h"
#include "clock.h"
#include "node.h"
#include "task.h"
#include "string.h"
#include "dio.h"
#include "config.h"

#ifdef ENABLE_CO2

#define TxCO2Buffer 100
#define RxCO2Buffer 100
static uint8 RxCO2Fifo[RxCO2Buffer];
static uint8 TxCO2Fifo[TxCO2Buffer];

extern void uart0InterruptHandler();

static uint8 RecDataCo2[100];
uint8 CurrentIndex=0;

uint16 Co2Concentration=0;

void Co2Init()
{
	vAHI_UartSetRTSCTS(E_AHI_UART_0,FALSE);
	bAHI_UartEnable(E_AHI_UART_0 , TxCO2Fifo , TxCO2Buffer , RxCO2Fifo , RxCO2Buffer);
	vAHI_UartSetBaudRate(E_AHI_UART_0, E_AHI_UART_RATE_9600);
}

uint8 pollingCo2(uint8 value)
{
	bool fix=0,flag=0;
	uint8 a=0;
	uint8 rxd=0;
	uint32 blockingNo=0;
	
	if (value == 1)
	{	
		uint8 temp_data=0;
		uint8 lvl =u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
		while (lvl)
    	{
			temp_data = u8AHI_UartReadData(E_AHI_UART_0);
			lvl--;
			RecDataCo2[CurrentIndex]=temp_data;
			CurrentIndex++;
		}
		uint8 i=0;
		while(RecDataCo2[i]!=0xFF && RecDataCo2[i+1]!=0x86)
		{
			i++;
			if(i>=CurrentIndex)
			{
				return -1;
			}
		}
		memcpy(&RecDataCo2[0],&RecDataCo2[i],CurrentIndex-i);

		if(RecDataCo2[0]==0xFF && RecDataCo2[1]==0x86)
		{
			Co2Concentration = RecDataCo2[2];
			Co2Concentration = Co2Concentration<<8;
			Co2Concentration |= RecDataCo2[3];
			CurrentIndex = 0;
			return 1;	
		}
		return 0;
	}

	if(value == 0)
	{
		do
		{
			do
			{
				blockingNo++;
				a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
				if (blockingNo == 10000000)
				{
					CurrentIndex = 0;
					return 0;
				}
			}while(a<1);
			rxd  =  u8AHI_UartReadData(E_AHI_UART_0);
			if(rxd == 0x0d && CurrentIndex != 0)
			{
				flag = 1;
			}
			else if (rxd == 0x0a && flag == 1)
			{
				flag = 0;
				fix=1;
		  	}
		  	else
		  	{
				flag=0;
		  	}

			if(CurrentIndex < 100)
			{
				RecDataCo2[CurrentIndex] = rxd;
			}
		  CurrentIndex++;
		}while (fix==0);

		memset(RecDataCo2,0,++CurrentIndex);
		CurrentIndex = 0;
	}
return 1;
}

uint8 EnableCO2Sensor()	
{
	uint8 i=0,result=0;
	CurrentIndex=0;
	Co2Concentration=0;
	setPin(1);
	// do
	// {
	// 	result = pollingCo2(0);
	// 	i++;
	// }while(i<10 && result == 0);
	result=1;
	return result;
}

void interruptHandling()
{
	vAHI_UartSetInterrupt(E_AHI_UART_0, FALSE , FALSE , FALSE ,
							FALSE , E_AHI_UART_FIFO_LEVEL_1);
	uart0InterruptHandler();
}


void ReqCo2ForData()
{
	
	uint8 CO2Request[9]={0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
	uint16 txfifolevel;

	// Enabling interrupt and set register callback function
	vAHI_UartSetInterrupt(E_AHI_UART_0, FALSE , FALSE , FALSE ,TRUE , E_AHI_UART_FIFO_LEVEL_1);
	vAHI_Uart0RegisterCallback(interruptHandling);

	// writing data on uart 0
	CurrentIndex=0;
	u16AHI_UartBlockWriteData(E_AHI_UART_0, CO2Request, 9);
	do
	{
	  txfifolevel=u16AHI_UartReadTxFifoLevel(E_AHI_UART_0);
	}while(txfifolevel!=0);
}

void Co2Stop()
{
	clearPin(1);
	vAHI_UartDisable(E_AHI_UART_0);
}


uint8 PollingForDataCo2()
{
	uint8 result;
	result = pollingCo2(1);
	return result;
}

void DisableCO2Sensor()
{
	clearPin(1);
}

#endif