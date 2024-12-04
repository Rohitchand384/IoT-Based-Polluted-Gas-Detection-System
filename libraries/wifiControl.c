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
#include <AppQueueApi.h>
#include "clock.h"  						//provides clock related functionality
#include "node.h"
#include "task.h"
#include "string.h"
#include "util.h"

/*&&&&&&&&&&&&&&&&&&&&& Do not modify above this line &&&&&&&&&&&&&&&&&&&&&&&&&&&*/
#include "dio.h"
#include "pcInterface.h"
#include "flash.h"
#include "node.h"
#include "wifiControl.h"
#include "mac.h"
#include "EEPROM.h"
#include "routing_bi_mbr.h"
#include "uart.h"
#include "config.h"

#ifdef WIFI_ENABLE
#ifdef UART0
/*&&&&&&& Variable specific to Protocol for Routing &&&&&&&&*/	


#define CONFIGURE_WIFI_PARAM 0x00
#define START_WIFI_GATEWAY_IN_WLAN 0x01
#define READ_IN_SOFTAP_MODE 0x02
#define CONFIGURE_DEVICE_IN_SOFTAP 0x03
#define WIFI_DATA_SECTOR_ADDR 0x00
#define WIFI_DATA_INDEX 0x02
#define START_GATEWAY 100

/*&&&&&&&&&&&&&&&&&&&& To Hanlde wifi Poll Flag is use &&&&&&&&&&&&&&&&&&&&&&&&&&*/					
static uint8 flag =0;

uint8 indexSize;
uint8 rx[200];
uint8 handleServerError = 1;

uint8 pollStartedForData=0;
uint8 pendingPackets=0;

streamTable streamInfo[8];      //this system can handle 8 parallel data streams
uint8 numStreams=0;

//one client per device supported. Extend this to an array and save data properly to save more clients
uint8 tcpClientHandle=0;

/*
 * Function to initlize wifi interface on sensenuts device
 * configure wifi in Machine mode
 * off all debug
 */


void wifiInit()
{
    
	uint8 wifiCmdLen = 0;
	wifiReset();
	
	uint8 enableSysHeader[100] =  "set system.cmd.header_enabled 0";           
    wifiCmdLen = strlen ((const char*)enableSysHeader);
    enableSysHeader[wifiCmdLen] = 0x0d;
    enableSysHeader[wifiCmdLen+1] = 0x0a;
    uartSend(E_AHI_UART_0,enableSysHeader, wifiCmdLen+2);
    wifiPoll();							        
	indexSize=0;
	msdelay (2000);
	
	/*&&&&&&&&&&& Command to disables echo from Wifi &&&&&&&&&&&&&&&&&&*/
	uint8 offEcho [25] = "set system.cmd.echo off";			        		
	wifiCmdLen = strlen ((const char*)offEcho);
	offEcho [wifiCmdLen] = 0x0d;
	offEcho [wifiCmdLen+1] = 0x0a;
	uartSend(E_AHI_UART_0,offEcho, wifiCmdLen+2);
	wifiPoll();							        
	indexSize=0;
	msdelay (2000);
	
	//check if stray bytes available if so discard 
	uint8 checkFIFO = u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
	if (checkFIFO != 0)
	{
		wifiPoll();							        
		indexSize=0;
	}
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
	
		
	/*&&&&&&&&& To disable the promo code from wifi Module &&&&&&&&&&&&*/
	uint8 offUserPromot[33] =  "set system.cmd.prompt_enabled 0";           
	wifiCmdLen = strlen ((const char*)offUserPromot);
	offUserPromot[wifiCmdLen] = 0x0d;
	offUserPromot[wifiCmdLen+1] = 0x0a;
	uartSend(E_AHI_UART_0,offUserPromot, wifiCmdLen+2);
	wifiPoll();							        
	indexSize=0;
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
	
	
	/*&&&&&&&&&&&&&&&&&& To disable Response codes &&&&&&&&&&&&&&&&&&&&*/
	uint8 offDebug[26] = "set system.print_level 0";		       			
	wifiCmdLen = strlen ((const char*)offDebug);
	offDebug[wifiCmdLen] = 0x0d;
	offDebug[wifiCmdLen+1] = 0x0a;
	uartSend(E_AHI_UART_0,offDebug, wifiCmdLen+2);
	wifiPoll();							        
	indexSize=0;
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
	
	
	/*&&&&&&&&&&&& To set Wifi UART Tx Buffer Size 2048 &&&&&&&&&&&&&&&*/
	uint8 wifiTxBufferSize [17] = "set bu c t 2048";							
	wifiCmdLen = strlen ((const char*)wifiTxBufferSize);
	wifiTxBufferSize [wifiCmdLen] = 0x0d;
	wifiTxBufferSize [wifiCmdLen+1] = 0x0a;
	uartSend(E_AHI_UART_0,wifiTxBufferSize, wifiCmdLen+2);
	wifiPoll();
	indexSize=0;
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
	
	
	/*&&&&&&&&&&&& To Set Wifi UART Rx Buffer Size 2048 &&&&&&&&&&&&&&&*/
	uint8 wifiRxBufferSize [17] = "set bu c r 2048";							
	wifiCmdLen = strlen ((const char*)wifiRxBufferSize);
	wifiRxBufferSize [wifiCmdLen] = 0x0d;
	wifiRxBufferSize [wifiCmdLen+1] = 0x0a;
	uartSend(E_AHI_UART_0,wifiRxBufferSize, wifiCmdLen+2);
	wifiPoll();
	indexSize=0;		
	/*&&&&&&&& To Reset Faults Counter to avoid device stucking &&&&&&&*/
	uint8 resetFaults[14] =  "faults_reset";           
	resetFaults[12] = 0x0d;
	resetFaults[13] = 0x0a;
	uartSend(E_AHI_UART_0,resetFaults, 14);
	wifiPoll();							        
	indexSize=0;
}

