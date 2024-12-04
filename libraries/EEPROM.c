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
#include "EEPROM.h"
#include "config.h"
#include "pcInterface.h"
#include "node.h"

/*&&&&&&&&&&&&&&&&&&& API to access SENSEnuts EEPROM &&&&&&&&&&&&&&&&&&&&*/


#define MAX_EEPROM_SECTOR 63
#define MAX_BYTE_PER_SECTOR 64

/*
 * Function to Initlize EEPROM of SENSEnuts Device
 * Input is pointer to receive No of Bytes in each memory Segment
 * Another input is to receive no of Available Memory Segment
 * Returns Nothing
 */

/*&&&&&&&&&&& Final Segment of Memory is already Reserved &&&&&&&&&&&&&*/

void EEPROMInit (uint8 *numOfBytePerSegment, uint16 *numOfSegment)
{
	*numOfSegment = u16AHI_InitialiseEEP(numOfBytePerSegment);
		
}



/*
 * Function to Write Data to EEPROM of SENSEnuts Device
 * Input is Segment Index, Segment ByteAddress, pointer to data Buffer in Ram and Length of Data
 * Return SUCCESS ==== 0
 * Return Failure ==== 1
 */

int EEPROMWrite (uint16 segmentIndexNum,uint8 byteIndexInSegment, uint8 *dataPtr, uint8 dataLen )
{
	int check = iAHI_WriteDataIntoEEPROMsegment(
						segmentIndexNum ,
						byteIndexInSegment ,
						dataPtr ,
						dataLen );
	return check;
}



/*
 * Function to Read Data from EEPROM of SENSEnuts Device
 * Input is Segment Index, Segment ByteAddress, pointer to data Buffer in Ram and Length of Data
 * Return SUCCESS ==== 0
 * Return Failure ==== 1
 */

int EEPROMRead (uint16 segmentIndexNum,uint8 byteIndexInSegment, uint8 *dataPtr, uint8 dataLen )
{
	int check = iAHI_ReadDataFromEEPROMsegment(
						segmentIndexNum ,
						byteIndexInSegment ,
						dataPtr ,
						dataLen );

	return check;
}


/* 
 * Functon to Erase EEPROM Segment
 * Input is Segment index to be erased
 * Return SUCCESS ==== 0
 * Return Failure ==== 1
 */

int EEPROMSectorErase (uint16 segmentIndex)
{
	int check = iAHI_EraseEEPROMsegment(segmentIndex);
	
	return check;
} 
