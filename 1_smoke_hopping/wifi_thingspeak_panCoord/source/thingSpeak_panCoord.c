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
#include "clock.h"  //provides clock related functionality
#include "node.h"
#include "task.h"
#include "util.h"
#include "string.h"

/*************************Do not modify above this line************************/
#include "pcInterface.h"
#include "mac.h"
#include "routing.h"
#include "phy.h"
#include "dio.h"
#include "wifiControl.h"
//add header files for temperature and light sensors
//#include "tmpSensor.h"
//#include "lightSensor.h"

#define USER_PACKET_TYPE 0x30

#define START_GATEWAY 0

extern streamTable streamInfo[8];      //this system can handle 8 parallel data streams
extern uint8 numStreams;

uint16 gas11=0;
uint16 gas22=0;

uint8* THINK_SPEAK_KEY = "NL99WFDYPTYAB827";

//*******************************************************************************
//Program execution starts from this function. It is like main function
//*******************************************************************************
void startNode()
{
    sendToPcInit();
    //tmpInit();
    //lightSensorInit();
    macInit();   
	addTask (USER,START_GATEWAY,5*SECOND);
}

//*******************************************************************************
//              All the tasks added by user expire in this function
//*******************************************************************************
void userTaskHandler(uint8 taskType)
{
	switch (taskType)
	{
		case START_GATEWAY:
		{
			//Reset wifi Device
			//Wifi Buffer size must be 2048
			wifiInit();
			saveWlanSsidPassword("Redmi9Power","12345678");
			int checkWlanStatus = wlanUdpServerStart((uint8 *)"8000");//this will create a udp server after connecting with wifi AP
			if (checkWlanStatus == -1)
			{
				addTask (USER,START_GATEWAY,1*SECOND);
				return;
			}
			ledOn();
			
			addTask(USER,0xa1,1*SECOND);
		
			break;
		}
		case 0xa1:
		{
		    uint16 gas1;
	        uint16 gas2;

	        //read sensor values
	        gas1=gas11;
	        gas2=gas22;
			sendDataToXAMPP(gas2, gas1, THINK_SPEAK_KEY);
		    // sendDataToThinkSpeakCloud(light,tmp,THINK_SPEAK_KEY);

		    addTask(USER,0xa1,1*SECOND);
		    break;
		}
		case POLL_DATA:                 //POLL_DATA defined in node.h
	    {
	        //ledOn();
	        uint8 check=pollWifiData();
	        if (check==0)
	            addTask(USER,POLL_DATA,2*SECOND);
	        else
	            addTask(USER,0xa2,2);        
	        break;
	    }
		case 0xa2:
		{
		    static uint8 count12=0;
		    if (count12<numStreams)
		    {
		        readWifiData(streamInfo[count12].streamHandle,streamInfo[count12].numBytes);

		        count12++;
		    }
		    else
		    {
		        count12=0;
		        numStreams=0;
		        addTask(USER,POLL_DATA,2*SECOND);
		           
		        return;
		    }
		    
		    addTask(USER,0xa2,2);
		    break;
		}
	}
}

//*******************************************************************************
//When a data packet from network is received, this function gets called from OS.
//*******************************************************************************
void userReceiveDataPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality)
{
    if (payload[0]==USER_PACKET_TYPE)
	{
		gas11=payload[1];
		gas11=gas11<<8;
		gas11=gas11 | payload[2];
		gas22=payload[3];
		gas22=gas22<<8;
		gas22=gas22 | payload[4];
	}
}


//*******************************************************************************
//                  A dio event is received in this function. 
//*******************************************************************************
void userCriticalTaskHandler(uint8 critTaskType)
{
	
}

//*******************************************************************************
//                  A UART0 interrupt is handled in this function. 
//*******************************************************************************
void uart0InterruptHandler()
{

}

//*******************************************************************************
//  this function gets called after device wakes from sleep if it is put to sleep
//*******************************************************************************
void wakeFromSleep()
{

}

