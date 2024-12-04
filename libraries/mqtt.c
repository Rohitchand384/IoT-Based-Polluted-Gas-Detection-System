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


#include "config.h"

#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include "clock.h"  						//provides clock related functionality
#include "node.h"
#include "task.h"
#include "string.h"
#include "util.h"
//#include "mqttClient.h"

///////////////////////////////////////////////
// Function to calculate mqtt format remaining length
// Input -- Total Pkt length and pointer to buffer where we want to fill data
// Return -- size in number of bytes
//
///////////////////////////////////////////////

uint8 calculateRemainingMQTTPacketLen (uint32 mqttPktRemLen, uint8 *pkt)
{
    uint8 byteCount=0;
    do{
        uint8 encodeByte = mqttPktRemLen%128;
        mqttPktRemLen = mqttPktRemLen/128;
        if (mqttPktRemLen>0)
            encodeByte = encodeByte | 128;
        pkt[byteCount] = encodeByte;
        byteCount ++; 
    }while (mqttPktRemLen>0);
    return byteCount;
}



///////////////////////////////////////////////
// mqtt packet is like we have maximum length is 4
// pkt [0], pkt [1], pkt [2], pkt [3]
// Input is -- Pointer to pkt and its index from where new data is available
// Retun -- Remaining Packet length
//
///////////////////////////////////////////////

uint32 calculateReverseMQTTPktLen (uint8 *pkt, uint8 *pktIndex)
{
    uint8 i=0;
    uint32 multiplier=1;
    uint32 mqttRemainingPktLenReverse = 0;
    for (i=0; i<4; i++)
    {
        mqttRemainingPktLenReverse += ((pkt[i] & 0x7F)*multiplier); 
        if ((pkt[i] & 0x80) != 0x80)
        {
            *pktIndex += (i+1);
            return mqttRemainingPktLenReverse;
        }
        else
            multiplier = multiplier*128;
    }
    return 0;
}

///////////////////////////////////////////
// Function to send connect packet to network
// Input -- Nothing
// Return -- Nothing
//
// Comment -- This fumction must be updated / fully dynamic will do it with ethernet
///////////////////////////////////////////

void createConnectReqPktToBroker(uint8* req, uint32* finalLength,uint8* username,uint8* password,uint32 *index)
{
    uint32 pktSize=5; 
    
       
    //Protocol Name length
    req[pktSize]=0x00;
    pktSize++;
    req[pktSize]=0x04;
    pktSize++;
    
    //Protocol Name MQTT
    req[pktSize]=0x4D;
    pktSize++;
    req[pktSize]=0x51;
    pktSize++;
    req[pktSize]=0x54;
    pktSize++;
    req[pktSize]=0x54;
    pktSize++;
    
    //Protocol level 0x04 now
    req[pktSize]=0x04;
    pktSize++;
    
    //Connect Flag bits here
    /////////////////////////////////////////////////////////////////////////////////////////
    //                                                                                     //
    // USER_NMAE+PASSWORD+WILL_RETAIN+[WILL_OoS+WILL_QoS]+WILL_FLAG+CLEAN_SESSION+RESERVED //
    //                                                                                     //
    /////////////////////////////////////////////////////////////////////////////////////////
    // 11000010
    req[pktSize]=0xc2;
    pktSize++;
    
    //Keep Alive period
    req[pktSize]=0xFF;
    pktSize++;
    req[pktSize]=0xFF;
    pktSize++;
    
    //client identifier
    uint8 myMacId [8];  
    uint64 mA=getMacAddress();
    
    memcpy(myMacId,(uint8*)&mA,8);

    req[pktSize]=0x00;
    pktSize++;
    req[pktSize]=16;            //Client Id len
    pktSize++;
    
    hexToStrConverter (myMacId, &req[pktSize], 8);
    pktSize +=16;
    ///////////////////////////////////////////
    //Copy user name here
    req[pktSize] =0;
    pktSize++;
    req[pktSize]=strlen ((const char*)username);
    pktSize++;
    memcpy(&req[pktSize], username, strlen ((const char*)username));
    pktSize += strlen ((const char*)username);
    
    //Copy Password here
    req[pktSize] =0;
    pktSize++;
    req[pktSize]=strlen ((const char*)password);
    pktSize++;
    memcpy(&req[pktSize], password, strlen ((const char*)password));
    pktSize += strlen ((const char*)password);
    
    
    ////////////////////////////////////////////////
    // Let's handle mqtt pkt remaining length and header
    uint8 mqttHeaderPkt [5];
    uint8 mqttHeaderPktLen=0;
    mqttHeaderPkt [0] = 0x10;
    mqttHeaderPktLen ++;
    uint32 effectiveMqttPktRemainingLen = pktSize - 5;
    mqttHeaderPktLen += calculateRemainingMQTTPacketLen (effectiveMqttPktRemainingLen, &mqttHeaderPkt [mqttHeaderPktLen]);
    uint32 effectiveMqttPktIndex = 5-mqttHeaderPktLen;
    memcpy (&req[effectiveMqttPktIndex], mqttHeaderPkt, mqttHeaderPktLen);
    effectiveMqttPktRemainingLen += mqttHeaderPktLen;
    
    memcpy(finalLength,&effectiveMqttPktRemainingLen,4);
    memcpy(index,&effectiveMqttPktIndex,4);
}


