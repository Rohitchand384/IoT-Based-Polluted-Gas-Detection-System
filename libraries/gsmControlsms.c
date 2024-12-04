/*
 *All rights reserved.
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
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include "clock.h"                          //provides clock related functionality
#include "config.h"
#include "node.h"
#include "task.h"
#include "string.h"
#include "util.h"

/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& Do not modify above this line &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
#include "dio.h"
#include "pcInterface.h"
#include "flash.h"
#include "node.h"
#include "mac.h"
#include "gsmControl.h"
#include "uart.h"
#include "uartSerial.h"

#ifdef ENABLE_GSM
/*&&&& Task Id from 1000  to 120 is specific to Gateway &&&&*/


/*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&*/
uint8 gatewayResHandle=0;

int16 rxBufferGSMSize=0;
uint8 rxBufferGSM[2000];

uint8 txBufferGSM[1000];
int16 txBufferGSMSize=0;
#ifdef SMS
uint8 smsAllow=0;
uint8 SIM_NO[10];
uint8 smsChk=0;
#endif
uint8 apn[30];
uint8 internetUp=0;
uint8 okHandled=0;

uint8 Num0xd0xaMatcher=2;

static uint8 retryCount=0;
static uint8 retryCount1=0;

struct socketSend
{
    uint8 socketId;
    uint8 sendBlocked;
};

struct socketSend socketSendBlocked[4]; //max 4 parallel sockets are possible. we will maintain which socket buffer is full and needs a push here
uint8 numSocketDetails=0;
static uint8 state=0; //handles ok

void sendDataToGSM(uint8* baseAddress, uint16 noBytes)
{
#ifdef GSM_MODULE_VER1
    uartSend(E_AHI_UART_0,baseAddress,noBytes);
#elif defined GSM_MODULE_VER2
    uartSend(E_AHI_UART_1,baseAddress,noBytes);
#elif defined GSM_MODULE_VER3
    uartSend(E_AHI_UART_0,baseAddress,noBytes);
#endif 
}

void setApn(uint8* simApn,uint8 len)
{
    apn[0]='"';
    memcpy(&apn[1],simApn,len);
    apn[len+1]='"';

}

void gsmPowerDown()
{
    internetUp=0;
    okHandled=0;
    numSocketDetails=0;
    state=0;
    retryCount=0;
    retryCount1=0;
    //uartSend(E_AHI_UART_1,rxBufferGSM,rxBufferGSMSize);
    deleteAllPendingTasks();        //deleting all user task means that existing user ask will also be deleted. 
                                    //individually delete gsm tasks if you need to keep some tasks intact
    gatewayResHandle=0;
#ifdef GSM_MODULE_VER1   
     vAHI_UartSetInterrupt(E_AHI_UART_0,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   FALSE ,          
                   E_AHI_UART_FIFO_LEVEL_1 );
#elif defined GSM_MODULE_VER2           
     vAHI_UartSetInterrupt(E_AHI_UART_1,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   E_AHI_UART_FIFO_LEVEL_1 );
#elif defined GSM_MODULE_VER3   
     vAHI_UartSetInterrupt(E_AHI_UART_0,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   FALSE ,          
                   E_AHI_UART_FIFO_LEVEL_1 );
#endif                                         
   
    uint8 packet [11] = "AT+CFUN=0";            //Command to off echos 
    packet [9] = 0x0d;
    packet [10] = 0x0a;
    sendDataToGSM(packet, 11);    
    msdelay(2000);
     
    clearPin (0);    
#ifdef GSM_MODULE_VER1
     vAHI_UartDisable(E_AHI_UART_0);
#elif defined GSM_MODULE_VER2  
    vAHI_UartDisable(E_AHI_UART_1); 
#elif defined GSM_MODULE_VER3
    clearPin(16);
    vAHI_UartDisable(E_AHI_UART_0);                                        

#endif     
   
    
}


/*
 * Function to intilize GSM device connected to senseunts
 * Communication is taking place using UART and AT commands
 * Input put is nothing
 * Return success or failure with Respect to internet connection
 *
 */

void gsmInit()
{    
    gatewayResHandle=0;
    rxBufferGSMSize=0;
  
#ifdef GSM_MODULE_VER1
    setPin(0);
    msdelay(5000);
    uartInit(E_AHI_UART_0,E_AHI_UART_RATE_115200);
    enUartInterrupt(E_AHI_UART_0);   
#elif defined GSM_MODULE_VER2
    setPin(0);
    msdelay(5000);
    uartInit(E_AHI_UART_1,E_AHI_UART_RATE_115200);
    enUartInterrupt(E_AHI_UART_1);
#elif defined GSM_MODULE_VER3
    setPin(16);//test versio 3    
    msdelay(5000);
    uartInit(E_AHI_UART_0,E_AHI_UART_RATE_115200);
    enUartInterrupt(E_AHI_UART_0);   
#endif
    uint8 packet [8] = "ATE000";            //Command to off echos 
    packet [6] = 0x0d;
    packet [7] = 0x0a;
    sendDataToGSM (packet, 8);  
    
    addTask(USER,HAS_GSM_STARTED,10*SECOND);
    
}

//this function is called when a byte is received on uart
void gsmHandleInterrupt()
{
    //this is the first byte, lets wait for some time(we may need to change this later
    //depending on , collect all the data and then handle it
    if (rxBufferGSMSize==0)
        addTask(USER,HANDLE_GW_RESPONSE,300*MILLI_SECOND);  

}


