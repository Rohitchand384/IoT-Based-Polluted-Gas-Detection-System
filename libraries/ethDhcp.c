// DHCP look-up functions based on the udp client
// http://www.ietf.org/rfc/rfc2131.txt
//
// Changed by: Dov Rosenberg <dovrose@gmail.com>
// Date: 01/05/2013
// Author: Andrew Lindsay
// Rewritten and optimized by Jean-Claude Wippler, http://jeelabs.org/
//
// Rewritten dhcpStateMachine by Chris van den Hooven 
// as to implement dhcp-renew when lease expires (jun 2012)
//
// Various modifications and bug fixes contributed by Victor Aprea (oct 2012)
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include "ether.h"
#include "config.h"
#include "eth_net.h"
#include "node.h"
#include "ether.h"
#include "string.h"
#include "config.h"

#ifdef ENABLE_ETH

#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTRESPONSE 2

// DHCP Message Type (option 53) (ref RFC 2132)
#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_RELEASE 7

// size 236
typedef struct {
    uint8 op, htype, hlen, hops;
    uint32 xid;
    uint16 secs, flags;
    uint8 ciaddr[4], yiaddr[4], siaddr[4], giaddr[4];
    uint8 chaddr[16], sname[64], file[128];
    // options
} DHCPdata;



/* 
   op            1  Message op code / message type.
                    1 = BOOTREQUEST, 2 = BOOTREPLY
   htype         1  Hardware address type, see ARP section in "Assigned
                    Numbers" RFC; e.g., '1' = 10mb ethernet.
   hlen          1  Hardware address length (e.g.  '6' for 10mb
                    ethernet).
   hops          1  Client sets to zero, optionally used by relay agents
                    when booting via a relay agent.
   xid           4  Transaction ID, a random number chosen by the
                    client, used by the client and server to associate
                    messages and responses between a client and a
                    server.
   secs          2  Filled in by client, seconds elapsed since client
                    began address acquisition or renewal process.
   flags         2  Flags (see figure 2).
   ciaddr        4  Client IP address; only filled in if client is in
                    BOUND, RENEW or REBINDING state and can respond
                    to ARP requests.
   yiaddr        4  'your' (client) IP address.
   siaddr        4  IP address of next server to use in bootstrap;
                    returned in DHCPOFFER, DHCPACK by server.
   giaddr        4  Relay agent IP address, used in booting via a
                    relay agent.
   chaddr       16  Client hardware address.
   sname        64  Optional server host name, null terminated string.
   file        128  Boot file name, null terminated string; "generic"
                    name or null in DHCPDISCOVER, fully qualified
                    directory-path name in DHCPOFFER.
   options     var  Optional parameters field.  See the options
                    documents for a list of defined options.
 */

// timeouts im ms 
#define DHCP_REQUEST_TIMEOUT 10000

static char hostname[] = "SENSEnutsEth";
static uint32 currentXid;
static uint32 stateTimer;
static uint32 leaseStart;
static uint32 leaseTime;

uint8 dhcpState=DHCP_STATE_INIT;
extern Time currentTime ;

// Main DHCP sending function

// implemented
// state             / msgtype       
// INIT              / DHCPDISCOVER 
// SELECTING         / DHCPREQUEST 
// BOUND (RENEWING)  / DHCPREQUEST

// ----------------------------------------------------------
// |              |SELECTING    |RENEWING     |INIT         |
// ----------------------------------------------------------
// |broad/unicast |broadcast    |unicast      |broadcast    |
// |server-ip     |MUST         |MUST NOT     |MUST NOT     | option 54
// |requested-ip  |MUST         |MUST NOT     |MUST NOT     | option 50
// |ciaddr        |zero         |IP address   |zero         |
// ----------------------------------------------------------

// options used (both send/receive)
// 12  Host Name Option
// 50  Requested IP Address
// 51  IP Address Lease Time
// 53  DHCP message type
// 54  Server-identifier
// 55  Parameter request list
// 58  Renewal (T1) Time Value
// 61  Client-identifier
// 255 End

