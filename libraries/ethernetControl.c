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
#include <node.h>
#include "spi.h"
#include "ethernetControl.h"
#include "ether.h"
#include "config.h"

#ifdef ENABLE_ETH
static uint8_t ethernetBank;
static uint16_t NextPacketPtr;
// CSACTIVE for start communication on spi
#define CSACTIVE selectSPiSlaveLine(1)
// CPASSIVE for stop communication bon spi
#define CSPASSIVE selectSPiSlaveLine(0)


/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetReadOp(uint8_t op, uint8_t address)					          *

*	Description  	     To check hardware driver working       						  *
*	Input				 uint8_t op and uint8_t address			 		                          *
*												  	  			  *

*	Return				 uint8_t data									          *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/



uint8_t ethernetReadOp(uint8_t op, uint8_t address)
{
        uint8_t ret;
        CSACTIVE;
       
        ret = SPI_ExchangeByte(7,op | (address & ADDR_MASK));
        ret = SPI_ExchangeByte(7,0x00);
        // do dummy read if needed (for mac and mii, see datasheet page 29)
        if(address & 0x80)
        {
           ret = SPI_ExchangeByte(7,0x00);
        }
        // release CS
        CSPASSIVE;
        return(ret);
}



/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*       Function Name                    ethernetWriteOp(uint8_t op, uint8_t address, uint8_t data)		                  *

*	Description  	     To write a byte of data on NIC       						          *
*	Input				 uint8_t op and uint8_t data			 		                          *
*												  	  			  *

*	Return				 none									                  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/



void ethernetWriteOp(uint8_t op, uint8_t address, uint8_t data)
{
    uint8_t ret;
    CSACTIVE;
       
    ret = SPI_ExchangeByte(7,op | (address & ADDR_MASK));
        
    ret=SPI_ExchangeByte(7,data);
    CSPASSIVE;
}



/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetReadBuffer(uint16_t len, uint8_t* data)					  *

*	Description  	     To read complete buffer of NIC       						  	  *
*	Input				 uint8_16 len and uint8_t data			 		                          *
*												  	  			  *

*	Return				 none									         	  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/

void ethernetReadBuffer(uint16_t len, uint8_t* data)
{
    uint8_t ret;
    CSACTIVE;
    
    ret = SPI_ExchangeByte(7,ethernet_READ_BUF_MEM);
    while(len)
    {
            len--;
            
            ret = SPI_ExchangeByte(7,0x0);
            *data = ret;
            data++;
    }
    *data='\0';
    CSPASSIVE;
}


/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetWriteBuffer(uint8_t op, uint8_t address, uint8_t data)		                  *

*	Description  	     To write data of data on NIC Buffer     						  *
*	Input				 uint16_t len and uint8_t data			 		                          *
*												  	  			  *

*	Return				 none									                  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/

void ethernetWriteBuffer(uint16_t len, uint8_t* data)
{
        
    CSACTIVE;
    
    SPI_ExchangeByte(7,ethernet_WRITE_BUF_MEM);
    while(len)
    {
            len--;
            
            SPI_ExchangeByte(7,*data);
            data++;
    }
    CSPASSIVE;
}



/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetSetBank(uint8_t address)		                  			  *

*	Description  	     This is to select the appropriate bank on NIC     				          *
*	Input				 uint8_t address				 		                          *
*												  	  			  *

*	Return				 none									                  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/