//disable gsm interrupt
void disableGSMInterrupt()
{
#ifdef GSM_MODULE_VER1 
    vAHI_UartSetInterrupt(E_AHI_UART_0,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   FALSE ,          
                   E_AHI_UART_FIFO_LEVEL_1 );     
#elif defined GSM_MODULE_VER2
    vAHI_UartSetInterrupt(E_AHI_UART_1,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   FALSE ,          
                   E_AHI_UART_FIFO_LEVEL_1 );  
#elif defined GSM_MODULE_VER3
    vAHI_UartSetInterrupt(E_AHI_UART_0,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   FALSE ,          
                   E_AHI_UART_FIFO_LEVEL_1 );  
#endif         
}

//enable gsm interrupt
void enableGSMInterrupt()
{
#ifdef GSM_MODULE_VER1
    vAHI_UartSetInterrupt(E_AHI_UART_0,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   TRUE ,           
                   E_AHI_UART_FIFO_LEVEL_1 );     
#elif defined GSM_MODULE_VER2
    vAHI_UartSetInterrupt(E_AHI_UART_1,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   TRUE ,           
                   E_AHI_UART_FIFO_LEVEL_1 );  
#elif defined GSM_MODULE_VER3
    vAHI_UartSetInterrupt(E_AHI_UART_0,
                   FALSE ,
                   FALSE ,
                   FALSE ,
                   TRUE ,           
                   E_AHI_UART_FIFO_LEVEL_1 );     
#endif         
}

