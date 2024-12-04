/*********************************************
 * Changed by: Dov Rosenberg
 * Date: 01/05/2013
 * Author: Guido Socher 
 * Copyright: GPL V2
 * See http://www.gnu.org/licenses/gpl.html
 *
 * IP, Arp, UDP and TCP functions.
 *
 * The TCP implementation uses some size optimisations which are valid
 * only if all data can be sent in one single packet. This is however
 * not a big limitation for a microcontroller as you will anyhow use
 * small web-pages. The TCP stack is therefore a SDP-TCP stack (single data packet TCP).
 *
 *********************************************/
 /*********************************************
 * Modified: Eigen Techologies: 16/05/18
 *********************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>

#include "node.h"
#include "string.h"
#include "ether.h"
#include "task.h"
#include "eth_net.h"
#include "ethernetControl.h"

#include "config.h"

#ifdef ENABLE_ETH

//packet buffer to store incoming packets

#define MAX_P_BUFFER_SIZE 5
#define HANDLE_REC_PACKET 5
#define HANDLE_TCP_FIN 240
#define HANDLE_TCP_RST 241
#define CHECK_TCP_ACK 242
#define CHECK_CLOSE_TCP 243

uint8 recPacketBuffer[514][MAX_P_BUFFER_SIZE];
uint8 recPacketBufferSize=0;
uint8 recPacketReadIndex=0;
uint8 recPacketWriteIndex=0;

uint8 bufferFlag=0;

uint16 dnsTxId=0;

extern Time currentTime;

uint8 insertInPacketBuffer(uint8* packet,uint16 length);
void fetchPacketFromBuffer(uint8* pack);
uint8 getPacketBufferSize();

uint8* searchRouteTable(uint8* destIp);
uint8 searchTupleTable(uint8* srcIp,uint8* destIp,uint8* srcPort,uint8* destPort, uint8 protocol,uint8* index);
uint8 searchPortListenTable(uint8 protocol,uint8* destPort);
void insertUpdateTupleTable(uint8* ipAr,uint8* destIp,uint8 state,uint8 protocol,uint16 sourceSocket,uint16 destinationSocket,uint32 time,uint8* socketId,uint32 seqNo, uint32 ackNo,uint8 tcpstate,uint8* destDomain);
int sendTcp(uint8* socketId,uint8 flag,uint8* data,uint16 dataLength);
//calculate checksum for ip packet



void ethTableInit()
{
    memset(aTuples,0,sizeof(allowedTuples)*20);
    memset(listenPorts,0,sizeof(openPortInfo)*5);
    memset(rTable,0,sizeof(routeTable)*5);
    
    memset(macAddr,0,6);
    memset(ipAddr,0,4);
    memset(networkMask,0,4);
    memset(gatewayIp,0,4);
    memset(dhcpSIp,0,4);
    memset(gwMac,0,6);
    memset(dnsIp,0,4);
    
    gwMacAvail=0;        
}


void setBuffer(uint8 val)
{
    bufferFlag=val;
}

uint8 getBuffer()
{
    return bufferFlag;
}


uint16 checksum(uint8 *buf, uint16 len,uint8 type)
{
    // type 0=ip 
    //      1=udp
    //      2=tcp
    uint32 sum = 0;

    if(type==1)
    {
        sum+=IP_PROTO_UDP_V; // protocol udp
        // the length here is the length of udp (data+header len)
        // =length - IP addr length
        sum+=len-8; // = real udp len
    }
    if(type==2)
    {
        sum+=IP_PROTO_TCP_V; 
        // the length here is the length of tcp (data+header len)
        // =length - IP addr length
        sum+=len-8; // = real tcp len
    }
    // build the sum of 16bit words
    while(len >1)
    {
        sum += 0xFFFF & (*buf<<8|*(buf+1));
        buf+=2;
        len-=2;
    }
    // if there is a byte left then add it (padded with zero)
    if (len)
    {
        sum += (0xFF & *buf)<<8;
    }
    // now calculate the sum over the bytes in the sum
    // until the result is only 16bit long
    while (sum>>16)
    {
        sum = (sum & 0xFFFF)+(sum >> 16);
    }
    // build 1's complement:
    return( (uint16) sum ^ 0xFFFF);
}

//check the type of packet if it is intended for me, 
//return 0 if the packet is not for me
//       1 if the packet is ARP
//       2 if the packet is IP 
//       3 if the packet is a DHCP packet   
uint8 ethPackTypeIfMine(uint8 *buf,uint16 len)
{ 
    //debug(buf,30);
    //msdelay(1000);
    if(buf[ETH_TYPE_H_P]!=ETHTYPE_IP_H_V || 
       buf[ETH_TYPE_L_P]!=ETHTYPE_IP_L_V)
    {
       if(buf[ETH_TYPE_H_P] == ETHTYPE_ARP_H_V || 
          buf[ETH_TYPE_L_P] == ETHTYPE_ARP_L_V)
       {
          if (!memcmp(&buf[ETH_ARP_DST_IP_P],ipAddr,4))
          {
             return 1;
          }
          else   
          {
            return 0;
          }  
       }
       else
       {
         return 0;
       }     
    }
    else
    {
        uint16 srcPort,destPort;
            
        memcpy(&srcPort,&buf[34],2);
        memcpy(&destPort,&buf[36],2);
            
        if (getDhcpSate()!=DHCP_STATE_BOUND && len>=236)
        {
        	
            //this could be a dhcp packet, lets try to match the headers
            if (buf[23]==IP_PROTO_UDP_V && buf[42]==2/*its dhcp reply*/&& srcPort==DHCP_SRC_PORT && destPort==DHCP_DEST_PORT)
            {
                uint32 xid=getCurrentXid();
                if (!memcmp(&xid,&buf[46],4))
                {  
                    return 3;
                }
                else
                    return 0;
            }
            else if (!memcmp(&buf[IP_DST_P],&ipAddr,4)&&ipAddr[0]!=0)
            {
               return 2;
            }
            else
             {
                return 0;
             }   
        }
        else if (!memcmp(&buf[IP_DST_P],ipAddr,4)&&srcPort==53)
        {
            //dns packet
            return 4;
        }
        else if (!memcmp(&buf[IP_DST_P],ipAddr,4))
        {
           
           return 2;
        }
        else   
        {
         
            return 0;
        }    
    }
}

