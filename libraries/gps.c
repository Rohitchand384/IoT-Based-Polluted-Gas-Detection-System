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
#include "i2c.h"
#include "uartSerial.h"
#include "dio.h"
#include "gps.h"
#include "node.h"
#include "pcInterface.h"
#include "uartSerial.h"
#include "config.h"

#ifdef GPS_ENABLE
//sequences to turn off the irrelavant data
uint8 configPort[]={ 0xB5, 0x62, 0x06, 0x00, 0x01, 0x00, 0x01,0x08,0x22};
uint8 stopGLL[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B };
uint8 stopGSA[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32 };
uint8 stopGSV[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39 };
uint8 stopRMC[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x40 };
uint8 stopVTG[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47 };
uint8 stopGps[]= { 0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x16, 0x74};
uint8 hotStart[]={ 0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x09, 0x00, 0x17, 0x76};
uint8 stopGGA[]= {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x24};
uint8 startGGA[]={0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x0A, 0x4C};

bool dataGatherFlag;	//flag to indicate that the data samples from GPS are now to be stored
void transferToGps(uint8 *data, uint8 len);	//API to send command to GPS
void gpsRead();	//Read data from GPS
void gpsConfigure(void);	//configure GPS to work in desired mode
void enableDataLogging(void);	//start data logging from GPS into memory
void disableDataLogging(void);	//stop data logging from GPS into memory
extern void disableUart1Interrupt();

/**************************************************************************
Function to initialize GPS. Turns on the power to GPS and enables serial
commmunication
**************************************************************************/
void gpsInit()
{
/************************enabling UART1 on which GPS is connected***********************/////*/
	uartSerialInit();
	enableDataLogging();

	setPin(13);	//turn on GPS switched by DIO13
	msdelay(40);	//wait for 40ms until the GPS is on
	
	gpsConfigure();	// configure the GPS to gather specific data
}

void gpsConfigure(void)
{
	transferToGps(stopRMC, 16);	//Stop RMC Data
	msdelay(50);
	transferToGps(stopGLL, 16);	//Stop GLL Data
	msdelay(50);
	transferToGps(stopGSA, 16);	//Stop GSA Data
	msdelay(50);
	transferToGps(stopGSV, 16);	//Stop GSV Data
	msdelay(50);
	transferToGps(stopVTG, 16);	//Stop VTG Data
	msdelay(50);
	transferToGps(stopGGA, 16);	//Stop GGA Data
	msdelay(100);	//wait till unrelavant response from GPS is dropped
	dataGatherFlag=1;	//indicate the processor to save the data from GPS into memory

}


//Transfer data of length "len" to GPS 
void transferToGps(uint8 *data, uint8 len)
{
	/*********************Written for UART 1*****************/
	uart1SerialWrite(data,len);

}

/************************************************************
Start collecting GPS data
************************************************************/
void gpsRead()
{
	transferToGps(startGGA,16);	// send the command to GPS to send GGA data
	msdelay(10);
	enableDataLogging();	// start logging data into memory

}


//enables the logging of data into gps buffer
void enableDataLogging(void)
{
	enableUart1Interrupt();

}

//disables the logging of data in gps buffer

void disableDataLogging(void)
{
	disableUart1Interrupt();

}


//turns of the GPS functionality. Does not switch off the power to GPS
void disableGps()
{
	transferToGps(stopGps,sizeof(stopGps));
}


//enables the GPS functionality
void enableGps()
{
	transferToGps(hotStart,sizeof(hotStart));
}

void gpsISR(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
	uint8 rcvData=0;
	vAHI_UartSetInterrupt(E_AHI_UART_1,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   E_AHI_UART_FIFO_LEVEL_1 );
	if (dataGatherFlag==0)
	{
		rcvData=u8AHI_UartReadData (E_AHI_UART_1);
	}

}
#endif