//read uart and get data n a buffer
uint8 readGSMData()
{
    uint8 fifoLevel;
    fifoLevel=0;
    #ifdef GSM_MODULE_VER1
    /****check Rx fifo level************/   
    fifoLevel=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
    #elif defined GSM_MODULE_VER2
    /****check Tx fifo level************/   
    fifoLevel=u8AHI_UartReadRxFifoLevel(E_AHI_UART_1);
    #elif defined GSM_MODULE_VER3
    /****check Rx fifo level************/   
    fifoLevel=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
    #endif   
    
    rxBufferGSMSize=0;

    if (fifoLevel==0)
    {
        //no data to read
        return 0;
    }
    else
    {
        uint8 numSeq=0;
    
       
        while(rxBufferGSMSize<fifoLevel)
        {
            #ifdef GSM_MODULE_VER1
            rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_0);
            
            #elif defined GSM_MODULE_VER2
            rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_1);
            
            #elif defined GSM_MODULE_VER3
            rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_0);
            #endif 
            

            if (rxBufferGSM[rxBufferGSMSize-1]==0x0a)
                numSeq++;
            
            if (numSeq==Num0xd0xaMatcher)    
            {
                Num0xd0xaMatcher=2;
                return rxBufferGSMSize;
            }    
        }
        uint8 tempfifo=0;
        #ifdef GSM_MODULE_VER1
        tempfifo=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
        
        #elif defined GSM_MODULE_VER2
        tempfifo=u8AHI_UartReadRxFifoLevel(E_AHI_UART_1);
        
        #elif defined GSM_MODULE_VER3
        tempfifo=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0); 

        #endif 
    
        int g=0;
        
        while (1)
        {
            if (g==20000)    
            {
                break;
            }
            
            int f=0;
            while(tempfifo<1)
            {
                #ifdef GSM_MODULE_VER1
                tempfifo=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
                
                #elif defined GSM_MODULE_VER2
                tempfifo=u8AHI_UartReadRxFifoLevel(E_AHI_UART_1);
                
                #elif defined GSM_MODULE_VER3
                tempfifo=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
                #endif 
    
                f++;
                if (f==20000)
                {
                    //didnt receive appropriate end marker, rejecting packet 
                   
                    rxBufferGSMSize=0;
                    return 0; 
                }
            }
            #ifdef GSM_MODULE_VER1
            rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_0);
                
            #elif defined GSM_MODULE_VER2
            rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_1);

            #elif defined GSM_MODULE_VER3
            rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_0);
            #endif 

            if (rxBufferGSM[rxBufferGSMSize-1]==0x0a)
            {
                return rxBufferGSMSize;
            }    
            g++;    
        }
        
        //didnt receive appropriate end marker, rejecting packet
        rxBufferGSMSize=0;
        Num0xd0xaMatcher=2;
        return 0;
                              
    }       
}
//parse data sent by gsm and make relevant info out of it
void parseGSMData()
{
    int i=0;    
    int16 h=rxBufferGSMSize-strlen(SIM_CARD_FAILURE);
    while(i<h)
    {
        if (memcmp(&rxBufferGSM[i],SIM_CARD_FAILURE,strlen(SIM_CARD_FAILURE))==0)
        {
            //nothing received in RESPONSE
            rxBufferGSMSize=0;
            gatewayResHandle = 0;     
            
            addTask (USER, GSM_POWER_DOWN, 0);
            enableGSMInterrupt();
            return;   
        }
        i++;
    }      

    i=0;
    h=rxBufferGSMSize-strlen(GSM_CALL_HANGUP_DETECTED);
    while(i<h)
    {
        if (memcmp(&rxBufferGSM[i],GSM_CALL_HANGUP_DETECTED,strlen(GSM_CALL_HANGUP_DETECTED))==0)
        {
            //nothing received in RESPONSE
            rxBufferGSMSize=0;
            gatewayResHandle = 0;
            addTask (USER, GSM_POWER_DOWN, 0);
            enableGSMInterrupt();
            return;
            
        }
        i++;
    }
    
    i=0;
    h=rxBufferGSMSize-strlen(GSM_CALL_RECEIVED);
    while(i<h)
    {
        if (memcmp(&rxBufferGSM[i],GSM_CALL_RECEIVED,strlen(GSM_CALL_RECEIVED))==0)
        {
            //nothing received in RESPONSE
            rxBufferGSMSize=0;
            gatewayResHandle = 0;
            
            addTask (USER, GSM_POWER_DOWN, 0);
            enableGSMInterrupt();
            return;
            
        }
        i++;
    }
   
    recGSMResponse();  
}
//receive GSM module response
void recGSMResponse ()
{   
    static int hanOk=0;
    switch (gatewayResHandle)
    {
        //////////////////////////////////////////////////////
        //To receive AT command ready on GSM Boot
        case HANDLE_GSM_POWER_UP: //Handle AT command Ready                     //0
        {
            uint8 checkSimStatusCmd [100] = "AT+CPIN?";  //Check sim status 
            uint8 cmdLen = strlen ((const char*)"AT+CPIN?");
            checkSimStatusCmd [cmdLen] = 0x0d;
            cmdLen++;
            checkSimStatusCmd [cmdLen] = 0x0a;
            cmdLen++;
            gatewayResHandle=1;
            rxBufferGSMSize=0;
            sendDataToGSM (checkSimStatusCmd, cmdLen);
            enableGSMInterrupt(); 
            break;
        }
        #ifdef SMS
        case SMS_VIEW_MODE_ACK:                                                 //51
        {
            int i=0;
            
            while (i< rxBufferGSMSize-2)   //12 is the length of GSM_SUCCESS_RES
            {
                if ((memcmp ((const char*)&(rxBufferGSM[i]), GSM_SUCCESS_RES,strlen(GSM_SUCCESS_RES)) == 0))                
                {    
                    if (hanOk==0)
                    {
                        hanOk=1;
                        rxBufferGSMSize=0;
                        enableGSMInterrupt(); 
                        return;
                    }  
                    gatewayResHandle=52;
                    rxBufferGSMSize=0;
                    smsTextMode(); 

                    return;
                }
                i++;
            } 
            i=0;
            
            while (i< rxBufferGSMSize-5)
            {    
                if (memcmp(&rxBufferGSM[i],"ERROR",5)==0)
                {                   
                        msdelay(2*SECOND);
                        
                        if (retryCount1<5)
                        {
                            rxBufferGSMSize=0;
                            smsViewMode();
                            retryCount1++;
                            return;
                        }
                        else
                        {
                             //sim not detected even after 20 tries. Restart gsm module
                        #ifdef GSM_MODULE_VER1                  
                            setPin(1);      //green led blinking 2 times means that the internet didnt connect after even 5 tries
                            msdelay(1000);
                            clearPin(1);
                            msdelay(1000);
                            setPin(1);     
                            msdelay(1000);
                            clearPin(1);
                        #endif
                            retryCount1=0;
                            retryCount=0;
                            rxBufferGSMSize=0;
                            
                            gatewayResHandle = 0;
                            
                            addTask (USER, GSM_POWER_DOWN, 0);
                            return;
                        }    
                }
                i++;   
            } 
            rxBufferGSMSize=0;
            enableGSMInterrupt();    
            break;
        }
        case SMS_TEXT_MODE_ACK:                                                     //52
        {
            int i=0;
            
            while (i< rxBufferGSMSize-2)   //2 is the length of OK
            {
                if ((memcmp ((const char*)&(rxBufferGSM[i]), GSM_SUCCESS_RES,strlen(GSM_SUCCESS_RES)) == 0))                
                {   
                    gatewayResHandle=53;
                    rxBufferGSMSize=0;
                    smsAlertOn();                  
                    return;
                }
                i++;
            }                  
            break;
        }
        case SMS_ALERT_ON_ACK:                                                      //53
        {
            int i=0;
            
            while (i< rxBufferGSMSize-2)   //2 is the length of OK
            {
                if ((memcmp ((const char*)&(rxBufferGSM[i]), GSM_SUCCESS_RES,strlen(GSM_SUCCESS_RES)) == 0))                
                {  
                    #ifdef SMS
                    gatewayResHandle=3;
                    rxBufferGSMSize=0;

                    #elif defined SMS_INTERNET
                    gatewayResHandle=2;
                    rxBufferGSMSize=0;
                    uint8 packet [100] = "AT+MIPCALL=1,";    //start internet on sim
                    retryCount=0;
                    retryCount1=0;
                                      
                    memcpy(&packet[13],apn,strlen((const char*)apn));
                    uint8 len = strlen ((const char*)packet);
                    packet [len] = 0x0d;
                    packet [len+1] = 0x0a;
                    len +=2;
                    sendDataToGSM (packet, len);
                    #endif
                    enableGSMInterrupt();                    
                    return;
                }
                i++;
            }                  
            break;
        }           
        #endif    
        //////////////////////////////////////////////////////
        //To Receive SIM Ready command from GSM module
        case HANDLE_SIM_DETECTION: //Handle +CPIN: READY                            //1
        {
            int i=0;
            
            while (i< rxBufferGSMSize-12)   //12 is the length of GSM_SIM_READY
            {
                if ((memcmp ((const char*)&(rxBufferGSM[i]), GSM_SIM_READY,strlen(GSM_SIM_READY)) == 0))      
                {    
                    #ifdef SMS
                    gatewayResHandle=51;
                    rxBufferGSMSize=0;
                    retryCount1=0;
                    retryCount=0;
                    hanOk=0;
                    smsViewMode();
                    return;

                    #endif

                    gatewayResHandle=2;
                    rxBufferGSMSize=0;
                    uint8 packet [100] = "AT+MIPCALL=1,";    //start internet on sim
                    retryCount=0;
                    retryCount1=0;
                    
                    memcpy(&packet[13],apn,strlen((const char*)apn));
                    uint8 len = strlen ((const char*)packet);
                    packet [len] = 0x0d;
                    packet [len+1] = 0x0a;
                    len +=2;
                    sendDataToGSM (packet, len);
                    enableGSMInterrupt();                    
                    return;
                }
                  
                i++;
            }                  
            
            retryCount++;
            
            if (retryCount==20)
            {
                    //sim not detected even after 20 tries. Restart gsm module
#ifdef GSM_MODULE_VER1                  
                msdelay(1000);
                setPin(1);      //green led blinking 3 times means that the sim is not detected even after 20 retries
                msdelay(1000);
                clearPin(1);
                msdelay(1000);
                setPin(1);     
                msdelay(1000);
                clearPin(1);
                msdelay(1000);
                setPin(1);      
                msdelay(1000);
                clearPin(1);
#endif
                retryCount=0;
                retryCount1=0;
                rxBufferGSMSize=0;
                
                gatewayResHandle = 0;
                
                addTask (USER, GSM_POWER_DOWN, 0);
                    
             }    
             //sim not detected yet
             else
             {
                msdelay(1500);
                rxBufferGSMSize=0;
                uint8 checkSimStatusCmd [100] = "AT+CPIN?";  //Check sim status 
                uint8 cmdLen = strlen ((const char*)"AT+CPIN?");
                checkSimStatusCmd [cmdLen] = 0x0d;
                cmdLen++;
                checkSimStatusCmd [cmdLen] = 0x0a;
                cmdLen++;
                sendDataToGSM (checkSimStatusCmd, cmdLen);
                //enableGSMInterrupt(); 
               
             }
             rxBufferGSMSize=0;
             enableGSMInterrupt(); 
             break;
        }
        ///////////////////////////////////////////////////////
                                                
                
        ///////////////////////////////////////////////////////
        //To Receive Internet start commad
        case HANDLE_INTERNET_ACTIVATION_RESPONSE:                                           //2
        {
            int i=0;
            //uartSend(E_AHI_UART_1,rxBufferGSM,rxBufferGSMSize);  
            while (i< rxBufferGSMSize-11)   
            {
                if (memcmp(&rxBufferGSM[i],"+MIPCALL: 0",strlen((const char*)"+MIPCALL: 0"))==0)
                {
                    retryCount1=0;
                    retryCount=0;
                    rxBufferGSMSize=0;
                    
                    gatewayResHandle = 0;
                    
                    addTask (USER, GSM_POWER_DOWN, 0);
                    return;
                }
                i++;
            }
            
            i=0;
            while (i< rxBufferGSMSize-16)
            {
                if (memcmp(&rxBufferGSM[i],"+MIPCALL: ",strlen((const char*)"+MIPCALL: "))==0)
                {
                    //uartSend(E_AHI_UART_1,rxBufferGSM,rxBufferGSMSize);  
                    gatewayResHandle=3;
                    okHandled=0;
                    rxBufferGSMSize=0;
                    retryCount1=0;
                    retryCount=0;
                    internetUp=1;
                    
                    deletePQTask(USER,IS_INTERNET_ON);
                    //internet connected
                    addTask(USER,INTERNET_CONNECTED,2); 
                    return;         
                }
                i++;
            } 
            
            i=0;
            
            while (i< rxBufferGSMSize-5)
            {    
                if (memcmp(&rxBufferGSM[i],"ERROR",5)==0)
                {                   
                    if (internetUp==0)
                    {
                        msdelay(15*SECOND);
                        
                        if (retryCount1<5)
                        {
                            rxBufferGSMSize=0;
                            uint8 packet [100] = "AT+MIPCALL=1,";    //start internet on sim

                            memcpy(&packet[13],apn,strlen((const char*)apn));
                            uint8 len = strlen ((const char*)packet);
                            packet [len] = 0x0d;
                            packet [len+1] = 0x0a;
                            len +=2;
                            
                            sendDataToGSM (packet, len);
                            enableGSMInterrupt(); 
                            retryCount1++;
                            return;
                        }
                        else
                        {
                             //sim not detected even after 20 tries. Restart gsm module
        #ifdef GSM_MODULE_VER1                  
                            msdelay(1000);
                            setPin(1);      //green led blinking 4 times means that the internet didnt connect after even 5 tries
                            msdelay(1000);
                            clearPin(1);
                            msdelay(1000);
                            setPin(1);     
                            msdelay(1000);
                            clearPin(1);
                            
        #endif
                            retryCount1=0;
                            retryCount=0;
                            rxBufferGSMSize=0;
                            
                            gatewayResHandle = 0;
                            
                            addTask (USER, GSM_POWER_DOWN, 0);
                            return;
                        }    
                    }
                    else
                    {
                        rxBufferGSMSize=0;
                            
                        gatewayResHandle = 0;
                        
                        addTask (USER, GSM_POWER_DOWN, 0);
                        return;
                    
                    } 
                }
                i++;   
            }
            
            i=0;
            while(i<rxBufferGSMSize-2)
            {
                      
                if (memcmp(&rxBufferGSM[i],"OK",strlen((const char*)"OK"))==0)
                {
                    if(okHandled==2)
                    {
                        enableGSMInterrupt(); 
                        return;
                    }    
                    //ok received, we now need to wait for +mipcall, if it doesn't arrive we need to reset the system somehow
                                       
                    retryCount1=0;
                    retryCount=0;
                    rxBufferGSMSize=0;
                    
                    if (okHandled==0)
                    {
                        okHandled=1;
                        enableGSMInterrupt(); 
                        return;
                    }
                    
                    addTask(USER,IS_INTERNET_ON,60*SECOND);
                    enableGSMInterrupt(); 
                    okHandled++;                        
                    return;
                }
                i++;
                   
            }
            rxBufferGSMSize=0;
            enableGSMInterrupt();     
            break;
        }
        
        case HANDLE_DATA_OPERATION:
        {
            int i=0;
            //uartSend(E_AHI_UART_1,rxBufferGSM,rxBufferGSMSize); 
            while(i<rxBufferGSMSize-5)
            {
                if (memcmp(&rxBufferGSM[i],"ERROR",strlen((const char*)"ERROR"))==0)
                {
                    //nothing received in RESPONSE
                    rxBufferGSMSize=0;
                    gatewayResHandle = 0;
                    retryCount=0;
                    retryCount1=0;
                    
                    addTask (USER, GSM_POWER_DOWN, 0);
                    return;
                    
                }
                i++;
            }
            //for sms
            #ifdef SMS
            i=0;
            while(i<rxBufferGSMSize-strlen(SMS_RECEIVED))
            {
                if (memcmp(&rxBufferGSM[i],"+CMT:",5)==0)
                {
                    rxBufferGSMSize=0;
                    gatewayResHandle = 3;
                    
                    addTask (USER, GSM_SMS_RECEIVE, 100);
                    return;
                    
                }
                i++;
            }
            #endif
            i=0;
            while(i<rxBufferGSMSize-9)
            {       
                if (memcmp(&rxBufferGSM[i],"+MIPSTAT:",9)==0)
                {
                     //uartSend(E_AHI_UART_1,rxBufferGSM,rxBufferGSMSize);
                    //response received
                    
                    i=i+9;
                    
                    uint16 loopmax=0;
                    while (rxBufferGSM[i]!=','&&loopmax<20000)
                    {    
                        i++;
                        loopmax++;
                    }
                    
                    if (loopmax==20000)
                    {
                        rxBufferGSMSize=0;
                        gatewayResHandle = 0;
                        retryCount=0;
                        retryCount1=0;
                        
                        addTask (USER, GSM_POWER_DOWN, 0);
                        return;
                    }
                    if (rxBufferGSM[i+1]!='0')
                    {
                        //either connection has been closed by the other end or something wrong happened.
                        //resettig module now. you may also do a reconnection seq without resetting module.
                        
                        deletePQTask(USER,DATA_OPERATIONS);                       
                        addTask (USER, HANDLE_BROKEN_SOCKET, 2);
                    }                             //'0' represents sent ack    
                    //else 
                    //    addTask(USER,DATA_OPERATIONS,2);    //ack received
                    
                    rxBufferGSMSize=0;                   
                    return;
                    
                }
                i++;
            }
            
            i=0;
            while(i<rxBufferGSMSize-9)
            {
                
                if (memcmp(&rxBufferGSM[i],"+MIPOPEN:",9)==0)
                {
                    //socket connection created. we will save socket details before emptying the rx buffer
                    uint8 j=0;
                    while (j<numSocketDetails)
                    {
                        if (socketSendBlocked[j].socketId==rxBufferGSM[i+9])
                        {
                            socketSendBlocked[j].sendBlocked=0;
                            break;
                        }
                        j++;
                    }
                    
                    if (j==numSocketDetails && numSocketDetails<3) //max 4 ports
                    {
                        numSocketDetails++;
                        socketSendBlocked[numSocketDetails].socketId=rxBufferGSM[i+9];
                        socketSendBlocked[numSocketDetails].sendBlocked=0;
                    }
                    
                    rxBufferGSMSize=0;
                    addTask(USER,DATA_OPERATIONS,2);
                    return;
                }
                i++;
            }
            
            i=0;
            while(i<rxBufferGSMSize-9)
            {               
                if (memcmp(&rxBufferGSM[i],"+MIPSEND:",9)==0)
                {
                    //send data to gsm module where it will be stored in data buffer of the socket
                    rxBufferGSMSize=0;
                    addTask(USER,DATA_OPERATIONS,2);
                    return;
                }
                i++;
            }
            
            i=0;           
            while(i<rxBufferGSMSize-9)
            {
                
                if (memcmp(&rxBufferGSM[i],"+MIPRUDP:",9)==0)
                {
                    //data received on udp socket
                    addTask(USER,RECEIVE_DATA_ON_SOCKET,2);
                    return;
                }

                i++;
            }
            
            i=0;
            while(i<rxBufferGSMSize-9)
            {
                
                if (memcmp(&rxBufferGSM[i],"+MIPRTCP:",9)==0)
                {
                    
                    addTask(USER,RECEIVE_DATA_ON_SOCKET,2);
                    return;
                }
                i++;
            }
                           
            i=0;
            while(i<rxBufferGSMSize-9)
            {
                if (memcmp(&rxBufferGSM[i],"+MIPXOFF:",9)==0)
                {
                    //modules data buffer is full. don't send any more data but push data 
                    uint8 j=0;
                    while (j<numSocketDetails)
                    {
                        if (socketSendBlocked[j].socketId==rxBufferGSM[i+9])
                        {
                            socketSendBlocked[j].sendBlocked=1;
                            break;
                        }
                        j++;
                    }
                    
                    if (j==numSocketDetails && numSocketDetails<3) //max 4 ports
                    {
                        numSocketDetails++;
                        socketSendBlocked[numSocketDetails].socketId=rxBufferGSM[i+9];
                        socketSendBlocked[numSocketDetails].sendBlocked=1;
                    }
                    enableGSMInterrupt();               
                    //addTask(USER,DATA_OPERATIONS,2);
                    return;
                }
                i++;
            }
            
            while(i<rxBufferGSMSize-8)
            {
                if (memcmp(&rxBufferGSM[i],"+MIPXON:",8)==0)
                {
                    
                    //modules data buffer is full. don't send any more data but push data 
                    uint8 j=0;
                    while (j<numSocketDetails)
                    {
                        if (socketSendBlocked[j].socketId==rxBufferGSM[i+9])
                        {
                            socketSendBlocked[j].sendBlocked=0;
                            break;
                        }
                        j++;
                    }
                    
                    if (j==numSocketDetails && numSocketDetails<3) //max 4 ports
                    {
                        numSocketDetails++;
                        socketSendBlocked[numSocketDetails].socketId=rxBufferGSM[i+9];
                        socketSendBlocked[numSocketDetails].sendBlocked=0;
                    }   
                    rxBufferGSMSize=0;                 
                    enableGSMInterrupt(); 
                    return;            
                }
                i++;
            }
            
            //for httpdata and SMS sending
            i=0;
            while(i<rxBufferGSMSize-1)
            {
                if (memcmp(&rxBufferGSM[i],">",strlen((const char*)">"))==0)
                {
                    //> received
                    #ifdef SMS
                    if(smsChk==1)   // if for sms then send sms data matter.
                    {
                        smsAllow=1;
                        rxBufferGSMSize=0;
                        sendSmsData();
                        break;    
                    }
                    #endif
                    //else run this for hhtpdata 
                    rxBufferGSMSize=0;
                    addTask(USER,DATA_OPERATIONS,2);
                    return;
                }
                i++;
            }

            i=0;
            while(i<rxBufferGSMSize-11)
            {
                if (memcmp(&rxBufferGSM[i],"+HTTPRES: <",strlen((const char*)"+HTTPRES: <"))==0)
                {
                    //http response received
                  
                    retryCount1=0;
                    retryCount=0;
                    rxBufferGSMSize=0;
                    
                    addTask(USER,DATA_OPERATIONS,2);
                    return;
                }
                i++;
            }
            
            i=0;
            while(i<rxBufferGSMSize-11)
            {
                if (memcmp(&rxBufferGSM[i],"+HTTPREAD: 0",strlen((const char*)"+HTTPREAD: 0"))==0)
                {
                    //nothing received in RESPONSE
                    rxBufferGSMSize=0;
                    gatewayResHandle = 0;
                    retryCount=0;
                    retryCount=1;
                    
                    addTask (USER, GSM_POWER_DOWN, 0);
                    return;
                    
                }
                else  if (memcmp(&rxBufferGSM[i],"+HTTPREAD: ",strlen((const char*)"+HTTPREAD: "))==0)
                {
                    
                    //right now the interrupt is disabled. We will be reading all the bytes in poll mode
                    //before enabling the interrupt again.
                    
                    uint8 dataSize;
                    dataSize=0;
                    uint8 data[5];  //capture number of bytes available to read in this
                    
                    while (rxBufferGSM[i+11+dataSize]!=0x0d)
                    {
                        data[dataSize]=rxBufferGSM[i+11+dataSize];            
                        dataSize++;
                    }
                     //uartSend(E_AHI_UART_1,data,dataSize);
                    uint16 dataLength=0;
                    
                    int8 z=dataSize-1;
                    while (z>=0)
                    
                    {    
                        dataLength+=((data[dataSize-1-z]-0x30)*power(10,z));
                       // uartSend(E_AHI_UART_1,&dataLength,2);
                       // msdelay(100);
                        z--;    
                    }
                    
                    rxBufferGSMSize=0;     
                                      
                    while (rxBufferGSMSize < dataLength)
                    {
#ifdef GSM_MODULE_VER1                    
                        rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_0);  
#elif defined GSM_MODULE_VER2            
                        rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_1);
#elif defined GSM_MODULE_VER3            
                        rxBufferGSM[rxBufferGSMSize++]=readByteFromUart(E_AHI_UART_0);
#endif                                      
                    }
                                                             
                    //check fifolevel and take out all the data if still something is left
                    uint16 fifoLevel;
                    fifoLevel=0;
#ifdef GSM_MODULE_VER1                    
                    fifoLevel=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);  
#elif defined GSM_MODULE_VER2            
                    fifoLevel=u8AHI_UartReadRxFifoLevel(E_AHI_UART_1);
#elif defined GSM_MODULE_VER3            
                    fifoLevel=u8AHI_UartReadRxFifoLevel(E_AHI_UART_0);
#endif                      
                    
                    uint8 f=0;                
                    while (f < fifoLevel)
                    {
#ifdef GSM_MODULE_VER1                    
                        rxBufferGSM[rxBufferGSMSize+f]=readByteFromUart(E_AHI_UART_0);  
#elif defined GSM_MODULE_VER2            
                        rxBufferGSM[rxBufferGSMSize+f]=readByteFromUart(E_AHI_UART_1);
#elif defined GSM_MODULE_VER3            
                        rxBufferGSM[rxBufferGSMSize+f]=readByteFromUart(E_AHI_UART_0);
#endif               
                        f++;            
                    }
                    //all data has been read
                    addTask(USER,DATA_OPERATIONS,2);
                    state=0;
                    return;
                } 
                
                i++;
            }
                         
            i=0;
            while(i<rxBufferGSMSize-2)
            {
                if (memcmp(&rxBufferGSM[i],"OK",strlen((const char*)"OK"))==0)
                {
                    //ok received
                   
                    retryCount1=0;
                    retryCount=0;
                    rxBufferGSMSize=0;
                    if (state==0)
                    {
                        addTask(USER,DATA_OPERATIONS,2);
                        state=1;
                    }else
                        enableGSMInterrupt();     
                    return;
                    
                }
                i++;
            }
            rxBufferGSMSize=0; 
            enableGSMInterrupt();           
            break;
        }
        //case for feature sms
       
    }
}

