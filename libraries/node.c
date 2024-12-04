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
#include <jendefs.h>

#include "AppApi.h"
#include "string.h"
#include "node.h"
#include "config.h"

uint16 eRand()
{
	uint16 random;

	vAHI_StartRandomNumberGenerator(
			E_AHI_RND_SINGLE_SHOT ,
			E_AHI_INTS_DISABLED);

	while(!bAHI_RndNumPoll());

	random=u16AHI_ReadRandomNumber();

	return random;
}

uint16 getNodeId()
{
#ifdef FFD_PAN_COORDINATOR
	return 0;
#else
	uint32 *pu32Mac = pvAppApiGetMacAddrLocation();
	uint16 lowAddress=pu32Mac[1];
	return lowAddress;	
#endif

}

uint32 getDeviceId()
{
	uint32 *pu32Mac = pvAppApiGetMacAddrLocation();
	uint32 lowAddress=pu32Mac[1];
	return lowAddress;	

}

uint64 getMacAddress()
{
	uint32 *pu32Mac = pvAppApiGetMacAddrLocation();
	uint64 macAddress;
	memcpy(&macAddress,pu32Mac,8);

    return macAddress;
}

uint64 getEthMacAddress()
{
	uint32 *pu32Mac = pvAppApiGetMacAddrLocation();
	uint64 macAddress;
	memcpy(&macAddress,pu32Mac,8);
	   
    macAddress=macAddress&0xfffffeffffffffff;

    return macAddress;
}


float remainingBattery()
{
	float voltage;
	uint16 temp;
		
	//increased the sampling interval to 8 clock periods instead of 4
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,	
				  E_AHI_AP_INT_DISABLE,
				  E_AHI_AP_SAMPLE_8,//you can change this value to 4, this will obtain the results as before. the results with 8 clock periods has to be tested
				  E_AHI_AP_CLOCKDIV_500KHZ,
				  E_AHI_AP_INTREF);
	
	vAHI_ApSetBandGap(E_AHI_AP_BANDGAP_ENABLE);
	
	while(!bAHI_APRegulatorEnabled());
	
	vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
				E_AHI_AP_INPUT_RANGE_2,
				E_AHI_ADC_SRC_VOLT);
	
	vAHI_AdcStartSample();
	
	while(bAHI_AdcPoll());
	
	temp=u16AHI_AdcRead();
	
	vAHI_AdcDisable();
	
	vAHI_ApSetBandGap(E_AHI_AP_BANDGAP_DISABLE);
	
	vAHI_ApConfigure(E_AHI_AP_REGULATOR_DISABLE,
					 E_AHI_AP_INT_DISABLE,
					 E_AHI_AP_SAMPLE_6,
					 E_AHI_AP_CLOCKDIV_500KHZ,
				     E_AHI_AP_INTREF);
	
	//do check this formula once to cross verify
	voltage=(temp*(2.4/1024))*(1/0.666);
	return voltage;
}

/************* Function to generate delay********************/
 /******INPUTS:- 32 bit value in milliseconds, eg:- just use 200 for a 200ms delay******/
 /******RETURN TYPE:- void********************************/

 void msdelay(uint32 milliSeconds)
  {
      uint16 countValue;    //need to count till 1000 in order to get delay of 10e-3 sec at 1MHz frequency

      vAHI_TimerEnable(E_AHI_TIMER_4,
 					   4,
 					   FALSE,
 					   FALSE,
 					   FALSE);            //enabling timer4 at 1MHz frequency without any interrupt

      while(milliSeconds>0)
      {
       //counting till 1100, need only till 1000, a 100 count buffer is taken to ensure that count of 1000
       //is not missed as from 1000 it would result in a 0 and check to be above 1000 may be missed
       vAHI_TimerStartSingleShot(E_AHI_TIMER_4,
 								500,
 								1100);
       do
       {
          countValue=u16AHI_TimerReadCount(E_AHI_TIMER_4);    //read the count value continually
       }while (countValue<999);    //ideally it should be 1000
       vAHI_TimerStop (E_AHI_TIMER_4);    //stop the timer once the count is above 1000
       milliSeconds--;
      }

      vAHI_TimerDisable (E_AHI_TIMER_4);    //disable the timer to stop clock and save power
  }
