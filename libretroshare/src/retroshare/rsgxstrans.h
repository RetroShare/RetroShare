#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsgxscommon.h"

class RsGxsTransGroup
{
	public:
	RsGroupMetaData mMeta;
};

class RsGxsTransMsg
{
public:
	RsGxsTransMsg() : size(0),data(NULL) {}
	virtual ~RsGxsTransMsg() { free(data) ; }

public:
	RsMsgMetaData mMeta;

	uint32_t size ;
	uint8_t *data ;
};

class RsGxsTrans: public RsGxsIfaceHelper
{
public:
	struct GxsTransStatistics
	{
		RsGxsGroupId prefered_group_id ;
	};

	RsGxsTrans(RsGxsIface *gxs) : RsGxsIfaceHelper(gxs) {}

	virtual ~RsGxsTrans() {}

	virtual bool getStatistics(GxsTransStatistics& stats)=0;

//	virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsTransGroup> &groups) = 0;
//	virtual bool getPostData(const uint32_t &token, std::vector<RsGxsTransMsg> &posts) = 0;
};

extern RsGxsTrans *rsgxstrans ;
