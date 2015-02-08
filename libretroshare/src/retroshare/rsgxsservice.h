#ifndef RSGXSSERVICE_H
#define RSGXSSERVICE_H


#include "retroshare/rsgxsifacetypes.h"
#include "retroshare/rstokenservice.h"

class RsMsgMetaData ;

typedef std::map<RsGxsGroupId, std::vector<RsMsgMetaData> > GxsMsgMetaMap;
typedef std::map<RsGxsGrpMsgIdPair, std::vector<RsMsgMetaData> > GxsMsgRelatedMetaMap;

/*!
 * The aim of this class is to abstract
 * how changes are represented so
 * they can be determined outside the
 * client API without explcitly
 * enumerating all possible changes
 * at the interface
 */
class RsGxsNotify
{
public:

	enum NotifyType { TYPE_PUBLISH, TYPE_RECEIVE, TYPE_PROCESSED, TYPE_PUBLISHKEY };

    virtual ~RsGxsNotify() {return; }
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

/*!
 * Relevant to message changes
 */
class RsGxsMsgChange : public RsGxsNotify
{
public:
	RsGxsMsgChange(NotifyType type, bool metaChange) : NOTIFY_TYPE(type), mMetaChange(metaChange) {}
    std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgChangeMap;
	NotifyType getType(){ return NOTIFY_TYPE;}
    bool metaChange() { return mMetaChange; }
private:
    const NotifyType NOTIFY_TYPE;
    bool mMetaChange;
};



#endif // RSGXSSERVICE_H
