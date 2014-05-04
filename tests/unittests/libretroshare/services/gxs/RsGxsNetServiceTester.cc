
#include "RsGxsNetServiceTester.h"
#include "services/p3gxscircles.h"

/* This is a wrapper class used to allow testing
 * of internal functions
 */

RsGxsNetServiceTester::RsGxsNetServiceTester(
    		uint16_t servType, RsGeneralDataService *gds,
                RsNxsNetMgr *netMgr,
                RsNxsObserver *nxsObs,  // used to be = NULL.
                const RsServiceInfo serviceInfo,
                RsGixsReputation* reputations, 
		RsGcxs* circles,
                PgpAuxUtils *pgpUtils,
                bool grpAutoSync)
:RsGxsNetService(servType, gds, netMgr, nxsObs, 
	serviceInfo, reputations, circles, pgpUtils, grpAutoSync)
{
	return;
}





// This Function mimics RsGxsNetService::handleRecvSyncGroup()
// It gets all GrpMetaData then checks canSendGrpId.
bool	RsGxsNetServiceTester::fetchAllowedGroups(const RsPeerId &peerId, std::vector<RsGxsGroupId> &groups)
{
	std::cerr << "RsGxsNetServiceTester::fetchAllowedGroups() for: ";
	std::cerr << peerId;
	std::cerr << std::endl;

	/* get data */
	std::map<RsGxsGroupId, RsGxsGrpMetaData*> grp;
	std::map<RsGxsGroupId, RsGxsGrpMetaData*>::iterator it;
        mDataStore->retrieveGxsGrpMetaData(grp);

	for(it = grp.begin(); it != grp.end(); it++)
	{
		RsGxsGrpMetaData& grpMeta = *(it->second);
		std::cerr << "GroupId: " << it->first << "  ";
		std::cerr << " CircleId: " << grpMeta.mCircleId << "  ";
		std::cerr << std::endl;
		if (grpMeta.mCircleType == GXS_CIRCLE_TYPE_EXTERNAL)
		{
			std::cerr << "External Circle Members:";
			std::cerr << std::endl;

        		std::list<RsPgpId> friendlist;
        		std::list<RsPgpId>::iterator fit;
        		mCircles->recipients(grpMeta.mCircleId, friendlist);
			for(fit = friendlist.begin(); fit != friendlist.end(); fit++)
			{
				std::cerr << "\t PgpId: " << *fit;
				std::cerr << std::endl;
			}

			std::cerr << "External Circle Details:";
			std::cerr << std::endl;

			RsGxsCircleDetails details;
			((p3GxsCircles*) mCircles)->getCircleDetails(grpMeta.mCircleId, details);

		        std::set<RsGxsId>::iterator uit;
			for(uit = details.mUnknownPeers.begin(); 
				uit != details.mUnknownPeers.end(); uit++)
			{
				std::cerr << "\t Unknown GxsId: " << *uit;
				std::cerr << std::endl;
			}
        		std::map<RsPgpId, std::list<RsGxsId> >::iterator mit;
			for(mit = details.mAllowedPeers.begin(); 
				mit != details.mAllowedPeers.end(); mit++)
			{
				std::cerr << "\t Allowed PgpId: " << mit->first;
				std::cerr << std::endl;
        			std::list<RsGxsId>::iterator lit;
				for(lit = mit->second.begin(); 
					lit != mit->second.end(); lit++)
				{
					std::cerr << "\t\t From GxsId: " << *lit;
					std::cerr << std::endl;
				}
			}

		}
		else
		{
			std::cerr << "Not External Circle";
			std::cerr << std::endl;
		}
	}


	for(it = grp.begin(); it != grp.end(); it++)
	{
		RsGxsGrpMetaData& grpMeta = *(it->second);
		std::vector<GrpIdCircleVet> toVet;
		bool result = false;
		int count = 0;
		while(!(result = canSendGrpId(peerId, grpMeta, toVet)))
		{
			if (toVet.empty())
			{
				// okay.
				break;
			}
			else
			{
				std::cerr << "RsGxsNetServiceTester::fetchAllowedGroups() pending group: ";
				std::cerr << grpMeta.mGroupId;
				std::cerr << " circleType: ";
				std::cerr << (uint32_t) grpMeta.mCircleType;
				std::cerr << " circleId: " << grpMeta.mCircleId;
				std::cerr << std::endl;
				sleep(1);
				if (count++ > 3)
				{
					std::cerr << "RsGxsNetServiceTester::fetchAllowedGroups() Giving up!";
					std::cerr << std::endl;
					break;
				}
			}
		}

		std::cerr << "\tGroupId: " << it->first << "  ";

		if (result)
		{
			groups.push_back(grpMeta.mGroupId);
			std::cerr << " Allowed";
		}
		else
		{
			std::cerr << " Not Allowed";
		}
		std::cerr << std::endl;

		delete (it->second);
	}
		
	return true;
}