// make a valid ethernet frame based on the arguments, crc is done by ENC28J60 itself as enabled in init
void sendEthFrame(uint8* payload,uint16 payloadLength,uint16 protocolType,uint8* destEthAddr)
{
    uint8 buf[512];
    memset(buf,0,512);
    //make the ethernet frame as per standard
    memcpy(&buf[ETH_DST_MAC],destEthAddr,6);
    memcpy(&buf[ETH_SRC_MAC],macAddr,6); 
    memcpy(&buf[12],&protocolType,2);
    memcpy(&buf[14],payload,payloadLength); 
    //debug(buf,14);
    ethernetPacketSend(payloadLength+14, buf); 
}

// make a valid ipv4 packet, fragmentation is not supported
int prepIpv4Header(uint8 *buf,uint8* destAddr,uint8* srcAddr,uint16 payloadLength,uint8 protocolNumber)
{
    if (payloadLength>800)      //payloadLength can't be greater than 512
        return -1;
    buf[0]=0x45;    //the first nibble is 4 meaning, it is ipv4 packet
                    //second nibble specifies the size in 32 bit words of ipv4 header. minimum size header is created.
    buf[1]=0x00;
    uint16 tempLen= payloadLength+20;
    memcpy(&buf[2],&tempLen,2);
    memset(&buf[4],0,4); 
    buf[6]=0x40;        //DF flaag set  
    buf[8]=0xff;    //max time to live value
    buf[9]=protocolNumber;             
        
    // clear the 2 byte checksum
    buf[10]=0;
    buf[11]=0;
    
    memcpy(&buf[12],srcAddr,4);
    memcpy(&buf[16],destAddr,4);
    
    uint16 ck=checksum(buf, IP_HEADER_LEN,0);
    buf[10]=ck>>8;
    buf[11]=ck& 0xff;
    

    return payloadLength+20;    //return net size of packet
}

//make udp header
int prepUdpHeader(uint8 *buf,uint16 sourcePort,uint16 destPort,uint16 payloadLength)
{
    memcpy(buf,&sourcePort,2);
    memcpy(&buf[2],&destPort,2);
    uint16 tempLen=payloadLength+8;
    memcpy(&buf[4],&tempLen,2);
    memset(&buf[6],0,2);    //checksum will be calculated later after packet is ready
    return tempLen;
}

void make_eth(uint8 *buf)
{
        uint8 i=0;
        //
        //copy the destination mac from the source and fill my mac into src
        while(i<6)
        {
                buf[ETH_DST_MAC +i]=buf[ETH_SRC_MAC +i];
                buf[ETH_SRC_MAC +i]=macAddr[i];
                i++;
        }
}

