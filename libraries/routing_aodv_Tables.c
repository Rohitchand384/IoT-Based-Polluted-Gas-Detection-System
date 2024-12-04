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

#include "AppApi.h"
#include "config.h"
#include "mac.h"
#include "routing.h"
#include "routing_aodv.h"
#include "routing_aodv_Tables.h"
#include "task.h"
#include "node.h"
#include "pcInterface.h"

extern AodvInfo aodvInfo;
/****************************************************************************
 ******************* Routing table related functions ************************
 ***************************************************************************/
//insert/update an entry in the routing table
void AodvInsertUpdateRoute(uint32 destSeq,
						   uint32 lifeTime,
						   uint16 destAddr,
						   uint16 nextHop,
						   uint8 hopCount)
{
	static uint8 sNo=1;
	bool isAvail=FALSE;

	aodvRouteEntry entry;
	aodvRouteEntry* foundRtEntry=NULL;

	uint8 routingTableSize = aodvInfo.routingTableSize;

	if (routingTableSize==AODV_MAX_ENTRIES)
		return;

	aodvRouteEntry* rtEntry=&(aodvInfo.routingEntry[routingTableSize]);

	isAvail=AodvGetRouteEntry(destAddr,&foundRtEntry);

	addTimeInCurrent(&(entry.lifetime),lifeTime);
	entry.destAddr=destAddr;
	entry.seqNo=destSeq;
	entry.nextHopAddr=nextHop;
	entry.hopCount=hopCount;
	entry.activated=1;
	entry.handle=sNo++;
	
	if (entry.handle>30)
	    entry.handle=1;
	    
	if (isAvail)
	{
		if (!foundRtEntry->activated)
		{
			taskInt tasktype=entry.handle<<3|AODV_TASK_ROUTE_EXPIRE;	/*in tasktype, currently first five bits contain handle and last 3 contain task type */
			addTask(ROUTING,tasktype,lifeTime);
		}
		memcpy(foundRtEntry,&entry,sizeof(aodvRouteEntry));

		//debug((uint8*)foundRtEntry,sizeof(aodvRouteEntry));

	}
	else
	{
		taskInt tasktype=entry.handle<<3|AODV_TASK_ROUTE_EXPIRE;	/*in tasktype, currently first five bits contain handle and last 3 contain task type */
		addTask(ROUTING,tasktype,lifeTime);
		routingTableSize++;
		memcpy(rtEntry,&entry,sizeof(aodvRouteEntry));
		memcpy(&(aodvInfo.routingTableSize),&routingTableSize,1);

		//debug((uint8*)rtEntry,sizeof(aodvRouteEntry));
		//debug(&(aodvInfo.routingTableSize),1);
	}
}

//look for the entry of an address in routing table
bool AodvisRouteAvail(uint16 destAddr,
					  aodvRouteEntry** foundRtEntry)
{	
	int i=0;
	uint8 routingTableSize=aodvInfo.routingTableSize;

	while( i<routingTableSize && destAddr!=aodvInfo.routingEntry[i].destAddr)
	{
		//debug((uint8*)&(aodvInfo.routingEntry[i]),sizeof(aodvRouteEntry));
		msdelay(10);
		i++;
	}

	if(i==routingTableSize || aodvInfo.routingEntry[i].activated==0)
	{
		*foundRtEntry=NULL;
		return FALSE;
	}
	else
	{
		*foundRtEntry=&(aodvInfo.routingEntry[i]);
		return TRUE;
	}
}


//look for the entry of an address in routing table even if the route is not valid right now
bool AodvGetRouteEntry(uint16 destAddr,
					 aodvRouteEntry** foundRtEntry)
{
	int i=0;
	uint8 routingTableSize=aodvInfo.routingTableSize;

	while( i<routingTableSize && destAddr!=aodvInfo.routingEntry[i].destAddr)
	{
		i++;
	}

	if(i==routingTableSize)
	{
		*foundRtEntry=NULL;
		return FALSE;
	}
	else
	{
		*foundRtEntry=&(aodvInfo.routingEntry[i]);
		return TRUE;
	}
}