/*function to start ap mode*/
void startUdpServerInSoftAPMode(const char *SOFTAP_SSID,const char *SOFTAP_PASSWORD)
{
	uint8 softAPAtoStart [25] = "set softap.auto_start 1";				//Start device in SoftAp mode after Reboot
	softAPAtoStart [23] = 0x0d;
	softAPAtoStart [24] = 0x0a;
	uint8 softAPSSID [50] = "set softap.ssid ";				//Give SSID in SoftAp mode
	uint8 softAPPassword [50] = "set softap.passkey ";			//Give softAp Password
	/*&&&&&&&&&&&&&&&&&&&&& Append softAp SSID from config.h file &&&&&&&&&&&&&&&&&&&*/	
	uint8 softApSsidCmdSize = strlen ((const char*)softAPSSID);
	uint8 softApPasswdCmdSize = strlen((const char*)softAPPassword);
	uint8 softApSsidSize = strlen(SOFTAP_SSID);
	uint8 softApPasswdSize = strlen(SOFTAP_PASSWORD);
	memcpy(&softAPSSID[softApSsidCmdSize],SOFTAP_SSID,softApSsidSize);
	softAPSSID[softApSsidCmdSize+softApSsidSize] = 0x0d;
	softAPSSID[softApSsidCmdSize+softApSsidSize+1] = 0x0a;
	memcpy(&softAPPassword[softApPasswdCmdSize],SOFTAP_PASSWORD,softApPasswdSize);
	softAPPassword [softApPasswdCmdSize+softApPasswdSize] = 0x0d;
	softAPPassword [softApPasswdCmdSize+softApPasswdSize+1] = 0x0a;
	
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
	uint8 softAPUDPServerAuto [38] = "set udp.server.auto_interface softap";
	softAPUDPServerAuto [36] = 0x0d;
	softAPUDPServerAuto [37] = 0x0a;
	uint8 softAPUdpServerAutoStart [29] = "set udp.server.auto_start 1";		//Set UDP auto start on wifi Reboot
	softAPUdpServerAutoStart [27] = 0x0d;
	softAPUdpServerAutoStart [28] = 0x0a;
	uint8 softAPUdpServerPort [26] = "set udp.server.port 8000";			// 8000 as PORT no
	softAPUdpServerPort [24] = 0x0d;
	softAPUdpServerPort [25] = 0x0a;
	uint8 softAPSave[6] = "save";							//save these settings to wifi Flash
	softAPSave [4] =0x0d;
	softAPSave [5] =0x0a;
	uint8 softAPReboot [8] = "reboot";						//Reboot Wifi
	softAPReboot [6] = 0x0d;
	softAPReboot [7] = 0x0a;

	/*&&&&&&&&&&&&&&&&&&&& Send All these above command to wifi &&&&&&&&&&&&&&&&&&&&&&&*/
	uartSend(E_AHI_UART_0,softAPAtoStart, 25);
	wifiPoll();
	indexSize =0;
	uartSend(E_AHI_UART_0,softAPSSID , softApSsidCmdSize+softApSsidSize+2);
	wifiPoll();
	indexSize=0;
	uartSend(E_AHI_UART_0,softAPPassword , softApPasswdCmdSize+softApPasswdSize+2);
	wifiPoll();
	indexSize=0;
	uartSend(E_AHI_UART_0,softAPUDPServerAuto,38);
	wifiPoll();
	indexSize=0;
	uartSend(E_AHI_UART_0,softAPUdpServerAutoStart,29);
	wifiPoll();
	indexSize=0;
	uartSend(E_AHI_UART_0,softAPUdpServerPort, 26);
	wifiPoll();
	indexSize =0;
	uartSend(E_AHI_UART_0,softAPSave, 6);
	wifiPoll();
	indexSize=0;
	uartSend(E_AHI_UART_0,softAPReboot,8);
	wifiPoll();
	indexSize=0;
	
	if (pollStartedForData==0)
	{    
	    
	    addTask(USER,POLL_DATA,1*SECOND);
	    pollStartedForData=1;
	}       
	return 1;
}

/*
 * Function to start wifi Station Mode 
 * SSID and Password must saved in this function
 * Return Nothing
 */
 
