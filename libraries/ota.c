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

#include "task.h"
#include "node.h"
#include "mac.h"
#include "pcInterface.h"
#include "routing.h"
#include "ota.h"
#include "flash.h"
#include "string.h"
#include "dio.h"
#include "config.h"

#ifdef ENABLE_OTA

//to come from application layer. contains data  and size of data to be given to ota to proccess
extern uint8 otaBuffer[128];

#define PACKET_SIZE 64

//this is to be extern
extern uint8 curPktDirection;						

//mac layer data variable
extern macLayerData macData;

static uint32 numNodesProg=0;
//receiving program total length
static uint32 OTADownloadLength = 0;
//current size of file downloaded
static uint32 currentDownloadSize=0;
//current size of file transmitted
static uint32 currentSentSize=0;

static uint8 programAvailable=0;
static uint8 otaSentPrgSeqNo =0;
static uint8 otaRcvPrgSeqNo =0;
static uint8 otaPacketRetry = 0;
static uint8 otaRetry = 0;

static uint8 ackExpected =0;

uint8 e=0;
//memory address where program has to be written
uint32 OTAFlashAddress;


//to initialize ota
void otaInit()
{
	OTAFlashAddress = 0x00020000;
	flashInit();
}

//ota task handler. mainly to handle the time of expiry of an ota update. 
void otaTaskHandler(uint8 taskType)
{
	switch (taskType)
	{
		case READ_OTA_IN_DEV_BUFFER: 
		{
			uint16 size;
			uint16 fiflvl=0;
			fiflvl=u16AHI_UartReadRxFifoLevel(E_AHI_UART_0);
			size = u16AHI_UartBlockReadData(E_AHI_UART_0, &otaBuffer[1], fiflvl);
			doOTATask(otaBuffer, size+1);
		 	// enable uart receive inteterrupt to receive again		
			receiveFrmPcInit();
			break;
		}
		case START_NETWORK_UPDATE:
		{
			if (macData.numEndDevices >0)
			{
				//send start OTA packet to first node and on acknowledgement start sending data	
				uint8 packet[7];
				packet[0]=0x45;
				packet[1]=0x01;
				packet[2]=OTADownloadLength;
				packet[3]=OTADownloadLength>>8;
				packet[4]=OTADownloadLength>>16;
				packet[5]=OTADownloadLength>>24;
				packet[6]=0x00;
				
				int i;
				for(i=0;i<6;i++)
					packet[6]=packet[6]^packet[i];

				uint16 destAddr =macData.endDeviceData[numNodesProg].u32ExtAdrL;
				sendDataToMac(packet,7,destAddr,255,FALSE);
				ackExpected = 1;
				
				#ifdef FFD_PAN_COORDINATOR
				uint8 data[10];

				data[0]=3;
				data[1]=0x46;
				data[2]=0x01;
				memcpy(&data[3],&destAddr,2);
				sendDataToPc(data,5);
				#endif
				
				// addTask(OTA,CHECK_OTA_SUCCESS,(120000)); 

			}
			else
			{
				uint8 data[10];
				static uint8 l=0;
				l++;
				data[0]=3;
				data[1]=0x46;
				data[2]=0x00;
				data[3]=0x01;
				sendDataToPc(data,4);
				if(l<5)
					addTask(OTA ,START_NETWORK_UPDATE ,20000); 
			}
			break;
		}
		case CHECK_OTA_ACK:
		{
			if (macData.endDeviceData[numNodesProg].ackReceived == 0)
			{
				otaPacketRetry++;
			}
			else
			{
				otaPacketRetry =0;
				currentSentSize += PACKET_SIZE;

				if (otaSentPrgSeqNo == 0xFF)
					otaSentPrgSeqNo = 0;
				else
					otaSentPrgSeqNo ++;
			}

			if (currentSentSize >= OTADownloadLength)
			{
				//all the data has been sent, send stop packet
				uint8 data[4];
				data[0]=0x45;
				data[1]=0x00;
				data[2]=0x02;
				data[3]=0x45^0x02;
				
				uint16 destAddr =macData.endDeviceData[numNodesProg].u32ExtAdrL;
				sendDataToMac(data,4,destAddr,254,FALSE);
				return;
			}

			if (otaPacketRetry  != 10)		//maximum retries per packet are assumed to be 10	
				sendProgramData();
			break;
		}
		/*case CHECK_OTA_SUCCESS:
		{
			if(macData.endDeviceData[numNodesProg].otaComplete==1)
			{
				//previous node programming was successful
				numNodesProg++;
				otaRetry =0;
			}
			else
				otaRetry++;
				///number of times the node retries ota for 1 particular are 10. After that the node will generate this info to the pan coordinator that ota has failed.
			
			if (otaRetry ==10)
			{
				//do perform action here.
				uint8 data[10];
				
				data[0]=3;
				data[1]=0x46;
				data[2]=0x03;
				data[3]=0x46^0x03;

				sendDataToPc(data,4);
			}

			if ((numNodesProg < macData.numEndDevices) && (otaRetry!=10))
			{
				currentSentSize=0;
				otaSentPrgSeqNo =0;
				uint8 packet[7];
				packet[0]=0x45;
				packet[1]=0x01;
				packet[2]=OTADownloadLength;
				packet[3]=OTADownloadLength>>8;
				packet[4]=OTADownloadLength>>16;
				packet[5]=OTADownloadLength>>24;
				packet[6]=0x00;
				
				int i;
				for(i=0;i<6;i++)
					packet[6]=packet[6]^packet[i];

				uint16 destAddr =macData.endDeviceData[numNodesProg].u32ExtAdrL;
				sendDataToMac(packet,7,destAddr,255,FALSE);
				#ifdef FFD_PAN_COORDINATOR
				uint8 data[10];
				
				data[0]=3;
				data[1]=0x46;
				data[2]=0x01;
				memcpy(&data[3],&destAddr,2);
				sendDataToPc(data,5);
				#endif
				uint16 delay=eRand()%100;
				// addTask(OTA,CHECK_OTA_SUCCESS,(120000)); 

				ackExpected = 1;
			}
			else
			{
				otaRetry=0;
				#ifdef FFD_PAN_COORDINATOR
				uint8 data[10];
				
				data[0]=3;
				data[1]=0x46;
				data[2]=0x02;
				data[3]=0x02^0x46;

				sendDataToPc(data,4);
				#endif
				//all nodes are now programmed
			}
			break;
		}*/
		case REBOOT:
		{
			vAHI_SwReset();
		}
	}
}