//////////////////////////////////////////
// Function to parse received data from module in mqtt format
// Input -- Nothing
// Return -- Nothing
//
//////////////////////////////////////////


int8 parseMqttResponsePkt (uint8 *data, uint16 dataPktSize,uint8 mqttState,uint8* remainPkt)
{
    //#ifdef DEBUG_ENABLE_UART_1
    //#endif
    
    switch (mqttState)
    {
        case 0://0x20:
        {
            //currently the mqtt connect is expected
            if ((data[0]==0x20) && (data[1] == 2) && (data[2] == 0) && (data[3] == 0))
            {
                //this is a connect request ack
                return 0x20;
            }
            break;
        }
        case 1:
        {
            if (data[0]==0x30)
            {
                //Publish received
                return 0x30;
            }
            else
                return -1;
            break;
        }
        case 2:
        {
            if (data[0]==0x90)
            {   
                if((data[0]^data[1]^data[2]) == data[3])
                {
                    return 0x90;
                }
                return -1;
            }
            else
                return -1;
            break;
        }

        default:
        {
            return -1;
        }
        /*case 0x90:
        {
            //90 04 00 01 00 00
            if (((data[1] == 4) ||(data[1] == 5)) && (data[2] == 0) && (data[3] == 1) && (data[4] == 0) && (data[5] == 0))
            {
                //uartSerialWrite((uint8*)"Sub Success",strlen ("Sub Success"));
                #if (DEVICE_TYPE == WIFI_GATEWAY)
                addTask (USER, MQTT_PUB_PKT, 5*SECOND);
                #endif
            }
            break;
        }
        case 0x30:
        {
            //uartSerialWrite(data, 10);
            //msdelay (10);
            //uartSerialWrite((uint8*)"Cmd Received",strlen ("Cmd Received"));
            //301700052F74696D655B30323232313631373031303731325D
            //parse complete pkt here as per mqtt std consider max cmd size 127
            //uint16 remainingLen = (uint16) rx[1];
            
            uint8 nodeCommandType = 0;    //useful if node is hybrid gcms type
            uint8 inPktIndex = 1;
            uint32 mqttPktLen = calculateReverseMQTTPktLen (&data[inPktIndex], &inPktIndex);
            #ifdef DEBUG_MQTT_CLIENT_APP
            uartSerialWrite(&mqttPktLen, 4); 
            #endif
            //uartSerialWrite(&mqttPktLen, 4);
            uint16 topicLen = 0;
            memcpy (&topicLen, &data[inPktIndex], 2); 
            
            #ifdef DEBUG_MQTT_CLIENT_APP
            uartSerialWrite(&topicLen, 2); 
            #endif
            
            
            ////////////////////////////////////////////////////////
            // this section is check if given device is of hybrid GCMS type
            if (APP_TYPE == 0xFF)
            {
            	uint8 mqttTopic [100];
            	memcpy (mqttTopic, &data[inPktIndex+2], topicLen);
            	if (mqttTopic [1] == '0')
            		nodeCommandType = 1;
            	else
            		nodeCommandType = 0;
            	uint8 computedGwId [GATEWAY_ID_LEN];
            	hexStringToHexNumConverter (computedGwId, &mqttTopic[1], topicLen-1);
            	if(!matchByteArray (computedGwId, gatewayInfoTable.gatewayId, GATEWAY_ID_LEN-1))
            	{
            		if (computedGwId [GATEWAY_ID_LEN-1] == gatewayInfoTable.gatewayId [GATEWAY_ID_LEN-1])
            			nodeCommandType = 0;
            		else if (computedGwId [GATEWAY_ID_LEN-1] == (gatewayInfoTable.gatewayId [GATEWAY_ID_LEN-1] & 0x0f))
            			nodeCommandType = 1;
            		else
            			nodeCommandType = 1;
            	}
            }
            else
            	nodeCommandType = APP_TYPE;
            	
            ////////////////////////////////////////////////////////
            
            
            	
            ////////////////////////////////////////////////////////
       
            inPktIndex = inPktIndex+(2+topicLen);
            
            /////////////////////////////////////////////////
            int newDataSize = mqttPktLen - topicLen - 2;
            
            uint8 dataBuf[540];
            // 30 17 00 05 / T i m e [ 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ]
            // 23 -2-5 = 16
            
            if ((data[inPktIndex] == '[') && (data[inPktIndex+newDataSize-1] == ']'))
            {
                #ifdef DEBUG_MQTT_CLIENT_APP
                uartSerialWrite(dataBuf ,newDataSize);
                #endif
                newDataSize -= 2;
                memcpy (dataBuf, &data[inPktIndex+1], newDataSize);
                #ifdef DEBUG_MQTT_CLIENT_APP
                uartSerialWrite(&newDataSize ,4);
                uartSerialWrite(dataBuf ,newDataSize);
                #endif
            }
            else
                vAHI_SwReset();
            uint8 commandTimeBuf[270];
            int i=0;
            int j=0;
            for (i=0; i<newDataSize;)
            {
                commandTimeBuf [j] = hexStrToHexVal(&dataBuf[i]);
                j++;
                i +=2;
            }
            //////////////////////////////////////////////////
            //uartSerialWrite(commandTimeBuf ,j);
            
            //////////////////////////////////////////////////
	        if (j > 0)
	        {
	            //////////////////////////////////////////
	            //Process time first 
	            uint8 packet[8];
                packet[0] = TIME_UPDATE_HEADER;
                memcpy (&packet[1], commandTimeBuf, 7);
                
                //////////////////////////////////////////
                //Check and update node local time
                ctrlCommandPraser (packet, 8, nodeCommandType);
	            
	            ///////////////////////////////////////////
	            memcpy (gatewayInfoTable.timePktFromServer,commandTimeBuf, 7);
	            if (j>7)
	                ctrlCommandPraser (&commandTimeBuf[7], j-7, nodeCommandType);

                break;
            }
            ///////////////////////////////////////////////////
            break;
        }
        case 0x00:
        {
            //uartSerialWrite((uint8*)"Wifi Config Received",strlen ("Wifi Config Received"));
            //uartSerialWrite(rx,indexSize);
            #if (DEVICE_TYPE == WIFI_GATEWAY)
            setSaveWifiConfigData (data, dataPktSize);
            #endif
            break;
        }*/
    }
    return -1;
}


