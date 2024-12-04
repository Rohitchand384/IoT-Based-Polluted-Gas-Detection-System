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

#include "i2c.h"

bool i2cOnFlag=FALSE;


/*Initializes I2C with prescalar value 7 which sets the 
I2C clock at 16/40 MHz enables the pulse suppression and 
interrupt*/
void i2cInit()
{
	vAHI_SiMasterConfigure(FALSE ,
						   FALSE ,
			               7);
}

/*Disablesi2c communication*/
void i2cDisable()
{
	vAHI_SiMasterDisable();
	
}

//command for some i2c slave devices that just need a write operation
//without any data
void i2cWriteCommand(uint8 address)
{
	vAHI_SiMasterWriteSlaveAddr(address ,
									   FALSE );

	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
							E_AHI_SI_STOP_BIT,
							E_AHI_SI_NO_SLAVE_READ,
							E_AHI_SI_SLAVE_WRITE,
							E_AHI_SI_SEND_ACK,
							E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());

}


//first write command that sends address on the i2c bus and
//8 bit data to the sensor addressed. It doesn't stop the
//operation. To stop after sending first data use i2cStop
void i2cWrite(uint8 address, uint8 data)
{
	vAHI_SiMasterWriteSlaveAddr(address,
					 FALSE );

	/*********setting the operation to be start and write***/
	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,                     //start before write
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());

	/************writing to pointer register of sensor to
	************* select config register of sensor*******/

	vAHI_SiMasterWriteData8(data);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());

}

//writes the 8 bit data and sends a stop signal. To be used
//when the operation has to be stopped after sending the data

//i2cWrite HAS TO BE USED BEFORE USING THIS API
void i2cWriteAndStop(uint8 data)
{
	vAHI_SiMasterWriteData8(data);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());
}

//writes the 8 bit data and does not stop the operation. To be
//used when subsequent data byte(s) has to be sent after sending 
//the data
//i2cWrite HAS TO BE USED BEFORE USING THIS API
void i2cWriteContinue(uint8 data)
{
	vAHI_SiMasterWriteData8(data);
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());
}

//request i2c slave to send data
void i2cRequestData(uint8 address)
{
	vAHI_SiMasterWriteSlaveAddr(address ,
								   TRUE );

	bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,
						E_AHI_SI_NO_STOP_BIT,
						E_AHI_SI_NO_SLAVE_READ,
						E_AHI_SI_SLAVE_WRITE,
						E_AHI_SI_SEND_ACK,
						E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());
}

//read data sent by i2c slave.
// Return Type: uint8 (8-bit)
uint8 i2cRead()
{
	uint8 i2cData;
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							E_AHI_SI_NO_STOP_BIT,
							E_AHI_SI_SLAVE_READ,
							E_AHI_SI_NO_SLAVE_WRITE,
							E_AHI_SI_SEND_ACK,
							E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());

	i2cData=u8AHI_SiMasterReadData8();

	return i2cData;

}

//read data sent by i2c slave and stop operation.
// Return Type: uint8 (8-bit)

uint8 i2cReadStop()
{
	uint8 i2cData;
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
							E_AHI_SI_STOP_BIT,
							E_AHI_SI_SLAVE_READ,
							E_AHI_SI_NO_SLAVE_WRITE,
							E_AHI_SI_SEND_NACK,
							E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());

	i2cData=u8AHI_SiMasterReadData8();

	return i2cData;

}

//stop communication with the slave
void i2cStop()
{
	bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,
								E_AHI_SI_STOP_BIT,
								E_AHI_SI_NO_SLAVE_READ,
								E_AHI_SI_NO_SLAVE_WRITE,
								E_AHI_SI_SEND_NACK,
								E_AHI_SI_NO_IRQ_ACK);

	while (bAHI_SiMasterPollTransferInProgress());

}




