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
#include "adc.h"
#include "dio.h"

void analogInterruptHandler(uint32 u32DeviceId, uint32 u32ItemBitmap );

// enables analog interface
void enableAnalog(adcInt interruptStatus, adcRef reference)
{
    if (interruptStatus == INTERRUPT_OFF)
    {
        if (reference == INTERNAL_REF)
        {
            vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
                             E_AHI_AP_INT_DISABLE,
                             E_AHI_AP_SAMPLE_8,
                             E_AHI_AP_CLOCKDIV_500KHZ,
                             E_AHI_AP_INTREF);
 						//ledOn();       
            while(!bAHI_APRegulatorEnabled());
          // msdelay(1000);

            //vAHI_ApSetBandGap(E_AHI_AP_BANDGAP_ENABLE);
        }
        else if (reference == EXTERNAL_REF)
        {
            vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
                             E_AHI_AP_INT_DISABLE,
                             E_AHI_AP_SAMPLE_8,
                             E_AHI_AP_CLOCKDIV_500KHZ,
                             E_AHI_AP_EXTREF);

            while(!bAHI_APRegulatorEnabled());
        }
    }
    else if (interruptStatus == INTERRUPT_ON)
    {
        if (reference == INTERNAL_REF)
        {
            vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
                             E_AHI_AP_INT_ENABLE,
                             E_AHI_AP_SAMPLE_8,
                             E_AHI_AP_CLOCKDIV_500KHZ,
                             E_AHI_AP_INTREF);
            
            while(!bAHI_APRegulatorEnabled());

            vAHI_ApSetBandGap(E_AHI_AP_BANDGAP_ENABLE);
	        vAHI_APRegisterCallback(analogInterruptHandler);
        }
        else if (reference == EXTERNAL_REF)
        {
            vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
                             E_AHI_AP_INT_ENABLE,
                             E_AHI_AP_SAMPLE_8,
                             E_AHI_AP_CLOCKDIV_500KHZ,
                             E_AHI_AP_EXTREF);

            while(!bAHI_APRegulatorEnabled());
	        vAHI_APRegisterCallback(analogInterruptHandler);
        }
        
    }
	
}

// disables analog interface
void disableAnalog()
{
    vAHI_ApSetBandGap(E_AHI_AP_BANDGAP_DISABLE);

    vAHI_ApConfigure(E_AHI_AP_REGULATOR_DISABLE,
                     E_AHI_AP_INT_DISABLE,
                     E_AHI_AP_SAMPLE_6,
                     E_AHI_AP_CLOCKDIV_500KHZ,
                     E_AHI_AP_INTREF);
}

// reads ADC value
float adcRead(adcDevice device, adcMode mode)
{
    float voltage;
    uint16 temp;

    // read from ADC specified by device,
    // Possible options are as below:
    // E_AHI_ADC_SRC_ADC_1 (ADC1 input)
    // E_AHI_ADC_SRC_ADC_2 (ADC2 input)
    // E_AHI_ADC_SRC_ADC_3 (ADC3 input)
    // E_AHI_ADC_SRC_ADC_4 (ADC4 input)
    
    switch (device)
    {
        case ADC1:
	    {
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_1);   
            ledOn();
            break;
        }
        case ADC2:
        {
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_2);   
            break;
        }
        case ADC3:
	    {
	        setInput(0);
	        disablePullUp(0);
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_3);   
            break;
        }
        case ADC4:
	    {
	        setInput(1);
	        disablePullUp(1);
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_4);   
            break;
        }    
    }
    // ledOn();


    vAHI_AdcStartSample();

    while(bAHI_AdcPoll());

    temp=u16AHI_AdcRead();        // ADC returns a 16 bit value corresponding to the voltage
    temp = temp & 0x03ff;
        
    voltage = (temp*(2.46/1023));
    return voltage;

    //do check this formula once to cross verify
    //  voltage=(temp*(2.4/1024));                // this is to return a voltage value corresponding to the 16-bit value
                    // returned by ADC
    // The above formula has been calculated as the resolution of ADC is 10 bit and full scale range is 2.4 volts when
    // internal reference voltage is used. Kindly contact us if you need to use external reference voltage. Using external
    // voltage of 1.6V can provide a full scale range of 3.2 volts.
    //return voltage ;        // returns the voltage back. For ease of calculation, kindly return uint16 from this function,
    // print the value of variable "temp" on GUI using debug statement and the covert the voltage manually if
    // you want to monitor the voltage.
}


// read adc conversion in critical task handler
float adcReadCrit()
{
    float voltage;
    uint16 temp;
    temp = u16AHI_AdcRead();        // ADC returns a 16 bit value corresponding to the voltage
    temp = temp & 0x03ff;
    voltage = (temp*(2.4/1024));
    return voltage;
}


// start adc conversion with interrupts enabled
void adcStart(adcDevice device)
{
    switch (device)
    {
        case ADC1:
	    {
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_1);   
            
            break;
        }
        case ADC2:
        {
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_2);   
            break;
        }
        case ADC3:
        {
            setInput(0);
    		disablePullUp(0);
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_3);   
            break;
        }
        case ADC4:
        {
            setInput(1);
    		disablePullUp(1);
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_4);   
            break;
        }    
    }
    
    vAHI_AdcStartSample();
}

// disables the ADC when on in continuous mode
void disableAdc()
{
    vAHI_AdcDisable();                    // disabling ADC to save power
}


// vAHI_APRegisterCallback()

void analogInterruptHandler(uint32 u32DeviceId, uint32 u32ItemBitmap )
{    
    vAppQApiPostCritInt(USER,ADC_CRIT_TASK);   
}


// read the hex value returned by ADC
uint16 adcHexRead(adcDevice device, adcMode mode)
{
    uint16 temp;

    // read from ADC specified by device,
    // Possible options are as below:
    // E_AHI_ADC_SRC_ADC_1 (ADC1 input)
    // E_AHI_ADC_SRC_ADC_2 (ADC2 input)
    // E_AHI_ADC_SRC_ADC_3 (ADC3 input)
    // E_AHI_ADC_SRC_ADC_4 (ADC4 input)
    
    switch (device)
    {
        case ADC1:
	    {
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_1);   
            //ledOn();
            break;
        }
        case ADC2:
        {
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_2);   
            //debug("here", 4);               
            break;
        }
        case ADC3:
        {
            setInput(0);
	    	disablePullUp(0);
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_3);   
            break;
        }
        case ADC4:
        {
            setInput(1);
	    	disablePullUp(1);
            vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
                           E_AHI_AP_INPUT_RANGE_2,
                           E_AHI_ADC_SRC_ADC_4);   
            break;
        }
    }
   
    vAHI_AdcStartSample();
		// debug("here1", 5);
    while(bAHI_AdcPoll());
    //msdelay(500);
    temp=u16AHI_AdcRead();        // ADC returns a 16 bit value corresponding to the voltage
    temp = temp & 0x03ff;
        
    return temp;
    
}

