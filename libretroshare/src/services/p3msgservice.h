/*******************************************************************************
 * libretroshare/src/services: p3msgservice.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 Robert Fernie <retroshare@lunamutt.com>                 *
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
#ifndef MESSAGE_SERVICE_HEADER
#define MESSAGE_SERVICE_HEADER

#include <list>
#include <map>
#include <iostream>

#include "retroshare/rsmsgs.h"

#include "pqi/pqi.h"
#include "pqi/pqiindic.h"

#include "pqi/pqiservicemonitor.h"
#include "pqi/p3cfgmgr.h"

#include "services/p3service.h"
#include "rsitems/rsmsgitems.h"
#include "util/rsthreads.h"
#include "util/rsdebug.h"
#include "retroshare/rsgxsifacetypes.h"

#include "grouter/p3grouter.h"
#include "grouter/grouterclientservice.h"
#include "turtle/p3turtle.h"
#include "turtle/turtleclientservice.h"
#include "gxstrans/p3gxstrans.h"

class p3LinkMgr;
class p3IdService;

// Temp tweak to test grouter
class p3MsgService :
        public p3Service, public p3Config, public pqiServiceMonitor, GRouterClientService,
        GxsTransClient
{
public:
	p3MsgService(p3ServiceControl *sc, p3IdService *id_service, p3GxsTrans& gxsMS);

	virtual RsServiceInfo getServiceInfo();

	/// @see RsMsgs::sendMail
	uint32_t sendMail(const RsGxsId from,
	        const std::string& subject,
	        const std::string& body,
	        const std::set<RsGxsId>& to = std::set<RsGxsId>(),
	        const std::set<RsGxsId>& cc = std::set<RsGxsId>(),
	        const std::set<RsGxsId>& bcc = std::set<RsGxsId>(),
	        const std::vector<FileInfo>& attachments = std::vector<FileInfo>(),
	        std::set<RsMailIdRecipientIdPair>& trackingIds =
	            RS_DEFAULT_STORAGE_PARAM(std::set<RsMailIdRecipientIdPair>),
	        std::string& errorMsg =
	            RS_DEFAULT_STORAGE_PARAM(std::string) );

    /* External Interface */
    bool 	getMessageSummaries(std::list<Rs::Msgs::MsgInfoSummary> &msgList);
    bool 	getMessage(const std::string &mid, Rs::Msgs::MessageInfo &msg);
	void	getMessageCount(uint32_t &nInbox, uint32_t &nInboxNew, uint32_t &nOutbox, uint32_t &nDraftbox, uint32_t &nSentbox, uint32_t &nTrashbox);

    bool decryptMessage(const std::string& mid) ;
    bool    removeMsgId(const std::string &mid); 
    bool    markMsgIdRead(const std::string &mid, bool bUnreadByUser);
    bool    setMsgFlag(const std::string &mid, uint32_t flag, uint32_t mask);
    bool    getMsgParentId(const std::string &msgId, std::string &msgParentId);
    // msgParentId == 0 --> remove
    bool    setMsgParentId(uint32_t msgId, uint32_t msgParentId);

	RS_DEPRECATED_FOR(sendMail)
    bool    MessageSend(Rs::Msgs::MessageInfo &info);
    bool    SystemMessage(const std::string &title, const std::string &message, uint32_t systemFlag);
    bool    MessageToDraft(Rs::Msgs::MessageInfo &info, const std::string &msgParentId);
    bool    MessageToTrash(const std::string &mid, bool bTrash);

    bool 	getMessageTag(const std::string &msgId, Rs::Msgs::MsgTagInfo& info);
    bool 	getMessageTagTypes(Rs::Msgs::MsgTagType& tags);
    bool  	setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color);
    bool    removeMessageTagType(uint32_t tagId);

    /* set == false && tagId == 0 --> remove all */
    bool 	setMessageTag(const std::string &msgId, uint32_t tagId, bool set);

    bool    resetMessageStandardTagTypes(Rs::Msgs::MsgTagType& tags);

    void    loadWelcomeMsg(); /* startup message */


    //std::list<RsMsgItem *> &getMsgList();
    //std::list<RsMsgItem *> &getMsgOutList();

    int	tick();

    /*** Overloaded from p3Config ****/
    virtual RsSerialiser *setupSerialiser();
    virtual bool saveList(bool& cleanup, std::list<RsItem*>&);
    virtual bool loadList(std::list<RsItem*>& load);
    virtual void saveDone();
    /*** Overloaded from p3Config ****/

    /*** Overloaded from pqiMonitor ***/
    virtual void    statusChange(const std::list<pqiServicePeer> &plist);

	/// iterate through the outgoing queue if online, send
	int checkOutgoingMessages();
    /*** Overloaded from pqiMonitor ***/

    /*** overloaded from p3turtle   ***/

    virtual void connectToGlobalRouter(p3GRouter *) ;

    struct DistantMessengingInvite
    {
	    rstime_t time_of_validity ;
    };
    struct DistantMessengingContact
    {
	    rstime_t last_hit_time ;
	    RsPeerId virtual_peer_id ;
	    uint32_t status ;
	    bool pending_messages ;
    };
    void enableDistantMessaging(bool b) ;
    bool distantMessagingEnabled() ;

    void setDistantMessagingPermissionFlags(uint32_t flags) ;
    uint32_t getDistantMessagingPermissionFlags() ;

	/// @see GxsTransClient::receiveGxsTransMail(...)
	virtual bool receiveGxsTransMail( const RsGxsId& authorId,
	                                  const RsGxsId& recipientId,
	                                  const uint8_t* data, uint32_t dataSize );

	/// @see GxsTransClient::notifyGxsTransSendStatus(...)
	virtual bool notifyGxsTransSendStatus( RsGxsTransId mailId,
	                                       GxsTransSendStatus status );

