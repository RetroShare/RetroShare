/*
 * NxsGrpSyncDelayed.cpp
 *
 *  Created on: 19 May 2014
 *      Author: crispy
 */

#include "nxsgrpsyncdelayed.h"
#include "nxsdummyservices.h"

namespace rs_nxs_test {

NxsGrpSyncDelayed::NxsGrpSyncDelayed()
 : NxsGrpSync(new rs_nxs_test::RsNxsDelayedDummyCircles(4), NULL) {


}

NxsGrpSyncDelayed::~NxsGrpSyncDelayed() {
	// TODO Auto-generated destructor stub
}

} /* namespace rs_nxs_test */