static void send_dhcp_message (void) 
{
    uint8 buf[512];
    memset(buf, 0, 512);
	
	buf[0]=DHCP_BOOTREQUEST;
	buf[1]=1;
	buf[2]=6;
	buf[3]=0;
	memcpy(&buf[4],&currentXid,4);
		
    if (dhcpState == DHCP_STATE_BOUND) 
    {
		memcpy(&buf[12],ipAddr,4);
	}	
    
    memcpy(&buf[28],macAddr,6);
    //to maintain compatibility with BOOTP, next 192 bits are blank
    
    static uint8 magicCookie[] = { 99, 130, 83, 99, 53, 1 };
    
     // DHCP magic cookie, followed by message type
    memcpy(&buf[236],magicCookie,6);
    
    buf[242]= (dhcpState == DHCP_STATE_INIT ? DHCP_DISCOVER : DHCP_REQUEST);
    
    uint8 ci[]={61,7,1};
    memcpy(&buf[243],ci,3);
    memcpy(&buf[246],macAddr,6);
        
    //hostname options
    uint8 sizeOfHostname=strlen((const char*)(hostname));
    
    buf[252]=12;
    buf[253]=sizeOfHostname;
    memcpy(&buf[254],hostname,sizeOfHostname);
    
    int if1=0;	
	if( dhcpState == DHCP_STATE_SELECTING) 
	{
	    //Request Ip
	    buf[254+sizeOfHostname]=50;
	    buf[255+sizeOfHostname]=4;
	    
	    memcpy(&buf[256+sizeOfHostname],ipAddr,4);
       

        // Request using server ip address
        buf[260+sizeOfHostname]=54;
	    buf[261+sizeOfHostname]=4;
	    
	    memcpy(&buf[262+sizeOfHostname],dhcpSIp,4);
	    if1=12;
    }
    
    // Additional info in parameter list - minimal list for what we need, subnet mask, domain name server, router
    static uint8 tail[] = { 55, 3, 1, 3, 6, 255 };
    memcpy(&buf[254+if1+sizeOfHostname],tail,6);
	
	uint8 udpPacket[410];
	memset(udpPacket,0,410);	
	uint16 udpLen=prepUdpHeader(udpPacket,DHCP_DEST_PORT,DHCP_SRC_PORT,260+if1+sizeOfHostname);
	
	memcpy(&udpPacket[8],buf,260+if1+sizeOfHostname);
    
    uint8 ipv4[450];
    memset(ipv4,0,450);
    uint8 destIp[]={255,255,255,255};
    
    uint8 srcIp[]={0,0,0,0};
    uint16 ipLen=prepIpv4Header(ipv4,destIp,srcIp,udpLen,IP_PROTO_UDP_V);
    memcpy(&ipv4[20],udpPacket,udpLen);
    
    updateUdpChecksum(ipv4,ipLen);
   
    uint16 ethType=ETHTYPE_IP_H_V;
    ethType=ethType<<8;
    ethType|=ETHTYPE_IP_L_V;
    uint8 bAddr[]={0xff,0xff,0xff,0xff,0xff,0xff};   
    sendEthFrame(ipv4,ipLen,ethType,bAddr);
    
    //enable broadcast receive, once receive disable
    //enableBroadcast ();
}

static void process_dhcp_offer (uint8* buf,uint16 len) 
{ 
    // Map struct onto payload
    DHCPdata *dhcpPtr = (DHCPdata*) (buf + UDP_DATA_P);
    // Offered IP address is in yiaddr
    memcpy(ipAddr, dhcpPtr->yiaddr,4);
    //debug(ipAddr,4);
    //msdelay(100);
   
    // Scan through variable length option list identifying options we want
    uint8 *ptr = (uint8*) (dhcpPtr + 1) + 4;
    do {
        uint8 option = *ptr++;
        uint8 optionLen = *ptr++;
        switch (option) 
        {
            case 1:  
            {
                memcpy(networkMask, ptr,4);
                break;
            }         
            case 3:
            {
                memcpy(gatewayIp, ptr,4);
                break;
            }    
            case 6:  
            {
                memcpy(dnsIp, ptr,4);
                break;
            }    
            case 51:
            case 58:
            {
                leaseTime = 0; // option 58 = Renewal Time, 51 = Lease Time
                uint8 i=0;
                for (i = 0; i<4; i++)
                   leaseTime = (leaseTime << 8) + ptr[i];
                leaseTime *= 1000;      // milliseconds
                break;
            }   
            case 54:
            { 
                memcpy(dhcpSIp, ptr,4);
                break;
            }    
        }
        ptr += optionLen;
    } while (ptr < buf + len);
}