//function to set http related parameter
void gsmHttpSet(uint8* wts,uint8* value)
{
    rxBufferGSMSize=0;
    uint8 myCompleteVal [500] = "at+httpset=\"";
    uint8 size=strlen((const char*)myCompleteVal);
    memcpy(&myCompleteVal[size],wts,strlen((const char*)wts));
    size=size+strlen((const char*)wts);
    memcpy(&myCompleteVal[size],"\",\"",strlen((const char*)"\",\""));
    size=size+strlen((const char*)"\",\"");
    memcpy(&myCompleteVal[size],value,strlen((const char*)value));
    size=size+strlen((const char*)value);
    memcpy(&myCompleteVal[size],"\"",strlen((const char*)"\""));
    size=size+strlen((const char*)"\"");
    myCompleteVal[size]=0x0d;
    size=size+1;
    myCompleteVal[size]=0x0a;
    size=size+1;
 
    sendDataToGSM(myCompleteVal,size);
    state=0;
}

//function called before updating http post data
void gsmHttpSetPostDataLength(uint8* length)
{
    rxBufferGSMSize=0;
    uint8 myCompleteVal [500] = "at+httpdata=";
    uint8 size=strlen((const char*)myCompleteVal);
    memcpy(&myCompleteVal[size],length,strlen((const char*)length));
    size=size+strlen((const char*)length);
    
    myCompleteVal[size]=0x0d;
    size=size+1;
    myCompleteVal[size]=0x0a;
    size=size+1;
 
    sendDataToGSM(myCompleteVal,size);
    state=0;
}

