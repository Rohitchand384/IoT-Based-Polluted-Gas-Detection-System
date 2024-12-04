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

#include<stdio.h>
#include<string.h>
#include "pcInterface.h"
#include "util.h"
#include "task.h"
#include "dio.h"
#include "uart.h"
#include "config.h"

#ifdef GPS_ENABLE
#include "gps.h"
#endif

#ifdef UART0

#define UART0_TX_BUF_SIZE 1024
#define UART0_RX_BUF_SIZE 1024

uint8 tx0FIFO[UART0_TX_BUF_SIZE];
uint8 rx0FIFO[UART0_RX_BUF_SIZE];

uint8 uart0Mutex=0;      // to be compiles later
#endif

#ifdef UART1

#define UART1_TX_BUF_SIZE 1024
#define UART1_RX_BUF_SIZE 1024

uint8 tx1FIFO[UART1_TX_BUF_SIZE];
uint8 rx1FIFO[UART1_RX_BUF_SIZE];

uint8 uart1Mutex=0;
#endif

#ifdef UART0
//every byte coming on uart0 is received in this function
extern void uart0InterruptHandler();

#elif (defined UART1)
//every byte coming on uart1 is received in this function
extern void uart1InterruptHandler();
#endif

#ifdef GPS_ENABLE
//every byte coming on uart1 is received in this function
extern void gpsISR();
#endif

//function to read a byte coming from given uart
uint8 readByteFromUart(uint8 uartNumber)
{
	return u8AHI_UartReadData(uartNumber);
}

#ifdef UART0

/*Definition of UART0 interrupt handler*/
void uart0ISR(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
	vAHI_UartSetInterrupt(E_AHI_UART_0,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   E_AHI_UART_FIFO_LEVEL_1 );
	uart0InterruptHandler();
	
	//don't forget to enabled uart interrupt when done
}

#endif

#ifdef UART1

/*Definition of UART0 interrupt handler*/
void uart1ISR(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
	vAHI_UartSetInterrupt(E_AHI_UART_1,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   E_AHI_UART_FIFO_LEVEL_1 );
	uart1InterruptHandler();
	
	//don't forget to enabled uart interrupt when done
}

#endif


//initialize uart 
void uartInit(uint8 uartNumber,uint8 baudRate)
{
	/*********** ENABLE TWO WIRE MODE TO RELEASE RTS AND CTS*****/
	vAHI_UartSetRTSCTS(uartNumber,
					   FALSE);
	
	/*********** enable UART0 with 32 byte Tx and Rx buffers*****/
    switch (uartNumber)
    {
#ifdef UART0 
        case E_AHI_UART_0:
        {
            bAHI_UartEnable(E_AHI_UART_0 ,
				 tx0FIFO ,
				 UART0_TX_BUF_SIZE ,
				 rx0FIFO ,
				 UART0_RX_BUF_SIZE);
            break;
        }
#endif
#ifdef UART1 
case E_AHI_UART_1:
        {
        	#ifdef GPS_ENABLE
        		vAHI_UartSetLocation(E_AHI_UART_1, TRUE);
        	#endif	
            bAHI_UartEnable(E_AHI_UART_1 ,
				 tx1FIFO ,
				 UART1_TX_BUF_SIZE ,
				 rx1FIFO ,
				 UART1_RX_BUF_SIZE);
            break;
        }	    
#endif 
    } 
	
	/********** setting baud rate***************/
	
	vAHI_UartSetBaudRate(uartNumber,
					     baudRate);
}

//initialize uart interrupt
void enUartInterrupt(uint8 uartNumber)
{
	/********** enabling interrupt for Rx buffer ************
	********** when a byte arrives ************************/

	/*********** registering interrupt handler for UART*****/
	switch (uartNumber)
    {
	#ifdef UART0 
        case E_AHI_UART_0:
        {
            vAHI_Uart0RegisterCallback(uart0ISR);
            break;
        }
	#endif
	
	#ifdef UART1 
		case E_AHI_UART_1:
        #ifdef GPS_ENABLE
		{
			vAHI_Uart1RegisterCallback(gpsISR);
			break;
		}
    	#endif
    	{				
        	vAHI_Uart1RegisterCallback(uart1ISR);
           	break;
        }	    	
	#endif 
    }   
	vAHI_UartSetInterrupt(uartNumber,
					   FALSE ,
					   FALSE ,
					   FALSE ,
					   TRUE ,
					   E_AHI_UART_FIFO_LEVEL_1 );
}

//final call to send data to uart
void uartSend(uint8 uartNumber,uint8* baseAddress, uint16 noBytes)
{
    switch (uartNumber)
    {
#ifdef UART0    
        case E_AHI_UART_0:
        {
	        if (uart0Mutex==0)
	        {
		        uart0Mutex=1;
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
				uart0Mutex=0;				          
	        }
	        break;
	    }   
#endif	 
#ifdef UART1    
        case E_AHI_UART_1:
        {
	        if (uart1Mutex==0)
	        {
		        uart1Mutex=1;
		        uint8 fifoLevel;
		        /****check Tx fifo level************/	
		        fifoLevel=u8AHI_UartReadTxFifoLevel(E_AHI_UART_1);
		
		        /*****wait for the availability of space in FIFO****/
		        while (fifoLevel !=0)
		        {
			        fifoLevel=u8AHI_UartReadTxFifoLevel(E_AHI_UART_1);
		        }
		
		        /*****writes data to the fifo************/
		        u16AHI_UartBlockWriteData(E_AHI_UART_1,
								          baseAddress,
								          noBytes);
                uart1Mutex=0;								          
	        }
	        break;
	    }   
#endif
    }	        
}


//add pad and add the data to uart transmit buffer
void txCmd(uint8 uartNumber,uint8 *a, uint8 sizeValue)
{
	uint16 size=sizeValue+6;
    uint8 header[MAX_STRING_LENGTH];
    header[0]=0x01;
    header[1]=0x02;
    header[2]=0x10;
    memcpy(&header[3],a,sizeValue);
    header[sizeValue+3]=0x03;
    header[sizeValue+4]=0x04;
    header[sizeValue+5]=0x17;
    
    uartSend(uartNumber, header, size);
}


void disableUartInterrupt(uint8 uartNumber)
{
	/********** Dissabling interrupt for Rx buffer ************/
	vAHI_UartSetInterrupt(uartNumber,
					   FALSE ,
					   FALSE ,
					   FALSE ,
					   FALSE ,
					   E_AHI_UART_FIFO_LEVEL_1 );

}