void saveWlanSsidPassword(const char *WLAN_SSID,const char *WLAN_PASSWORD)
{
	uint8 wifiSSID [50] = "set wlan.ssid ";						
	uint8 wifiPasskey [50] = "set wlan.passkey ";					
	int ssidCmdSize = strlen((const char*) wifiSSID);
	int passwdCmdSize = strlen((const char*) wifiPasskey);
	int ssidSize = strlen((const char*) WLAN_SSID);
	int passwdSize = strlen((const char*) WLAN_PASSWORD);
	memcpy(&wifiSSID[ssidCmdSize],WLAN_SSID,ssidSize);
	wifiSSID[ssidCmdSize+ssidSize] = 0x0d;
	wifiSSID[ssidCmdSize+ssidSize+1] = 0x0a;
	memcpy(&wifiPasskey[passwdCmdSize],WLAN_PASSWORD,passwdSize);
	wifiPasskey[passwdCmdSize+passwdSize] = 0x0d;
	wifiPasskey[passwdCmdSize+passwdSize+1] = 0x0a;
	
	/*&&&&&&&&&&&&&&&&&&& Send commands to Wifi &&&&&&&&&&&&&&&&&*/
	uartSend(E_AHI_UART_0,wifiSSID, ssidCmdSize+ssidSize+2);
	wifiPoll();
	indexSize = 0;
	uartSend(E_AHI_UART_0,wifiPasskey , passwdCmdSize+passwdSize+2);
	wifiPoll();
	indexSize = 0;
	
	/*&&&&&&&&&&&&&& Command to save Certificate for HTTPS &&&&&&&&&&&&*/
	uint8 certCmd1 [6] = "save";
	certCmd1 [4]=0x0d;
	certCmd1 [5]=0x0a;
	uartSend(E_AHI_UART_0,certCmd1, 6);
	wifiPoll();							        
	indexSize=0;

}

/*
 * Function to start wifi in Station Mode
 * take ssid and password from argument
 * start udp server on portNo 
 * Return SUCCESS = 1
 * Return FAILURE = -1
 */

int wlanUdpServerStart(uint8 *portNo)
{
	uint8 retryCounter=0;
	uint8 wifiCmdLen = 0;

#ifdef STATIC_IP

if (pollStartedForData==0)
{     
  #ifdef WIFI_VER1
    uint8 setStaticIp[30]="set st i ";  //ip
    uint8 staticIp[16]="192.168.1.10";
    memcpy(&setStaticIp[9],staticIp,strlen((const char*)staticIp));
    uint8 size=9+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();
     
       
    setStaticIp[7]='g';    //gateway
    memcpy(staticIp,"192.168.1.1",sizeof("192.168.1.1"));
    memcpy(&setStaticIp[9],staticIp,strlen((const char*)staticIp));
    size=9+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();    
    
    setStaticIp[7]='n';    //network mask
    memcpy(staticIp,"255.255.255.0",sizeof("255.255.255.0"));
    memcpy(&setStaticIp[9],staticIp,strlen((const char*)staticIp));
    size=9+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset(); 
        
    setStaticIp[7]='d';    //network mask
    memcpy(staticIp,"8.8.8.8",sizeof("8.8.8.8"));
    memcpy(&setStaticIp[9],staticIp,strlen((const char*)staticIp));
    size=9+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();
        
    uint8 dhcpDisable[16]="set ne d e off";
    dhcpDisable[14]=0x0d;
    dhcpDisable[15]=0x0a;
    uartSend(E_AHI_UART_0,dhcpDisable,16);         
     wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();     
  #else
    uint8 setStaticIp[30]="set wl t d ";  //dns
    uint8 staticIp[16]="8.8.8.8";
    memcpy(&setStaticIp[11],staticIp,strlen((const char*)staticIp));
    uint8 size=11+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();
        
    setStaticIp[9]='g';    //gateway
    memcpy(staticIp,"192.168.1.1",sizeof("192.168.1.1"));
    memcpy(&setStaticIp[11],staticIp,strlen((const char*)staticIp));
    size=11+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();  
        
    setStaticIp[9]='i';    //ip
    memcpy(staticIp,"192.168.1.10",sizeof("192.168.1.10"));
    memcpy(&setStaticIp[11],staticIp,strlen((const char*)staticIp));
    size=11+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset(); 
        
    setStaticIp[9]='n';    //netmask
    memcpy(staticIp,"255.255.255.0",sizeof("255.255.255.0"));
    memcpy(&setStaticIp[11],staticIp,strlen((const char*)staticIp));
    size=11+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();
        
    uint8 dhcpDisable[25]="set wlan.dhcp.enabled 0";
    dhcpDisable[23]=0x0d;
    dhcpDisable[24]=0x0a;
    uartSend(E_AHI_UART_0,dhcpDisable,25);         
     wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();                
  #endif    
}        	
#endif	
	int portNoSize = strlen ((const char*) portNo);
	uint8 startUDPServer [25] = "udp_server start ";
	wifiCmdLen = strlen((const char*) startUDPServer);
	memcpy (&startUDPServer[wifiCmdLen], portNo, portNoSize);				
	startUDPServer [wifiCmdLen+portNoSize] = 0x0d;
	startUDPServer [wifiCmdLen+portNoSize+1] = 0x0a;
	uartSend(E_AHI_UART_0,startUDPServer, wifiCmdLen+portNoSize+2);
	wifiPoll();
	
	/*&&&&&&&&&&&&&&&& Retry 20 times untill Device on find &&&&&&&&&&&&&&*/
	while (rx[0] == 'c' ||rx[0] == 'C' || rx[0] == 'U')	
	{
		retryCounter++;
		indexSize=0;
		/*&&&&&&&& To Reset Faults Counter to avoid device stucking &&&&&&&*/
		uint8 resetFaults[14] =  "faults_reset";           
		resetFaults[12] = 0x0d;
		resetFaults[13] = 0x0a;
		uartSend(E_AHI_UART_0,resetFaults, 14);
		wifiPoll();							        
		indexSize=0;
		/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/

		uartSend(E_AHI_UART_0,startUDPServer, 23);
		wifiPoll();
		
		if (retryCounter == 10)
		{
		    handleServerError=1;
			indexSize=0;
			return -1;
		}
	}
	indexSize=0;
	handleServerError=0;
	
	if (pollStartedForData==0)
	{    
	    
	    addTask(USER,POLL_DATA,1*SECOND);
	    pollStartedForData=1;
	}
	indexSize=0;        
	return 1;
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
}

