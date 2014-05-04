
#include "gxs/rsgxsnetservice.h"

/* This is a wrapper class used to allow testing
 * of internal functions
 */

class RsGxsNetServiceTester: public RsGxsNetService
{
public:

RsGxsNetServiceTester(
                uint16_t servType, RsGeneralDataService *gds,
                RsNxsNetMgr *netMgr,
                RsNxsObserver *nxsObs,  // used to be = NULL.
                const RsServiceInfo serviceInfo,
                RsGixsReputation* reputations = NULL, RsGcxs* circles = NULL,
                PgpAuxUtils *pgpUtils = NULL,
                bool grpAutoSync = true);


bool	fetchAllowedGroups(const RsPeerId &peerId, std::vector<RsGxsGroupId> &groups);

};





