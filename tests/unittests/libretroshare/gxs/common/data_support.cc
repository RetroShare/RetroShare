/*******************************************************************************
 * unittests/libretroshare/gxs/common/data_support.cc                          *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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
 ******************************************************************************/

#include "libretroshare/serialiser/support.h"
#include "data_support.h"

template<class T> void init_random(T& t) { t = T::random() ; }

bool operator==(const RsNxsGrp& l, const RsNxsGrp& r){

    if(l.grpId != r.grpId) return false;
    if(!(l.grp == r.grp) ) return false;
    if(!(l.meta == r.meta) ) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsMsg& l, const RsNxsMsg& r){


    if(l.msgId != r.msgId) return false;
    if(l.grpId != r.grpId) return false;
    if(! (l.msg == r.msg) ) return false;
    if(! (l.meta == r.meta) ) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}


bool operator ==(const RsGxsGrpMetaData& l, const RsGxsGrpMetaData& r)
{
    if(!(l.signSet == r.signSet)) return false;
    if(!(l.keys == r.keys)) return false;
    if(l.mGroupFlags != r.mGroupFlags) return false;
    if(l.mPublishTs != r.mPublishTs) return false;
    if(l.mSignFlags != r.mSignFlags) return false;
    if(l.mAuthorId != r.mAuthorId) return false;
    if(l.mGroupName != r.mGroupName) return false;
    if(l.mGroupId != r.mGroupId) return false;

    return true;
}

bool operator ==(const RsGxsMsgMetaData& l, const RsGxsMsgMetaData& r)
{

    if(!(l.signSet == r.signSet)) return false;
    if(l.mGroupId != r.mGroupId) return false;
    if(l.mAuthorId != r.mAuthorId) return false;
    if(l.mParentId != r.mParentId) return false;
    if(l.mOrigMsgId != r.mOrigMsgId) return false;
    if(l.mThreadId != r.mThreadId) return false;
    if(l.mMsgId != r.mMsgId) return false;
    if(l.mMsgName != r.mMsgName) return false;
    if(l.mPublishTs != r.mPublishTs) return false;
    if(l.mMsgFlags != r.mMsgFlags) return false;

    return true;
}



#if 0
void init_item(RsNxsGrp& nxg)
{

    nxg.clear();

    nxg.grpId.random();
    nxg.transactionNumber = rand()%23;
    init_item(nxg.grp);
    init_item(nxg.meta);
    return;
}

void init_item(RsNxsMsg& nxm)
{
    nxm.clear();

    nxm.msgId.random();
    nxm.grpId.random();
    init_item(nxm.msg);
    init_item(nxm.meta);
    nxm.transactionNumber = rand()%23;

    return;
}
#endif


void init_item(RsGxsGrpMetaData* metaGrp)
{

    init_random(metaGrp->mGroupId);
    init_random(metaGrp->mOrigGrpId);
    init_random(metaGrp->mAuthorId);
    init_random(metaGrp->mCircleId);
    init_random(metaGrp->mParentGrpId);
    randString(SHORT_STR, metaGrp->mGroupName);
    randString(SHORT_STR, metaGrp->mServiceString);

    init_item(metaGrp->signSet);// This is not stored in db.
    init_item(metaGrp->keys);

    metaGrp->mPublishTs = rand()%3452;
    metaGrp->mGroupFlags = rand()%43;
    metaGrp->mSignFlags = rand()%43;
    metaGrp->mAuthenFlags = rand()%43;

    metaGrp->mSubscribeFlags = rand()%2251;
    metaGrp->mPop = rand()%5262;
    metaGrp->mVisibleMsgCount = rand()%2421;
    metaGrp->mLastPost = rand()%2211;
    metaGrp->mReputationCutOff = rand()%5262;

    metaGrp->mGroupStatus = rand()%313;
    metaGrp->mRecvTS = rand()%313;

    metaGrp->mOriginator = RsPeerId::random();
    metaGrp->mInternalCircle = RsGxsCircleId::random();
    metaGrp->mHash = RsFileHash::random();
    metaGrp->mGrpSize = 0;// This was calculated on db read.
}