/*Function to make a TCP server at the specific port of device*/

int wlanTcpServerStart(uint8 *portNo)
{
	uint8 retryCounter=0;
	uint8 wifiCmdLen = 0;

#ifdef STATIC_IP

if (pollStartedForData==0)
{     
  #ifdef WIFI_VER1
    uint8 setStaticIp[30]="set st i ";  //ip
    uint8 staticIp[16]="192.168.1.10";
    memcpy(&setStaticIp[9],staticIp,strlen((const char*)staticIp));
    uint8 size=9+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();
     
       
    setStaticIp[7]='g';    //gateway
    memcpy(staticIp,"192.168.1.1",sizeof("192.168.1.1"));
    memcpy(&setStaticIp[9],staticIp,strlen((const char*)staticIp));
    size=9+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();    
    
    setStaticIp[7]='n';    //network mask
    memcpy(staticIp,"255.255.255.0",sizeof("255.255.255.0"));
    memcpy(&setStaticIp[9],staticIp,strlen((const char*)staticIp));
    size=9+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset(); 
        
    setStaticIp[7]='d';    //network mask
    memcpy(staticIp,"8.8.8.8",sizeof("8.8.8.8"));
    memcpy(&setStaticIp[9],staticIp,strlen((const char*)staticIp));
    size=9+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();
        
    uint8 dhcpDisable[16]="set ne d e off";
    dhcpDisable[14]=0x0d;
    dhcpDisable[15]=0x0a;
    uartSend(E_AHI_UART_0,dhcpDisable,16);         
     wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();     
  #else
    uint8 setStaticIp[30]="set wl t d ";  //dns
    uint8 staticIp[16]="8.8.8.8";
    memcpy(&setStaticIp[11],staticIp,strlen((const char*)staticIp));
    uint8 size=11+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();
        
    setStaticIp[9]='g';    //gateway
    memcpy(staticIp,"192.168.1.1",sizeof("192.168.1.1"));
    memcpy(&setStaticIp[11],staticIp,strlen((const char*)staticIp));
    size=11+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();  
        

    setStaticIp[9]='i';    //ip
    memcpy(staticIp,"192.168.1.10",sizeof("192.168.1.10"));
    memcpy(&setStaticIp[11],staticIp,strlen((const char*)staticIp));
    size=11+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset(); 
        
    setStaticIp[9]='n';    //netmask
    memcpy(staticIp,"255.255.255.0",sizeof("255.255.255.0"));
    memcpy(&setStaticIp[11],staticIp,strlen((const char*)staticIp));
    size=11+strlen((const char*)staticIp);
    setStaticIp[size++]=0x0d;
    setStaticIp[size++]=0x0a;
    uartSend(E_AHI_UART_0,setStaticIp,size);
    wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();
        
    uint8 dhcpDisable[25]="set wlan.dhcp.enabled 0";
    dhcpDisable[23]=0x0d;
    dhcpDisable[24]=0x0a;
    uartSend(E_AHI_UART_0,dhcpDisable,25);         
     wifiPoll();
    indexSize=0;
    if (rx[0]!='S')
        vAHI_SwReset();                
  #endif    
}        	
#endif	
	int portNoSize = strlen ((const char*) portNo);
	uint8 startTcpServer [25] = "tcp_server start ";
	wifiCmdLen = strlen((const char*) startTcpServer);
	memcpy (&startTcpServer[wifiCmdLen], portNo, portNoSize);				
	startTcpServer [wifiCmdLen+portNoSize] = 0x0d;
	startTcpServer [wifiCmdLen+portNoSize+1] = 0x0a;
	uartSend(E_AHI_UART_0,startTcpServer, wifiCmdLen+portNoSize+2);
	wifiPoll();
	
	/*&&&&&&&&&&&&&&&& Retry 20 times untill Device on find &&&&&&&&&&&&&&*/
	while (rx[0] == 'c' ||rx[0] == 'C' || rx[0] == 'U')	
	{
		retryCounter++;
		indexSize=0;
		/*&&&&&&&& To Reset Faults Counter to avoid device stucking &&&&&&&*/
		uint8 resetFaults[14] =  "faults_reset";           
		resetFaults[12] = 0x0d;
		resetFaults[13] = 0x0a;
		uartSend(E_AHI_UART_0,resetFaults, 14);
		wifiPoll();							        
		indexSize=0;
		/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/

		uartSend(E_AHI_UART_0,startTcpServer, 23);
		wifiPoll();
		
		if (retryCounter == 10)
		{
		    handleServerError=1;
			indexSize=0;
			return -1;
		}
	}
	indexSize=0;
	handleServerError=0;
	
	if (pollStartedForData==0)
	{    
	    
	    addTask(USER,POLL_DATA,1*SECOND);
	    pollStartedForData=1;
	}
	indexSize=0;        
	return 1;
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
}

