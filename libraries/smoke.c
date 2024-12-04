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
#include <string.h>

#include "config.h"

#ifdef ENABLE_SMOKE
#include "phy.h"
#include "clock.h"  				//provides clock related functionality
#include "node.h"					
#include "task.h"   				//provides addTask
#include "dio.h"
#include "mac.h"
#include "genMac.h"
#include "adc.h"
#include "dio.h"

void enableSmoke()
{
	setPin(2);
}

void disableSmoke()
{
	clearPin(2);
}


uint16 ReadSmoke()
{
  volatile uint8 i;
 // uint16 smokeSample=0;
 uint16 smokeSample=0;
  enableAnalog(INTERRUPT_OFF, INTERNAL_REF);
  for (i = 0; i < 10; i++)
  {
    smokeSample += adcHexRead(ADC2, SINGLE_SHOT); //ADC3
  }
  smokeSample = smokeSample/10;
  disableAnalog();
  disableSmoke();
  //debug(&smokeSample, 2);
  return smokeSample;
}

#endif