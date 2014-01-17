/*
 * rsversion.cc
 *
 *  Created on: Jun 23, 2009
 *      Author: alexandrut
 */

#include "rsversion.h"

#define LIB_VERSION "0.5.5c"

std::string RsUtil::retroshareVersion()
{
	return std::string(LIB_VERSION) + " " + std::string(SVN_REVISION);

}

uint32_t RsUtil::retroshareRevision()
{
	return SVN_REVISION_NUMBER;
}