//this function gets called when the node receives start ota packet either from gui or from its coordinator.
void startOta()
{	
	currentDownloadSize=0;
	otaRcvPrgSeqNo=0;
	// ledOn();
	setupOTA();
	//ack the packet
	sendOTAAck(0x01);
}

//this function gets called when the node receives stop ota packet either from gui or from its coordinator.
void stopOta(uint8 * data)
{
	if (currentDownloadSize != OTADownloadLength)
	{
	  ledOn();
		//OTA program not completely received and failed
		//send ack
		sendOTAAck(0x04);
		sendDataToPc("abcd", 4);
		return;
	}

	sendOTAAck(0x00);  // ack the device and restart with new program
	#ifdef FFD_PAN_COORDINATOR		//pan coordinator is the gateway
	
	if(data[2]==0x01)
	{
		uint8 corruptBIR[16];
		int j;
		for  (j=0;j<16;j++)
			corruptBIR[j]=0x00;
		
		msdelay (100);
		flashWrite(0,16,corruptBIR);
		msdelay(100);
		addTask(USER, REBOOT, 1000);
	}
	else 
	#endif
	if (data[2]==0x02)	//for network
	{
		//update the info in the network and once done, reset yourself

		/*for network update, we need to program all the nodes associated with current node, one by one.
		a timer will be placed on the node when it starts to program one particular node. After the timer
		expires, node will be required to check whether the programming of that node was successful or not.
		A node may try to program 1 particular node maximum 3 times after which a failure message has to be 
		generated for the GUI specifying the address of the node that failed to program.
		*/

		/*OTA will use both mac layer tables and routing protocol tables to program the node. Mac layer table 
		will be used to find out which all nodes are associated with current node. Accordingly, they will be 
		programmed one by one. Once the node is done with all the nodes, it will tell other nodes associated
		with it to start programming nodes associated to them. To maintain that same program is running on all 
		the nodes, the nodes will not immediately restart themselves to run the new program on receiving it, 
		but rather inform the pan coordinator that they have received the program. PAN coordinator will update 
		this information in its routing table. After the network programming has been started by the pan coordinator, 
		it will check, if it has received program received successful packet from all the nodes in it routing 
		table or not. The moment it sees that packet from all the nodes, it will send a restart message to all 
		the nodes in the network and they will reset to work with the new program.
		*/
		#ifdef FFD_COORDINATOR
		uint8 corruptBIR[16];
		int j;
		for  (j=0;j<16;j++)
			corruptBIR[j]=0x00;
		
		msdelay (100);
		flashWrite(0,16,corruptBIR);
		msdelay(100);
		vAHI_SwReset();
		
		#elif defined FFD_PAN_COORDINATOR
		uint16 delay=eRand()%10;
		addTask(OTA, START_NETWORK_UPDATE ,1000+delay);
		#endif
		
	}
}

//save program data in flash memory
void saveOTAData(uint8* data)
{
	msdelay (5);
	flashWrite(OTAFlashAddress+currentDownloadSize,PACKET_SIZE,data);
	msdelay(5);
	currentDownloadSize=currentDownloadSize+PACKET_SIZE;
}

