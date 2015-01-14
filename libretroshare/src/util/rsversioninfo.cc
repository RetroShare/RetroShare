/*
 * rsversion.cc
 *
 *  Created on: Jun 23, 2009
 *      Author: alexandrut
 */

#include "rsversioninfo.h"
#include "retroshare/rsversion.h"
#include "rsstring.h"

std::string RsUtil::retroshareVersion()
{
	std::string version;
	rs_sprintf(version, "%d.%d.%d%s Revision %d", RS_MAJOR_VERSION, RS_MINOR_VERSION, RS_BUILD_NUMBER, RS_BUILD_NUMBER_ADD, RS_REVISION_NUMBER);

	return version;
}

uint32_t RsUtil::retroshareRevision()
{
	return RS_REVISION_NUMBER;
}
