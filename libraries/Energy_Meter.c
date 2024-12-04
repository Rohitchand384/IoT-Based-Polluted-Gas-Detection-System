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
 
#include "config.h"
#include <jendefs.h>
#include <stdio.h>
#include <string.h>
#include <AppHardwareApi.h>
#include "util.h"
#include "task.h"
#include "node.h"
#include "dio.h"
#include "uartSerial.h"
#include "pcInterface.h"
#include "energyMeter.h"
#include "EEPROM.h"
#include "config.h"

#ifdef ENABLE_ENERGY
#ifdef DEBUG_STPM33_APP
//#define DEBUG_GUI_ASCII
//#define DEBUG_GUI_HEX
#endif

#define CRC_8 (0x07)
#define RESET_DEVICE_ADDR 0x05
#define STPM3x_FRAME_LEN 5
#define DUMMY_NEXT_BYTE_ADDR 0xFF
#define FREQUENCY_ADDR 0x2E
#define VOLTAGE_CURRENT_ADDR 0x48
#define PHASE_ADDR 0x4E
#define ACTIVE_POWER 0x5C
#define APPARENT_POWER 0x62


///////////////////////////////////////////
static uint8 CRC_u8Checksum = 0x00;
static uint8 packetFromSTPM33 [5];
static MeterData meterData;
static float currCalibValue;
static uint8 meterCalibDoneFlag=0;


/* 
 * Function to calculate 8 bit CRC using above polynomial
 * Input is first 4 byte of packet
 * Return -- 8bit CRC
 * Author -- Tech. Team Eigen Tech. P. Ltd Lighting
 */

uint8 CalcCRC8(uint8 *pBuf)
{
    uint8 i;   
    CRC_u8Checksum = 0x00;
    for (i=0; i<STPM3x_FRAME_LEN-1; i++)
    {
        Crc8Calc(pBuf[i]);
    }
    return CRC_u8Checksum;
}


/* 
 * Function to calculate 8 bit CRC using above polynomial
 * Input is 8 bit data
 * Return -- Nothing
 * Author -- Tech. Team Eigen Tech. P. Ltd Lighting
 */

void Crc8Calc (uint8 u8Data)
{
    uint8 loc_u8Idx;
    uint8 loc_u8Temp;
    loc_u8Idx=0;
    while(loc_u8Idx<8)
    {
        loc_u8Temp = u8Data^CRC_u8Checksum;
        CRC_u8Checksum<<=1;
        if(loc_u8Temp&0x80)
        {
            CRC_u8Checksum^=CRC_8;
        }
        u8Data<<=1;
        loc_u8Idx++;
    }
}


/*
 * Function to calculate CRC in UART mode basically in reverse bit order
 * Input  -- buffer to packet
 * Return -- Nothing
 * Author -- Tech. Team Eigen Tech. P. Ltd Lighting
 */
 
void FRAME_for_UART_mode(uint8 *pBuf)
{
    uint8 temp[4],x,CRC_on_reversed_buf;
    for (x=0;x<(STPM3x_FRAME_LEN-1);x++)
    {
        temp[x] = byteReverse(pBuf[x]);
    }
    CRC_on_reversed_buf = CalcCRC8(temp);
    pBuf[4] = byteReverse(CRC_on_reversed_buf);
}

/*
 * Function to reverse bit order of a byte
 * Input  -- data to reversed
 * Return -- reverse data
 * Author -- Tech. Team Eigen Tech. P. Ltd Lighting
 */

uint8 byteReverse(uint8 n)
{
    n = ((n >> 1) & 0x55) | ((n << 1) & 0xaa);
    n = ((n >> 2) & 0x33) | ((n << 2) & 0xcc);
    n = ((n >> 4) & 0x0F) | ((n << 4) & 0xF0);
    return n;
}


/*
 * Function to Reset Metering Device Firmware 
 * Input -- Nothing
 * Return -- Reset Success
 * Author -- Tech. Team Eigen Technologies P. Ltd. Lighting'
 */
 
