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

#include "stdlib.h"
#include "string.h"

#include <jendefs.h>
#include <AppHardwareApi.h>

#include "AppApi.h"
#include "config.h"
#include "mac.h"
#include "node.h"
#include "routing.h"
#include "routing_lbr.h"
#include "task.h"

#include "dio.h"

//if mbr support is there, include the header file
#ifdef MBR_SUPPORT
#include"routing_mbr.h"
#endif

//define the possible task types for this protocol
#define SEND_SETUP_PACKET 0
#define CONNECTIVITY_CHK 1

//define a variable of data type LBRinfo to contain LBR related info for a node.
static LBRinfo lbrInfo;
uint8 linkUpdateChk=0x01;
//below two statements must be included in all routing protocols
extern macLayerData macData;
extern void userReceiveDataPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality);

uint8 connectivityChk=0;
/*to initialize LBR routing protocol. This routing protocol can be used
in the cases when sink is always PAN coordinator*/
void LBRInit()
{
	/*during initialization in LBR, PAN coordinator will set a timer to broadcast
	  a route setup packet.	*/

#ifdef FFD_PAN_COORDINATOR
	lbrInfo.isLevelSet=TRUE;
	lbrInfo.level=0;
	/*this task must be set in such a way that the LBR setup broadcast is done
	  only after the mac setup is complete. Here we are assuming that mac will 
	  setup in 60 secs.                                                                                                                                                                              	  setup in 60 seconds. This time value can be set according to users requiremen.
	*/
	addTask(ROUTING,SEND_SETUP_PACKET,6*SECOND);
	//ledOn();			for testing
#else
	lbrInfo.isLevelSet=FALSE;
	lbrInfo.level=0;
	addTask(ROUTING,CONNECTIVITY_CHK,35000);
#endif

    lbrInfo.seqNo=0;
	lbrInfo.size=0;
	lbrInfo.index=0;
	lbrInfo.failure=0;
	memset(lbrInfo.prevHop,0,8);
}

//to check if the route to destination is available or not
int8 LBRisRouteAvail(uint16* nextHop,uint16 destAddr)
{
	int8 check=-1;

#ifdef MBR_SUPPORT
	if (lbrInfo.failure==1)
	{
		check=MBRisRouteAvail(nextHop,destAddr);
		return check;
	}
#endif

#ifndef	FFD_PAN_COORDINATOR					//no reason why a pan coordinator will send data to itself
	if (lbrInfo.isLevelSet==TRUE && lbrInfo.size!=0)
	{
		check=1;
		*nextHop=lbrInfo.prevHop[lbrInfo.index];
	}
#endif	

	return check;
}

//send data packet by route found by routing protocol
int8 LBRsendData(uint8 *data, uint8 len, uint16 destAddr)
{

#ifdef MBR_SUPPORT
	if (lbrInfo.failure==1)
	{
		MBRsendData(data,len,destAddr);
		return 1;
	}
#endif

	uint8 i;
	uint8 payload[127];
	int8 check=-1;
	uint16 nextHop;

	//check if path  exists
	check=LBRisRouteAvail(&nextHop,destAddr);

	if (check==1)
	{
		//path available
		payload[0]=LBR_DATA_PACKET;
		for (i = 1; i < (len+1); i++)
		{
			payload[i] = *data++;
		}
		sendDataToMac(payload,len+1,nextHop,lbrInfo.index,TRUE);
		lbrInfo.index++;
		if(lbrInfo.index==lbrInfo.size)
			lbrInfo.index=0;
	}
	return check;
}


/*function called when a routing task posted by LBR earlier expires.*/
void LBRroutingTaskHandler(uint8 taskType)
{
	if (taskType==SEND_SETUP_PACKET)
	{
		//pan coordinator or coordinator is supposed to broadcast its information now.
		uint8 lbrPacket[3];
		uint8 check;
		lbrPacket[0]=LBR_SETUP_BROADCAST;
		lbrPacket[1]=lbrInfo.level; //level of PAN Coordinator is set to 0.
		//if it is pan coordinator, update seq no and then add it in the packet
        if(lbrInfo.isLevelSet==TRUE&&lbrInfo.level==0)
            lbrPacket[2]=++lbrInfo.seqNo;
        else
            lbrPacket[2]=lbrInfo.seqNo;    

		check=sendDataToMac(lbrPacket, 3,0xFFFF,255,FALSE);	//16 bit broadcast address with no ack req. Mac ack should be disabled
															//when broadcast is done.

		if (check==FALSE)
		{
			/*do something to let user know that broadcast failed as the
			  node is not associated with any pan coordinator right now */
		}
		
		//if it is pan coordinator, send the packet again after 10 seconds
		if(lbrInfo.isLevelSet==TRUE&&lbrInfo.level==0)
		    addTask(ROUTING,SEND_SETUP_PACKET,10*SECOND);
	}
	#ifdef FFD_COORDINATOR 								//added on 20/feb/2020
	else if(taskType==CONNECTIVITY_CHK)
	{
		if(connectivityChk==0)
		{
			//connectivity lost
			vAHI_SwReset();
		}
		else
		{
			connectivityChk=0;
		}
		addTask(ROUTING,CONNECTIVITY_CHK,35000);
	}
	#endif
}