//delete an entry from the routing table after delete period
void AodvDeleteRouteEntry(int index)
{
    //delete the route
	uint8 routingTableSize=aodvInfo.routingTableSize;

	if (routingTableSize==1)
	{
		memset(&(aodvInfo.routingEntry[0]),0,sizeof(aodvRouteEntry));
	}
	else
	{
		while (index+1<routingTableSize)
		{
			aodvInfo.routingEntry[index].lifetime.clockLow=aodvInfo.routingEntry[index+1].lifetime.clockLow;
			aodvInfo.routingEntry[index].lifetime.clockHigh=aodvInfo.routingEntry[index+1].lifetime.clockHigh;
			aodvInfo.routingEntry[index].destAddr=aodvInfo.routingEntry[index+1].destAddr;
			aodvInfo.routingEntry[index].seqNo=aodvInfo.routingEntry[index+1].seqNo;
			aodvInfo.routingEntry[index].nextHopAddr=aodvInfo.routingEntry[index+1].nextHopAddr;
			aodvInfo.routingEntry[index].hopCount=aodvInfo.routingEntry[index+1].hopCount;
			aodvInfo.routingEntry[index].activated=aodvInfo.routingEntry[index+1].activated;

			aodvInfo.routingEntry[index].handle=aodvInfo.routingEntry[index+1].handle;

			index++;
		}
	}

	routingTableSize--;
	memcpy(&(aodvInfo.routingTableSize),&routingTableSize,1);
}


/****************************************************************************
 ******************* RREQ Handled table related functions *******************
 ***************************************************************************/
//insert/update an entry in the rreq handle table
void AodvInsertRreqHandledEntry(uint32 rreq_Id,
								uint16 srcAddr)
{
	static uint8 sNo=1;

	aodvRreqHandledEntry entry;

	uint8 rhTableSize=aodvInfo.rreqHandledTableSize;

	entry.rreq_Id=rreq_Id;
	entry.srcAddr=srcAddr;
	entry.handle=sNo++;
	
	if (entry.handle>30)
	    entry.handle=1;

	if (rhTableSize==AODV_MAX_ENTRIES)
		return;

	aodvRreqHandledEntry* rhEntry=&(aodvInfo.rreqHandledEntry[rhTableSize]);

	taskInt tasktype=(entry.handle<<3)|AODV_TASK_RHE_DELETE;	/*in tasktype, index in rreq handled of current entry has to be given together with task type.
		    		 	 	 	 	 	 	 	 	 	 	  	  	  currently first five bits contain index and last 3 contain task type */
	addTask(ROUTING,tasktype,2*1000);   //node will not accept similar request for 2 seconds 

	rhTableSize++;

	memcpy(rhEntry,&entry,sizeof(aodvRreqHandledEntry));
	memcpy(&(aodvInfo.rreqHandledTableSize),&rhTableSize,1);
}


//look for the entry of an address in rreq handled table
bool AodvFindInRreqHandleTable(uint16 srcAddr,
							   uint32 rreq_Id)
{
	int i=0;
	uint8 rhTableSize=aodvInfo.rreqHandledTableSize;

	while( i<rhTableSize)
	{
		if (srcAddr==aodvInfo.rreqHandledEntry[i].srcAddr
			&& aodvInfo.rreqHandledEntry[i].rreq_Id==rreq_Id)
		{
			//ledOn();
			return TRUE;
		}

		i++;
	}

	return FALSE;
}


//delete an entry from the rreq Handled table after delete period
void AodvDeleteRreqHandledEntry(int index)
{
    //delete the entry
	uint8 rhTableSize=aodvInfo.rreqHandledTableSize;

	if(rhTableSize==1)
	{
		memset(&(aodvInfo.rreqHandledEntry[0]),0,sizeof(aodvRreqHandledEntry));
	}
	else
	{
		while (index+1<rhTableSize)
		{
			aodvInfo.rreqHandledEntry[index].srcAddr=aodvInfo.rreqHandledEntry[index+1].srcAddr;
			aodvInfo.rreqHandledEntry[index].rreq_Id=aodvInfo.rreqHandledEntry[index+1].rreq_Id;
			aodvInfo.rreqHandledEntry[index].handle=aodvInfo.rreqHandledEntry[index+1].handle;

			index++;
		}
	}
	rhTableSize--;
	memcpy(&(aodvInfo.rreqHandledTableSize),&rhTableSize,1);
}


/****************************************************************************
 ******************* Rreq Sent table related functions **********************
 ***************************************************************************/
//insert/update an entry in the sent table
int8 AodvInsertRreqSentTable(uint16 destAddr,
							uint8 ttl,
							uint8 times)
{
	static uint8 sNo=1;

	aodvRreqSentEntry entry;

	uint8 rsTableSize=aodvInfo.rreqSentTableSize;

	if (rsTableSize==AODV_MAX_ENTRIES)
		return -1;

	aodvRreqSentEntry* rsEntry=&(aodvInfo.rreqSentEntry[rsTableSize]);

	entry.ttl=ttl;
	entry.times=times;
	entry.destAddr=destAddr;
	entry.replyReceived=0;
	entry.handle=sNo++;
	
	if (entry.handle>30)
	    entry.handle=1;


	rsTableSize++;

	memcpy(rsEntry,&entry,sizeof(aodvRreqSentEntry));

    memcpy(&(aodvInfo.rreqSentTableSize),&rsTableSize,1);
    return entry.handle;
}