void ethernetSetBank(uint8_t address)
{
    // set the bank (if needed)
    if((address & BANK_MASK) != ethernetBank)
    {
            
            // set the bank
            ethernetWriteOp(ethernet_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
            ethernetWriteOp(ethernet_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
            ethernetBank = (address & BANK_MASK);
    }
}



/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetRead(uint8_t address)			                  			  *

*	Description  	     To read address location on ethernet module    				          		  *
*	Input				 uint8_t address				 		                          *
*												  	  			  *

*	Return				 data at the address									               *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/



uint8_t ethernetRead(uint8_t address)
{
    // set the bank
    ethernetSetBank(address);
    // do the read
    return ethernetReadOp(ethernet_READ_CTRL_REG, address);
}


/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetWrite(uint8_t address)			                  			  *

*	Description  		 To write on address location on NIC     				                  *
*	Input				 uint8_t address, uint8_t data				 		                  *
*												  	  			  *

*	Return				 none 											  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/


void ethernetWrite(uint8 address, uint8 data)
{
    // set the bank
    ethernetSetBank(address);
    // do the write
    ethernetWriteOp(ethernet_WRITE_CTRL_REG, address, data);
}


/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetPhyWrite (uint8_t address,uint16_t data)			                  *

*	Description  		 To read address location on ethernet module    				          		  *
*	Input				 uint8_t address, uint16_t data				 		                  *
*												  	  			  *

*	Return				 data at the address									  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/



void ethernetPhyWrite(uint8_t address, uint16_t data)
{
	
    // set the PHY register address
    ethernetWrite(MIREGADR, address);
    // write the PHY data
    ethernetWrite(MIWRL, data);
    ethernetWrite(MIWRH, data>>8);
    // wait until the PHY write completes
    while(ethernetRead(MISTAT) & MISTAT_BUSY){
           // _delay_us(15);
            msdelay(1);
    }
}

/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetReset (uint8_t* macaddr)			                  *

*	Description  		 To reset the mac layer and set its mac address and other default configurations    				          		  *
*	Input				 uint8_t* macaddr (mac address to be alloted to ethernet)				 		                  *
*												  	  			  *

*	Return				 None									  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/
void ethernetReset(uint8_t* macaddr)
{
    //uint8 packet [2];
	
	ethernetWriteOp(ethernet_SOFT_RESET, 0, ethernet_SOFT_RESET);
	          
	msdelay(5);     //in errata b7 it is mentioned that we should wait for 1 ms after soft reset command
	
	NextPacketPtr = RXSTART_INIT;
    // Rx buffer start address
	ethernetWrite(ERXSTL, RXSTART_INIT&0xFF);
	
	ethernetWrite(ERXSTH, RXSTART_INIT>>8);
    // set receive pointer address
	ethernetWrite(ERXRDPTL, RXSTART_INIT&0xFF);
	ethernetWrite(ERXRDPTH, RXSTART_INIT>>8);  
    // RX buffer end address
	ethernetWrite(ERXNDL, RXSTOP_INIT&0xFF);
	ethernetWrite(ERXNDH, RXSTOP_INIT>>8);
	// TX buffer start address
	ethernetWrite(ETXSTL, TXSTART_INIT&0xFF);
	ethernetWrite(ETXSTH, TXSTART_INIT>>8);
	// TX buffer end address
	ethernetWrite(ETXNDL, TXSTOP_INIT&0xFF);
	ethernetWrite(ETXNDH, TXSTOP_INIT>>8);
	// do bank 1 stuff, packet filter:
    // For broadcast packets we allow only ARP packtets
    // All other packets should be unicast only for our mac (MACADDR)
    //
    // The pattern to match on is therefore
    // Type     ETH.DST
    // ARP      BROADCAST
    // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
    // in binary these poitions are:11 0000 0011 1111
    // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
	ethernetWrite(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN);  //enabling default broadcast mode as enableBroadcast doesn't work
	ethernetWrite(EPMM0, 0x3f);
	ethernetWrite(EPMM1, 0x30);
	ethernetWrite(EPMCSL, 0xf9);
	ethernetWrite(EPMCSH, 0xf7);
	
	// do bank 2 stuff
	// Enable MAC receive
	ethernetWrite(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	ethernetWrite(MACON2, 0x00);
	// Enable automatic padding to 60bytes and CRC operations
	ethernetWriteOp(ethernet_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);	
	// set inter-frame gap (non-back-to-back)
	ethernetWrite(MAIPGL, 0x12);
	ethernetWrite(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	ethernetWrite(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
    // Do not send packets longer than MAX_FRAMELEN:
	ethernetWrite(MAMXFLL, MAX_FRAMELEN&0xFF);
	ethernetWrite(MAMXFLH, MAX_FRAMELEN>>8);
    
    // write MAC address
    // NOTE: MAC address in ethernet is byte-backward
    ethernetWrite(MAADR5, macaddr[0]);
    ethernetWrite(MAADR4, macaddr[1]);
    ethernetWrite(MAADR3, macaddr[2]);
    ethernetWrite(MAADR2, macaddr[3]);
    ethernetWrite(MAADR1, macaddr[4]);
    ethernetWrite(MAADR0, macaddr[5]);
    
	// no loopback of transmitted frames
	ethernetPhyWrite(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
	
	ethernetSetBank(ECON1);
	
	// enable interrupt on packet receive
	ethernetWriteOp(ethernet_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	ethernetWriteOp(ethernet_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	
	//make leds blink on tx rx
	ethernetPhyWrite(PHLCON,0x476);
}


/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetInit (uint8_t *macaddr)			 		                  *

*	Description  	     To read address location on ethernet module     				          		  *
*	Input				 To init the NIC			 		                  		  *
*												  	  			  *

*	Return				 none									                  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/



void ethernetInit(uint8_t* macaddr)
{
    spiInit(0,
            FALSE,
            FALSE,
            FALSE,
            2,
            FALSE,
            FALSE);
   
   ethernetReset(macaddr);
}

// read the revision of the chip:


/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetgetrev ()			                  				  *

*	Description  	     Silicon Rev. of  NIC     				          		  	  *
*	Input				 uint8_t address, uint16_t data				 		                  *
*												  	  			  *

*	Return				 rev value									  	  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/



uint8_t ethernetGetRev(void)
{
	//return(ethernetRead(EREVID));
    return(ethernetRead(ECOCON));
}

/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetPacketSend(uint16_t len, uint8_t* packet)			          	  *

*	Description  		 This is to send packet on network     				          	  *
*	Input				 uint8_t len, uint16_t* packet				 		                  *
*												  	  			  *

*	Return				 none									  		  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/


void ethernetPacketSend(uint16 len, uint8* packet)
{
    // Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	/*if((ethernetRead(EIR) & (EIR_TXIF|EIR_TXERIF)) )
	{
       ethernetWriteOp(ethernet_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
    }*/
    
    while (ethernetReadOp(ethernet_READ_CTRL_REG, ECON1) & ECON1_TXRTS)
    {
        if ((ethernetRead(EIR) & (EIR_TXIF|EIR_TXERIF)))
        {
            ethernetWriteOp(ethernet_BIT_FIELD_SET, ECON1, ECON1_TXRST);
            ethernetWriteOp(ethernet_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
            ethernetWriteOp(ethernet_BIT_FIELD_CLR, EIR, EIR_TXERIF);
        }
    }
    ethernetWrite(EWRPTL, TXSTART_INIT&0xFF);
	ethernetWrite(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	ethernetWrite(ETXNDL, (TXSTART_INIT+len)&0xFF);
	ethernetWrite(ETXNDH, (TXSTART_INIT+len)>>8);
    ethernetWriteOp(ethernet_WRITE_BUF_MEM, 0, 0x00);
    ethernetWriteBuffer(len, packet);
    ethernetWriteOp(ethernet_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

// Gets a packet from the network receive buffer, if one is available.
// The packet will by headed by an ethernet header.
// maxlen  The maximum acceptable length of a retrieved packet.
// packet  Pointer where packet data should be stored.
// Returns: Packet length in bytes if a packet was retrieved, zero otherwise.


/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetPacketReceive(uint16_t maxlen, uint8_t* packet)			          *

*	Description  	     This is to receive packet from network    				          *
*	Input				 uint8_t address, uint16_t data				 		                  *
*												  	  			  *

*	Return				 length of data 									  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/


uint16_t ethernetPacketReceive(uint16_t maxlen, uint8_t* packet)
{       
	uint16_t rxstat;
	uint16_t len;
	uint16 pktCnt=ethernetRead(EPKTCNT);
	//See Rev. B4 Silicon Errata point 6.
	if( pktCnt ==0 )
	{
		return(0);
    }
	// Set the read pointer to the start of the received packet
	ethernetWrite(ERDPTL, (NextPacketPtr));
	ethernetWrite(ERDPTH, (NextPacketPtr)>>8);
	// read the next packet pointer
	NextPacketPtr  = ethernetReadOp(ethernet_READ_BUF_MEM, 0);
	NextPacketPtr |= ethernetReadOp(ethernet_READ_BUF_MEM, 0)<<8;
	// read the packet length (see datasheet page 43)
	len  = ethernetReadOp(ethernet_READ_BUF_MEM, 0);
	len |= ethernetReadOp(ethernet_READ_BUF_MEM, 0)<<8;
    len-=4; //remove the CRC count
	// read the receive status (see datasheet page 43)
	rxstat  = ethernetReadOp(ethernet_READ_BUF_MEM, 0);
	rxstat |= ethernetReadOp(ethernet_READ_BUF_MEM, 0)<<8;
	// limit retrieve length
    if (len>maxlen-1)
    {
      len=maxlen-1;
    }
    // check CRC and symbol errors (see datasheet page 44, table 7-3):
    // The ERXFCON.CRCEN is set by default. Normally we should not
    // need to check this.
    if ((rxstat & 0x80)==0)
    {
        // invalid 
        len=0;
    }
    else
    {
        // copy the packet from the receive buffer
        ethernetReadBuffer(len, packet);
    }
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	//making chnges as per errata sheet b7, making sure that ERXRDPT is always odd
	
	uint16 ptrToWrite;
	if ((NextPacketPtr-1 < RXSTART_INIT) || (NextPacketPtr-1 > RXSTOP_INIT))         //not very sure about this-ankur
    {
        ptrToWrite=RXSTOP_INIT;
    }
    else
    {
        ptrToWrite= NextPacketPtr-1;
    }

	ethernetWrite(ERXRDPTL, (ptrToWrite));
	ethernetWrite(ERXRDPTH, (ptrToWrite)>>8);
	// decrement the packet counter indicate we are done with this packet
	ethernetWriteOp(ethernet_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
	return(len);
}

// Functions to enable broadcast receive
void enableBroadcast() 
{
    //ethernetWrite(ERXFCON,ethernetRead(ERXFCON)|ERXFCON_BCEN);
    
}

// Functions to disable broadcast receive
void disableBroadcast () 
{
    ethernetWrite(ERXFCON, ethernetRead(ERXFCON) & (~ERXFCON_BCEN));
}


/**********************************************************************************************************************************
***********************************************************************************************************************************
*																  *

*   Function Name        ethernetPhyRead (uint8_t address,uint16_t data)			                  *

*	Description  		 To read address location on ethernet module    				          		  *
*	Input				 uint8_t address, uint16_t data				 		                  *
*												  	  			  *

*	Return				 data at the address									  *
*	Author				 Eigen							  		  	 	  *

*	Date				 25-04-2016								  		  *
*																  *
**********************************************************************************************************************************
**********************************************************************************************************************************/



uint16 ethernetPhyRead(uint8 address)
{
	uint16 val=0;
    // set the PHY register address
    ethernetWrite(MIREGADR, address);
    
    ethernetWrite(MICMD, MICMD_MIIRD);
    // wait until the PHY write completes
    while(ethernetRead(MISTAT) & MISTAT_BUSY){
           // _delay_us(15);
            msdelay(1);
    }
    ethernetWrite(MICMD, 0x00);
   
    ethernetSetBank(MIRDL);
    // do the read
    val=ethernetReadOp(ethernet_READ_CTRL_REG, MIRDL);
	
	val |= (0x0000|ethernetReadOp(ethernet_READ_CTRL_REG, MIRDH))<<8;

    return val;
}

/*
 * Access the PHY to determine link status
 */
uint8 ethernetCheckLinkStatus()
{
    uint16 status=ethernetPhyRead(PHSTAT1);
    
    if ((status&PHSTAT1_LLSTAT)==0)
        return 0;
    else
        return 1;    	
}

uint16 getCurrentPacketCount()
{
    return ethernetRead(EPKTCNT);
}

uint16 ethernetCheckPHSTAT2()
{
    uint16 status=ethernetPhyRead(PHSTAT2);
    return status;   	
}

#endif //#ifdef ENABLE_ETH