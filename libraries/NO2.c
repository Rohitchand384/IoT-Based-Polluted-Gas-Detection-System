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

#include "clock.h"
#include "node.h"
#include "task.h"
#include "dio.h"
#include "uart.h"
#include "string.h"
#include "config.h"
#include "NO2.h"
#ifdef ENABLE_NO2

extern void uart0InterruptHandler();

static uint8 RecDataNo2[100];
uint16 CurrentIndex=0;

int32 no2Concentration=0;

void no2Init()
{
	uartInit(E_AHI_UART_0, E_AHI_UART_RATE_9600);
}


uint8 pollingNo2(uint8 value)
{
	if(value == 1)
	{
		uint8 a=0,rxd,flag=0,fix=0;
		uint32 blockingNo=0;
		no2Concentration=0;
		do
		{
			do
			{
				blockingNo++;
				a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
				if (blockingNo == 3200000)
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
				RecDataNo2[CurrentIndex] = rxd;
			}
		  CurrentIndex++;
		}while (fix==0);

		uint16 i=0;
		uint8 multiply=0;
		while (RecDataNo2[i]!=',')
		{
			i++;
		}
		i++;
		while (RecDataNo2[i]!=',')
		{
			if(RecDataNo2[i]=='-')
			{
				multiply=1;
			}
			else if(RecDataNo2[i]!=' ')
			{
				no2Concentration = no2Concentration*10 + (RecDataNo2[i]-'0');
			}
			i++;
		}
		if (multiply == 1)
		{
			no2Concentration = no2Concentration * (-1);
		}
		memset(RecDataNo2,0,++CurrentIndex);
		CurrentIndex=0;
		return 1;
	}

	else if(value == 2 || value ==3)
	{
		uint8 a=0;
		uint32 blockingNo=0;
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
		msdelay(100);

		while (a!=0)
		{
			RecDataNo2[CurrentIndex++] = readByteFromUart(E_AHI_UART_0);
			if (CurrentIndex==1023)
			{
				vAHI_SwReset();
			}
			a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
		}
		if (memcmp(RecDataNo2,"\r\nEnter Average Total: ",CurrentIndex)==0 || memcmp(RecDataNo2,"Enter Average Total: ",CurrentIndex)==0|| memcmp(RecDataNo2,"300\r\r\n",CurrentIndex)==0)
		{
			CurrentIndex = 0;
		
			return 1;
		}
		else if (memcmp(RecDataNo2,"\r\n",CurrentIndex)==0 )
		{
			vAHI_SwReset();
		}
		CurrentIndex = 0;
		return 0;
	}
	CurrentIndex = 0;
	return 0;
}

int32 no2Data()
{
	return no2Concentration;
}

uint8 setAverage(uint8* sampleForAverage,uint8 len)
{
	uartSend(E_AHI_UART_0,(uint8*) "A",1);
	uint8 i=pollingNo2(2);
	if (i)
	{
		uartSend(E_AHI_UART_0, sampleForAverage,len);
		return pollingNo2(3);
	}
	return i;
}

uint8 enableNo2Sensor()	
{
	setPin(0);
	// press any key to exit from stand by mod
	uartSend(E_AHI_UART_0, (uint8*)"\n",1);
	msdelay(1000);
	return setAverage((uint8*)"300\r",4);
}

void reqNo2ForData()
{
	enUartInterrupt(E_AHI_UART_0);
	uartSend(E_AHI_UART_0, (uint8*)"\r",1);
}
uint8 isDataReceived()
{
	return pollingNo2(1);
}
void no2Stop()
{
	vAHI_UartDisable(E_AHI_UART_0);
}

void disableNo2Sensor()
{
	clearPin(0);
}

#endif