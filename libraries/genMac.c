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
#include <mac_sap.h>
#include <mac_pib.h>
#include "AppApi.h"
#include "config.h"
#include "coordinator.h"
#include "mac.h"
#include "node.h"
#include "dio.h"
#include "pcInterface.h"

#ifdef BI_MBR
#include "routing_bi_mbr.h"
#include "task.h"
#endif

//////////////////////////////////////////////////////////////////

//define variables that will contain required mac related information for this coordinator.
extern void *pvMac;
extern MAC_Pib_s *psPib;
extern macLayerData macData;
//added for debug
extern uint32 timeCounter ;

#ifdef BI_MBR
extern BiMBRroutingEntry routingEntry [MAX_BIMBR_TABLE_SIZE];		
extern int routingTableSize;
extern uint8 curTaskType;
#endif

extern int mlmeMutex;
extern int mlmeMutexCount;
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

//function declarations here
void startActiveScan(void);
void startAssociate(void);

void genMacReset()
{
   startActiveScan();
}

//////////////////////////////////////////////////////////////////


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void startActiveScan(void)
{
#ifdef GEN_MAC_DEBUG 
    //debug ((uint8*)"AAAA", 4);
#endif
    mlmeMutex=1;
    mlmeMutexCount=0;
    MAC_MlmeReqRsp_s  sMlmeReqRsp;
    MAC_MlmeSyncCfm_s sMlmeSyncCfm;

    macData.curState = STATE_ACTIVE_SCANNING;

    /* Request scan */
    sMlmeReqRsp.u8Type = MAC_MLME_REQ_SCAN;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqScan_s);
    sMlmeReqRsp.uParam.sReqScan.u8ScanType = MAC_MLME_SCAN_TYPE_ACTIVE;
    sMlmeReqRsp.uParam.sReqScan.u32ScanChannels = SCAN_CHANNELS;
    sMlmeReqRsp.uParam.sReqScan.u8ScanDuration = SCAN_DURATION;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int mlmeMutex=0;
int mlmeMutexCount=0;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void genMacInit()
{
   //if required, register the callback handlers of mlme and mcps. Currently not used.
	
	MAC_MlmeReqRsp_s  sMlmeReqRsp;
        MAC_MlmeSyncCfm_s sMlmeSyncCfm;
    
        /* Request Reset */
        sMlmeReqRsp.u8Type = MAC_MLME_REQ_RESET;
        sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqReset_s);
        sMlmeReqRsp.uParam.sReqReset.u8SetDefaultPib = FALSE; /* Reset PIB
        */

        vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
        /* Handle synchronous confirm */
       if (sMlmeSyncCfm.u8Status != MAC_MLME_CFM_OK)
       {
        /* Error during MLME-Reset */
       }
#ifndef RFD_DEVICE 
	macData.numEndDevices=0;						