//receive a routing packet or data packet to be relayed
void LBRreceiveDataPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality)
{
#ifdef MBR_SUPPORT
	if (lbrInfo.failure==1)
	{
		MBRreceiveDataPacket(payload,length,prevAddr,linkQuality);
		return;
	}
#endif
	uint8 check;
	uint16 nextHop;
	uint16 nodeId=macData.myAddr;


	if (nodeId==0)
	{
		//if this is pan coordinator, hand over the packet to application layer
		userReceiveDataPacket(&payload[1],length-1,prevAddr,linkQuality);
	}
	else
	{
		//forward the packet
		check=LBRisRouteAvail(&nextHop,0);

		if (check==1)
		{
			//route exists
			uint8 packet[127];
			uint16 nodeId=macData.myAddr;
			int i;
			for (i=0;i<length;i++)
		    {
			    packet[i]=payload[i];
		    }

		    packet[length]=nodeId>>8;       //for routing path
		    packet[length+1]=nodeId;

			sendDataToMac(packet,length+2,nextHop,255,TRUE);    //next hop will not be deleted if the packet gets dropped while relaying data for some other node
			//increment index to enable multi path transfer. next indexed path will be taken by packet next time, if more than 1 path exists
			lbrInfo.index++;
			if(lbrInfo.index==lbrInfo.size)
				lbrInfo.index=0;
		}
		else
		{
			//something terribly went wrong.
			vAHI_SwReset();
		}
	}

}

//receive a routing packet or data packet to be relayed
void LBRreceiveControlPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality)
{
	//only setup packet can be received here, but still a sanity check
	if (payload[0]==LBR_SETUP_BROADCAST)
	{
		//ledOff();					//for testing
		if(linkQuality>=linkUpdateChk)
		{
			connectivityChk=1;
			if (lbrInfo.isLevelSet==FALSE || lbrInfo.seqNo!=payload[2])
			{
				// first setup packet received in a round, whole routing table is reset in every round
				lbrInfo.level=payload[1]+1;		//set self level= previous hop level + 1
				lbrInfo.isLevelSet=TRUE;
				lbrInfo.size=1;
				lbrInfo.index=0;
				//update seq no as present in the packet
				lbrInfo.seqNo=payload[2];
				lbrInfo.prevHop[0]=prevAddr;
				lbrInfo.failure=0;

				#ifndef RFD_DEVICE
				//generate a random number
				uint16 delay=eRand()%200;
				addTask(ROUTING,SEND_SETUP_PACKET,delay*2*MILLI_SECOND); //2 milli second clock is working in the device.
				#endif
			}
			else if (lbrInfo.size<4 && lbrInfo.level==payload[1]+1 &&lbrInfo.seqNo==payload[2])
			{
				// level already set, if packet coming from previous level, add details in table for enabling multi path
				lbrInfo.prevHop[lbrInfo.size]=prevAddr;
				lbrInfo.size++;
			}
		}
	}
}


//handle route error
void LBRhandleError(uint8 u8Handle)
{
	//if this error is for broadcast(handle sent in broadcast is 255), do nothing
	if(u8Handle==255)
		return;

#ifdef MBR_SUPPORT
	if (lbrInfo.failure==1)
	{
		MBRhandleError();
		return;
	}
#endif
	//remove the entry of the next hop towards sink which has dropped the current packet.
	int i=u8Handle;
	
	if (lbrInfo.size==1)
	{
	  /* lbrInfo.size=0; 
	   lbrInfo.index=0;
	   lbrInfo.isLevelSet=FALSE;
	   lbrInfo.failure=1;
	   macInit();
	   
	   return;*/
		vAHI_SwReset();
	}
	
	do
	{
		lbrInfo.prevHop[i]=lbrInfo.prevHop[i+1];
	}while (i>lbrInfo.size-1);

	lbrInfo.size--;
	
	if (lbrInfo.index==lbrInfo.size)
		lbrInfo.index=0;
}

uint8 getLevel()
{
	return lbrInfo.level;
}

void updateLinkQuality(uint8 linkChk)
{
	linkUpdateChk=linkChk;
}