void gsmHttpPerform(uint8 val)
{
   rxBufferGSMSize=0;
   uint8 sendd[20]="at+httpact=";
   uint8 size=strlen((const char*)sendd);
   memcpy(&sendd[11],&val,1);
   sendd[12]=0x0d;
   sendd[13]=0x0a;
   sendDataToGSM (sendd,size+3); 
}

void gsmHttpRead()
{
   //the data will be received in the polling mode here 
   rxBufferGSMSize=0;
   uint8 sendd[20]="at+httpread";
   uint8 size=strlen((const char*)sendd);
   sendd[11]=0x0d;
   sendd[12]=0x0a;
   sendDataToGSM (sendd,size+2); 
}

//open a socket of udp or tcp type
void createSocket(uint8* socketId, uint8* sourcePort,uint8* remoteIp, uint8* remotePort,uint8 protocol)
{
    uint8 toSend[150];
    int currentSize=0;
    memcpy(toSend,"AT+MIPOPEN=",11);
    currentSize+=11;
    toSend[currentSize++]=socketId[0];
    toSend[currentSize++]=',';
    memcpy(&toSend[currentSize],sourcePort,strlen((const char*)sourcePort));
    currentSize+=strlen((const char*)sourcePort);
    memcpy(&toSend[currentSize],",\"",strlen((const char*)",\""));
    currentSize+=strlen((const char*)",\"");
    memcpy(&toSend[currentSize],remoteIp,strlen((const char*)remoteIp)); //send ip in double quotes
    currentSize+=strlen((const char*)remoteIp);
    memcpy(&toSend[currentSize],"\",",strlen((const char*)"\","));
    currentSize+=strlen((const char*)"\",");
    memcpy(&toSend[currentSize],remotePort,strlen((const char*)remotePort));
    currentSize+=strlen((const char*)remotePort);
    toSend[currentSize++]=',';
    memcpy(&toSend[currentSize],&protocol,1);
    currentSize++;
    toSend[currentSize++]=0x0d;
    toSend[currentSize++]=0x0a;
    
    sendDataToGSM(toSend,currentSize);
    state=1;    //we don't want to add a task when ok is received
}

