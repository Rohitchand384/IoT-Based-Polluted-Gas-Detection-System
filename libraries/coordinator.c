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

//////////////////////////////////////////////////////////////////

//define variables that will contain required mac related information for this coordinator.
extern void *pvMac;
extern MAC_Pib_s *psPib;
extern macLayerData macData;

//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

//declare all the functions to be used here
PRIVATE void defCoordReset();
PRIVATE void doEnergyScan();

//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////

//functions definition starts from here

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//define the initialization function of PAN coordinator

void panCoordinatorInit()
{
   //if required, register the callback handlers of mlme and mcps. Currently not used.
   defCoordReset();
   
   doEnergyScan();    //can't use active or passive scan for coordinator. 
                  	  //here coordinator will scan all the channel and will
                      //find the residual energy on them.
   
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//to set default values in mac PIB of a coordinator

PRIVATE void defCoordReset()
{
	//structures used to hold data for MLME request and response 
	
	MAC_MlmeReqRsp_s sMlmeReqRsp;
	MAC_MlmeSyncCfm_s sMlmeSyncCfm;

	uint32 *pu32Mac = pvAppApiGetMacAddrLocation();
	uint16 lowAddress=pu32Mac[1];
	
	//give default value to variables defined in coordinator Data structure
	macData.numEndDevices=0;
	macData.curState=STATE_IDLE;
	macData.channelId=CHANNEL_MIN; 
	macData.panId=lowAddress;
	macData.myAddr=0;
	
	pvMac = pvAppApiGetMacHandle();
	psPib = MAC_psPibGetHandle(pvMac);
	
	//----------------------------reset the mac------------------

	sMlmeReqRsp.u8Type = MAC_MLME_REQ_RESET;
	sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqReset_s);
	sMlmeReqRsp.uParam.sReqReset.u8SetDefaultPib = TRUE;
	
	vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
	
	//-----------------------------------------------------------

	// set Pan ID and short address in PIB to last 16 bits of mac address. This way there won't be any ambiguity
	
	MAC_vPibSetPanId(pvMac, macData.panId);
	MAC_vPibSetShortAddr(pvMac, 0);        //only short address is supported in current version
										   //Pan Coordinator is given address 0.
										   //All other nodes will use their mac id as address.
	
	//Enable receiver to be on when idle
	MAC_vPibSetRxOnWhenIdle(pvMac, TRUE, TRUE);  
	//allow nodes to associate 
	psPib->bAssociationPermit = 1;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
PRIVATE void doEnergyScan()
{
   //structures used to hold data for MLME request and response 
   MAC_MlmeReqRsp_s   sMlmeReqRsp;
   MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

   macData.curState = STATE_ENERGY_SCANNING;

   //start energy detect scan
   sMlmeReqRsp.u8Type = MAC_MLME_REQ_SCAN;
   sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
   sMlmeReqRsp.uParam.sReqScan.u8ScanType = MAC_MLME_SCAN_TYPE_ENERGY_DETECT;

   //the following macros are defined in coordinator.h and their value can be changed there.
   sMlmeReqRsp.uParam.sReqScan.u32ScanChannels = SCAN_CHANNELS;
   sMlmeReqRsp.uParam.sReqScan.u8ScanDuration = SCAN_DURATION;

   vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//go through each channel, select the best 1 and inform mac that scanning is complete
void handleEnergyScanResponse(MAC_MlmeDcfmInd_s *psMlmeInd)
{
	 if (macData.curState == STATE_ENERGY_SCANNING)
	 {
				                      //process energy scan results							 
		uint8 i = 0;
		uint8 u8MinEnergy;
	
		u8MinEnergy = (psMlmeInd->uParam.sDcfmScan.uList.au8EnergyDetect[0]) ;
	
		macData.channelId = CHANNEL_MIN;
	
		/* Search list to find quietest channel */
		while (i < psMlmeInd->uParam.sDcfmScan.u8ResultListSize)
		{
			if ((psMlmeInd->uParam.sDcfmScan.uList.au8EnergyDetect[i]) < u8MinEnergy)
			{
				u8MinEnergy = (psMlmeInd->uParam.sDcfmScan.uList.au8EnergyDetect[i]);
				macData.channelId = i + CHANNEL_MIN;
			}
			i++;
		}
#ifdef STATIC_CHANNEL
		macData.channelId=CHANNEL_MIN; //currently channel no 11 is selected as static channel
#endif
		startCoordinator();
	 }

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//start the coordinator once energy scan/active scan is done
void startCoordinator(void)
{
    /* Structures used to hold data for MLME request and response */
    MAC_MlmeReqRsp_s   sMlmeReqRsp;
    MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

    macData.curState = STATE_COORDINATOR_STARTED;

    /* Start Pan */
    sMlmeReqRsp.u8Type = MAC_MLME_REQ_START;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqStart.u16PanId = macData.panId;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = macData.channelId;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
#ifdef FFD_PAN_COORDINATOR    
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
#else
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = FALSE;
#endif    
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;

	sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;


    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//handle association request sent by nearby node
void  handleNodeAssociation(MAC_MlmeDcfmInd_s *psMlmeInd)
{	
	if (macData.curState == STATE_COORDINATOR_STARTED)
	{
		uint8 i=0,flag=0;
		uint16 endDeviceIndex;

		MAC_MlmeReqRsp_s   sMlmeReqRsp;
		MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

		while (i<macData.numEndDevices)
		{
			if (macData.endDeviceData[i].u32ExtAdrL==psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32L
				&&macData.endDeviceData[i].u32ExtAdrH==psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32H)
			{
				flag=1;
				break;
			}
			i++;
		}

		if (flag==0)
		{
			if (macData.numEndDevices < MAX_END_DEVICES)
			{
				/* Store end device address data */
				endDeviceIndex    = macData.numEndDevices;
				
				//*************************************************
				//changed
				macData.endDeviceData[endDeviceIndex].rxSeqNo=0;
				//*************************************************
				    
				macData.endDeviceData[endDeviceIndex].u16ShortAdr = psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32L;
	            
				macData.endDeviceData[endDeviceIndex].u32ExtAdrL  =
				psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32L;
	
				macData.endDeviceData[endDeviceIndex].u32ExtAdrH  =
				psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32H;
#ifdef ENABLE_OTA
				macData.endDeviceData[endDeviceIndex].otaComplete=0;
				macData.endDeviceData[endDeviceIndex].ackReceived=0;
#endif				
				macData.numEndDevices++;
	                
				sMlmeReqRsp.uParam.sRspAssociate.u8Status = 0; /* Access granted */
			}
			else
			{
				sMlmeReqRsp.uParam.sRspAssociate.u8Status = 2; /* Denied */
			}
		}
		else
			sMlmeReqRsp.uParam.sRspAssociate.u8Status = 0; /* Access granted */

		/* Create association response */
		sMlmeReqRsp.u8Type = MAC_MLME_RSP_ASSOCIATE;
		sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeRspAssociate_s);
		sMlmeReqRsp.uParam.sRspAssociate.sDeviceAddr.u32H = psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32H;
		sMlmeReqRsp.uParam.sRspAssociate.sDeviceAddr.u32L = psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32L;
		sMlmeReqRsp.uParam.sRspAssociate.u16AssocShortAddr = psMlmeInd->uParam.sIndAssociate.sDeviceAddr.u32L;

		sMlmeReqRsp.uParam.sRspAssociate.u8SecurityEnable = FALSE;

		/* Send association response. There is no confirmation for an association
		   response, hence no need to check */
		vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
	
	}
}