uint32 createMqttPubMessage (uint8* mqttPubTopic,uint16 mqttPubTopicLen,uint8* finalPkt,uint8* dataToSend,uint16 dataSize)
{
    finalPkt[0]=0x30;   //publish header, qos=0
    
    uint16 remainLength=2+mqttPubTopicLen+dataSize;  //2 bytes used to save topic length in packet
    
    uint8 remainLenPkt[4];
    uint8 remainLengthsize=calculateRemainingMQTTPacketLen (remainLength, remainLenPkt);    //number of bytes required to save remaining packet length in packet is saved in remainLengthSize
    
    memcpy(&finalPkt[1],remainLenPkt,remainLengthsize);
    
    //Publish topic length
    memcpy (&finalPkt[remainLengthsize+1], &mqttPubTopicLen, 2);
    
    //Publish topic 
    memcpy (&finalPkt[remainLengthsize+3], mqttPubTopic, mqttPubTopicLen);
    
    //publish data
    memcpy(&finalPkt[remainLengthsize+3+mqttPubTopicLen],dataToSend,dataSize);
    
    return remainLengthsize+3+mqttPubTopicLen+dataSize;
}


uint32 createMqttSubMessage (uint8* mqttSubTopic,uint16 mqttSubTopicLen,uint8* finalPkt)
{
    finalPkt[0]=0x82;   //subscribe header, qos=0
   
    uint16 remainLength=5+mqttSubTopicLen;  //2 bytes for PKT_identifier 2 bytes used to save topic length in packet + 1 QOS
    
    uint8 remainLenPkt[4];
    uint8 remainLengthsize=calculateRemainingMQTTPacketLen (remainLength, remainLenPkt);    //number of bytes required to save remaining packet length in packet is saved in remainLengthSize
    
    memcpy(&finalPkt[1],remainLenPkt,remainLengthsize);
    
    //Subscribe topic length
    finalPkt[remainLengthsize+1]=0x00;
    finalPkt[remainLengthsize+2]=0x10;

    finalPkt[remainLengthsize+3] = 0x00 ;
    
    finalPkt[remainLengthsize+4] = mqttSubTopicLen ;
    
    //Subscribe topic 
    memcpy (&finalPkt[remainLengthsize+5], mqttSubTopic, mqttSubTopicLen);
    
    finalPkt[remainLengthsize+5+mqttSubTopicLen]=0;         //qos
    
    return remainLengthsize+5+mqttSubTopicLen+1;
}