void changeGSMDataFormat()
{
    uint8 toSend[150];
    int currentSize=0;
    memcpy(toSend,"AT+GTSET=\"iprfmt\",2",strlen((const char*)"AT+GTSET=\"iprfmt\",2"));
    currentSize+=strlen((const char*)"AT+GTSET=\"iprfmt\",2");   
    toSend[currentSize++]=0x0d;
    toSend[currentSize++]=0x0a;
    
    sendDataToGSM(toSend,currentSize);
    state=0;    //we want to add a task when ok is received   
}

uint8 selectSktBufferGSM(uint8* socketId, uint8* intStr)
{
    uint8 j=0;
    while (j<numSocketDetails)
    {
        if (socketSendBlocked[j].socketId==socketId&&socketSendBlocked[j].sendBlocked==1)
        {
            return 0;
        }
        j++;
    }
    uint8 toSend[150];
    int currentSize=0;
    memcpy(toSend,"AT+MIPSEND=",11);
    currentSize+=11;
    toSend[currentSize++]=socketId[0];
    toSend[currentSize++]=',';
    
    int il=0;
    for(il=0;il<strlen((const char*)intStr);il++)
    {
       toSend[currentSize++]=intStr[il];
    }
  
    toSend[currentSize++]=0x0d;
    toSend[currentSize++]=0x0a;
    
    sendDataToGSM(toSend,currentSize); 
    state=1;
    return 1;
}

