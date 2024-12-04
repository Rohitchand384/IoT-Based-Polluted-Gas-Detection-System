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

#include "flash.h"
#include "node.h"

//for OTA
#include "config.h"

/**************************************************************************
Function to initialize internal flash memory. It has to be made sure by the 
user that while doing a flash related operation, especially a flash write 
operation, user must make sure, not to write where the program is written. 
To avoid any confusion, we are allocating last 4 sectors (size of each 
sector = 32K) for user usage. 
**************************************************************************/
bool flashInit()
{
	bool check= bAHI_FlashInit(E_FL_CHIP_AUTO,NULL);

	return check;
}

/**************************************************************************
Function to erase a sector in flash. Sensenuts device consists of 8 sectors 
numbered 0-7. Use of sector 0-4 for user data must be avoided. In case it is
known that program size fits in a specific number of sectors let say 2, 
in that case user may use sector 0,1 for program and 2-7 to save data. Before
any write operation, we first of all need to erase the entire sector on which
data is to be written. A partial erase of sector is not allowed.
**************************************************************************/
bool flashErase(uint8 sectorNum)
{
	bool check=bAHI_FlashEraseSector(sectorNum);
	return check;
}


/**************************************************************************
Function to write data in the flash memory. A sector can be written into 
only when a erase of the same sector has been performed earlier.
The starting address of all sectors are

sector 0: 0x00000000
sector 1: 0x00008000
sector 2: 0x00010000
sector 3: 0x00018000
sector 4: 0x00020000
sector 5: 0x00028000
sector 6: 0x00030000
sector 7: 0x00038000

A user must always keep a track of where the data was written last, to avoid 
any data corruption due to over writing. 

length must be a multiple of 16 with maximum value = 32k.
**************************************************************************/

bool flashWrite(uint32 address,uint16 length,uint8 * data)
{
	bool check=bAHI_FullFlashProgram(address,
									  length,		//must be a multiple of 16
									  data);

	return check;
}

/**************************************************************************
Function to read data from flash memory.

length must have value <= 32k.
**************************************************************************/
bool flashRead(uint32 address,uint16 length,uint8 * data)
{
	bool check=bAHI_FullFlashRead(address,
								   length,
								   data);

	return check;
}


#ifdef ENABLE_OTA
/**************************************************************************
Function to setup OTA operation
**************************************************************************/
void setupOTA()
{
	//initially erase any data written on flash memory in sectors 4-7. This is necessary to perform any write operation
	//if data of any sector has to be saved at node startup, you need to comment corresponding line below. 
	bAHI_FlashEraseSector(4);
	msdelay(40);
	bAHI_FlashEraseSector(5);
	msdelay(40);
	bAHI_FlashEraseSector(6);
	msdelay(40);
	bAHI_FlashEraseSector(7);
	msdelay(40);
}
#endif