//function called when an OTA packet is received
void receiveOTAPacket(uint8* payload,uint8 length,uint16 prevAddr,uint8 linkQuality)
{
	// check if it is OTA ack packet and ack was actually expected
	if (payload[0]==0x45)
	{
		if (length == 3)							
		{	
			// if(ackExpected==1)
			{	//check what kind of ack packet has come and act accordingly

				switch (payload[1])
				{
					case 0x00:
					{
						//stop ota packet acknowledged
						macData.endDeviceData[numNodesProg].otaComplete=1;
						ackExpected =0;
						currentSentSize=0;
						otaSentPrgSeqNo=0;
						#ifdef FFD_PAN_COORDINATOR
						uint8 data[10];
						
						data[0]=3;
						data[1]=0x46;
						data[2]=0x04;
						data[3]=0x10;
						
						sendDataToPc(data,4);
						#endif
						break;
					}
					case 0x01:
					{
						//start sending program
						sendProgramData();
						#ifdef FFD_PAN_COORDINATOR
						uint8 data[10];
						
						data[0]=3;
						data[1]=0x46;
						data[2]=0x04;
						data[3]=0x01;
						
						sendDataToPc(data,4);
						#endif
						break;
					}
					case 0x02:
					{
						//intermediate program packet to be sent
						macData.endDeviceData[numNodesProg].ackReceived = 1;
						#ifdef FFD_PAN_COORDINATOR
						uint8 data[10];
						
						data[0]=3;
						data[1]=0x46;
						data[2]=0x04;
						data[3]=0x02;
						
						sendDataToPc(data,4);
						#endif
						break;
					}
					#ifdef FFD_PAN_COORDINATOR
					case 0x04:
					{
						sendOTAAck(0x04);
					}
					#endif
				}
			}		
		}
		else
		{
			if (e==0)
			{
				ledOn();		
				e=1;
			}
			else
			{
				ledOff();
				e=0;
			}
			doOTATask(payload,length);	
		}
	}
}


void doOTATask(uint8* data,uint8 length)
{	
	//check if the packet is received without error or not
	uint8 checksum=0x00;
	int i;
	for(i=0;i<length-1;i++)
	{
		checksum=checksum^data[i];
	}
	// if (checksum!=data[length-1])
	// {	
	// 	return;
	// }

	if(data[0]==0x45)
	{
		//0x00 is stop OTA	packet	
		//0x01 is start OTA packet
		//0x02 is Data OTA packet
		switch (data[1])							
		{
			case 0x00:
				//procedure to follow when the stop packet is received 
				stopOta(data);
				break;
			case 0x01:
			{	//procedure to follow when the start packet is received
				
				OTADownloadLength = OTADownloadLength |((0x00000000 | data[5])<<24);
				OTADownloadLength = OTADownloadLength |((0x00000000 | data[4])<<16);
				OTADownloadLength = OTADownloadLength |((0x00000000 | data[3])<<8);
				OTADownloadLength = OTADownloadLength | data[2];

				startOta();
				break;
			}
			case 0x02:
			{			//save the data in the flash
				#ifndef FFD_PAN_COORDINATOR
					if (otaRcvPrgSeqNo == 0xFF)
					{
						if (data[2]==0)
						{
							saveOTAData(&data[3]);
							otaRcvPrgSeqNo =0;
						}
					}
					else
					{
						if (data[2]==otaRcvPrgSeqNo+1)
						{
							saveOTAData(&data[3]);
							otaRcvPrgSeqNo++;
						}
					}
					sendOTAAck(0x02);
				#else
					// if same packet is received again, drop it
					if (otaRcvPrgSeqNo == data[2])
						return;

					saveOTAData(&data[3]);
					otaRcvPrgSeqNo=data[2];
					sendOTAAck(0x02);
				#endif
				break;
			}
		}
	}
}

//function to send program
void sendProgramData()
{
	if (e==0)
	{
		ledOn();		
		e=1;
	}
	else
	{
		ledOff();
		e=0;
	}
	uint8 data[PACKET_SIZE+4];
	int i;

	data[0]=0x45;
	data[1]=0x02;
	data[2]=otaSentPrgSeqNo+1;

	msdelay (5);
	flashRead(OTAFlashAddress + currentSentSize,PACKET_SIZE,&data[3]);
	msdelay (5);
	data[67]=0;
	
	for(i=0;i<67;i++)
		data[67]=data[i]^data[67];

	macData.endDeviceData[numNodesProg].ackReceived =0;
	
	uint16 destAddr =macData.endDeviceData[numNodesProg].u32ExtAdrL;
	sendDataToMac(data,PACKET_SIZE+4,destAddr,254,FALSE);

	addTask(OTA,CHECK_OTA_ACK,20);
}	

void sendOTAAck(uint8 type)
{
	uint8 packet[4];
	if(type==0x00)
	{
		ledOn();
		msdelay(500);
		ledOff();
	}
	packet[0]=3;
	packet[1]=0x45;
	packet[2]=type;
	packet[3]=packet[0]^packet[1]^packet[2];

	#ifdef FFD_PAN_COORDINATOR
		sendDataToPc(packet,4);
	#else
		sendDataToMac(&packet[1],3,0x0000,255,FALSE);
	#endif
}

#endif

#ifndef ENABLE_OTA 
//declare to prevent from resriction to compile in non ota mode
//already in appcode if enable ota functionality is there
void otaTaskHandler(uint8 taskType)
{
	return;
}
#endif