#endif    
    macData.curState=STATE_IDLE;
    macData.myAddr=getNodeId();	
    
    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
	pvMac = pvAppApiGetMacHandle();
	psPib = MAC_psPibGetHandle(pvMac);
	
	/* Enable receiver to be on when idle */
	MAC_vPibSetShortAddr(pvMac, macData.myAddr);
	MAC_vPibSetRxOnWhenIdle(pvMac, TRUE, FALSE);
	
	startActiveScan();
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#ifdef FFD_COORDINATOR
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void handleActiveScanResponse(MAC_MlmeDcfmInd_s *psMlmeInd)
{
        mlmeMutex=0;
        mlmeMutexCount=0;
    	MAC_PanDescr_s *psPanDesc;
    	uint8 i,u8LinkQuality;
    	//setPin(LED);
	if (psMlmeInd->uParam.sDcfmScan.u8Status == MAC_ENUM_SUCCESS)
	{
		i=0;
		while (i < psMlmeInd->uParam.sDcfmScan.u8ResultListSize)
		{
			psPanDesc = &psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[i];
        #ifdef GEN_MAC_DEBUG 
			// debug((uint8*)&psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[i].sCoord.u16PanId,2);
			// debug((uint8*)&psPanDesc->sCoord.uAddr.u16Short,2);
        #endif

			if((psPanDesc->sCoord.uAddr.u16Short == 0)
				#if defined PAN_FIXED != 0x0000
					&&  (psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[i].sCoord.u16PanId==PAN_FIXED)
				#endif

			  // Check it is accepting association requests 
			 && (psPanDesc->u16SuperframeSpec & 0x8000))
			{
			#ifdef GEN_MAC_DEBUG
			 //   debug ((uint8*)"1111",4);
			#endif
				macData.channelId=psPanDesc->u8LogicalChan;
				macData.coordAddr=psPanDesc->sCoord.uAddr.u16Short;
				macData.panId=psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[i].sCoord.u16PanId;
				startAssociate();

				return;
			}
			i++;
		}
		
		i=0;
		u8LinkQuality=0;

		//test
		int flag=0;

#ifdef BI_MBR				
		BiMBRroutingEntry* rtEntry=NULL;
#endif
		
		//if we are not in range of pan coordinator, select the node with 
		//best link quality
		#ifdef FFD_COORDINATOR
			#if defined PAN_FIXED != 0x0000 
				while((i < psMlmeInd->uParam.sDcfmScan.u8ResultListSize)
				&&(psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[i].sCoord.u16PanId==PAN_FIXED))
			#endif
		#else
			while(i < psMlmeInd->uParam.sDcfmScan.u8ResultListSize)
		#endif
		{
			if (((psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[i].u8LinkQuality) > u8LinkQuality))
				{
		     psPanDesc = &psMlmeInd->uParam.sDcfmScan.uList.asPanDescr[i];
#ifdef BI_MBR		     
		     uint8 check =BiMBRisRouteAvail(&rtEntry,psPanDesc->sCoord.uAddr.u16Short, 1);
#endif		  
			if ((psPanDesc->u8LinkQuality) > u8LinkQuality
#ifdef BI_MBR				
			     && rtEntry==NULL
#endif			     
                  )
			{
				u8LinkQuality = (psPanDesc->u8LinkQuality);
				macData.channelId = psPanDesc->u8LogicalChan;
				
				//addrMode should be 2.
				macData.coordAddr=psPanDesc->sCoord.uAddr.u16Short;
				macData.panId=psPanDesc->sCoord.u16PanId;
				flag=1;
			}
			}
			i++;
		}
		
		if (flag)
		{
			startAssociate();
			return;
		}
	}
	startActiveScan();    
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void startAssociate(void)
{
    mlmeMutex=1;
    mlmeMutexCount=0;
    
    MAC_MlmeReqRsp_s  sMlmeReqRsp;
    MAC_MlmeSyncCfm_s sMlmeSyncCfm;

    macData.curState = STATE_ASSOCIATING;

    /* Create associate request. We know short address and PAN ID of
       coordinator as this is preset and we have checked that received
       beacon matched this */

    sMlmeReqRsp.u8Type = MAC_MLME_REQ_ASSOCIATE;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqAssociate_s);
    sMlmeReqRsp.uParam.sReqAssociate.u8LogicalChan = macData.channelId;
#ifdef FFD_COORDINATOR 
    sMlmeReqRsp.uParam.sReqAssociate.u8Capability = 0x80; 		
#else
    sMlmeReqRsp.uParam.sReqAssociate.u8Capability = 0x00;
#endif    

	sMlmeReqRsp.uParam.sReqAssociate.u8SecurityEnable = FALSE;

    sMlmeReqRsp.uParam.sReqAssociate.sCoord.u8AddrMode = 2;
    sMlmeReqRsp.uParam.sReqAssociate.sCoord.u16PanId = macData.panId;
    sMlmeReqRsp.uParam.sReqAssociate.sCoord.uAddr.u16Short =macData.coordAddr ;

    //debug((uint8*)&macData.coordAddr,2);

    /* Put in associate request and check immediate confirm. Should be
       deferred, in which case response is handled by event handler */
    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void handleAssociateResponse(MAC_MlmeDcfmInd_s *psMlmeInd)
{

    mlmeMutex=0;
    mlmeMutexCount=0;
    /* If successfully associated with network coordinator */
	if (macData.curState==STATE_ASSOCIATING)
	{
#ifdef DEBUG_ENABLE
		//debug(&psMlmeInd->uParam.sDcfmAssociate.u8Status,1);
#endif
		if (psMlmeInd->uParam.sDcfmAssociate.u8Status == MAC_ENUM_SUCCESS || psMlmeInd->uParam.sDcfmAssociate.u8Status == 0xDE)
		{
			macData.curState = STATE_ASSOCIATED;
			MAC_vPibSetShortAddr(pvMac,macData.myAddr);
			MAC_vPibSetPanId(pvMac, macData.panId);

#ifdef BI_MBR

		//lets add a task to send the path info to PAN coordinator after 1 second + a random amount of time between 0 and 100 ms. 
			if (routingTableSize==0)
				routingTableSize=1;
			//update routing entry
			routingEntry[0].destAddr=0;
			routingEntry[0].nextAddr=macData.coordAddr;
			routingEntry[0].isActivated=1;
			curTaskType++;
			if (curTaskType == 254)
				curTaskType = 1;
			uint16 delay=eRand()%1000;
		#ifdef QUEUE_ENABLE
			addTask(ROUTING,255, 5*MILLI_SECOND);
		#endif
			//addTask(ROUTING,curTaskType, (SECOND + (delay*MILLI_SECOND)));
			deletePQTask (ROUTING,222);
			addTask(ROUTING, 222, 1000+delay);
			
#endif
			
#ifdef FFD_COORDINATOR			
			psPib->bAssociationPermit = 1;
			startCoordinator();
#endif
		}
		else
		{
			startActiveScan();
		}
	}
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool isAssociated()
{
	if (macData.curState==STATE_ASSOCIATED)
	{
		return TRUE;
	}
	else
		return FALSE;
}
