/*******************************************************************************
 * libretroshare/src/gxs: gxsdataservice.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2012 by Evi-Parker Christopher                               *
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
#ifndef RSDATASERVICE_H
#define RSDATASERVICE_H

#include "gxs/rsgds.h"
#include "util/retrodb.h"

class MsgUpdate
{
public:

    //MsgUpdate(){}
    //MsgUpdate(const MsgUpdate& ){}//hier müsste ein echter constructor sein
	RsGxsMessageId msgId;
	ContentValue cv;
};

class RsDataService : public RsGeneralDataService
{
public:

    RsDataService(const std::string& serviceDir, const std::string& dbName, uint16_t serviceType,
    		RsGxsSearchModule* mod = NULL, const std::string& key = "");
    virtual ~RsDataService();

    /*!
     * Retrieves all msgs
     * @param reqIds requested msg ids (grpId,msgId), leave msg list empty to get all msgs for the grp
     * @param msg result of msg retrieval
	 * @param cache IGNORED whether to store results of this retrieval in memory
	 *	for faster later retrieval
	 * @param strictFilter if true do not request any message if reqIds is empty
     * @return error code
	 */
	int retrieveNxsMsgs(
	        const GxsMsgReq& reqIds, GxsMsgResult& msg, bool cache,
	        bool withMeta = false );

    /*!
     * Retrieves groups, if empty, retrieves all grps, if map is not empty
     * only retrieve entries, if entry cannot be found, it is removed from map
     * @param grp retrieved groups
     * @param withMeta this initialise the metaData member of the nxsgroups retrieved
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    int retrieveNxsGrps(std::map<RsGxsGroupId, RsNxsGrp*>& grp, bool withMeta, bool cache);

    /*!
     * Retrieves meta data of all groups stored (most current versions only)
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    int retrieveGxsGrpMetaData(RsGxsGrpMetaTemporaryMap& grp);

    /*!
     * Retrieves meta data of all groups stored (most current versions only)
     * @param grpIds grpIds for which to retrieve meta data
     * @param msgMeta meta data result as map of grpIds to array of metadata for that grpId
     * @param cache whether to store retrieval in mem for faster later retrieval
     * @return error code
     */
    int retrieveGxsMsgMetaData(const GxsMsgReq& reqIds, GxsMsgMetaResult& msgMeta);

    /*!
     * remove msgs in data store
     * @param grpId group Id of message to be removed
     * @param msgIds ids of messages to be removed
     * @return error code
     */
    int removeMsgs(const GxsMsgReq& msgIds);

    /*!
     * remove groups in data store listed in grpIds param
     * @param grpIds ids of groups to be removed
     * @return error code
     */
    int removeGroups(const std::vector<RsGxsGroupId>& grpIds);

    /*!
     * Retrieves all group ids in store
     * @param grpIds all grpids in store is inserted into this vector
     * @return error code
     */
    int retrieveGroupIds(std::vector<RsGxsGroupId> &grpIds);

    /*!
     * Retrives all msg ids in store
     * @param grpId groupId of message ids to retrieve
     * @param msgId msgsids retrieved
     * @return error code
     */
    int retrieveMsgIds(const RsGxsGroupId& grpId, RsGxsMessageId::std_set& msgId);

    /*!
     * @return the cache size set for this RsGeneralDataService in bytes
     */
    uint32_t cacheSize() const;

    /*!
     * @param size size of cache to set in bytes
     */
    int setCacheSize(uint32_t size);

    /*!
     * Stores a list of signed messages into data store
     * @param msg map of message and decoded meta data information
     * @return error code
     */
    int storeMessage(const std::list<RsNxsMsg*>& msg);

    /*!
     * Stores a list of groups in data store
     * @param grp map of group and decoded meta data
     * @return error code
     */
    int storeGroup(const std::list<RsNxsGrp*>& grp);

    /*!
	 * Updates group entries in Db
	 * @param grp map of group and decoded meta data
	 * @return error code
	 */
    int updateGroup(const std::list<RsNxsGrp*>& grsp);

    /*!
     * @param metaData The meta data item to update
     * @return error code
     */
    int updateMessageMetaData(MsgLocMetaData& metaData);

    /*!
     * @param metaData The meta data item to update
     * @return error code
     */
    int updateGroupMetaData(GrpLocMetaData& meta);

    /*!
     * Completely clear out data stored in
     * and returns this to a state
     * as it was when first constructed
     * @return error code
     */
    int resetDataStore();

    bool validSize(RsNxsMsg* msg) const;
    bool validSize(RsNxsGrp* grp) const;

    /*!
     * Convenience function used to only update group keys. This is used when sending
     * publish keys between peers.
     * @return SQL error code
     */

    int updateGroupKeys(const RsGxsGroupId& grpId,const RsTlvSecurityKeySet& keys, uint32_t subscribe_flags) ;