//look for the entry of an address in sent table
bool AodvFindInRreqSentTable(uint16 destAddr,
							aodvRreqSentEntry** foundRreqStEntry)
{
	int i=0;

	uint8 rsTableSize=aodvInfo.rreqSentTableSize;

	while( i<rsTableSize && destAddr!=aodvInfo.rreqSentEntry[i].destAddr)
	{
		i++;
	}

	if (i==rsTableSize)
	{
		*foundRreqStEntry=NULL;
		return FALSE;
	}
	else
	{
		*foundRreqStEntry=&(aodvInfo.rreqSentEntry[i]);
		return TRUE;
	}
}



//delete an entry from the table
void AodvDeleteRreqSentEntry(int index)
{
    //delete the entry

	uint8 rsTableSize=aodvInfo.rreqSentTableSize;

	if(rsTableSize==1)
	{
		memset(&(aodvInfo.rreqSentEntry[0]),0,sizeof(aodvRreqSentEntry));
	}
	else
	{
		while (index+1<rsTableSize)
		{
			aodvInfo.rreqSentEntry[index].destAddr=aodvInfo.rreqSentEntry[index+1].destAddr;
			aodvInfo.rreqSentEntry[index].times=aodvInfo.rreqSentEntry[index+1].times;
			aodvInfo.rreqSentEntry[index].ttl=aodvInfo.rreqSentEntry[index+1].ttl;
			aodvInfo.rreqSentEntry[index].handle=aodvInfo.rreqSentEntry[index+1].handle;

			index++;
		}
	}
	rsTableSize--;
	memcpy(&(aodvInfo.rreqSentTableSize),&rsTableSize,1);
}


/****************************************************************************
 ******************* blacklist table related functions **********************
 ***************************************************************************/
//insert/update an entry in the routing table
void AodvInsertBlacklistTable(uint16 destAddr)
{
	static uint8 sNo=1;

	aodvRreqSentEntry entry;

	uint8 blacklistTableSize=aodvInfo.blacklistTableSize;

	if (blacklistTableSize==AODV_MAX_ENTRIES)
		return;

	aodvBlacklistEntry* blEntry=&(aodvInfo.blacklistEntry[blacklistTableSize]);

	entry.destAddr=destAddr;
	entry.handle=sNo++;
	
	if (entry.handle>30)
	    entry.handle=1;

	blacklistTableSize++;

	memcpy(blEntry,&entry,sizeof(aodvBlacklistEntry));
	memcpy(&(aodvInfo.blacklistTableSize),&blacklistTableSize,1);

	taskInt tasktype=(entry.handle<<3)|AODV_TASK_BLE_DELETE;	/*in tasktype, index in rreq handled of current entry has to be given together with task type.			    		    		 	 	 	 	 	 	 	 	 	 	  	  	  currently first five bits contain index and last 3 contain task type */
	addTask(ROUTING,tasktype,BLACKLIST_TIMEOUT);
}


//look for the entry of an address in routing table
bool AodvFindInBlacklistTable(uint16 destAddr)
{
	int i=0;

	uint8 blacklistTableSize=aodvInfo.blacklistTableSize;

	while( i<blacklistTableSize && destAddr!=aodvInfo.blacklistEntry[i].destAddr)
	{
		i++;
	}

	if (i==blacklistTableSize)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}


//delete an entry from the routing table after delete period
void AodvDeleteBlacklistEntry(int index)
{
    //delete the entry
	uint8 blacklistTableSize=aodvInfo.blacklistTableSize;

	if(blacklistTableSize==1)
	{
		memset(&(aodvInfo.blacklistEntry[0]),0,sizeof(aodvBlacklistEntry));
	}
	else
	{
		while (index+1<blacklistTableSize)
		{
			aodvInfo.blacklistEntry[index].destAddr=aodvInfo.blacklistEntry[index+1].destAddr;
			aodvInfo.blacklistEntry[index].handle=aodvInfo.blacklistEntry[index+1].handle;

			index++;
		}
	}

	blacklistTableSize--;
	memcpy(&(aodvInfo.blacklistTableSize),&blacklistTableSize,1);
}
