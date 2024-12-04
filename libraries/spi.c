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
 
 
#include <AppHardwareApi.h>
#include "AppQueueApi.h"
#include "dio.h"
#include "task.h"
#include "pcInterface.h"
#include "spi.h"

/*
* Function to enable spi 
u8SlaveEnable: Number of SPI slaves to control. Valid values are 0-3 (higher 
               values are truncated to 3)
bLsbFirst:     Enable/disable data transfer with the least significant bit (LSB) 
               transferred first:
                            TRUE - enable
                            FALSE - disable
bPolarity:
               Clock polarity:
                            FALSE - unchanged
                            TRUE - inverted 
bPhase:
               Phase: 
                            FALSE - latch data on leading edge of clock
                            TRUE - latch data on trailing edge of clock
u8ClockDivider:
               Clock divisor in the range 0 to 63. Peripheral clock is divided 
               by 2 x u8ClockDivider, but 0 is a special value used when no 
               clock division is required
bInterruptEnable:
               Enable/disable interrupt when an SPI transfer has completed:
               TRUE - enable
               FALSE - disable
bAutoSlaveSelect:
               Enable/disable automatic slave selection:
               TRUE - enable
               FALSE - disable                            
*/
 
void spiInit(uint8 u8SlaveEnable
             ,bool_t bLsbFirst
             ,bool_t bPolarity
             ,bool_t bPhase
             ,uint8 u8ClockDivider
             ,bool_t bInterruptEnable
             ,bool_t bAutoSlaveSelect)
{
	vAHI_SpiConfigure(u8SlaveEnable
                     ,bLsbFirst
                     ,bPolarity
                     ,bPhase
                     ,u8ClockDivider
                     ,bInterruptEnable
                     ,bAutoSlaveSelect);

}

/*
 * select spi slave line. 0 means deselect all.
 * 
 */
 
void selectSPiSlaveLine(uint8 sel)
{
    vAHI_SpiSelect(sel);
}


/*
 * funtion to disable SPI protocol
 */

void disableSpi()
{
	vAHI_SpiDisable();
}


/*
 * funtion to Deselect SPI line
 */

void deselectSpiSlaveLine()
{	
	vAHI_SpiSelect(0);
}

/*
 * funtion to Exchange Byte with SPI slave device
 */
uint32 SPI_ExchangeByte(uint8 bitLength,uint32 val)
{
	
	uint8 a;
	uint32 i;
	vAHI_SpiStartTransfer(bitLength,val);
	
	do
	{
		a= bAHI_SpiPollBusy();	
	}while(a==1);
	
	i = u32AHI_SpiReadTransfer32();
	
	do
	{
		a= bAHI_SpiPollBusy();
	}while(a==1);
	
	return i;
}
