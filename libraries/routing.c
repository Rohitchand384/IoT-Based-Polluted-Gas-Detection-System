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
#include "routing.h"
#include "routing_mbr.h"
#include "routing_bi_mbr.h"
#include "routing_lbr.h"
#include "routing_aodv.h"
#include "task.h"

//to initialize routing protocol
void routingInit()
{
#ifdef LEVEL_BASED_ROUTING
	LBRInit();
#elif defined AODV
	AODVInit();
#elif defined BI_MBR
	BiMBRInit();
#endif
}

//send data packet by route found by routing protocol
int8 routingSendData(uint8 *data, uint8 len, uint16 destAddr)
{
	int8 check;
#ifdef MAC_BASED_ROUTING	
	check=MBRsendData(data,len,destAddr);
#elif defined LEVEL_BASED_ROUTING
	check=LBRsendData(data,len,destAddr);
#elif defined AODV
	check=AODVsendData(data,len,destAddr);
#elif defined BI_MBR
	check=BiMBRsendData(data,len,destAddr);
#endif	
	return check;
}


//receive a routing packet or data packet to be relayed
void routingReceivePacket(uint8* payload,uint8 length,uint16 prevAddr, uint16 curAddr,uint8 linkQuality)
{
//check which routing protocol is in use in current implementation
#ifdef MAC_BASED_ROUTING	
	/* in mac based routing only data packet exists. there are no control packets. so no need to check
	   the type of packet received.*/
	MBRreceiveDataPacket(payload,length,prevAddr,linkQuality);
#elif defined LEVEL_BASED_ROUTING
	/*two types of packets exist in this. setup packet and data packet. check which one is being received right now.
	 */
	if (payload[0]==LBR_SETUP_BROADCAST)
	{
		LBRreceiveControlPacket(payload,length,prevAddr,linkQuality);
	}
	else if (payload[0]==LBR_DATA_PACKET)
	{
		LBRreceiveDataPacket(payload,length,prevAddr,linkQuality);
	}
#elif defined AODV

	if (payload[0]==AODV_DATA)
	{
		AODVreceiveDataPacket(payload,length,prevAddr,linkQuality);
	}
	else
	{
		AODVreceiveControlPacket(payload,length,prevAddr,linkQuality);
	}
#elif defined BI_MBR
	if (payload[0]==BiMBR_PACKET_TYPE)
	{
		BiMBRreceiveDataPacket(payload,length,prevAddr,linkQuality);
	}
	else
	{
		BiMBRreceiveControlPacket(payload,length,prevAddr,linkQuality);
	}
#endif
}


/*function called when a routing task posted by LBR earlier expires.*/
void routingTaskHandler(taskInt taskType)
{
#ifdef LEVEL_BASED_ROUTING
	LBRroutingTaskHandler(taskType);
#elif defined AODV
	AODVroutingTaskHandler(taskType);
#elif defined BI_MBR
	BiMBRroutingTaskHandler(taskType);
#endif
}

//handle packet loss condition informed by  mac
void routingHandleError(uint8 u8Handle)
{
	//ledOn();
#ifdef MAC_BASED_ROUTING	
	MBRhandleError();
#elif defined LEVEL_BASED_ROUTING
	LBRhandleError(u8Handle);
#elif defined BI_MBR
	BiMBRhandleError(u8Handle);
#elif defined AODV
	AODVhandleError(u8Handle);
#else
	vAHI_SwReset();
#endif
}
