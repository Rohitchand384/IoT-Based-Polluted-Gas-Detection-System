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
#include <string.h>
#include "util.h"
#include "task.h"
#include "node.h"
#include "dio.h"

#include "uartSerial.h"
#include "uart.h"
#include "ultrasonic.h"
#include "config.h"

#ifdef ENABLE_ULTRASONIC     

#define READ_DIST 0
#define READ_TEMP 1

/*******************************************
Initialize UltraSonic Sensor
********************************************/
void USInit()
{
    setPin(0);         //power pin for CO Sensor
    uartInit(E_AHI_UART_1, E_AHI_UART_RATE_19200);
    enUartInterrupt(E_AHI_UART_1);
}

/**********************************************
Disable UltraSonic Sensor from SENSEnuts Device
***********************************************/
void disableUS()
{
    clearPin(0);
}

/**************************************************************************************
Send Command to get type of data for UltraSonic Sensor to read Distance or Temperature
***************************************************************************************/
void getUSData(uint8 type)
{
    uint8 txdat[6];
    switch(type)
    {
        case READ_DIST:
        {
            txdat[0]=0x55;
            txdat[1]=0xAA;
            txdat[2]=0x11;
            txdat[3]=0x00;
            txdat[4]=0x02;
            txdat[5]=0x12; //switch to read distance mode
            uartSend(E_AHI_UART_1,&txdat[0],6);
            break;
        }

        case READ_TEMP:
        {
            txdat[0]=0x55;
            txdat[1]=0xAA;
            txdat[2]=0x11;
            txdat[3]=0x00;
            txdat[4]=0x03;
            txdat[5]=0x13; //switch to read  mode

            uartSend(E_AHI_UART_1,&txdat[0],6);
            break;
        }
    }
}
	
/*******************************************
Read the data from UltraSonic Sensor
********************************************/
uint16 readData(uint8 type)
{
    unsigned char data[8];
    uint8 sum=0;
    uint16 f;
    uint8 i=0;

    for(i=0;i<8;i++)  
    {    
        data[i]=readByteFromUart(E_AHI_UART_1);
    }

    if(type == READ_DIST)
    {
        if(data[3]!=0)   //if length is not equals to 0
        {
        	ledOn();	
    	    //checksum
    	    for(i=2;i<7;i++)  
    	    {    
    	        sum=sum+data[i];         
    	    }

    	    if(data[4]==0x02) //0x02 means distance
    	    {
    		    f=((data[5]<<8)|data[6]);
    			 
    		    addTask(USER, LED_OFF, 100);
    		    if(data[7]==sum)
    		    {
    			    //displayData(f);
    		        //addToTable(f);
    		        return f;
    		    }
    		}  
    	}
    }
    else if(type == READ_TEMP)
    {
        /*NO NEED*/
    }
    return 0;
}

/*******************************************
Send data of UltraSonic Sensor to GUI
********************************************/
void addToTable(uint16 us_dist)
{
    uint16 nodeId=getNodeId();
    uint8 packetToPC[10];
    packetToPC[0]=0x80;
    packetToPC[1]=nodeId>>8;
    packetToPC[2]=nodeId;
    memcpy(&packetToPC[3],&us_dist,2);

    txCmd(E_AHI_UART_1,packetToPC,5);
}

#endif