private:

    /*!
     * Retrieves all the msg results from a cursor
     * @param c cursor to result set
     * @param msgs messages retrieved from cursor are stored here
     */
    void locked_retrieveMessages(RetroCursor* c, std::vector<RsNxsMsg*>& msgs, int metaOffset);

    /*!
     * Retrieves all the grp results from a cursor
     * @param c cursor to result set
     * @param grps groups retrieved from cursor are stored here
     * @param withMeta this initialise the metaData member of the nxsgroups retrieved
     */
    void locked_retrieveGroups(RetroCursor* c, std::vector<RsNxsGrp*>& grps, int metaOffset);

    /*!
     * Retrieves all the msg meta results from a cursor
     * @param c cursor to result set
     * @param metaSet message metadata retrieved from cursor are stored here
     */
    void locked_retrieveMsgMeta(RetroCursor* c, std::vector<RsGxsMsgMetaData*>& msgMeta);

    /*!
     * extracts a msg meta item from a cursor at its
     * current position
     */
    RsGxsMsgMetaData* locked_getMsgMeta(RetroCursor& c, int colOffset);

    /*!
     * extracts a grp meta item from a cursor at its
     * current position
     */
    RsGxsGrpMetaData* locked_getGrpMeta(RetroCursor& c, int colOffset, bool use_cache);

    /*!
     * extracts a msg item from a cursor at its
     * current position
     */
    RsNxsMsg* locked_getMessage(RetroCursor& c);

    /*!
     * extracts a grp item from a cursor at its
     * current position
     */
    RsNxsGrp* locked_getGroup(RetroCursor& c);

    /*!
     * Creates an sql database and its associated file
     * also creates the message and groups table
     * @param isNewDatabase is new database
     */
    void initialise(bool isNewDatabase);

    /*!
     * Remove entries for data base
     * @param msgIds
     */
    bool locked_removeMessageEntries(const GxsMsgReq& msgIds);
    bool locked_removeGroupEntries(const std::vector<RsGxsGroupId>& grpIds);

private:
    /*!
     * Start release update
     * @param release
     * @return true/false
     */
    bool startReleaseUpdate(int release);

    /*!
     * Finish release update
     * @param release
     * @param result
     * @return true/false
     */
    bool finishReleaseUpdate(int release, bool result);

private:

    RsMutex mDbMutex;

    std::list<std::string> mMsgColumns;
    std::list<std::string> mMsgMetaColumns;
    std::list<std::string> mMsgColumnsWithMeta;
    std::list<std::string> mMsgIdColumn;

    std::list<std::string> mGrpColumns;
    std::list<std::string> mGrpMetaColumns;
    std::list<std::string> mGrpColumnsWithMeta;
    std::list<std::string> mGrpIdColumn;

    // Message meta column
    int mColMsgMeta_GrpId;
    int mColMsgMeta_TimeStamp;
    int mColMsgMeta_NxsFlags;
    int mColMsgMeta_SignSet;
    int mColMsgMeta_NxsIdentity;
    int mColMsgMeta_NxsHash;
    int mColMsgMeta_MsgId;
    int mColMsgMeta_OrigMsgId;
    int mColMsgMeta_MsgStatus;
    int mColMsgMeta_ChildTs;
    int mColMsgMeta_MsgParentId;
    int mColMsgMeta_MsgThreadId;
    int mColMsgMeta_Name;
    int mColMsgMeta_NxsServString;
    int mColMsgMeta_RecvTs;
    int mColMsgMeta_NxsDataLen;

    // Message columns
    int mColMsg_GrpId;
    int mColMsg_NxsData;
    int mColMsg_MetaData;
    int mColMsg_MsgId;

    // Message columns with meta
    int mColMsg_WithMetaOffset;

    // Group meta columns
    int mColGrpMeta_GrpId;
    int mColGrpMeta_TimeStamp;
    int mColGrpMeta_NxsFlags;
//    int mColGrpMeta_SignSet;
    int mColGrpMeta_NxsIdentity;
    int mColGrpMeta_NxsHash;
    int mColGrpMeta_KeySet;
    int mColGrpMeta_SubscrFlag;
    int mColGrpMeta_Pop;
    int mColGrpMeta_MsgCount;
    int mColGrpMeta_Status;
    int mColGrpMeta_Name;
    int mColGrpMeta_LastPost;
    int mColGrpMeta_OrigGrpId;
    int mColGrpMeta_ServString;
    int mColGrpMeta_SignFlags;
    int mColGrpMeta_CircleId;
    int mColGrpMeta_CircleType;
    int mColGrpMeta_InternCircle;
    int mColGrpMeta_Originator;
    int mColGrpMeta_AuthenFlags;
    int mColGrpMeta_ParentGrpId;
    int mColGrpMeta_RecvTs;
    int mColGrpMeta_RepCutoff;
    int mColGrpMeta_NxsDataLen;

    // Group columns
    int mColGrp_GrpId;
    int mColGrp_NxsData;
    int mColGrp_MetaData;

    // Group columns with meta
    int mColGrp_WithMetaOffset;

    // Group id columns
    int mColGrpId_GrpId;

    // Msg id columns
    int mColMsgId_MsgId;

    std::string mServiceDir;
    std::string mDbName;
    std::string mDbPath;
    uint16_t mServType;

    RetroDb* mDb;
    
    // used to store metadata instead of reading it from the database.
    // The boolean variable below is also used to force re-reading when 
    // the entre list of grp metadata is requested (which happens quite often)
    
    void locked_clearGrpMetaCache(const RsGxsGroupId& gid);
	void locked_updateGrpMetaCache(const RsGxsGrpMetaData& meta);

    std::map<RsGxsGroupId,RsGxsGrpMetaData*> mGrpMetaDataCache ;
	std::list<std::pair<rstime_t,RsGxsGrpMetaData*> > mOldCachedItems ;

    bool mGrpMetaDataCache_ContainsAllDatabase ;
};

#endif // RSDATASERVICE_H