/*
 * Function to Write data on wifi UDP socket

 * we have to send data destination IP, portNo, data and size as argument
 * This is on udp port
 * Max Data Packet size is 256 byte
 */


void sendUdpPkt_Wifi(const char* ip, uint16 portNo, uint8 * data, int size)
{
    if (handleServerError==1)
    {
        vAHI_SwReset();
    }
	uint8 command[100]="udp_server write ";
	uint8 newSize = strlen ((const char*)command);
	newSize += intToStr (size, &command[newSize] );
	command [newSize] = 0x20;
	newSize++;
	memcpy (&command[newSize], ip, strlen ((const char*)ip));
	newSize += strlen ((const char*)ip);
	command [newSize] = 0x20;
	newSize++;
	newSize += intToStr (portNo, &command[newSize]);
	command [newSize] = 0x0d;
	command [newSize+1] = 0x0a;
	newSize +=2;
	uartSend(E_AHI_UART_0,command,newSize);
	uartSend(E_AHI_UART_0,data,size);

#ifdef WIFI_GATEWAY_VER2    //amw037 device sends its data back
    wifiPoll();
    indexSize=0;    	
#endif	
	
	wifiPoll();
	if (rx[0] == 'C' && rx[1] == 'o' && rx[2] == 'm' && rx[3] == 'm' && rx[4] == 'a' && rx[5] == 'n' && rx[6] == 'd')
		addTask (USER,START_GATEWAY,0*SECOND);
	indexSize=0;
	
}

//this function checks if there is any packet available to be read
uint8 pollWifiData()
{
   uint8 pollData[13] = "poll all -c";
   pollData[11]=0x0d;
   pollData[12]=0x0a;
   indexSize=0;
   uartSend(E_AHI_UART_0,pollData, 13);
   wifiPoll();
   
   if (indexSize-2>4)             //if no stream is open "NONE" is sent by wifi module
   {
        int i=0;
        int numCommas=0;
        uint8 AvaiFlag=0;
        while (i<indexSize-1)   //remove last 0x0a
        {
            int j=0;
            uint8 num[10];
            while(1)
            {
                if (rx[i]==',' || rx[i]=='|' || rx[i]==0x0d)
                    break;
                num[j]=rx[i];
                j++;
                i++;    
            }
            i++;
                                    
            switch(numCommas)
            {
                case 0:
                {
                    streamInfo[numStreams].streamHandle=num[0]; //stream handle is always 1 byte
                    numCommas++;
                    break;
                }   
                case 1:
                {
                    if (num[0]=='1')
                    {
                        AvaiFlag=1;
                    }    
                    numCommas++;
                    break;
                }
                case 2:
                {
                    if (AvaiFlag==1 && num[0]!='0')
                    {
                      memcpy(&streamInfo[numStreams].numBytes,num,j);
                     
                      if (j<4)
                        num[j]=0;
                      else
                        num[5]=0; 
                      numStreams++;
                    }  
                    AvaiFlag=0;
                    numCommas=0;
                    break;
                } 
            }       
        }
        
        indexSize=0;
        uint8 tempNumStreams=0;
        
        while (tempNumStreams<numStreams)
        {
            uint8 stream_list[15] = "stream_list ";
            stream_list[12]=streamInfo[tempNumStreams].streamHandle;   //we know that stream handle can never be 2 bytes
            stream_list[13]=0x0d;
            stream_list[14]=0x0a;
            
            uartSend(E_AHI_UART_0,stream_list, 15);
            
            //quick but dirty fix for amw037
#ifdef WIFI_GATEWAY_VER2

            uint16 a=0;
		    uint8 isFound=0;
		    uint8 cmp[3]="# ";
            cmp[2]=streamInfo[tempNumStreams].streamHandle;
		    
		    do
		    {
		        uint32 counterrr=0;
		        do
		        {
		            a=0;
		            counterrr++;
		            if (counterrr == 9600000)
		            {
		                vAHI_SwReset();
		                return 0;
		            }
			        a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
		        }while (a<1);
		        
		        uint8 rxd = u8AHI_UartReadData(E_AHI_UART_0);
		        uint8 rxt[3];
		        if(rxd == '#')
		        {
		            rxt[0]=rxd;
		            do
		            {
		                a=0;
		                counterrr++;
		                if (counterrr == 9600000)
		                {
		                    vAHI_SwReset();
		                    return 0;
		                }
			            a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
		            }while (a<2);
		            
		            rxt[1]=u8AHI_UartReadData(E_AHI_UART_0);
		            rxt[2]=u8AHI_UartReadData(E_AHI_UART_0);
		            if((memcmp(rxt,cmp,3))==0)
                    {
                        isFound=1;
                        do
		                {
		                    a=0;
		                    counterrr++;
		                    if (counterrr == 9600000)
		                    {
		                        vAHI_SwReset();
		                        return 0;
		                    }
			                a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
		                }while (a<1);
                        rxd = u8AHI_UartReadData(E_AHI_UART_0);
                        //indexSize=indexSize-zdf-4;
                        //memcpy(rx,&rx[zdf+4],indexSize);
                    }
		        }    
		          
		    }while(isFound==0);
#endif
            wifiPoll();
                        
            if (rx[0]=='H')
            {
               indexSize=0;
               tempNumStreams++;
               continue; 
            }
          
            uint8 reqSpaces=0;
            if (rx[0]=='U')
                reqSpaces=3;
            else if(rx[0]=='T')
                reqSpaces=2;    
            
            uint8 f=0;
            uint8 numSpaces=0;
            while (numSpaces!=reqSpaces)
            {
                if (rx[f]==0x20)
                    numSpaces++;
                f++;    
            }
            
            uint8 g=0;
            while (rx[f]!=':')
            {
                streamInfo[tempNumStreams].ip[g]=rx[f];
                f++;
                g++;
            }
           
            f++;
            g=0;
            while(1)
            {
                if (rx[f]==0x0d || rx[f]==0x20)
                    break;
                streamInfo[tempNumStreams].port[g]=rx[f];
                g++;
                f++;
            }
            streamInfo[tempNumStreams].port[g]=0;
            
            indexSize=0;
            tempNumStreams++;   
        }
        
        if(numStreams==0)
            return 0;
        else    
            return 1;
   }
   else
   {
     indexSize=0;
     return 0;
   }
   
    
}