void make_arp_answer_from_request(uint8 *buf,uint8 len)
{
        uint8 i=0;
        //
        make_eth(buf);
        buf[ETH_ARP_OPCODE_H_P]=ETH_ARP_OPCODE_REPLY_H_V;
        buf[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V;
        // fill the mac addresses:
        while(i<6){
                buf[ETH_ARP_DST_MAC_P+i]=buf[ETH_ARP_SRC_MAC_P+i];
                buf[ETH_ARP_SRC_MAC_P+i]=macAddr[i];
                i++;
        }
        i=0;
        while(i<4){
                buf[ETH_ARP_DST_IP_P+i]=buf[ETH_ARP_SRC_IP_P+i];
                buf[ETH_ARP_SRC_IP_P+i]=ipAddr[i];
                i++;
        }
        // eth+arp is 42 bytes:
        ethernetPacketSend(42,buf); 
}

void fill_ip_hdr_checksum(uint8 *buf)
{
        uint16 ck;
        // clear the 2 byte checksum
        buf[IP_CHECKSUM_P]=0;
        buf[IP_CHECKSUM_P+1]=0;
        buf[IP_FLAGS_P]=0x40; // don't fragment
        buf[IP_FLAGS_P+1]=0;  // fragement offset
        buf[IP_TTL_P]=64; // ttl
        // calculate the checksum:
        ck=checksum(&buf[IP_P], IP_HEADER_LEN,0);
        buf[IP_CHECKSUM_P]=ck>>8;
        buf[IP_CHECKSUM_P+1]=ck& 0xff;
}

void make_ip(uint8 *buf)
{
        uint8 i=0;
        while(i<4){
                buf[IP_DST_P+i]=buf[IP_SRC_P+i];
                buf[IP_SRC_P+i]=ipAddr[i];
                i++;
        }
        fill_ip_hdr_checksum(buf);
}

void make_echo_reply_from_request(uint8 *buf,uint8 len)
{
        make_eth(buf);
        make_ip(buf);
        buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREPLY_V;
        // we changed only the icmp.type field from request(=8) to reply(=0).
        // we can therefore easily correct the checksum:
        if (buf[ICMP_CHECKSUM_P] > (0xff-0x08)){
                buf[ICMP_CHECKSUM_P+1]++;
        }
        buf[ICMP_CHECKSUM_P]+=0x08;
        
        ethernetPacketSend(len,buf);
}

// you can send a max of 220 bytes of data
void make_udp_reply_from_request(uint8 *buf,char *data,uint8 datalen,uint16 port)
{
        uint8 i=0;
        uint16 ck;
        make_eth(buf);
        if (datalen>220){
                datalen=220;
        }
        // total length field in the IP header must be set:
        buf[IP_TOTLEN_H_P]=0;
        buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
        make_ip(buf);
        // send to port:
        //buf[UDP_DST_PORT_H_P]=port>>8;
        //buf[UDP_DST_PORT_L_P]=port & 0xff;
        // sent to port of sender and use "port" as own source:
        buf[UDP_DST_PORT_H_P]=buf[UDP_SRC_PORT_H_P];
        buf[UDP_DST_PORT_L_P]= buf[UDP_SRC_PORT_L_P];
        buf[UDP_SRC_PORT_H_P]=port>>8;
        buf[UDP_SRC_PORT_L_P]=port & 0xff;
        // source port does not matter and is what the sender used.
        // calculte the udp length:
        buf[UDP_LEN_H_P]=0;
        buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
        // zero the checksum
        buf[UDP_CHECKSUM_H_P]=0;
        buf[UDP_CHECKSUM_L_P]=0;
        // copy the data:
        while(i<datalen){
                buf[UDP_DATA_P+i]=data[i];
                i++;
        }
        ck=checksum(&buf[IP_SRC_P], 16 + datalen,1);
        buf[UDP_CHECKSUM_H_P]=ck>>8;
        buf[UDP_CHECKSUM_L_P]=ck& 0xff;
        ethernetPacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

/*needed if we need to communicate with server when it is deployed in local network directly */
void make_arp_request(uint8 *dest_ip)
{
    uint8 buf[512];
    uint8 i=0;
    while(i<6)
    {
        buf[ETH_DST_MAC +i]=0xff;
        buf[ETH_SRC_MAC +i]=macAddr[i];
        i++;
    }

    buf[ ETH_TYPE_H_P ] = ETHTYPE_ARP_H_V;
    buf[ ETH_TYPE_L_P ] = ETHTYPE_ARP_L_V;

    // generate arp packet
    buf[ARP_OPCODE_H_P]=ARP_OPCODE_REQUEST_H_V;
    buf[ARP_OPCODE_L_P]=ARP_OPCODE_REQUEST_L_V;

    // fill in arp request packet
    // setup hardware type to ethernet 0x0001
    buf[ ARP_HARDWARE_TYPE_H_P ] = ARP_HARDWARE_TYPE_H_V;
    buf[ ARP_HARDWARE_TYPE_L_P ] = ARP_HARDWARE_TYPE_L_V;

    // setup protocol type to ip 0x0800
    buf[ ARP_PROTOCOL_H_P ] = ARP_PROTOCOL_H_V;
    buf[ ARP_PROTOCOL_L_P ] = ARP_PROTOCOL_L_V;

    // setup hardware length to 0x06
    buf[ ARP_HARDWARE_SIZE_P ] = ARP_HARDWARE_SIZE_V;

    // setup protocol length to 0x04
    buf[ ARP_PROTOCOL_SIZE_P ] = ARP_PROTOCOL_SIZE_V;

    // setup arp destination and source mac address
    for ( i=0; i<6; i++)
    {
        buf[ ARP_DST_MAC_P + i ] = 0x00;
        buf[ ARP_SRC_MAC_P + i ] = macAddr[i];
    }

       // setup arp destination and source ip address
    for ( i=0; i<4; i++)
    {
        buf[ ARP_DST_IP_P + i ] = dest_ip[i];
        buf[ ARP_SRC_IP_P + i ] = ipAddr[i];
    }
     // eth+arp is 42 bytes:
    ethernetPacketSend(42,buf);

}

uint8 arp_packet_is_myreply_arp ( uint8 *buf )
{
    uint8 i;

    // if packet type is not arp packet exit from function
    if( buf[ ETH_TYPE_H_P ] != ETHTYPE_ARP_H_V || buf[ ETH_TYPE_L_P ] != ETHTYPE_ARP_L_V)
           return 0;
    // check arp request opcode
    if ( buf[ ARP_OPCODE_H_P ] != ARP_OPCODE_REPLY_H_V || buf[ ARP_OPCODE_L_P ] != ARP_OPCODE_REPLY_L_V )
           return 0;
    // if destination ip address in arp packet not match with my ip address
    for(i=0; i<4; i++)
    {
        if(buf[ETH_ARP_DST_IP_P+i] != ipAddr[i])
        {
            return 0;
        }
    }
    return 1;
}


uint8 f[512];
//main function that gets called if packet is found on ethernet interface
uint8 ethHandleIncomingPacket(uint8* buf,uint16 length)
{
    memcpy(f,buf,length);
   
    uint8 check=ethPackTypeIfMine(f,length);
    uint8 index=0; 
    switch (check)
    {
        case 0:
        {
            //not my packet
            
            break;
        }
        case 1:
        {
            //arp
            if (arp_packet_is_myreply_arp(f)) 
            {
                if (!memcmp(gatewayIp,&f[ ARP_SRC_IP_P],4))
                {
                    memcpy(gwMac,&f[ETH_SRC_MAC],6);  
                    gwMacAvail=1;
			    }
			    /*else
			    {
			        debug("6666",4);
			        msdelay(100);
			        checkUpdateRouteTable(NULL,&buf[ETH_ARP_SRC_IP_P],&buf[ETH_SRC_MAC]);
				}*/
				
				checkUpdateRouteTable(NULL,&f[ETH_ARP_SRC_IP_P],&f[ETH_SRC_MAC]);
            }
            else 
            {
			    //Answering ARP request
                make_arp_answer_from_request(f,length);
            }
            break;
        }

        case 2:
        {
            //ip packet for me
            //check if it is icmp packet
                     
            if (f[IP_PROTO_P] == IP_PROTO_UDP_V  
            &&(searchPortListenTable(UDP,&f[UDP_DST_PORT_H_P])
               ||searchTupleTable(&f[IP_SRC_P],&f[IP_DST_P],&f[UDP_SRC_PORT_H_P],&f[UDP_DST_PORT_H_P], UDP,&index)))
            {
                //save the packet in buffer and add task to handle packet
                insertInPacketBuffer(f,length);
                
                if(getBuffer()==0)
                {
                    //debug("ll",2);
                    //msdelay(100);
                    setBuffer(1);
                    addTask(USER,HANDLE_REC_PACKET,5);
                }   
            }
            else if(f[IP_PROTO_P] == IP_PROTO_TCP_V)
            {
                
                if (searchTupleTable(&f[IP_SRC_P],&f[IP_DST_P],&f[TCP_SRC_PORT_H_P],&f[TCP_DST_PORT_H_P], TCP,&index))
                {
                    //a similar entry exists in tuple table, lets see if we are expecting this packet for this tuple
                                 
                    switch (aTuples[index].tcpState)
                    {
                        case SYN_SENT:
                        {
                            uint32 seq=aTuples[index].seqNo+1;                            
                            if (f[TCP_FLAGS_P] == (TCP_FLAG_SYN_V | TCP_FLAG_ACK_V))
                            {
                                if(!(memcmp(&seq,&f[TCP_SEQACK_P],4)))
                                {
                                    aTuples[index].seqNo+=1;
                                    memcpy(&aTuples[index].ackNo,&f[TCP_SEQ_P],4);
                                    aTuples[index].ackNo++;
                                    aTuples[index].tcpState=ACK_SENT;
                                    aTuples[index].lastUsedTime=currentTime.clockLow;
                                    aTuples[index].seqInc=0;
                                    aTuples[index].state=SOCKET_CONNECTED;
                                    
                                    //generate ack
                                    sendTcp(&index,TCP_FLAG_ACK_V,NULL,0);
                                }
                                
                            }
                            else if (f[TCP_FLAGS_P] & (TCP_FLAG_RST_V))
                            {
                                seq=seq-1;
                                if(!(memcmp(&seq,&f[TCP_SEQACK_P],4)))
                                {
                                    addTask(USER,HANDLE_TCP_RST,5);
                                    aTuples[index].seqNo=0;
                                    aTuples[index].ackNo=0;
                                    aTuples[index].seqInc=0;
                                }
                                    
                            }
                            break;
                        }
                        case ACK_SENT:
                        case DATA_SENT:
                        {    
                            uint32 seq=aTuples[index].seqNo+aTuples[index].seqInc;  
                            uint16 iplen;
                                
                            memcpy(&iplen,&f[IP_TOTLEN_H_P],2);
                            uint16 inc=iplen-(IP_HEADER_LEN+((f[TCP_HEADER_LEN_P]>>4)*4));                                                    
                            if (f[TCP_FLAGS_P] & (TCP_FLAG_ACK_V))    //ack for my data push
                            {
                                if((!(memcmp(&seq,&f[TCP_SEQACK_P],4)))&&(!(memcmp(&aTuples[index].ackNo,&f[TCP_SEQ_P],4))))
                                {
                                    aTuples[index].seqNo=seq;
                                    
                                    aTuples[index].waitForAck++;
                                    aTuples[index].seqInc=0; 
                                    memcpy(&aTuples[index].ackNo,&f[TCP_SEQ_P],4);
                                    aTuples[index].lastUsedTime=currentTime.clockLow;                                    
                                                             
                                }
                                 
                            }
                            
                            if (inc>0 && !(memcmp(&aTuples[index].seqNo,&f[TCP_SEQACK_P],4)) 
                                               && !(memcmp(&aTuples[index].ackNo,&f[TCP_SEQ_P],4)))
                            {
                                //this contains data, save data and generate ack
                               
                                //save the packet in buffer and add task to handle packet
                                insertInPacketBuffer(f,length);
                                
                                if(getBuffer()==0)
                                {
                                    //debug("ll",2);
                                    //msdelay(100);
                                    setBuffer(1);
                                    addTask(USER,HANDLE_REC_PACKET,5);
                                }
                                
                                aTuples[index].ackNo+=inc;    
                                sendTcp(&index,TCP_FLAG_ACK_V,NULL,0);
                            }
                            
                            if (f[TCP_FLAGS_P] & (TCP_FLAG_FIN_V))
                            {
                                if(!(memcmp(&aTuples[index].seqNo,&f[TCP_SEQACK_P],4))) 
                                   //&& !(memcmp(&aTuples[index].ackNo,&f[TCP_SEQ_P],4)))
                                {
                                    if(aTuples[index].finSent==0)
                                    {
                                        aTuples[index].ackNo++;
                                        aTuples[index].finSent=1;
                                    }
                                        
                                    sendTcp(&index,TCP_FLAG_FIN_V|TCP_FLAG_ACK_V,NULL,0);
                                    addTask(USER,HANDLE_TCP_FIN,5);
                                }
                            }
                            
                            if (f[TCP_FLAGS_P] & (TCP_FLAG_RST_V))
                            {
                                if(!(memcmp(&aTuples[index].seqNo,&f[TCP_SEQACK_P],4)))
                                {
                                    addTask(USER,HANDLE_TCP_RST,5);
                                    aTuples[index].seqNo=0;
                                    aTuples[index].ackNo=0;
                                    aTuples[index].seqInc=0;
                                }
                            }
                                                
                            break;
                        }
                    
                    }
                    
                    
                }
                //received a tcp packet, lets see what it is and act accordingly
            }
            else if (f[IP_PROTO_P] == IP_PROTO_ICMP_V &&
            f[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V) 
            {
                make_echo_reply_from_request(f, length);
            }
            break;
        }
        
        case 3:
        {
            //dhcp packet
            
            dhcpReceiveMessage(f,length);
            break;
        }
        
        case 4:
        {
            //dns packet
           
            //let's match the transaction id first
            uint16 txId;
            memcpy(&txId,&f[42],2);
                        
            if (dnsTxId==txId&& ((f[44]&0x80)>0))
            {
                //response to my query
                uint8 domainName[100];
                memset(domainName,0,100);
                uint8 prev=54;
        
               // uint8 c;
                uint16 n=0;
                uint16 index=0;
                
                do  //fetch domain name
                {               
                    for(index=0;index<f[prev];index++) 
                    {
                      domainName[n+index]=f[prev+1+index];
                      
                    }
                    
                    prev=prev+index+1;
                    
                    if (f[prev]!=0)
                    {
                        domainName[n+index]=0x2e;
                        n++;
                    }
                    n+=index;
       
                } while (f[prev]!=0);
                
                if (f[length-4]!=0)  //must be valid ip
                {    
                    uint8 gd=checkUpdateRouteTable(domainName,&f[length-4],NULL);
                    if (gd==1)
                    {
                        //we can start the connection request here after fetching the socket id
                        //int za=0;
                        //yaha se shuru
                    }
                }
            }
        
            break;   
        }
    }
    return 1;
}

//call this function after ipv4 header has been added to udp and get the checkssum for udp updated
void updateUdpChecksum(uint8* buf, uint16 len)
{
  uint16 ck=checksum(&buf[12],len-12,1);
  buf[26]=ck>>8;
  buf[27]=ck& 0xff;
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 socket related functions
 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//returns -1- socket can't be created
//            SOCKET_CONNECTED,
//            CONNECTION_REQUESTED,
//            DNS_REQUEST_ON,
//            ARP_REQUEST_ON
int createEthSocket(uint8 protocol,uint8* destIp,uint8* domainName,uint16 sourceSocket,uint16 destinationSocket,uint8* socketId)
{
    //insert info in tuple table
    if (allowedTupleSize<20)
    {
        int f=checkUpdateRouteTable(domainName,destIp,NULL); 
                
        if (f==-1)  //no space in arp table for new entry
            return -1;
                
        switch (protocol)
        {
            case UDP:
            {
               uint8 state;
               state=0;
               switch(f)
               {
                    case 1:
                    {
                        state=SOCKET_CONNECTED;
                        break;
                    }
                    case 2:
                    {
                        state=ARP_REQUEST_ON;
                        break;
                    }
                    case 3:
                    {
                        state=DNS_REQUEST_ON;
                        break;
                    }    
                }  
                insertUpdateTupleTable(ipAddr,destIp,state,UDP,sourceSocket,destinationSocket,currentTime.clockLow,socketId,0,0,0,domainName);
                return state;
                
            }
            case TCP:
            {
               
               uint8 state;
               state=0;
               switch(f)
               {
                    case 1:
                    {
                        //start the connect request from here
                       
                        state=CONNECTION_REQUESTED;
                        break;
                    }
                    case 2:
                    {
                        state=ARP_REQUEST_ON;
                        break;
                    }
                    case 3:
                    {
                        state=DNS_REQUEST_ON;
                        break;
                    }    
                }  
                insertUpdateTupleTable(ipAddr,destIp,state,TCP,sourceSocket,destinationSocket,currentTime.clockLow,socketId,0x67,0,SYN_SENT,domainName);
                if (state==CONNECTION_REQUESTED)
                     sendTcpConnect(socketId);
                return state;
            }
        }
    }
    return -1;    
}

int listenPort(uint8 protocol,uint16 port)
{
    int g=0;
    
    for (;g<openPortSize;g++)
    {
        if (listenPorts[g].protocol==protocol && listenPorts[g].openPort==port)
        {
            //entry already exists
            
            return 0;
        }
    }
    
    if(openPortSize<5)
    {
        listenPorts[openPortSize].protocol=protocol;
        listenPorts[openPortSize].openPort=port;
        openPortSize++;
        return 0;
    }
    else
        return -1; 
         
}

int closePort(uint8 protocol,uint16 port)
{
    //Lets handle tuple expiry also here
	int y=0;
	for(;y<openPortSize;y++)
	{
	    if (listenPorts[y].protocol==protocol && listenPorts[y].openPort==port)
	    {
	        //delete entry
	        int j=y;
	        for(;j<openPortSize-1;j++)
	        {
	            memcpy(&listenPorts[j],&listenPorts[j+1],sizeof(openPortInfo));
	        }
	        openPortSize--;
	        return 1;
	    }
	}
	return -1;
}

uint8 sendUdpPacket(uint8 socketId,uint8* data,uint16 dataLen)
{
    uint8 udpPacket[512];
	memset(udpPacket,0,512);	
    
    uint16 udpLen=prepUdpHeader(udpPacket,aTuples[socketId].srcPort,aTuples[socketId].destPort,dataLen);
	
	memcpy(&udpPacket[8],data,dataLen);
	
	uint8 ipv4[512];
    memset(ipv4,0,512);
    
    uint16 ipLen=prepIpv4Header(ipv4,aTuples[socketId].destAddr,aTuples[socketId].srcAddr,udpLen,IP_PROTO_UDP_V);
    memcpy(&ipv4[20],udpPacket,udpLen);
    
    updateUdpChecksum(ipv4,ipLen);
   
    uint16 ethType=ETHTYPE_IP_H_V;
    ethType=ethType<<8;
    ethType|=ETHTYPE_IP_L_V;   
    
    uint8* nextAddr=searchRouteTable(aTuples[socketId].destAddr);
    
    if(nextAddr!=NULL)
    {     
        sendEthFrame(ipv4,ipLen,ethType,nextAddr);
        aTuples[socketId].lastUsedTime= currentTime.clockLow;
        return 1;
    }
   return -1;    
}

int sendTcp(uint8* socketId,uint8 flag,uint8* data,uint16 dataLength)
{
   uint8 sock;
   memcpy(&sock,socketId,1);
   
   uint8 buf[512];
   memset(buf,0,512);
   
   //lets make tcp header
   memcpy(buf,&aTuples[sock].srcPort,2);
   memcpy(&buf[2],&aTuples[sock].destPort,2);
   memcpy(&buf[4],&aTuples[sock].seqNo,4);
   memcpy(&buf[8],&aTuples[sock].ackNo,4);
  
   memcpy(&buf[13],&flag,1);
   
   //max window size
   buf[14] = ((600 - IP_HEADER_LEN - ETH_HEADER_LEN)>>8) & 0xff;
   buf[15] = (600 - IP_HEADER_LEN - ETH_HEADER_LEN) & 0xff; 
   
   //checksum will be zero here and will be updated later
   //urgent pointers are also 0
    
   // sequence numbers:
   // add the rel ack num to SEQACK
		
	if (flag == TCP_FLAG_SYN_V) 
	{
	    //data offset=0x60;   //first nibble specifies length of tcp header in 32 bits words
        buf[12]=0x60;
        
        // setup maximum segment size
		buf[20]=2;
		buf[21]=4;
		buf[22]=0x05;
		buf[23]=0x80;
		
		aTuples[sock].tcpState = SYN_SENT;
	} 
	else if (flag & TCP_FLAG_ACK_V) 
	{
	    //data offset=0x60;   //first nibble specifies length of tcp header in 32 bits words
        buf[12]=0x50;
        aTuples[sock].tcpState = ACK_SENT;
	}
	else if(flag==TCP_FLAG_RST_V)
	{
	    buf[12]=0x50;
        aTuples[sock].tcpState = SYN_SENT;
       
        
	}
	/*	if (flags & TCP_FLAG_PUSH_V) 
		{
			if (state == ACK_PSH)
			{
				copy4byte(net.buf + TCP_SEQ_H_P, lastack);
				copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);
			} else if (state == ACK) 
			{
				copy4byte(net.buf + TCP_SEQ_H_P, lastseq);
				copy4byte(net.buf + TCP_SEQACK_H_P, lastack);
			}
			
			saveSEQ(buf);
			saveACK(buf);
			
			state = ACK_PSH;
		}else if (flags & TCP_FLAG_SYN_V) {
			copy4byte(net.buf + TCP_SEQ_H_P, lastack);

			copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);

			saveSEQ(buf);
			saveACK(buf);
			
			state = ACK;

		}else {
			// in SYN state and recived ACK due to SYN|ACK recived
			if (state == SYN){
				saveSEQ(buf);
				saveACK(buf);
				copy4byte(net.buf + TCP_SEQ_H_P, lastack);

				addTonumber(lastseq,1);
				copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);

				saveSEQ(buf);
				saveACK(buf);
				state = ACK;
			} else if ((state == ACK) || (state == ACK_PSH)){
				saveSEQ(buf);
				saveACK(buf);
				copy4byte(net.buf + TCP_SEQ_H_P, lastack);

				addTonumber(lastseq,tcp_get_dlength(buf));
				copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);

				saveSEQ(buf);
				saveACK(buf);
				state = ACK;
			}
			
		}
		// no options - 20 bytes:
		buf[TCP_HEADER_LEN_P]=0x50;
	} else if (flags & TCP_FLAG_RST_V) {
		saveSEQ(buf);
		saveACK(buf);
		copy4byte(net.buf + TCP_SEQ_H_P, lastack);
		
		addTonumber(lastseq,1);
		copy4byte(net.buf + TCP_SEQACK_H_P, lastseq);
	
		saveSEQ(buf);
		saveACK(buf);
		state = ACK;
		buf[TCP_HEADER_LEN_P]=0x50;
	} */
    uint8 ipv4[512];
    memset(ipv4,0,512);
    
    uint16 tcpLength= (buf[12]>>4)*4;
    if (dataLength>0)
        memcpy(&buf[tcpLength],data,dataLength);
    
    tcpLength=tcpLength+dataLength;
    uint16 ipLen=prepIpv4Header(ipv4,aTuples[sock].destAddr,aTuples[sock].srcAddr,tcpLength,IP_PROTO_TCP_V);
    
    memcpy(&ipv4[20],buf,tcpLength);
    
    uint16 ck=checksum(&ipv4[12],ipLen-12,2);
    ipv4[36]=ck>>8;
    ipv4[37]=ck& 0xff;
  
    uint16 ethType=ETHTYPE_IP_H_V;
    ethType=ethType<<8;
    ethType|=ETHTYPE_IP_L_V;   
    
    uint8* nextAddr=searchRouteTable(aTuples[sock].destAddr);
    
    if(nextAddr!=NULL)
    {     
        sendEthFrame(ipv4,ipLen,ethType,nextAddr);
        aTuples[sock].lastUsedTime= currentTime.clockLow;
        
        /*if(flag==TCP_FLAG_RST_V)            //there could be a better way to handle this
	    {
	        msdelay(1000);
	        vAHI_SwReset();        
	    } */ 
        
        return 1;
    }  	
return -1;
}

void sendTcpConnect(uint8 *socketId)
{
    uint8 index;
    memcpy(&index,socketId,1);
    
	aTuples[index].finSent=0;

    sendTcp(socketId,TCP_FLAG_SYN_V,NULL,0);
}

void sendTcpPacket(uint8* socketId,uint8* data,uint16 len)
{
    uint8 in;
    memcpy(&in,socketId,1);
    aTuples[in].tcpState=DATA_SENT;
    aTuples[in].seqInc=len;
    aTuples[in].waitForAck=1;
    sendTcp(socketId,TCP_FLAG_PUSH_V|TCP_FLAG_ACK_V,data,len);
    addTask(USER,CHECK_TCP_ACK,3*SECOND);
}

void closeTcp(uint8* socketId)
{
    uint8 in;
    memcpy(&in,socketId,1);
    aTuples[in].waitForAck=50;
    sendTcp(socketId,TCP_FLAG_FIN_V|TCP_FLAG_ACK_V,NULL,0);
    addTask(USER,CHECK_CLOSE_TCP,3000);
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
various table related functions
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

int checkUpdateRouteTable(uint8* domainName,uint8* destIp,uint8* nextAddr)
{
    int routeSet=-1;
    
    if (destMacTableSize<5)
    {
        if (destIp==NULL&&domainName==NULL)
            return -1;
        
        if (destIp!=NULL)
        {
            int g=0;
            int checkFlag=0;
            for (;g<destMacTableSize;g++)
            {
                if ((!memcmp(rTable[g].destIpAddr,destIp,4)&&domainName==NULL)||(!memcmp(rTable[g].destDomain,domainName,strlen((const char*)(domainName)))))
                {
                    //entry already exists
                   
                    checkFlag=1;
                    break;
                }
            }
            
            memcpy(rTable[g].destIpAddr,destIp,4);
            
            if (checkFlag==0)
               destMacTableSize++;
            
            if (domainName!=NULL)
                memcpy(rTable[g].destDomain,domainName,strlen((const char*)(domainName)));
            
            if (nextAddr!=NULL)
            {  
                memcpy(rTable[g].nextMac,nextAddr,6);
                int s=0;
                int ceckFlag=0;
                for (;s<allowedTupleSize;s++)
                {
                    if ((!memcmp(aTuples[s].destAddr,destIp,4)))
                    {
                        //entry already exists
                       
                        ceckFlag=1;
                        break;
                    }
                }
                
                if (ceckFlag==1 && aTuples[s].state==ARP_REQUEST_ON && aTuples[s].protocol==UDP)
                {
                   
                    aTuples[s].state=SOCKET_CONNECTED;
                       
                    return 1;
                }
                else if (ceckFlag==1 && aTuples[s].state==ARP_REQUEST_ON && aTuples[s].protocol==TCP)
                {
                    sendTcpConnect((uint8*)s);
                    //sendTcpConnect((uint8*)s);
                                        
                    aTuples[s].state=CONNECTION_REQUESTED;
                    
                    //send tcp connect request from here
                    
                    return 1;   //for tcp
                }                                                    
            }
            else if (destIp[3]==0xff)   //check if destination ip is broadcast address
            {
                memset(rTable[g].nextMac,0xff,6);
                routeSet=1;
            }
            else if (((networkMask[0]&destIp[0])==(networkMask[0]&ipAddr[0]))
                    &&((networkMask[1]&destIp[1])==(networkMask[1]&ipAddr[1]))
                    &&((networkMask[2]&destIp[2])==(networkMask[2]&ipAddr[2]))
                    &&((networkMask[3]&destIp[3])==(networkMask[3]&ipAddr[3]))
                    ) //check if destination pesent out of my subnet
            {
                //need to start arp request to destination
               
                make_arp_request(destIp);
                routeSet=2;               
            }
            else
            {   
                
                memcpy(rTable[g].nextMac,gwMac,6);
                
                if (domainName!=NULL)
                {
                    int s=0;
                   // int ceckFlag=0;
                    for (;s<allowedTupleSize;s++)
                    {
                        if ((!memcmp(aTuples[s].destDomain,domainName,4)))
                        {
                            //entry already exists
                           
                    //       ceckFlag=1;
                            break;
                        }
                    }
                    
                    memcpy(&aTuples[s].destAddr,destIp,4);
                    if (aTuples[s].protocol==TCP)
                    {
                        aTuples[s].state=CONNECTION_REQUESTED;
                        //endTcpConnect(&s);
                        sendTcpConnect((uint8*)s);
                    
                    }
                    else if (aTuples[s].protocol==UDP)
                    {
                        aTuples[s].state=SOCKET_CONNECTED;
                    }
                        
                }
                              
                routeSet=1;
                
            }    
            
            if (checkFlag==0)
                  destMacTableSize++;              
            return routeSet;
        }
        else
        {
            int g=0;
            int checkFlag=0;
            for (;g<destMacTableSize;g++)
            {
                if (!memcmp(rTable[g].destDomain,domainName,4))
                {
                    //entry already exists
                    if (rTable[g].destIpAddr[0]!=0)
                    {
                        if(rTable[g].nextMac==NULL)
                        {
                            if( (networkMask[0]&destIp[0])==(networkMask[0]&ipAddr[0])
                                &&(networkMask[1]&destIp[1])==(networkMask[1]&ipAddr[1])
                                &&(networkMask[2]&destIp[2])==(networkMask[2]&ipAddr[2])
                                &&(networkMask[3]&destIp[3])==(networkMask[3]&ipAddr[3])
                                ) //check if destination pesent out of my subnet
                            {
                                //need to start arp request to destination
                                make_arp_request(destIp);
                                return 2;
                                
                            }
                            else
                            {   
                                memcpy(rTable[g].nextMac,gwMac,6);
                                return 1;
                            }
                        }    
                        else
                        { 
                            return 1;
                        }       
                     }
                     else
                     {
                       
                        checkFlag=1; 
                        break;        
                     }   
                }
            }
            
            memcpy(rTable[g].destDomain,domainName,strlen((const char*)(domainName)));
            if (checkFlag==0)
                destMacTableSize++;    
                        
            if (dnsIp[0]==0)
                 memset(dnsIp, 8, 4);   //google 
                 
            //start DNS request
            uint8 dnsRequest[512];
            memset(dnsRequest,0,512);
            
            dnsTxId=eRand();
            
            memcpy(dnsRequest,&dnsTxId,2);
            dnsRequest[2]=0x01;
            dnsRequest[3]=0x00;
            dnsRequest[5]=1;    //num queries
            
            uint8 prev=12;
        
            uint8 c;
            uint16 n=0;
            uint16 index=0;
            do 
            {               
                for(;;) 
                {
                  c = domainName[n+index];
                  if (c == '.' || c == 0)
                    break;
                  dnsRequest[prev+1+index]=c;
                  index++;
                }
                dnsRequest[prev]=index;
                prev=prev+1+index;
                n+=index+1;
                index=0;
   
            } while (c != 0);
            
            
            dnsRequest[prev]=0;
            dnsRequest[prev+2]=1;
            dnsRequest[prev+4]=1;
            
            uint8 udpPacket[512];
            uint16 udpLen=prepUdpHeader(udpPacket,0x9191,53,prev+5);
	
	        memcpy(&udpPacket[8],dnsRequest,prev+5);
	        
	        uint8 ipv4[512];
            memset(ipv4,0,512);
            
            uint16 ipLen=prepIpv4Header(ipv4,dnsIp,ipAddr,udpLen,IP_PROTO_UDP_V);
            memcpy(&ipv4[20],udpPacket,udpLen);
            
            updateUdpChecksum(ipv4,ipLen);
           
            uint16 ethType=ETHTYPE_IP_H_V;
            ethType=ethType<<8;
            ethType|=ETHTYPE_IP_L_V;  
            
            sendEthFrame(ipv4,ipLen,ethType,gwMac);//need to change this if dns server can be inside the network 
            
        }    
    }
   return -1;
}

void insertUpdateTupleTable(uint8* ipAr,uint8* destIp,uint8 state,
                            uint8 protocol,uint16 sourceSocket,
                            uint16 destinationSocket,uint32 time,
                            uint8* socketId,uint32 seqNo, uint32 ackNo,
                            uint8 tcpstate,uint8* destDomain)
{
    uint8 g=0;
    int checkFlag=0;
    for (;g<allowedTupleSize;g++)
    {
        if ((!memcmp(aTuples[g].destAddr,destIp,4))&&aTuples[g].destPort==destinationSocket)
        {
            //entry already exists
            checkFlag=1;
            break;
        }
    }
    
    if (destIp!=NULL)
        memcpy(aTuples[g].destAddr,destIp,4);
    memcpy(aTuples[g].srcAddr,ipAr,4);
    memcpy(aTuples[g].destDomain,destDomain,strlen((const char*)(destDomain)));
    aTuples[g].state=state;
    aTuples[g].protocol=protocol;
    aTuples[g].srcPort=sourceSocket;
    aTuples[g].destPort=destinationSocket;
    aTuples[g].lastUsedTime=time;
    aTuples[g].tcpState=tcpstate;
    aTuples[g].seqNo=seqNo;
    aTuples[g].ackNo=ackNo;
   
    
    memcpy(socketId,&g,1);
    
    if (checkFlag==0)
          allowedTupleSize++;    
}

int8 pollSocketStatus(uint8 socketId)
{
    if (socketId>=20)
        return -1;
    
    return aTuples[socketId].state;
}

uint8* searchRouteTable(uint8* destIp)
{
    int g=0;
    //int checkFlag=0;
        
    for (;g<destMacTableSize;g++)
    {
        if (!memcmp(rTable[g].destIpAddr,destIp,4))
        {
            return rTable[g].nextMac; 
        }
    }
  
    return NULL; 
}

uint8 searchPortListenTable(uint8 protocol,uint8* destPort)
{
    int g=0;
   
    for(;g<openPortSize;g++)
    {
        if (protocol==listenPorts[g].protocol && !memcmp(destPort,&(listenPorts[g].openPort),2))
        {
            return 1;
        }    
    }
    
    return 0;        
                    
}

uint8 searchTupleTable(uint8* srcIp,uint8* destIp,uint8* srcPort,uint8* destPort, uint8 protocol,uint8* index)
{
    int g=0;
    for (;g<allowedTupleSize;g++)
    {       
        if (!memcmp(aTuples[g].destAddr,srcIp,4)
            &&!memcmp(aTuples[g].srcAddr,destIp,4)
            &&!memcmp(&aTuples[g].srcPort,destPort,2)
            &&!memcmp(&aTuples[g].destPort,srcPort,2)
            &&(aTuples[g].state==SOCKET_CONNECTED||aTuples[g].state==CONNECTION_REQUESTED))
        {
            memcpy(index,&g,1);
            return 1;
        }
    }
    return 0;
}

uint8 insertInPacketBuffer(uint8* packet,uint16 length)
{
    uint8 bufferSize=getPacketBufferSize();
    
    if (bufferSize==MAX_P_BUFFER_SIZE)
    {
        //drop packet. no more packets can be accomodated in Buffer
        return 0;
    }
    
    if (length > 512)
        return 0;
    
    memcpy((recPacketBuffer[recPacketWriteIndex]+514),packet,length);
    memcpy((recPacketBuffer[recPacketWriteIndex]+514+512),&length,2);
    
    recPacketWriteIndex++;
    
    if (recPacketWriteIndex==5)
        recPacketWriteIndex=0;   
    recPacketBufferSize++; 
    return 1;     
}

void fetchPacketFromBuffer(uint8* pack)
{
    if (recPacketBufferSize==0)
        return;
    
    memcpy(pack,(recPacketBuffer[recPacketReadIndex]+514),514);
    
    recPacketReadIndex++;
    if (recPacketReadIndex==5)
        recPacketReadIndex=0;   
    recPacketBufferSize--;     
}

uint8 getPacketBufferSize()
{
    return recPacketBufferSize;
}

#endif //#ifdef ENABLE_ETH