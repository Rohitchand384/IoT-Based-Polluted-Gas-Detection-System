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
#include "pcInterface.h"
#include "AppApi.h"
#include "config.h"
#include "nw_param.h"
#include "node.h"
#include "clock.h"
#include "string.h"

extern Time currentTime;

void nwParamInit()
{    
    memset(nwParamTable,0,sizeof(nwParam)*30);
    nwPrmTblSize=0;
}

void insertUpdateNwParamTable(uint16 nodeId)
{
    int i=0;
    int found=0; 
    uint64 curTime=currentTime.clockLow;   
    while (i<nwPrmTblSize)
    {
        if(nwParamTable[i].nodeId==nodeId)
        {
            found=1;
            break;
        }
        i++;
    }
    
    nwParamTable[i].nodeId=nodeId;
    if ( found==0)
        nwParamTable[i].firstPacketTime=curTime;
    nwParamTable[i].lastPacketTime=curTime;
    
    if (nwParamTable[i].prevPacketTime !=0)
    {
    #ifdef PACKET_INTERVAL    
        nwParamTable[i].delay += (curTime-nwParamTable[i].prevPacketTime);        //packet interval to be taken from config.h  
    #else 
         nwParamTable[i].delay += (curTime-nwParamTable[i].prevPacketTime);                     //packet interval assumed to be 1
    #endif 
    } 
    msdelay(100);
    //debug(&curTime,8);
    //debug(&nwParamTable[i].prevPacketTime,8);
    //debug(&nwParamTable[i].delay,8);          
    nwParamTable[i].prevPacketTime=curTime;     
    nwParamTable[i].numPacketsReceived++;  
    
    if (found==0)
        nwPrmTblSize++;     
}

void calValueAndUpdateUI()
{
    int i=0;
    while (i < nwPrmTblSize)
    {
        float th;
        float pdr;
#ifdef PACKET_SIZE    
        th= nwParamTable[i].numPacketsReceived*PACKET_SIZE*8;          //packet Size to be taken from config.h 
#else
        th= nwParamTable[i].numPacketsReceived*6*8;          //packet Size to be taken from config.h 
#endif             
        if (  nwParamTable[i].lastPacketTime > nwParamTable[i].firstPacketTime)         
            th = th*1000.0/(nwParamTable[i].lastPacketTime - nwParamTable[i].firstPacketTime); 
        else 
            th=0;       //throughput cant be calculated if last packet received time is not more than first packet received time  
        
        //debug(& nwParamTable[i].delay,4);    
        float del = nwParamTable[i].delay/(nwParamTable[i].numPacketsReceived*1000.0);
#ifdef TOTAL_PACKET_TO_SEND        
        pdr = (nwParamTable[i].numPacketsReceived*1.0)/TOTAL_PACKETS_TO_SEND;        
#else
         pdr = (nwParamTable[i].numPacketsReceived*1.0)/100;  
#endif

        //send this data to UI
        uint8 packet[15];
        
        packet[0]= 0x78;    //header to be updated in UI
        memcpy(&packet[1],&nwParamTable[i].nodeId,2);
        memcpy(&packet[3],&th,4);
        memcpy(&packet[7],&pdr,4);
        memcpy(&packet[11],&del,4);
        
        updateGendb(packet,15);
        i++;
        msdelay(100);
    }
}


