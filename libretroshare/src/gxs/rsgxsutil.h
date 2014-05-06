/*
 * libretroshare/src/gxs: rsgxsutil.h
 *
 * RetroShare C++ Interface. Generic routines that are useful in GXS
 *
 * Copyright 2013-2013 by Christopher Evi-Parker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef GXSUTIL_H_
#define GXSUTIL_H_

#include <vector>
#include "serialiser/rsnxsitems.h"
#include "rsgds.h"

/*!
 * Handy function for cleaning out meta result containers
 * @param container
 */
template <class Container, class Item>
void freeAndClearContainerResource(Container container)
{
	typename Container::iterator meta_it = container.begin();

	for(; meta_it != container.end(); meta_it++)
	{
		delete meta_it->second;

	}
	container.clear();
}

inline RsGxsGrpMsgIdPair getMsgIdPair(RsNxsMsg& msg)
{
	return RsGxsGrpMsgIdPair(std::make_pair(msg.grpId, msg.msgId));
}

inline RsGxsGrpMsgIdPair getMsgIdPair(RsGxsMsgItem& msg)
{
	return RsGxsGrpMsgIdPair(std::make_pair(msg.meta.mGroupId, msg.meta.mMsgId));
}

/*!
 * Does message clean up based on individual group expirations first
 * if avialable. If not then deletion s
 */
class RsGxsMessageCleanUp : public RsThread
{
public:

	/*!
	 *
	 * @param dataService
	 * @param mGroupTS
	 * @param chunkSize
	 * @param sleepPeriod
	 */
	RsGxsMessageCleanUp(RsGeneralDataService* const dataService, uint32_t messageStorePeriod, uint32_t chunkSize);

	/*!
	 * On construction this should be called to progress deletions
	 * Deletion will process by chunk size
	 * @return true if no more messages to delete, false otherwise
	 */
	bool clean();

	/*!
	 * TODO: Rather than manual progressions consider running through a thread
	 */
	void run(){}

private:

	RsGeneralDataService* const mDs;
	const uint32_t MESSAGE_STORE_PERIOD, CHUNK_SIZE;
	std::vector<RsGxsGrpMetaData*> mGrpMeta;
};

/*!
 * Checks the integrity message and groups
 * in rsDataService using computed hash
 */
class RsGxsIntegrityCheck : public RsThread
{

	enum CheckState { CheckStart, CheckChecking };

public:


	/*!
	 *
	 * @param dataService
	 * @param mGroupTS
	 * @param chunkSize
	 * @param sleepPeriod
	 */
	RsGxsIntegrityCheck(RsGeneralDataService* const dataService);


	bool check();
	bool isDone();

	void run();

private:

	RsGeneralDataService* const mDs;
	std::vector<RsNxsItem*> mItems;
	bool mDone;
	RsMutex mIntegrityMutex;

};

class GroupUpdate
{
public:
	GroupUpdate() : oldGrpMeta(NULL), newGrp(NULL), validUpdate(false)
	{}
	RsGxsGrpMetaData* oldGrpMeta;
	RsNxsGrp* newGrp;
	bool validUpdate;
};

class GroupUpdatePublish
{
public:
        GroupUpdatePublish(RsGxsGrpItem* item, uint32_t token)
            : grpItem(item), mToken(token) {}
	RsGxsGrpItem* grpItem;
	uint32_t mToken;
};

class GroupDeletePublish
{
public:
        GroupDeletePublish(RsGxsGrpItem* item, uint32_t token)
            : grpItem(item), mToken(token) {}
	RsGxsGrpItem* grpItem;
	uint32_t mToken;
};

#endif /* GXSUTIL_H_ */