/*
 * Function to read any data available on 802.11 network
 * this will return 127 byte
 * restart wifi if hard fault occurs
 */

int8 readWifiData(uint8 streamId,uint8* bytesToRead)               //stream id is assumed to be 1 byte
{
	if (handleServerError == 1)
		vAHI_SwReset();
	uint8 readWifiDataFinal[19] = "stream_read ";
	readWifiDataFinal[12]=streamId;
	readWifiDataFinal[13]=0x20;
	uint8 size=strlen((const char*)bytesToRead);
	memcpy(&readWifiDataFinal[14],bytesToRead,size);				//command to read wifi data
	readWifiDataFinal[14+size] = 0x0d;
	readWifiDataFinal[15+size] = 0x0a;
	indexSize=0;
	uartSend(E_AHI_UART_0,readWifiDataFinal ,15+size);
	indexSize=0;
	uint8 fix=0;
	do
	{
		uint8 a=0;
		uint8 rxd;
		uint32 counterrr=0;

		do{
			counterrr++;
			a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
			if (counterrr == 322500)
				return -1;

		  }while(a<1);

		  rxd  =  u8AHI_UartReadData(E_AHI_UART_0);
		  if(rxd == 0x0d)
		  {
			flag = 1;
		  }
		  else if (rxd == 0x0a && flag == 1)
		  {
			  flag = 0;
			  fix=1;
		  }
		  else
		  {
			  flag=0;
		  }

		  if(indexSize < 127)
		  {
		  	rx[indexSize] = rxd;
		  }
		  
		  if(indexSize==127)
            indexSize=0;
		  indexSize++;
	}while (fix==0);
	
	if (rx[0] == 'C' && rx[1] == 'o' && rx[2] == 'm' && rx[3] == 'm' && rx[4] == 'a' && rx[5] == 'n' && rx[6] == 'd')
	{
	    vAHI_SwReset();
	    
	}
	/*&&&&&&&&&&&&&& Handle Hard Fault of Wifi Device &&&&&&&&&&&&&&&&&*/
	return 1;
	
}


void wifiPoll()
{
	if (handleServerError == 1)
		vAHI_SwReset();
	indexSize=0;
	uint8 fix=0;
	uint32 counterrr=0;
	do
	{
		uint8 a=0;
		uint8 rxd;

		do{
		    counterrr++;
		    if (counterrr == 9600000)
		    {
		        vAHI_SwReset();
		        return;
		    }
			a=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
		  }while(a<1);

		  rxd  =  u8AHI_UartReadData(E_AHI_UART_0);
		  if(rxd == 0x0d)
		  {
			flag = 1;
		  }
		  else if (rxd == 0x0a && flag == 1)
		  {
				flag = 0;
				fix=1;
			  	//if (rx[0] == 'c' ||rx[0] == 'C' || rx[0] == 'U')	
				//{
				//	facResetWifi();
				//	addTask (USER,START_GATEWAY,5*SECOND);
				//}
		  }
		  else
		  {
			  flag=0;
		  }

		  if(indexSize < 127)
		  {
		  	rx[indexSize] = rxd;
		  }

          //maximum size for data receive buffer for now is 127
          if (indexSize==127)
                indexSize=0;
		  indexSize++;
	}while (fix==0);
}