int resetSTPM33 ()
{
    uint8 packetToSTPM33 [5];
    packetToSTPM33 [0] = 0x04;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    uint8 checkDataStatus = receiveDataFromWirelessMeter (0);
    if (checkDataStatus == 0)
        return -1;  
    //debug (packetFromSTPM33, 5);
    ////////////////////////////////////////////////////
    
    packetToSTPM33 [0] = 0xFF;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    checkDataStatus = receiveDataFromWirelessMeter (0);
    if (checkDataStatus == 0)
        return -1;
    
    //debug (packetFromSTPM33, 5);
    ////////////////////////////////////////////////////
    
    packetToSTPM33 [0] = 0xFF;
    packetToSTPM33 [1] = RESET_DEVICE_ADDR;
    packetToSTPM33 [2] = 0x10;
    packetToSTPM33 [3] = 0x00;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    checkDataStatus = receiveDataFromWirelessMeter (0);
    if (checkDataStatus == 0)
        return -1;
    return 1;
    
}


/*
 * Function to Read Metering Paramters 
 * Input -- Nothing
 * Return -- Paramters with read Status
 * Author -- Tech. Team Eigen Technologies P. Ltd. Lighting'
 */

int readElectricalParameters ()
{
    int value=0;
    uint8 packetToSTPM33 [5];
    packetToSTPM33 [0] = 0x04;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    value = receiveDataFromWirelessMeter (0);   
    if (value == 0)   
        return 0;  
    ////////////////////////////////////////////////////
    
    
    packetToSTPM33 [0] = 0xFF;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    receiveDataFromWirelessMeter (1);
    //debug (packetFromSTPM33, 5);
    ////////////////////////////////////////////////////
    
    packetToSTPM33 [0] = 0xFF;
    packetToSTPM33 [1] = 0x05;
    //Write 11 on Latch bit of Registers
    packetToSTPM33 [2] = 0x60;
    packetToSTPM33 [3] = 0x00;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    value = receiveDataFromWirelessMeter (1);
    if (value == 0)
        return 0;
    //debug (packetFromSTPM33, 5);    
    ////////////////////////////////////////////////////
    
    packetToSTPM33 [0] = FREQUENCY_ADDR;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    value = receiveDataFromWirelessMeter (1);
    if (value == 0)
        return 0;
      
    ////////////////////////////////////////////////////
    
    packetToSTPM33 [0] = VOLTAGE_CURRENT_ADDR;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);

    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    value = receiveDataFromWirelessMeter (1);
    if (value == 0)
        return 0;
    //A0 00 00 04
    uint32 freqValue=0;
    memcpy (&freqValue, packetFromSTPM33, 4);
    meterData.frequency = (50.0*2500.0)/(freqValue & 0x00000fff);
    ///////////////////////////////////////////////////
    //Configuration alert related paramters regardiing 

    ///////////////////////////////////////////////////
    
    
    
    ////////////////////////////////////////////////////
    
    packetToSTPM33 [0] = ACTIVE_POWER;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    
    //Wait here to receive data from UART1
    value = receiveDataFromWirelessMeter (1);
    if (value == 0)
        return 0;

    ////////////////////////////////////////////////////    
    //Current and voltage values are received here
    
    uint32 voltageReg=0;
    uint32 valueReg=0;
    memcpy (&voltageReg, packetFromSTPM33, 4);
    valueReg = voltageReg & 0x7FFF;
    meterData.voltage = .03608*valueReg - 4.37;
    
    ////////////////////////////////////////////////////
    //Handle voltage related alerts under voltage condition
    ////////////////////////////////////////////////////
    
    valueReg = voltageReg & 0xFFFF8000;
    valueReg = (valueReg >> 15) & 0x0001FFFF;
    meterData.current = (valueReg * 0.000424)*currCalibValue;
    
    
    packetToSTPM33 [0] = APPARENT_POWER;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    value = receiveDataFromWirelessMeter (1);
    if (value == 0)
        return 0;

    uint32 activePower = 0;
    memcpy (&activePower, packetFromSTPM33, 4);
    activePower = activePower & 0x0FFFFFFF;

    packetToSTPM33 [0] = 0xFF;
    packetToSTPM33 [1] = 0xFF;
    packetToSTPM33 [2] = 0xFF;
    packetToSTPM33 [3] = 0xFF;
    FRAME_for_UART_mode(packetToSTPM33);
    uart1SerialWrite(packetToSTPM33,  STPM3x_FRAME_LEN);
    
    ////////////////////////////////////////////////////
    //Wait here to receive data from UART1
    value = receiveDataFromWirelessMeter (1);
    if (value == 0)
        return 0;

    uint32 apparentPower = 0;
    memcpy (&apparentPower, packetFromSTPM33, 4);
    apparentPower = apparentPower & 0x0FFFFFFF;
    meterData.powerFactor = (float) ((activePower*1.0)/apparentPower);
    if (meterData.powerFactor > 1.0)
        meterData.powerFactor=1.00;
    return 1;
}


