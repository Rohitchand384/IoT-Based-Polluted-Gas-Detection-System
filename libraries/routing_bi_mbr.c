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
#include "string.h"

#include "AppApi.h"
#include "config.h"
#include "mac.h"
#include "routing.h"
#include "routing_bi_mbr.h"
#ifdef BI_MBR
#include "node.h"
#include "task.h"

//testing
#include "pcInterface.h"

#ifdef ENABLE_OTA
#include "ota.h"
#endif

extern macLayerData macData;
extern void userReceiveDataPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality);

//everytime association takes place the node with add a task to update the information of route. 
//If the node was up already, it may have similar periodic task already working. With the help of 
//this flag, we will try to close that periodic task
#ifdef MAX_ROUTING_TABLE_SIZE
BiMBRroutingEntry routingEntry[MAX_ROUTING_TABLE_SIZE];	//maximum MAX_ROUTING_TABLE_SIZE node entries assumed
#else
BiMBRroutingEntry routingEntry[30];	//maximum 30 node entries assumed
#endif
int routingTableSize=0;
uint8 curTaskType=0;

//initialize Bidirectional MBR protocol 
void BiMBRInit()
{
	memset((void*)routingEntry,0,30*sizeof(BiMBRroutingEntry));
}

//to check if the route to destination is available or not
int8 BiMBRisRouteAvail(BiMBRroutingEntry** nextHopEntry,uint16 destAddr,uint8 index)
{
     
	int8 check=-1;
	int  i=index;       //index variable contains from where in the routing table do we want to search for the entry. 
	                    //routing protocol will always send 0 in this, mac layer will always send 1 in this
	                    //this is used to avoid routing loops. The node while association now checks whether the node 
	                    //it is trying to associate to is in the hop above it or below is. 0th entry is always the route
	                    //towards the pan coordinator.

	for(;i<routingTableSize;i++)
	{
		//debug((uint8*)&routingEntry[i].destAddr,2);
		if(routingEntry[i].destAddr == destAddr)
		{
			*nextHopEntry=&(routingEntry[i]);
			return i;
		}
	}

	nextHopEntry=NULL;
	return check;
}


//send data packet by route found by routing protocol
int8 BiMBRsendData(uint8 *data, uint8 len, uint16 destAddr)
{
	uint8 i;
	uint8 payload[127];

	BiMBRroutingEntry* rtEntry=NULL;

	uint8 check =BiMBRisRouteAvail(&rtEntry,destAddr,0);

	if (rtEntry!=NULL)
	{
		//debug((uint8*)&(rtEntry->nextAddr),2);
		payload[0]=BiMBR_PACKET_TYPE;
		payload[1]=destAddr>>8;
		payload[2]=destAddr;

		for (i = 3; i < (len+3); i++)
		{
			payload[i] = *data++;
		}

		sendDataToMac(payload,len+3,rtEntry->nextAddr,check,TRUE);

		return 1;
	}

	return -1;
}


//receive a routing packet or data packet to be relayed
void BiMBRreceiveDataPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality)
{
	uint16 nodeId=getNodeId();
	
	uint16 destAddr=payload[1];
	destAddr=destAddr<<8;
	destAddr=destAddr|payload[2];

	if (nodeId==destAddr)
	{
#ifdef ENABLE_OTA						//if OTA is enabled
		if (payload[3]==0x61)
		{
			receiveOTAPacket(&payload[3],length-3,prevAddr,linkQuality);
		}
		else
#endif
		{
			userReceiveDataPacket(&payload[3],length-3,prevAddr,linkQuality);
		}
	}
	else
	{
		BiMBRroutingEntry* rtEntry=NULL;

		uint8 check=BiMBRisRouteAvail(&rtEntry,destAddr,0);

		//debug((uint8*)rtEntry,sizeof(BiMBRroutingEntry));

		if (rtEntry!=NULL)
		{
			sendDataToMac(payload,length,rtEntry->nextAddr,check,TRUE);
		}
		else
		{
			//packet is getting dropped at intermediate node
		}
	}
}


//receive a routing packet to be relayed
void BiMBRreceiveControlPacket(uint8* payload,uint8 length,uint16 prevAddr, uint8 linkQuality)
{
	uint16 nodeId=macData.myAddr;

	uint16 srcAddr=payload[1];
	srcAddr=srcAddr<<8;
	srcAddr=srcAddr|payload[2];

	BiMBRroutingEntry* rtEntry=NULL;

	BiMBRisRouteAvail(&rtEntry,srcAddr,0);

	if (rtEntry ==NULL)
	{
		if (routingTableSize < 30)
		{
			routingEntry[routingTableSize].destAddr=srcAddr;
			routingEntry[routingTableSize].nextAddr=prevAddr;
			routingEntry[routingTableSize].isActivated=1;

			//debug((uint8*)&routingEntry[routingTableSize],sizeof(routingEntry[routingTableSize]));

			routingTableSize++;

		}
		else
		{
			//cannot fill any more info in routing table. Evasive action required.
			//this situation will never arise but for sanity let us restart this node now.

			vAHI_SwReset();
		}
	}
	else
	{
		rtEntry->nextAddr=prevAddr;
		rtEntry->isActivated=1;
	}

	if (nodeId!=0)
	{
		if (routingTableSize>0)
		{
			sendDataToMac(payload,length,routingEntry[0].nextAddr,0,TRUE);
		}
		else
		{
			//packet is getting dropped at intermediate node
		}
	}
	else
	{
		//debug((uint8*)&srcAddr,2);
		//status packet received
	}
}


//handle route error
void BiMBRhandleError(uint8 u8Handle)
{
     // its a dirty way of handling it. The entry should actually be deleted. To be done if the routing table size 
     // is expected to be big
	//disable the route
	routingEntry[u8Handle].isActivated=0;

	//initialize mac layer again
	if (u8Handle==0)
	     macInit();	
}


//handle tasks placed
void BiMBRroutingTaskHandler(uint8 taskType)
{
	//if the node is still associated do the periodic transfer for the latest tasktype

	if (routingEntry[0].isActivated ==1 && curTaskType == taskType)
	{
		//send routing update packet towards PAN ccordinator
		uint8 packet[10];
		uint16 nodeId=getNodeId();

		packet[0]=BiMBR_ROUTING_UPDATE;
		packet[1]=nodeId>>8;
		packet[2]=nodeId;

		sendDataToMac(packet,3,routingEntry[0].nextAddr,0,TRUE);

		//now let us make this route update process periodic
		uint32 delay=eRand()/2000;
		addTask(ROUTING,curTaskType, ((2*SECOND) + (delay*MILLI_SECOND)));         // if faster routing updates upstream are
		                                                                           // are not required kep the time value high
		                                                                           //at around 60 from 2 in seconds
	}
}

#endif