void switchWifiMode()
{
	vAHI_DioSetPullup(0x00000001<<1, 0x00000000);
	vAHI_DioInterruptEdge(0x00000000,0x00000001<<1);					
	vAHI_DioInterruptEnable(0x00000001<<1,0x00000000);
	vAHI_SysCtrlRegisterCallback(dioInterruptHandler);
}

void switchWifiModeHandler ()
{
	vAHI_DioInterruptEnable(0,2);
	uint32 checkDio=0;
	uint32 counter=0;

	while (counter < 6400000)
	{
		counter ++;
		checkDio = u32AHI_DioReadInput ();
		checkDio=checkDio & 0x02;
		if (checkDio == 0x02)
		{
			vAHI_DioInterruptEnable(2,0);
			return;	
		}
	}
	EEPROMSectorErase (0x00);
	vAHI_SwReset();
	
}

/*
 * Function to factory reset wifi Device
 */

void facResetWifi()
{
	msdelay (10000);
	vAHI_UartDisable(E_AHI_UART_0);	
	msdelay(10000);

	uartInit(E_AHI_UART_0,E_AHI_UART_RATE_115200); 
	uint8 i;
	uint8 getMac [10] = "get wl m";
	getMac[8] = 0x0d;
	getMac[9] = 0x0a;
	uint8 facReset [22] = "fac ";
	indexSize=0;
	uartSend(E_AHI_UART_0,getMac, 9);
	wifiPoll();
	for(i=0;i<17;i++)
	{
		facReset[4+i] = rx[i];
	}
	facReset [21] = 0x0d;
	uartSend(E_AHI_UART_0,facReset,22);
	wifiPoll();
	indexSize=0;
	wifiPoll();
	indexSize=0;
	msdelay (10000);
	vAHI_UartDisable(E_AHI_UART_0);	
	msdelay(5000);
	uartInit(E_AHI_UART_0,E_AHI_UART_RATE_115200); 
}


/* 
 * function to reset wifi 
 * using sesenuts DIO0
 * and NMOS

 */

void wifiReset()
{
	/*&&&&&&&&&&& Reset BCM43362 and Communication protocol &&&&&&&&&&&*/
#ifdef WIFI_GATEWAY_VER2
	#ifdef GW_ID
	clearPin (0);
	msdelay (100);
	setPin (0);
	#else
	clearPin (1);
	msdelay (100);
	setPin (1);
	#endif
#else
	clearPin (0);
	msdelay (100);
	setPin (0);
#endif

	handleServerError=0;
	msdelay (3000);
	vAHI_UartDisable(E_AHI_UART_0);	
	msdelay(1000);
	uartInit(E_AHI_UART_0,E_AHI_UART_RATE_115200); 
#ifdef AP_MODE_INVOLVED
	facResetWifi();
#endif	
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
}

uint8 makeTCPConnectionToServer (uint8* ipAddr, uint8* portAddr)
{
   // uint8 retryCounter=0;
    uint8 tcpConPkt [100] = "tcpc ";
    uint8 size = strlen ((const char*)tcpConPkt);
    memcpy (&tcpConPkt [size], ipAddr, strlen ((const char*)ipAddr));
    size += strlen ((const char*)ipAddr);
    tcpConPkt [size] = 0x20;
    size++;
    memcpy (&tcpConPkt [size], portAddr, strlen ((const char*)portAddr));
    size += strlen ((const char*)portAddr);
    tcpConPkt [size] = 0x0d;
    size ++;
    tcpConPkt [size] = 0x0a;
    size ++;
    uartSend(E_AHI_UART_0,tcpConPkt, size);
	wifiPoll();
    
    /*&&&&&&&&&&&&&&&& Retry 20 times untill Device on find &&&&&&&&&&&&&&*/
	if (rx[0] == 'C')	
	{
	    indexSize=0;
	    uint8 resetFaults[14] =  "faults_reset";           
		resetFaults[12] = 0x0d;
		resetFaults[13] = 0x0a;
		uartSend(E_AHI_UART_0,resetFaults, 14);
		wifiPoll();							        
		indexSize=0;
	    
		return 0;
	}
	
	tcpClientHandle=rx[0];
	indexSize=0;
	return 1;
	/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/

}

uint8 writeDataToTCPServer (uint8* data, uint8 size)
{
    uint8 command[100] = "stream_write 1 ";
    command[13]=tcpClientHandle;
	uint8 newSize = strlen ((const char*)command);
	newSize += intToStr (size, &command[newSize] );
	command [newSize] = 0x0d;
	command [newSize+1] = 0x0a;
	newSize +=2;
	uartSend(E_AHI_UART_0,command,newSize);
	uartSend(E_AHI_UART_0,data,size);
	wifiPoll();
	indexSize=0;
	
#ifdef WIFI_GATEWAY_VER2
    wifiPoll();
    indexSize=0;    	
#endif	
	if (rx[0] == 'S' && rx[1] == 'u' && rx[2] == 'c' && rx[3] == 'c' && rx[4] == 'e' && rx[5] == 's' && rx[6] == 's')
	{
	    return 1;
	}
	else
	{
	    return 0;
	}
}


