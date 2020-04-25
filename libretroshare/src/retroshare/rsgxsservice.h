/*******************************************************************************
 * libretroshare/src/retroshare: rsgxsservice.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2015  Retroshare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RSGXSSERVICE_H
#define RSGXSSERVICE_H


#include "retroshare/rsgxsifacetypes.h"
#include "retroshare/rstokenservice.h"

struct RsMsgMetaData ;
typedef uint32_t TurtleRequestId;

typedef std::map<RsGxsGroupId, std::vector<RsMsgMetaData> > GxsMsgMetaMap;
typedef std::map<RsGxsGrpMsgIdPair, std::vector<RsMsgMetaData> > GxsMsgRelatedMetaMap;

class RsNxsGrp;

/*!
 * The aim of this class is to abstract how changes are represented so they can
 * be determined outside the client API without explcitly enumerating all
 * possible changes at the interface
 */
struct RsGxsNotify
{
	enum NotifyType
	{
        TYPE_UNKNOWN                           = 0x00,
        TYPE_PUBLISHED                         = 0x01,
        TYPE_RECEIVED_NEW                      = 0x02,
        TYPE_PROCESSED                         = 0x03,
        TYPE_RECEIVED_PUBLISHKEY               = 0x04,
        TYPE_RECEIVED_DISTANT_SEARCH_RESULTS   = 0x05,
        TYPE_STATISTICS_CHANGED                = 0x06,
        TYPE_UPDATED                           = 0x07,
	};

	virtual ~RsGxsNotify() {}
	virtual NotifyType getType() = 0;
};

/*!
 * Relevant to group changes
 */
class RsGxsGroupChange : public RsGxsNotify
{
public:
	RsGxsGroupChange(NotifyType type, bool metaChange) : mNotifyType(type), mMetaChange(metaChange) {}
    std::list<RsGxsGroupId> mGrpIdList;
    NotifyType getType() override { return mNotifyType;}
    bool metaChange() { return mMetaChange; }

protected:
    NotifyType mNotifyType;
    bool mMetaChange;
};

class RsGxsGroupUpdate : public RsGxsNotify
{
public:
    RsGxsGroupUpdate() : mOldGroup(nullptr),mNewGroup(nullptr) {}

    RsGxsGroupId mGroupId;
    RsNxsGrp *mOldGroup;
    RsNxsGrp *mNewGroup;

    NotifyType getType() override { return RsGxsNotify::TYPE_UPDATED;}
};


class RsGxsDistantSearchResultChange: public RsGxsNotify
{
public:
    RsGxsDistantSearchResultChange(TurtleRequestId id,const RsGxsGroupId& group_id) : mRequestId(id),mGroupId(group_id){}

    NotifyType getType() { return TYPE_RECEIVED_DISTANT_SEARCH_RESULTS ; }

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
