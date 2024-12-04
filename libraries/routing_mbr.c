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

#include "AppApi.h"
#include "config.h"
#include "mac.h"
#include "routing.h"
#include "routing_mbr.h"

extern macLayerData macData;
extern void userReceiveDataPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality);

//to check if the route to destination is available or not
int8 MBRisRouteAvail(uint16* nextHop,uint16 destAddr)
{
	int8 check=-1;
#ifndef	FFD_PAN_COORDINATOR					//no reason why a pan coordinator will send data to itself
	if (macData.curState==STATE_ASSOCIATED||
		macData.curState==STATE_COORDINATOR_STARTED)
	{
		check=1;
		*nextHop=macData.coordAddr;
	}
	else check=0;
#endif	
	return check;
}

//send data packet by route found by routing protocol
int8 MBRsendData(uint8 *data, uint8 len, uint16 destAddr)
{
	uint8 i;
	uint8 payload[127];
	int8 check=-1;
	uint16 nextHop;

	check=MBRisRouteAvail(&nextHop,destAddr);

	if (check==1)
	{
		payload[0]=MBR_PACKET_TYPE;
		for (i = 1; i < (len+1); i++)
		{
			payload[i] = *data++;
		}
		sendDataToMac(payload,len+1,nextHop,0,TRUE);
	}

	return check;
}


//receive a routing packet or data packet to be relayed
void MBRreceiveDataPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality)
{
	uint16 nodeId=macData.myAddr;
	uint16 nextHop;
	int8 check=-1;

	int i;
	uint8 packet[127];

	if (nodeId==0)
	{
		userReceiveDataPacket(&payload[1],length-1,prevAddr,linkQuality);
	}
	else
	{
		check=MBRisRouteAvail(&nextHop,0);

		//*****************************************************************************************
		//testing for multihop tree.
		//The number of intermediate nodes must be such that packet size never goes beyond 80 bytes.

		for (i=0;i<length;i++)
		{
			packet[i]=payload[i];
		}


		packet[length]=nodeId>>8;
		packet[length+1]=nodeId;

		if (check==1)
		{
			sendDataToMac(packet,length+2,nextHop,0,TRUE);
		}
		else
		{
			//packet is getting dropped at intermediate node
		}
		//******************************************************************************************
	}
}

//handle route error
void MBRhandleError()
{
	vAHI_SwReset();
}