private:
	void sendDistantMsgItem(RsMsgItem *msgitem);
    bool locked_getMessageTag(const std::string &msgId, Rs::Msgs::MsgTagInfo& info);

	/** This contains the ongoing tunnel handling contacts.
	 * The map is indexed by the hash */
	std::map<GRouterMsgPropagationId, uint32_t> _ongoing_messages;

	/// Contains ongoing messages handed to gxs mail
	std::map<RsGxsTransId, uint32_t> gxsOngoingMessages;
	RsMutex gxsOngoingMutex;

    // Overloaded from GRouterClientService
    virtual bool acceptDataFromPeer(const RsGxsId& gxs_id) ;
    virtual void receiveGRouterData(const RsGxsId& destination_key,const RsGxsId& signing_key, GRouterServiceId &client_id, uint8_t *data, uint32_t data_size) ;
    virtual void notifyDataStatus(const GRouterMsgPropagationId& msg_id,const RsGxsId& signer_id,uint32_t data_status) ;

    // Utility functions

    bool createDistantMessage(const RsGxsId& destination_gxs_id,const RsGxsId& source_gxs_id,RsMsgItem *msg) ;
    bool locked_findHashForVirtualPeerId(const RsPeerId& pid,Sha1CheckSum& hash) ;
    void sendGRouterData(const RsGxsId &key_id,RsMsgItem *) ;

    void manageDistantPeers() ;

    void handleIncomingItem(RsMsgItem *) ;

    uint32_t getNewUniqueMsgId();
    uint32_t sendMessage(RsMsgItem *item);
    uint32_t sendDistantMessage(RsMsgItem *item,const RsGxsId& signing_gxs_id);
    void    checkSizeAndSendMessage(RsMsgItem *msg);
    void cleanListOfReceivedMessageHashes();

    int 	incomingMsgs();
    void    processIncomingMsg(RsMsgItem *mi) ;
    bool checkAndRebuildPartialMessage(RsMsgItem*) ;

    void 	initRsMI(RsMsgItem *msg, Rs::Msgs::MessageInfo &mi);
    void 	initRsMIS(RsMsgItem *msg, Rs::Msgs::MsgInfoSummary &mis);

    RsMsgItem *initMIRsMsg(const Rs::Msgs::MessageInfo &info, const RsPeerId& to);
    RsMsgItem *initMIRsMsg(const Rs::Msgs::MessageInfo &info, const RsGxsId& to);
    void initMIRsMsg(RsMsgItem *item,const Rs::Msgs::MessageInfo &info) ;

    void    initStandardTagTypes();

    p3IdService *mIdService ;
    p3ServiceControl *mServiceCtrl;
    p3GRouter *mGRouter ;

    /* Mutex Required for stuff below */

    RsMutex mMsgMtx;
    RsMsgSerialiser *_serialiser ;

    /* stored list of messages */
    std::map<uint32_t, RsMsgItem *> imsg;
    /* ones that haven't made it out yet! */
    std::map<uint32_t, RsMsgItem *> msgOutgoing; 

    std::map<RsPeerId, RsMsgItem *> _pendingPartialMessages ;

    /* maps for tags types and msg tags */

    std::map<uint32_t, RsMsgTagType*> mTags;
    std::map<uint32_t, RsMsgTags*> mMsgTags;

	uint32_t mMsgUniqueId;
	std::map<Sha1CheckSum, uint32_t> mRecentlyReceivedMessageHashes;
	RsMutex recentlyReceivedMutex;

    // used delete msgSrcIds after config save
    std::map<uint32_t, RsMsgSrcId*> mSrcIds;

    // temporary storage. Will not be needed when messages have a proper "from" field. Not saved!
    std::map<uint32_t, RsGxsId> mDistantOutgoingMsgSigners;

    // save the parent of the messages in draft for replied and forwarded
    std::map<uint32_t, RsMsgParentId*> mParentId;

    std::string config_dir;

    bool mDistantMessagingEnabled ;
    uint32_t mDistantMessagePermissions ;
    bool mShouldEnableDistantMessaging ;

	p3GxsTrans& mGxsTransServ;

	RS_SET_CONTEXT_DEBUG_LEVEL(3)
};

#endif // MESSAGE_SERVICE_HEADER