static bool dhcp_received_message_type (uint8* buf,uint16 len, uint8 msgType) 
{
    // Map struct onto payload
    DHCPdata *dhcpPtr = (DHCPdata*) (buf + UDP_DATA_P);
	
	if (len >= 70 && buf[UDP_SRC_PORT_L_P] == DHCP_SRC_PORT &&
       dhcpPtr->op == DHCP_BOOTRESPONSE && dhcpPtr->xid == currentXid ) 
    {
	
		int optionIndex = UDP_DATA_P + sizeof( DHCPdata ) + 4;
		return buf[optionIndex] == 53 ? buf[optionIndex+2] == msgType : FALSE;
	}
	else 
	{
		return FALSE;
	}
}

void dhcpReceiveMessage(uint8* buf,uint16 len)
{
    switch (dhcpState) 
	{
	    case DHCP_STATE_SELECTING:
	    {
	        if (dhcp_received_message_type(buf,len, DHCP_OFFER)) 
	        {
				process_dhcp_offer(buf,len);
				send_dhcp_message();
				dhcpState = DHCP_STATE_REQUESTING;
			    stateTimer = currentTime.clockLow;
			} 
			break;
	        
	    }
	    case DHCP_STATE_REQUESTING:
	    case DHCP_STATE_RENEWING:
	    {
	        if (dhcp_received_message_type(buf,len, DHCP_ACK)) 
	        {
				//disableBroadcast
				dhcpState = DHCP_STATE_BOUND;
				leaseStart = currentTime.clockLow;
				
				make_arp_request(gatewayIp);
				if (!memcmp(dhcpSIp,gatewayIp,4))
				{
				    //gateway is the dhcp server
				    //save its mac address
				    memcpy(gwMac,&buf[6],6);
				    
				    gwMacAvail=1;     
				}
				/*else
				{
				    //need to start the arp request for gateway
				    make_arp_request(gatewayIp);
				}*/
				dhcpState = DHCP_STATE_BOUND;
			}
	        break;
	    }	     
	}    
}

uint8  dhcpStateMachine() 
{
	switch (dhcpState) 
	{
	    case DHCP_STATE_INIT:
	    {
	        destMacTableSize=0;
	        gwMacAvail=0;
	        openPortSize=0;
	        allowedTupleSize=0;
	        uint32 a=eRand();
	        a=a<<16;
	        a|=eRand();
			currentXid = a;//currentTime.clockLow;
			memset(ipAddr,0,4); // force ip 0.0.0.0
			send_dhcp_message();
			//enableBroadcast();
			dhcpState = DHCP_STATE_SELECTING;
			stateTimer = currentTime.clockLow;
			break;
	    }
	    
	    case DHCP_STATE_BOUND:
		{
		    if (currentTime.clockHigh>0)        //case to handle when all times in milliseconds go beyond 32 bits
		        vAHI_SwReset();
		        
		    if (currentTime.clockLow >= leaseStart + leaseTime) {
				send_dhcp_message(); 
				dhcpState = DHCP_STATE_RENEWING;
				stateTimer = currentTime.clockLow;
			} 
			
			//Lets handle tuple expiry also here
			int y=0;
			
			if (allowedTupleSize==1&&currentTime.clockLow-aTuples[0].lastUsedTime>TUPLE_NO_USE_EXPIRY_TIME)
			    allowedTupleSize=0;
			else    
			    for(;y<allowedTupleSize;y++)
			    {
			        if (currentTime.clockLow-aTuples[y].lastUsedTime>TUPLE_NO_USE_EXPIRY_TIME)
			        {
			            //delete entry
			           
			            int j=y;
			            for(;j<allowedTupleSize-1;j++)
			            {
			                memcpy(&aTuples[j],&aTuples[j+1],sizeof(allowedTuples));
			            }
			            allowedTupleSize--;
			        }
			    } 
			
			if (gwMacAvail==0)
			{
			    make_arp_request(gatewayIp);
			   
			 }   
			break;
	    }
					
		case DHCP_STATE_REQUESTING:
		case DHCP_STATE_RENEWING:
		case DHCP_STATE_SELECTING: 
	    {
			if (currentTime.clockLow > stateTimer + DHCP_REQUEST_TIMEOUT) 
			{
				dhcpState = DHCP_STATE_INIT;
			}
			
			break;
	    }	 
	}
	return dhcpState;
}

void resetDhcpState()
{
    memset(ipAddr,0,4);
    dhcpState=DHCP_STATE_INIT;
}

uint8 getDhcpSate()
{
    return dhcpState;
}

uint32 getCurrentXid()
{
    return currentXid;
}

#endif //#ifdef ENABLE_ETH