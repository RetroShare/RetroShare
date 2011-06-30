/*
 * rsversion.cc
 *
 *  Created on: Jun 23, 2009
 *      Author: alexandrut
 */

#include "rsversion.h"

std::string RsUtil::retroshareVersion()
{
	return std::string(LIB_VERSION) + " " + std::string(SVN_REVISION);

}