/*
 * Function to Read Metering Device Registers 
 * Input -- Type to Check firmware bug
 * Return -- Success 
 * Author -- Tech. Team Eigen Technologies P. Ltd. Lighting
 */


uint8 receiveDataFromWirelessMeter (uint8 pollCheckType)
{
    int Len=0;
    while (Len != 5)
    {
        uint32 counterrr=0;
        uint8 fifoLevel=0;
        do{
            counterrr++;
		    fifoLevel=u8AHI_UartReadRxFifoLevel(E_AHI_UART_1);
		    if (counterrr == 32000)
		        return 0;       //Incase meter is disconnected 
	    }while (fifoLevel ==0);
	    if (Len == 4)
	    {
	        packetFromSTPM33[Len]=uartSerialReadByte();     //Receive CRC byte here
	    }
	    else
	    {
	        packetFromSTPM33[3-Len]=uartSerialReadByte();   //Receive Data bytes here
	    }
	    Len++;
    }
    return 1;
}


/*
 * Function to Init Wireless meter
 * It is connected on UART1 
 * Input -- Nothing
 * Return -- Nothing
 * Author -- Tech. Team Eigen Tech. P. Ltd Lighting
 */
 
void wirelessMeterInit ()
{
    uartSerialInit();
    resetSTPM33 (); 
    uint8 a;
    uint16 b;
    EEPROMInit (&a, &b);
   	//EEPROMSectorErase (0);
    uint8 packet[8];
    EEPROMRead (0,0, packet, 8);
    if ((packet[0] == 1) && (packet[1] == 2) && (packet[6] == 3) && (packet[7] == 4))
    {
        memcpy (&currCalibValue, &packet[2], 4); 
        meterCalibDoneFlag = 1;
    }
    else
        currCalibValue=1.0;

}

/*
 * Function to return metering data
 * Input nothing
 * Return -- Metering Data
 * Author -- Tech. Team Eigen Tech. P. Ltd. Lighting
 */
 
MeterData getMeteringData ()
{
    readElectricalParameters ();
    
    if(meterData.current==0)
        return meterData;
        
    if (meterCalibDoneFlag == 0)
    {
        
        uint8 packet[8];
        packet[0] = 1;
        packet[1] = 2;
        
        float actualCurrValue = 40.5/(meterData.voltage*meterData.powerFactor);
        
        currCalibValue = actualCurrValue/meterData.current;
        
        meterCalibDoneFlag = 1;
        memcpy (&packet[2], &currCalibValue, 4);
        
        packet[6] = 3;
        packet[7] = 4;
        EEPROMWrite (0,0, packet, 8);
    }
    return meterData;
}

#endif//#ifdef ENABLE_ENERGY