void init_item(RsGxsMsgMetaData* metaMsg)
{

    init_random(metaMsg->mGroupId) ;
    init_random(metaMsg->mMsgId) ;
    init_random(metaMsg->mThreadId) ;
    init_random(metaMsg->mParentId) ;
    init_random(metaMsg->mOrigMsgId) ;
    init_random(metaMsg->mAuthorId) ;

    init_item(metaMsg->signSet);

    randString(SHORT_STR, metaMsg->mServiceString);

    randString(SHORT_STR, metaMsg->mMsgName);

    metaMsg->mPublishTs = rand()%313;
    metaMsg->mMsgFlags = rand()%224;
    metaMsg->mMsgStatus = rand()%4242;
    metaMsg->mChildTs = rand()%221;
	 metaMsg->recvTS = rand()%2327 ;

	 init_random(metaMsg->mHash) ;
	 metaMsg->validated = true ;
}




void init_item(RsNxsGrp& nxg,RsSerialType **ser)
{
    nxg.clear();

    init_random(nxg.grpId) ;
    nxg.transactionNumber = rand()%23;
    init_item(nxg.grp);
    init_item(nxg.meta);

    if(ser)
    *ser = new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}


void init_item(RsNxsMsg& nxm,RsSerialType **ser)
{
    nxm.clear();

    init_random(nxm.msgId) ;
    init_random(nxm.grpId) ;
    init_item(nxm.msg);
    init_item(nxm.meta);
    nxm.transactionNumber = rand()%23;

    if(ser)
    *ser = new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

void init_item(RsNxsSyncGrpReqItem& rsg,RsSerialType **ser)
{
    rsg.clear();
    rsg.flag = RsNxsSyncGrpItem::FLAG_USE_SYNC_HASH;
    rsg.createdSince = rand()%2423;
    randString(3124,rsg.syncHash);

    if(ser)
    *ser = new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

void init_item(RsNxsSyncMsgReqItem& rsgm,RsSerialType **ser)
{
    rsgm.clear();

    rsgm.flag = RsNxsSyncMsgItem::FLAG_USE_SYNC_HASH;
    rsgm.createdSinceTS = rand()%24232;
    rsgm.transactionNumber = rand()%23;
    init_random(rsgm.grpId) ;
    randString(SHORT_STR, rsgm.syncHash);

    if(ser)
    *ser = new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

void init_item(RsNxsSyncGrpItem& rsgl,RsSerialType **ser)
{
    rsgl.clear();

    rsgl.flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
    rsgl.transactionNumber = rand()%23;
    rsgl.publishTs = rand()%23;
    init_random(rsgl.grpId) ;

    if(ser)
    *ser = new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

void init_item(RsNxsSyncMsgItem& rsgml,RsSerialType **ser)
{
    rsgml.clear();

    rsgml.flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
    rsgml.transactionNumber = rand()%23;
    init_random(rsgml.grpId) ;
    init_random(rsgml.msgId) ;

    if(ser)
		*ser = new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

void init_item(RsNxsTransacItem &rstx,RsSerialType **ser)
{
    rstx.clear();

    rstx.timestamp = rand()%14141;
    rstx.transactFlag = rand()%2424;
    rstx.nItems = rand()%33132;
    rstx.transactionNumber = rand()%242112;

	if(ser)
		*ser = new RsNxsSerialiser(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}


bool operator==(const RsNxsSyncGrpReqItem& l, const RsNxsSyncGrpReqItem& r)
{

    if(l.syncHash != r.syncHash) return false;
    if(l.flag != r.flag) return false;
    if(l.createdSince != r.createdSince) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsSyncMsgReqItem& l, const RsNxsSyncMsgReqItem& r)
{

    if(l.flag != r.flag) return false;
    if(l.createdSinceTS != r.createdSinceTS) return false;
    if(l.syncHash != r.syncHash) return false;
    if(l.grpId != r.grpId) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsSyncGrpItem& l, const RsNxsSyncGrpItem& r)
{
    if(l.flag != r.flag) return false;
    if(l.publishTs != r.publishTs) return false;
    if(l.grpId != r.grpId) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsSyncMsgItem& l, const RsNxsSyncMsgItem& r)
{
    if(l.flag != r.flag) return false;
    if(l.grpId != r.grpId) return false;
    if(l.msgId != r.msgId) return false;
    if(l.transactionNumber != r.transactionNumber) return false;

    return true;
}

bool operator==(const RsNxsTransacItem& l, const RsNxsTransacItem& r){

    if(l.transactFlag != r.transactFlag) return false;
    if(l.transactionNumber != r.transactionNumber) return false;
    // timestamp is not serialised, see rsnxsitems.h
    //if(l.timestamp != r.timestamp) return false;
    if(l.nItems != r.nItems) return false;


    return true;
}

