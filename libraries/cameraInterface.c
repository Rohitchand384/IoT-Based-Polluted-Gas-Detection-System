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

#include "cameraInterface.h"
#include "util.h"
#include "task.h"
#include "dio.h"
#include "uart.h"
#include "config.h"

#ifdef ENABLE_CAMERA

#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 256

uint8 txFIFO[UART_TX_BUF_SIZE];
uint8 rxFIFO[UART_RX_BUF_SIZE];

extern void uart0InterruptHandler();

extern uint16 imageLength;

void cameraOff()
{
	clearPin(1);
}

void cameraOn()
{
	setPin(1);
}
//function to read a byte coming from Camera

uint8 readByteFromCamera()
{ 
    uint8 data = u8AHI_UartReadData(E_AHI_UART_0);
    return data;
}

/*Definition of UART0 interrupt handler*/
void uart00ISR(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
	vAHI_UartSetInterrupt(E_AHI_UART_0,   // disable UART_0 then read Byte
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   E_AHI_UART_FIFO_LEVEL_1 );
						   
    uart0InterruptHandler();                // read Byte then 
	
	receiveFrmCameraInit();
					   
}


void camera_block_read() //////////////////////////////////////////////////////////////////////// Ajai

{
	vAHI_UartSetInterrupt(E_AHI_UART_0,    // disable UART_0
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   E_AHI_UART_FIFO_LEVEL_1 );
	
	addTask(USER, CAMERA_BLOCK_READ_TASK, 5); // wait for some time and then read block then enable uart_0
 
}

void resetCamera() //3
{
	uint8 command[] = {0x56, 0x00, 0x26, 0x00}; //56 00 26 00 // R-   76 00 26 00 00							//reset camera
	sendDataToCamera(command, sizeof(command));	
}

void setResolution()//4
{
	uint8 command[] = {0x56, 0x00, 0x54, 0x01, 0x22}; // // 56 00 31 05 04 01 00 19 22 //  56 00 54 01 22
	sendDataToCamera(command, sizeof(command));	
}

void capImage()   //5
{   
	uint8 command[] = {0x56, 0x00, 0x36, 0x01, 0x00}; //56 00 36 01 00  // R-   76 00 36 00 00
	sendDataToCamera(command, sizeof(command));
}

void readImgLen() //6
{
	uint8 command[] = {0x56, 0x00, 0x34, 0x01, 0x00}; //56 00 34 01 00  //R-   76 00 34 00 04 00 00 MSB LSB 
	sendDataToCamera(command, sizeof(command));
}
void readImage()  //7
{   
	uint8 imageSize[2];

	imageSize[0] = imageLength>>8;  //MSB
	imageSize[1] = imageLength;     //LSB
    
	uint8 command[] = {0x56, 0x00, 0x32, 0x0C, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, imageSize[0], imageSize[1], 0x00, 0x0A }; 
	                       //5600320C000A             00 00 00 00 00 00 31 4C 00 0A
	                                                  //56 00 32 0C 00 0A 00 00 MM MM 00 00 KK KK XX XX 
	                                                  //00 00 MM MM Init address - 00 00 00 00 
                                                      //00 00 KK KK data length  - 00 00 ImgLen MSB first, then LSB 
	                                                  //XX XX == 00 0A 
	                                                  //R- 76 00 32 00 00 ImgLen 76 00 32 00 00 
	sendDataToCamera(command, sizeof(command));
}

void motionDetection() //8
{
	uint8 command[] = {0x56, 0x00, 0x37, 0x01, 0x01}; // 0x56+0x00+0x37+0x01+0x01  //R-   0x76+0x00+0x37+0x00+0x00
    sendDataToCamera(command, sizeof(command));
}

//initialize uart0 as pc connecting uart
void sendToCameraInit()
{
	/*********** ENABLE TWO WIRE MODE TO RELEASE RTS AND CTS*****/
	vAHI_UartSetRTSCTS(E_AHI_UART_0,
					   FALSE);
	
	/*********** enable UART0 with 32 byte Tx and Rx buffers*****/
	
	bAHI_UartEnable(E_AHI_UART_0 ,
				 txFIFO ,
				 UART_TX_BUF_SIZE ,
				 rxFIFO ,
				 UART_RX_BUF_SIZE);
	
	/********** setting baud rate to 38400 ***************/
	
	vAHI_UartSetBaudRate(E_AHI_UART_0,
					     E_AHI_UART_RATE_38400); 
}

//initialize uart0 interrupt (this will help us receive data from pc)
void receiveFrmCameraInit()
{
	/********** enabling interrupt for Rx buffer ************
	********** when a byte arrives ************************/

	vAHI_UartSetInterrupt(E_AHI_UART_0 ,
					   FALSE ,
					   FALSE ,
					   FALSE ,
					   TRUE ,
					   E_AHI_UART_FIFO_LEVEL_1 );

	/*********** registering interrupt handler for UART0*****/

	vAHI_Uart0RegisterCallback(uart00ISR);
	
}

void receiveFrmCameraInit_BlockRead() ////////////////////////////////////////////////
{
	/********** enabling interrupt for Rx buffer ************
	********** when a byte arrives ************************/

	vAHI_UartSetInterrupt(E_AHI_UART_0 , // ENABLE interrupt of uart_0
					   FALSE ,
					   FALSE ,
					   FALSE ,
					   TRUE ,
					   E_AHI_UART_FIFO_LEVEL_1 );

	/*********** registering interrupt handler for UART0*****/

	vAHI_Uart0RegisterCallback(camera_block_read); 
}

//final call to send data to uart
void uart0(uint8* baseAddress, uint16 noBytes)
{
	uint8 fifoLevel;
	/****check Tx fifo level************/	
	fifoLevel=u8AHI_UartReadTxFifoLevel(E_AHI_UART_0);
	
	/*****wait for the availability of space in FIFO****/
	while (fifoLevel !=0)
	{
		fifoLevel=u8AHI_UartReadTxFifoLevel(E_AHI_UART_0);
	}
	
	/*****writes data to the fifo************/
	u16AHI_UartBlockWriteData(E_AHI_UART_0,
							  baseAddress,
							  noBytes);
}

void sendDataToCamera(uint8* baseAddress, uint16 noBytes)
{
	uart0(baseAddress,noBytes);
}


#endif   /*  ENABLE_CAMERA */



