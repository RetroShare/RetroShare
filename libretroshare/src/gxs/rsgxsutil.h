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
#include "rsitems/rsnxsitems.h"
#include "rsgds.h"

class RsGixs ;
class RsGenExchange ;

/*!
 * Handy function for cleaning out meta result containers
 * @param container
 */
template <class Container, class Item>
void freeAndClearContainerResource(Container container)
{
	typename Container::iterator meta_it = container.begin();

	for(; meta_it != container.end(); ++meta_it)
        	if(meta_it->second != NULL)
			delete meta_it->second;

	container.clear();
}

// temporary holds a map of pointers to class T, and destroys all pointers on delete.

template<class T>
class RsGxsMetaDataTemporaryMap: public std::map<RsGxsGroupId,T*>
{
public:
    virtual ~RsGxsMetaDataTemporaryMap()
    {
        clear() ;
    }

    virtual void clear()
    {
        for(typename RsGxsMetaDataTemporaryMap<T>::iterator it = this->begin();it!=this->end();++it)
            if(it->second != NULL)
		    delete it->second ;

        std::map<RsGxsGroupId,T*>::clear() ;
    }
};

template<class T>
class RsGxsMetaDataTemporaryMapVector: public std::map<RsGxsGroupId,std::vector<T*> >
{
public:
    virtual ~RsGxsMetaDataTemporaryMapVector()
    {
        clear() ;
    }

    virtual void clear()
    {
        for(typename RsGxsMetaDataTemporaryMapVector<T>::iterator it = this->begin();it!=this->end();++it)
        {
            for(uint32_t i=0;i<it->second.size();++i)
				delete it->second[i] ;

            it->second.clear();
        }

        std::map<RsGxsGroupId,std::vector<T*> >::clear() ;
    }
};
#ifdef UNUSED
template<class T>
class RsGxsMetaDataTemporaryMapVector: public std::vector<T*>
{
public:
    virtual ~RsGxsMetaDataTemporaryMapVector()
    {
        clear() ;
    }

    virtual void clear()
    {
        for(typename RsGxsMetaDataTemporaryMapVector<T>::iterator it = this->begin();it!=this->end();++it)
            if(it->second != NULL)
		    delete it->second ;
        std::vector<T*>::clear() ;
    }
};
#endif


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
class RsGxsMessageCleanUp //: public RsThread
{
public:

	/*!
	 *
	 * @param dataService
	 * @param mGroupTS
	 * @param chunkSize
	 * @param sleepPeriod
	 */
	RsGxsMessageCleanUp(RsGeneralDataService* const dataService, RsGenExchange *genex, uint32_t chunkSize);

	/*!
	 * On construction this should be called to progress deletions
	 * Deletion will process by chunk size
	 * @return true if no more messages to delete, false otherwise
	 */
	bool clean();

	/*!
	 * TODO: Rather than manual progressions consider running through a thread
	 */
    //virtual void data_tick(){}

private:

	RsGeneralDataService* const mDs;
    RsGenExchange *mGenExchangeClient;
	uint32_t CHUNK_SIZE;
	std::vector<RsGxsGrpMetaData*> mGrpMeta;
};

/*!
 * Checks the integrity message and groups
 * in rsDataService using computed hash
 */
class RsGxsIntegrityCheck : public RsSingleJobThread
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
	RsGxsIntegrityCheck(RsGeneralDataService* const dataService, RsGenExchange *genex, RsGixs *gixs);

	bool check();
	bool isDone();

	void run();

	void getDeletedIds(std::list<RsGxsGroupId>& grpIds, std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >& msgIds);

private:

	RsGeneralDataService* const mDs;
    RsGenExchange *mGenExchangeClient;
	bool mDone;
	RsMutex mIntegrityMutex;
	std::list<RsGxsGroupId> mDeletedGrps;
	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > mDeletedMsgs;
    
    	RsGixs *mGixs ;
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
        GroupDeletePublish(const RsGxsGroupId& grpId, uint32_t token)
            : mGroupId(grpId), mToken(token) {}
	RsGxsGroupId mGroupId;
	uint32_t mToken;
};


class MsgDeletePublish
{
public:
        MsgDeletePublish(const GxsMsgReq& msgs, uint32_t token)
            : mMsgs(msgs), mToken(token) {}
        
	GxsMsgReq mMsgs ;
	uint32_t mToken;
};

#endif /* GXSUTIL_H_ */
