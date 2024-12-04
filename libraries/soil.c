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
#include "AppQueueApi.h"
#include "soil.h"
#include "adc.h"
#include "config.h"
#include "dio.h"

#ifdef ENABLE_SOIL

extern void setDigOut(uint8 doVal);
extern void	clearDigOut(uint8 doVal);

/* SNSMT 2.0 for Providing Power*/
void soilSensorOn()
{
	setPin(17);
}

/* SNSMT 2.0 for Clearing Power*/
void soilSensorOff()
{
	clearPin(17);
}

/* SNSMT 1.0 for Providing Power*/
void soilPower(bool status)
{
	
	if (status == TRUE)
	{
		setDigOut(0);
	}
	else
	{
		clearDigOut(0);
	}
}

void enableSoilSensors()
{
	soilSensorOn();
	enableAnalog(INTERRUPT_OFF, INTERNAL_REF);
}

void disableSoilSensors()
{
	soilSensorOff();	
	disableAnalog();
}

uint16 soilMoisture()
{
	uint16 moisture;
	
	moisture = adcHexRead(ADC1, SINGLE_SHOT);
	return moisture;

}

uint16 soilTemp()
{
	uint16 temp;
	temp = adcHexRead(ADC4, SINGLE_SHOT);
	return temp;
}

float floatMoist(uint16 moisture)
{
	float moisVolt = 0.0, vwc=0.0;
	moisVolt = moisture*2.4/1024;
	if(moisVolt <= 1.1)
	{
		// converting voltage to VWC
		vwc = 10 * moisVolt - 1;
	}
	else if (moisVolt > 1.1 && moisVolt <= 1.3)
	{
		// converting voltage to VWC
		vwc = 25 * moisVolt -17.5;
	}
	else if (moisVolt > 1.3 && moisVolt <= 1.82)
	{
		// converting voltage to VWC
		vwc = 48.08 * moisVolt - 47.5;
	}
	else if (moisVolt > 1.82 && moisVolt <= 2.2)
	{
		// converting voltage to VWC
		vwc = 26.32 * moisVolt -7.89;
	}
	else if (moisVolt > 2.2)
	{
	  
		// converting voltage to VWC
		vwc = 62.5*moisVolt - 87.5;
	}
	
	return vwc;
}

#endif