void sendDataToThinkSpeakCloud(uint32 num,uint32 numb,uint8* key)
{
    //deprecated function, better to use wifiget here in place of this
	uint32 div = num;
	uint16 size=0;	

	uint8 closeStream [20] = "stream_close all";
	closeStream [16] = 0x0d;

	uint8 strForCloud [90] = "hge http://api.thingspeak.com/update?key=";
	size+=strlen((const char*)strForCloud);
	memcpy(&strForCloud[size],key,strlen((const char*)key));
	size+=strlen((const char*)key);
	memcpy(&strForCloud[size],"&field2=",8);
	size+=8;
	//HTTP_GET command

	if (num==0)
	{
		strForCloud [size++] = 48;	
	}
	else
	{
		uint8 arr[5];
		uint8 siz=0;

		while(div != 0)
		{
			div = div/10;
			siz++;
		}
		
		div = num;
		intToStr (div,arr);
		memcpy(&strForCloud[size],&arr[0],siz);
		size+=siz;		
	}


	memcpy(&strForCloud[size],"&field1=",8);
	size+=8;
	div=numb;

	if (numb==0)
	{
		strForCloud [size++] = 48;	
	}
	else
	{
		uint8 arr[5];
		uint8 siz=0;

		while(div != 0)
		{
			div = div/10;
			siz++;
		}
		
		div = numb;
		intToStr (div,arr);
		memcpy(&strForCloud[size],&arr[0],siz);
		size+=siz;		
	}
	strForCloud [size++] = 0x0d;
	
	uartSend(E_AHI_UART_0, strForCloud, size);
	wifiPoll();
	indexSize=0;
	msdelay(6000);
	uartSend(E_AHI_UART_0, closeStream , 17);
	wifiPoll();
	indexSize=0;
}

void wifiHttpGet(uint8* url)
{
	uint32 length=strlen((const char*)url);
	
	uint8 strForCloud [500] = "hge ";	//HTTP_GET command
	//modify size of strForCloud based on mac url size
	memcpy(&strForCloud[4], url, length);


	strForCloud [4+length] = 0x0d;
	uartSend(E_AHI_UART_0,strForCloud,5+length);
	wifiPoll();
	indexSize=0;
	
	if (rx[0] == 'c' ||rx[0] == 'C' || rx[0] == 'U')	
	{
	    vAHI_SwReset();
	}
	//when the data will be read stream will automatically close
}

void wifiHttpPost(uint8* url)
{
	uint32 length=strlen((const char*)url);
	
	uint8 strForCloud [500] = "hpo ";	//HTTP_GET command
	//modify size of strForCloud based on mac url size
	memcpy(&strForCloud[4],url,length);


	strForCloud [4+length] = 0x0d;
	uartSend(E_AHI_UART_0,strForCloud,5+length);
	wifiPoll();
	indexSize=0;
	
	if (rx[0] == 'c' ||rx[0] == 'C' || rx[0] == 'U')	
	{
	    vAHI_SwReset();
	}
	
	//when the data will be read stream will automatically close
}
void sendDataToXAMPP(uint32 num,uint32 numb,uint8* key)
{
    //deprecated function, better to use wifiget here in place of this
	uint32 div = num;
	uint16 size=0;	

	uint8 closeStream [20] = "stream_close all";
	closeStream [16] = 0x0d;

	uint8 strForCloud [90] = "hge http://192.168.227.92/sensenuts_project/post_sensor_data.php?key=";
	size+=strlen((const char*)strForCloud);
	memcpy(&strForCloud[size],key,strlen((const char*)key));
	size+=strlen((const char*)key);
	memcpy(&strForCloud[size],"&field2=",16);
	size+=8;
	//HTTP_GET command

	if (num==0)
	{
		strForCloud [size++] = 48;	
	}
	else
	{
		uint8 arr[5];
		uint8 siz=0;

		while(div != 0)
		{
			div = div/10;
			siz++;
		}
		
		div = num;
		intToStr (div,arr);
		memcpy(&strForCloud[size],&arr[0],siz);
		size+=siz;		
	}


	memcpy(&strForCloud[size],"&field1=",16);
	size+=8;
	div=numb;

	if (numb==0)
	{
		strForCloud [size++] = 48;	
	}
	else
	{
		uint8 arr[5];
		uint8 siz=0;

		while(div != 0)
		{
			div = div/10;
			siz++;
		}
		
		div = numb;
		intToStr (div,arr);
		memcpy(&strForCloud[size],&arr[0],siz);
		size+=siz;		
	}
	strForCloud [size++] = 0x0d;
	
	uartSend(E_AHI_UART_0, strForCloud, size);
	wifiPoll();
	indexSize=0;
	msdelay(1000);
	uartSend(E_AHI_UART_0, closeStream , 17);
	wifiPoll();
	indexSize=0;
}

void closeAllStreams()
{
    uint8 closeStream [20] = "stream_close all";
	closeStream [16] = 0x0d;
	closeStream [17] = 0x0a;
	uartSend(E_AHI_UART_0,closeStream,18);
	wifiPoll();
	indexSize=0;   
}

#endif

#endif //#ifdef WIFI_ENABLE
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
//ALTAIR04-900
/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/


