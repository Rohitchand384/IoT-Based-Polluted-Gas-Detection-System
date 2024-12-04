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
#include <AppHardwareApi.h>

#include "util.h"
#include "task.h"
#include "node.h"
#include "dio.h"

#include "uartSerial.h"
#include "uart.h"
#include "CO.h"
#include "config.h"
#include "pcInterface.h"
#include "util.h"
#ifdef ENABLE_CO

#define QUESTION_ANSWER_MODE 0
#define CONTINUOUS_MODE 1

void coInit()
{
    setPin(0);         //power pin for CO Sensor
    uartInit(E_AHI_UART_0, E_AHI_UART_RATE_9600);
}


void mode(uint8 mode)
{
    uint8 txdat[9];
    switch(mode)
    {
        case QUESTION_ANSWER_MODE:
        {
            txdat[0]=0xFF;
            txdat[1]=0x01;
            txdat[2]=0x78;
            txdat[3]=0x41;
            txdat[4]=0x00;
            txdat[5]=0x00;
            txdat[6]=0x00;
            txdat[7]=0x00;
            txdat[8]=0x46; //switch to question answer mode
            uartSend(E_AHI_UART_0,&txdat[0],9);
            break;
        }

        case CONTINUOUS_MODE:
        {
            txdat[0]=0xFF;
            txdat[1]=0x01;
            txdat[2]=0x78;
            txdat[3]=0x40;
            txdat[4]=0x00;
            txdat[5]=0x00;
            txdat[6]=0x00;
            txdat[7]=0x00;
            txdat[8]=0x47; //switch to initiative mode/continous mode

            uartSend(E_AHI_UART_0,&txdat[0],9);
            break;
        }
    }
}

void reqData(uint8 mode)
{
    uint8 txdat[9];
    if(mode==QUESTION_ANSWER_MODE)
    {  
        txdat[0]=0xFF;
        txdat[1]=0x01;
        txdat[2]=0x86;
        txdat[3]=0x00;
        txdat[4]=0x00;
        txdat[5]=0x00;
        txdat[6]=0x00;
        txdat[7]=0x00;
        txdat[8]=0x79;
        uartSend(E_AHI_UART_0,&txdat[0],9);
    }   

    else if(mode==CONTINUOUS_MODE)
    {  
        enUartInterrupt(E_AHI_UART_0);
    }
}

float readData(uint8 mode)
{

    ledOn();
    static signed char data[10];
    int sum=0;
    float conc;
    int i=0;

    for(i=0;i<=8;i++)  
    {    
        data[i]=readByteFromUart(E_AHI_UART_0);
    }
    
    if(mode==QUESTION_ANSWER_MODE)
    {  
        conc=((data[2]*256)+data[3])*0.100;
    }
    else if(mode==CONTINUOUS_MODE)
    {  
        disableUartInterrupt(E_AHI_UART_0);
        conc=((data[4]*256)+data[5])*0.100;
    }

    //checksum
    for(i=1;i<=7;i++)  
    {    
        sum=sum+data[i];         
    }

    if(data[8]==((~sum)+1) && data[3]!=0x00)
    {
        displayData(conc);
        //addToTable(conc);
        return(conc);
    } 
    else
    {
        return(0);
    }
    addTask(USER, LED_OFF, 100);
}

void displayData(float num)
{
    uint8 len;
    uint8 str[10];
    len=floatToStrConverter(num,&str[0]);
    //debug(&str[0],len);
    //sendDataToMac(str,len,0xFFFF,0,FALSE);
    ledOff();
}


void addToTable(float conc)
{
    uint16 nodeId=getNodeId();
    uint8 packetToPC[10];
    packetToPC[0]=0x56;
    packetToPC[1]=nodeId>>8;
    packetToPC[2]=nodeId;
    memcpy(&packetToPC[3],&conc,4);

    txCmd(E_AHI_UART_0,packetToPC,7);
}



#endif