void sendSktDataGSM(uint8* data,uint32 dataLength)
{    
    sendDataToGSM(data,dataLength);
    state=1;
}


#ifdef SMS

void smsSendToGSM(uint8* message, uint8 len)
{
    uint8 packet[150];
    uint8 msglength[10];
    uint8 z=0;
    uint8 size=0;
    memcpy(&packet[0],"AT+CMGS=",9);
    size=size+strlen("AT+CMGS=");
    packet[size++]='"';
    memcpy(&packet[size],SIM_NO,strlen((const char*)SIM_NO));
    size+=strlen((const char*)SIM_NO);
    packet[size++]='"';
    packet[size++]=',';

    z = intToStr(len,&msglength[0]);
    
    memcpy(&packet[size],&msglength[0],z);
    size+=z;
    packet[size++]=0x0d;
    packet[size++]=0x0a;
    smsChk=1;
    memcpy(txBufferGSM,message,len);
    sendDataToGSM(packet,size);
    enableGSMInterrupt();
}

void smsTextMode()
{
    uint8 packet[15];
    uint8 size=0;
    memcpy(&packet[0],"AT+CMGF=1",9);
    size+=9;
    packet [size++] = 0x0d;
    packet [size++] = 0x0a;
    sendDataToGSM (packet, 11); 
    enableGSMInterrupt();
}
void smsViewMode()
{
    uint8 packet[15];
    uint8 size=0;
    memcpy(&packet[0],"AT+CSDH=0",9);
    size=size+strlen("AT+CSDH=0");
    packet [size++] = 0x0d;
    packet [size++] = 0x0a;
    sendDataToGSM(packet,size);
    enableGSMInterrupt();
}
void smsAlertOn()
{   
    uint8 packet[20];
    uint8 size=0;
    memcpy(&packet[0],"AT+CNMI=2,2,0,1,0",17);
    size=size+strlen("AT+CNMI=2,2,0,1,0");
    packet [size++] = 0x0d;
    packet [size++] = 0x0a;
    sendDataToGSM(packet,size);
    enableGSMInterrupt();
}
void smsReadFromGSM(uint8 index)
{
    uint8 packet[15];
    uint8 size=0;
    memcpy(&packet[0],"AT+CMGR=",8);
    size=size+strlen("AT+CMGR=");
    intToStr(index,&packet[size++]);
    packet [size++] = 0x0d;
    packet [size++] = 0x0a;
    sendDataToGSM(packet,size);
    enableGSMInterrupt();
}


