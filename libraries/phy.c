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

#include "AppApi.h"
#include "config.h"

void setTransmitPower(int32 powerLevel)
{
	eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER,(uint32) powerLevel);
}


void  setCcaMode(uint8 ccaMode)
{
	eAppApiPlmeSet(PHY_PIB_ATTR_CCA_MODE,ccaMode);
}
