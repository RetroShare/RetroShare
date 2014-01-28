/*
 * rsversion.h
 *
 *  Created on: Jun 23, 2009
 *      Author: alexandrut
 */

#include <string>
#include <inttypes.h>

// These versioning parameters are in the header because plugin versioning requires it.
// Please use the functions below, and don't refer directly to the #defines.

#define SVN_REVISION "Revision 7062"
#define SVN_REVISION_NUMBER  7062

namespace RsUtil {

	uint32_t retroshareRevision();
	std::string retroshareVersion();

}
