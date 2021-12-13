/*******************************************************************************
 * libretroshare/src/gxs: rsgxsutil.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2013 Christopher Evi-Parker                                   *
 * Copyright (C) 2018-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
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

#pragma once

#include <vector>
#include "rsitems/rsnxsitems.h"
#include "rsgds.h"

class RsGixs ;
class RsGenExchange ;
class RsGeneralDataService ;

// temporary holds a map of pointers to class T, and destroys all pointers on delete.

class non_copiable
{
public:
    non_copiable() {}
private:
    non_copiable& operator=(const non_copiable&) { return *this ;}
    non_copiable(const non_copiable&) {}
};

template<class IdClass,class IdData>
class t_RsGxsGenericDataTemporaryMap: public std::map<IdClass,IdData *>, public non_copiable
{
public:
    virtual ~t_RsGxsGenericDataTemporaryMap()
    {
        clear() ;
    }

    virtual void clear()
    {
        for(typename t_RsGxsGenericDataTemporaryMap<IdClass,IdData>::iterator it = this->begin();it!=this->end();++it)
            if(it->second != NULL)
            delete it->second ;

        std::map<IdClass,IdData*>::clear() ;
    }
};

template<class T>
class t_RsGxsGenericDataTemporaryMapVector: public std::map<RsGxsGroupId,std::vector<T*> >, public non_copiable
{
public:
    virtual ~t_RsGxsGenericDataTemporaryMapVector()
    {
        clear() ;
    }

    virtual void clear()
    {
        for(typename t_RsGxsGenericDataTemporaryMapVector<T>::iterator it = this->begin();it!=this->end();++it)
        {
            for(uint32_t i=0;i<it->second.size();++i)
                delete it->second[i] ;

            it->second.clear();
        }

        std::map<RsGxsGroupId,std::vector<T*> >::clear() ;
    }
};

template<class T>
class t_RsGxsGenericDataTemporaryList: public std::list<T*>, public non_copiable
{
public:
    virtual ~t_RsGxsGenericDataTemporaryList()
    {
        clear() ;
    }

    virtual void clear()
    {
        for(typename t_RsGxsGenericDataTemporaryList<T>::iterator it = this->begin();it!=this->end();++it)
            delete *it;

        std::list<T*>::clear() ;
    }
};

typedef std::map<RsGxsGroupId,            std::shared_ptr<RsGxsGrpMetaData> >   RsGxsGrpMetaTemporaryMap; // This map doesn't need to delete elements since it holds
typedef std::map<RsGxsGroupId,std::vector<std::shared_ptr<RsGxsMsgMetaData> > > RsGxsMsgMetaTemporaryMap; // shared_ptr's.

typedef t_RsGxsGenericDataTemporaryMap<RsGxsGroupId,RsNxsGrp>         RsNxsGrpDataTemporaryMap;
typedef t_RsGxsGenericDataTemporaryMapVector<RsNxsMsg>                RsNxsMsgDataTemporaryMap ;

typedef t_RsGxsGenericDataTemporaryList<RsNxsGrp>                     RsNxsGrpDataTemporaryList ;
typedef t_RsGxsGenericDataTemporaryList<RsNxsMsg>                     RsNxsMsgDataTemporaryList ;

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
class RsGxsCleanUp
{
public:

    /*!
     *
     * @param dataService
     * @param mGroupTS
     * @param chunkSize
     * @param sleepPeriod
     */
    RsGxsCleanUp(RsGeneralDataService* const dataService, RsGenExchange *genex, uint32_t chunkSize);

    /*!
     * On construction this should be called to progress deletions
     * Deletion will process by chunk size
     * @return true if no more messages to delete, false otherwise
     */
    bool clean(RsGxsGroupId& next_group_to_check,std::vector<RsGxsGroupId>& grps_to_delete,GxsMsgReq& messages_to_delete);

private:

    RsGeneralDataService* const mDs;
    RsGenExchange *mGenExchangeClient;
    uint32_t CHUNK_SIZE;
};

/*!
 * Checks the integrity message and groups
 * in rsDataService using computed hash
 */
class RsGxsIntegrityCheck : public RsThread
{

    enum CheckState { CheckStart, CheckChecking };

public:
	RsGxsIntegrityCheck( RsGeneralDataService* const dataService,
	                     RsGenExchange* genex, RsSerialType&,
	                     RsGixs* gixs );

    static bool check(uint16_t service_type, RsGixs *mgixs, RsGeneralDataService *mds);
    bool isDone();

    void run();

    void getDeletedIds(std::vector<RsGxsGroupId> &grpIds, GxsMsgReq &msgIds);

private:

    RsGeneralDataService* const mDs;
    RsGenExchange *mGenExchangeClient;
    bool mDone;
    RsMutex mIntegrityMutex;
    std::vector<RsGxsGroupId> mDeletedGrps;
    GxsMsgReq mDeletedMsgs;

    RsGixs* mGixs;
};

/*!
 * Checks the integrity message and groups
 * in rsDataService using computed hash
 */
class RsGxsSinglePassIntegrityCheck
{
public:
	static bool check(
	        uint16_t service_type, RsGixs* mgixs, RsGeneralDataService* mds,
	        std::vector<RsGxsGroupId>& grpsToDel, GxsMsgReq& msgsToDel );
};

class GroupUpdate
{
public:
    GroupUpdate() : oldGrpMeta(NULL), newGrp(NULL), validUpdate(false)
    {}
    const RsGxsGrpMetaData* oldGrpMeta;
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