void deleteMsg()
{
    uint8 packet[15];
    uint8 size=0;
    memcpy(&packet[0],"AT+CMGD=1,4",11);
    size=size+strlen("AT+CMGD=1,4");
    packet [size++] = 0x0d;
    packet [size++] = 0x0a;
    
    sendDataToGSM(packet,size);
    enableGSMInterrupt();
}


uint8 msgRead(uint8* message)
{   uint8 done=0;
    uint8 temp_data;
    uint8 pkt[200];
    uint8 dataSize=0;
    while (done!=1)
    {
        temp_data = u8AHI_UartReadData(E_AHI_UART_0);
        if(dataSize != 0)
        {
            if((temp_data == 0x0a) && (pkt[dataSize-1]==0x0d))
            {
                vAHI_UartSetInterrupt(E_AHI_UART_0,                  // enable UART_0
                                       FALSE ,
                                       FALSE ,
                                       FALSE ,
                                       TRUE ,
                                       E_AHI_UART_FIFO_LEVEL_1 );   
                done=1;
            }
        }
        pkt[dataSize]=temp_data;
        dataSize++;
    }
    if(dataSize>2  && dataSize<120)
    {
        memcpy(message,&pkt[0],dataSize-2);
    }
    else
    {
        memcpy(message,"drop",4);
        dataSize=6;
    }
    return dataSize-2;
}
void sendSmsData()
{
    if(smsAllow==1)//need to allow this when appcode receive > on the behalf of at+cmgs
    {
        sendDataToGSM(txBufferGSM,txBufferGSMSize);
        enableGSMInterrupt();
    }
break;
}
#endif  //sms

#endif//ENABLE_GSM