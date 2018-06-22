#ifndef RSGXSSERVICE_H
#define RSGXSSERVICE_H


#include "retroshare/rsgxsifacetypes.h"
#include "retroshare/rstokenservice.h"

struct RsMsgMetaData ;
typedef uint32_t TurtleRequestId;

typedef std::map<RsGxsGroupId, std::vector<RsMsgMetaData> > GxsMsgMetaMap;
typedef std::map<RsGxsGrpMsgIdPair, std::vector<RsMsgMetaData> > GxsMsgRelatedMetaMap;

/*!
 * The aim of this class is to abstract how changes are represented so they can
 * be determined outside the client API without explcitly enumerating all
 * possible changes at the interface
 */
struct RsGxsNotify
{
	enum NotifyType
	{ TYPE_PUBLISHED, TYPE_RECEIVED_NEW, TYPE_PROCESSED, TYPE_RECEIVED_PUBLISHKEY, TYPE_RECEIVED_DISTANT_SEARCH_RESULTS };

	virtual ~RsGxsNotify() {}
	virtual NotifyType getType() = 0;
};

/*!
 * Relevant to group changes
 */
class RsGxsGroupChange : public RsGxsNotify
{
public:
	RsGxsGroupChange(NotifyType type, bool metaChange) : NOTIFY_TYPE(type), mMetaChange(metaChange) {}
    std::list<RsGxsGroupId> mGrpIdList;
    NotifyType getType(){ return NOTIFY_TYPE;}
    bool metaChange() { return mMetaChange; }
private:
    const NotifyType NOTIFY_TYPE;
    bool mMetaChange;
};

class RsGxsDistantSearchResultChange: public RsGxsNotify
{
public:
    RsGxsDistantSearchResultChange(TurtleRequestId id,const RsGxsGroupId& group_id) : mRequestId(id),mGroupId(group_id){}

    NotifyType getType() { return TYPE_RECEIVED_DISTANT_SEARCH_RESULTS ; }
private:
    TurtleRequestId mRequestId ;
 	RsGxsGroupId mGroupId;
};

/*!
 * Relevant to message changes
 */
class RsGxsMsgChange : public RsGxsNotify
{
public:
	RsGxsMsgChange(NotifyType type, bool metaChange) : NOTIFY_TYPE(type), mMetaChange(metaChange) {}
    std::map<RsGxsGroupId, std::set<RsGxsMessageId> > msgChangeMap;
	NotifyType getType(){ return NOTIFY_TYPE;}
    bool metaChange() { return mMetaChange; }
private:
    const NotifyType NOTIFY_TYPE;
    bool mMetaChange;
};



#endif // RSGXSSERVICE_H
