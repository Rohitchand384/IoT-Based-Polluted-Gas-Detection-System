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
#include "genMac.h"
#include "mac.h"
#include "routing.h"

#ifdef ENABLE_OTA
#include "ota.h"
#endif

//mac layer pib handlers
void *pvMac;
MAC_Pib_s *psPib;

macLayerData macData;

//external link to user defined code.
extern void userReceiveDataPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality);

//functions to handle deferred confirmation from mac and data packets
PRIVATE void handleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void handleMcpsDcfmInd(MAC_McpsDcfmInd_s *psMcpsInd);
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//initialize mac layer
void macInit()
{
    macData.txSeqNo=1;                  //changed
#ifdef FFD_PAN_COORDINATOR
	panCoordinatorInit();
#else
	//initialization process of both RFD and Coordinator is same.
	//once the active scan will be complete, we will then check
	//if a coordinator is to be started or not based on variable 
	//set in config file.

	genMacInit();
#endif	
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//parse the deferred mlme confirmation and take necessary action
void processIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{	
   switch (psMlmeInd->u8Type)
	 {
#ifndef RFD_DEVICE   
		   case MAC_MLME_IND_ASSOCIATE: /* Incoming association request */
		   {
				 handleNodeAssociation(psMlmeInd);
				 break;
		   }
#endif
		   case MAC_MLME_DCFM_SCAN: 
		   { // check the type of scan 
#ifdef FFD_PAN_COORDINATOR
			 //energy detect scan
			   if (psMlmeInd->uParam.sDcfmScan.u8ScanType == MAC_MLME_SCAN_TYPE_ENERGY_DETECT)
			   {
				   handleEnergyScanResponse(psMlmeInd);
			   }
#else
			   if (psMlmeInd->uParam.sDcfmScan.u8ScanType == MAC_MLME_SCAN_TYPE_ACTIVE)
			   {
				   handleActiveScanResponse(psMlmeInd);
			   }

#endif			 
			   break;
		   }
		   
		   case MAC_MLME_DCFM_ASSOCIATE:
		   {
			   handleAssociateResponse(psMlmeInd);
			   break;
		   }
	 
		   default:
			   break;	 
	 }   		   
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//parse the deferred mlme confirmation and take necessary action
void processIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
{
#ifdef RFD_DEVICE	
	if(macData.curState==STATE_ASSOCIATED)
#else
	if(macData.curState==STATE_COORDINATOR_STARTED)
#endif	
	{
		switch(psMcpsInd->u8Type)
		{
		case MAC_MCPS_IND_DATA:  /* Incoming data frame */
			handleMcpsDataInd(psMcpsInd);
			break;
		case MAC_MCPS_DCFM_DATA: /* Incoming acknowledgement or ack timeout */
			handleMcpsDcfmInd(psMcpsInd);
			break;
		default:
			break;
		}
	}
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool sendDataToMac(uint8 *data, uint8 len, uint16 destAddr,uint8 u8Handle,bool isAckReq) //handle helps upper layer to check which packet got dropped
																						 //in case it gets dropped at mac.
{
	if (len>80)			//maximum packet size of 80 to cover the limit of mac of 127 bytes
		return FALSE;
#ifndef FFD_PAN_COORDINATOR
	if(macData.curState==STATE_ASSOCIATED ||macData.curState==STATE_COORDINATOR_STARTED)
#endif
	{
		MAC_McpsReqRsp_s  sMcpsReqRsp;
		MAC_McpsSyncCfm_s sMcpsSyncCfm;
		uint8 *payload, i = 0;
	
		/* Create frame transmission request */
		sMcpsReqRsp.u8Type = MAC_MCPS_REQ_DATA;
		sMcpsReqRsp.u8ParamLength = sizeof(MAC_McpsReqData_s);
		/* Set handle so we can match confirmation to request */
		sMcpsReqRsp.uParam.sReqData.u8Handle = u8Handle; //should be provided from upper layer
		/* Use short address for source */
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u8AddrMode = 2;
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.u16PanId = macData.panId;
		sMcpsReqRsp.uParam.sReqData.sFrame.sSrcAddr.uAddr.u16Short = macData.myAddr;
		/* Use short address for destination */
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u8AddrMode = 2;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.u16PanId = macData.panId;
		sMcpsReqRsp.uParam.sReqData.sFrame.sDstAddr.uAddr.u16Short = destAddr;
		/* Frame requires ack but not security, indirect transmit or GTS */
		if (isAckReq==TRUE)
			sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = MAC_TX_OPTION_ACK;
		else
			sMcpsReqRsp.uParam.sReqData.sFrame.u8TxOptions = 0;

		payload = sMcpsReqRsp.uParam.sReqData.sFrame.au8Sdu;
	
	    //***********************************************
	    //changed
		                                
	
		for (i = 0; i < (len); i++)
		{
			payload[i] = *data++;
		}
		
		payload[i++]=macData.txSeqNo++;
	
		/* Set frame length */
		sMcpsReqRsp.uParam.sReqData.sFrame.u8SduLength = i;
	    //***********************************************
	    
		/* Request transmit */
		vAppApiMcpsRequest(&sMcpsReqRsp, &sMcpsSyncCfm);
		return TRUE;
	}

#ifndef FFD_PAN_COORDINATOR
	return FALSE;
#endif
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void handleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
	MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsInd->uParam.sIndData.sFrame;
    
    //****************************************************************
    //changed
    int i=0;
    //debug("here",4);
    while (i<macData.numEndDevices)
	{
		if (macData.endDeviceData[i].u16ShortAdr==psFrame->sSrcAddr.uAddr.u16Short)			
		{
			break;
		}
		i++;
	}
	
	if (i<macData.numEndDevices)
	{
	    //node found in table
	    if (macData.endDeviceData[i].rxSeqNo==psFrame->au8Sdu[psFrame->u8SduLength-1])      //check seq no
	    {
	        //drop this packet as seq no same as received earlier
	        return;
	    }
	    else
	    {
	        macData.endDeviceData[i].rxSeqNo=psFrame->au8Sdu[psFrame->u8SduLength-1];
	    }
	}
	else
	{
	    //insert entry of the node in the mac table as this is an unknown neighbour
	    if (macData.numEndDevices < MAX_END_DEVICES)
		{
			/* Store end device address data */
			uint8 endDeviceIndex    = macData.numEndDevices;
			
			macData.endDeviceData[endDeviceIndex].u16ShortAdr = psFrame->sSrcAddr.uAddr.u16Short;
            macData.endDeviceData[endDeviceIndex].rxSeqNo=psFrame->au8Sdu[psFrame->u8SduLength-1];
#ifdef ENABLE_OTA
			macData.endDeviceData[endDeviceIndex].otaComplete=0;
			macData.endDeviceData[endDeviceIndex].ackReceived=0;
#endif				
			macData.numEndDevices++;			
		}
	}
	//********************************************************

	/*compare data packet type and check if it belongs to a routing protocol
	or user. A type value present in first byte of packet,greater than 0x23
	signifies data packet.*/

    //currently only packet from same PAN will be accepted.

	//modification made on 21/12/2015
	//a slight change with the introduction of OTA in how the packets are processed by mac 
#ifdef ENABLE_OTA
	//packet with id between 0x45 and 0x47 will be taken as OTA packet
	if ((psFrame->au8Sdu[0]>=0x45 && psFrame->au8Sdu[0]<=0x47) && (psFrame->sDstAddr.u16PanId==macData.panId)			
		&&(psFrame->sDstAddr.uAddr.u16Short==macData.myAddr || psFrame->sDstAddr.uAddr.u16Short==0xFFFF))														//OTA packet can never be broadcasted
	{
		//its a data packet and of same pan.
		receiveOTAPacket(&psFrame->au8Sdu[0],
				(psFrame->u8SduLength-1),psFrame->sSrcAddr.uAddr.u16Short,psFrame->u8LinkQuality);

	}
	else 
#endif
	if ((psFrame->au8Sdu[0]>0x23) && (psFrame->sDstAddr.u16PanId==macData.panId)								//0x23 is a random number
		&&(psFrame->sDstAddr.uAddr.u16Short==macData.myAddr || psFrame->sDstAddr.uAddr.u16Short==0xFFFF))			//0xFFFF represents broadcast address
	{
		//its a data packet and of same pan.
		userReceiveDataPacket(&psFrame->au8Sdu[0],
				(psFrame->u8SduLength-1),psFrame->sSrcAddr.uAddr.u16Short,psFrame->u8LinkQuality);

	}else if((psFrame->sDstAddr.u16PanId==macData.panId) && (macData.myAddr==psFrame->sDstAddr.uAddr.u16Short
															|| psFrame->sDstAddr.uAddr.u16Short==0xFFFF ))
	{
		routingReceivePacket(&psFrame->au8Sdu[0],
				(psFrame->u8SduLength-1),psFrame->sSrcAddr.uAddr.u16Short,psFrame->sDstAddr.uAddr.u16Short,psFrame->u8LinkQuality);

	}
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void handleMcpsDcfmInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
	switch (psMcpsInd->uParam.sDcfmData.u8Status)
	{
		case MAC_ENUM_SUCCESS:
			/* Data frame transmission successful. We could increase the
			   trust level of the node if required here. */
			break;

		case MAC_ENUM_CHANNEL_ACCESS_FAILURE:
			//channel currently not available

			break;
		
		default:
			/* Data transmission failed after 3 tries. We should either 
			   start an orphaned or active scan. Also, we should inform 
			   routing protocol about the same. further routing related tasks
			   could be completed only after the node has associated with a
			   new coordinator.*/
					
			//inform routing protocol.let it take a decision.
			
			routingHandleError(psMcpsInd->uParam.sDcfmData.u8Handle);
			break;
	}
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
