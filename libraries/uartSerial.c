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
#include <stdio.h>
#include <string.h>
#include <AppHardwareApi.h>

#include "util.h"
#include "task.h"
#include "node.h"
#include "dio.h"

#include "uartSerial.h"
#include "uart.h"
#include "config.h"

//initialize uart1
void uartSerialInit()
{
    #ifndef UART1
        #define UART1
    #endif    
	uartInit(E_AHI_UART_1, E_AHI_UART_RATE_115200);
}

void uartSerialStop()
{
	vAHI_UartDisable(E_AHI_UART_1);

}

void enableUart1Interrupt(void)
{
    enUartInterrupt(E_AHI_UART_1);
}

void disableUart1Interrupt(void)
{
	vAHI_UartSetInterrupt(E_AHI_UART_1 ,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   FALSE ,
						   E_AHI_UART_FIFO_LEVEL_1 );

}


//final call to send data to uart
void uart1SerialWrite(uint8* baseAddress, uint16 noBytes)
{
	uartSend(E_AHI_UART_1, baseAddress, noBytes);
}

uint8 uartSerialReadByte(void)
{
	uint8 readByte;
	readByte=readByteFromUart(E_AHI_UART_1);
	return readByte;
}