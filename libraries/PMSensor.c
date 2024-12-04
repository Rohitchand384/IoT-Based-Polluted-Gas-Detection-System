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

#include "PMSensor.h"
#include "clock.h"
#include "node.h"
#include "task.h"
#include "string.h"
#include "dio.h"
#include "uart.h"
#include "config.h"
#ifdef ENABLE_PM2_5 

extern void uart0InterruptHandler();
uint8 recBuffPM2_5[500]="";
uint32 indexPM2_5=0;
uint16 pm2_5Value=0,pm10Value=0;
static uint16 pm2_5ValueReadBuff[10];
uint8 pm2_5ValueReadBuffIndex=0;

uint8 pollingPM2_5(uint8 value)
{
	if(value == 1)
	{
		uint32 blockingNo=0;
		uint8 a=0;
		indexPM2_5=0;
		while (indexPM2_5<8)
		{
			blockingNo=0;
			a=0;
			do
			{
				blockingNo++;
				a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
				if (blockingNo == 10000000)
				{
					indexPM2_5 = 0;
					memset(recBuffPM2_5,0,500);
					return 0;
				}
			}while(a<1);		

			recBuffPM2_5[indexPM2_5++] = readByteFromUart(E_AHI_UART_0);	
		}

		if (indexPM2_5 ==8 && recBuffPM2_5[0]==0x40 && recBuffPM2_5[1]==0x05  && recBuffPM2_5[2]==0x04 )
		{
			pm2_5ValueReadBuff[pm2_5ValueReadBuffIndex]=recBuffPM2_5[3]*256 + recBuffPM2_5[4];
			pm2_5ValueReadBuffIndex++;
			if(pm2_5ValueReadBuffIndex!=10)
			{
				uint16 firstFive=0,lastfive=0;
				for (a=0;a<5;a++)
				{
					firstFive += pm2_5ValueReadBuff[a];
					lastfive += pm2_5ValueReadBuff[a+5];
				}
				pm2_5Value = (firstFive + lastfive)/10;
				memset(pm2_5ValueReadBuff,0,10);
				pm2_5ValueReadBuffIndex=0;
				uart0InterruptHandler();
			}
			pm10Value=recBuffPM2_5[5]*256 + recBuffPM2_5[6];
			indexPM2_5=0;
			memset(recBuffPM2_5,0,500);
				
			return 1;
		}
		else
		{
			indexPM2_5=0;
			memset(recBuffPM2_5,0,500);
				
			return 0;
		}
	}
	else if(value==0)
	{
		uint32 blockingNo=0;
		
		int a=0;
		indexPM2_5=0;

		while (indexPM2_5<500)
		{
			
			blockingNo=0;
			a=0;
			do
			{
				blockingNo++;
				a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
				if (blockingNo == 10000000)
				{
					indexPM2_5 = 0;
					memset(recBuffPM2_5,0,500);
					return 0;
				}
			}while(a<1);		

			recBuffPM2_5[indexPM2_5] = readByteFromUart(E_AHI_UART_0);

			if(indexPM2_5 >0 && recBuffPM2_5[indexPM2_5]==0xA5 && recBuffPM2_5[indexPM2_5-1]==0xA5)
			{
				indexPM2_5=0;
				memset(recBuffPM2_5,0,500);
				
				return 1;
			}
			else if(indexPM2_5>0 && recBuffPM2_5[indexPM2_5]==0x96 && recBuffPM2_5[indexPM2_5-1]==0x96)
			{
				memset(recBuffPM2_5,0,500);
				
				indexPM2_5=0;
				return 0;
			}

			indexPM2_5++;

		}

		a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);

		int g=0;

		while (g<a)
		{
			readByteFromUart(E_AHI_UART_0);
		}
		memset(recBuffPM2_5,0,500);
				
		indexPM2_5=0;
		return 0;
	}
	return 0;
}
void pm2_5Init()
{
	setPin(0);
}

uint8 stopAutoSend()
{
	uint8 str[4];

	str[0]=0x68;
	str[1]=0x01;
	str[2]=0x20;
	str[3]=0x77;

		
	//msdelay(1000);
	uartSend(E_AHI_UART_0,str,4);
	return 1;//pollingPM2_5(0);
}

uint8 startAutoSend()
{
	uint8 str[4];

	str[0]=0x68;
	str[1]=0x01;
	str[2]=0x40;
	str[3]=0x57;

		
	//msdelay(1000);
	uartSend(E_AHI_UART_0,str,4);
	return 1;//pollingPM2_5(0);
}


uint8 enablePm2_5()
{
	uartInit(E_AHI_UART_0,E_AHI_UART_RATE_9600);
	
	return 1;
}

uint8 readParticalSensor()
{
	uint8 str[50] = { 0x68, 0x01, 0x04, 0x93};
	uartSend(E_AHI_UART_0,str,4);
	return pollingPM2_5(1);
}
uint8 startMeasurement()
{
	uint8 str[50] = { 0X68, 0X01, 0X01, 0X96};
	uartSend(E_AHI_UART_0,str,4);
	//enUartInterrupt(E_AHI_UART_0);
	return 1;//pollingPM2_5(0);
}
uint8 readData()
{
	if(pollingPM2_5(0))
	{
		return readParticalSensor();
	}
	return 0;
}
uint8 stopMeasurement()
{
	uint8 str[50] = { 0X68, 0X01, 0X02, 0X95};
	uartSend(E_AHI_UART_0,str,4);
	return 1;//pollingPM2_5(0);
}

uint16 readValue2_5()
{
	return pm2_5Value;
} 
uint16 readValue10()
{
	return pm10Value;
} 
#endif