/*
 * libretroshare/src/services msgservice.h
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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
#include "serialiser/rsmsgitems.h"
#include "util/rsthreads.h"

#include "retroshare/rsgxsifacetypes.h"

#include "grouter/p3grouter.h"
#include "grouter/grouterclientservice.h"
#include "turtle/p3turtle.h"
#include "turtle/turtleclientservice.h"

class p3LinkMgr;
class p3IdService;

// Temp tweak to test grouter
class p3MsgService: public p3Service, public p3Config, public pqiServiceMonitor, public GRouterClientService
{
public:
    p3MsgService(p3ServiceControl *sc, p3IdService *id_service);
    virtual RsServiceInfo getServiceInfo();

    /* External Interface */
    bool 	getMessageSummaries(std::list<Rs::Msgs::MsgInfoSummary> &msgList);
    bool 	getMessage(const std::string &mid, Rs::Msgs::MessageInfo &msg);
    void    getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox);

    bool decryptMessage(const std::string& mid) ;
    bool    removeMsgId(const std::string &mid); 
    bool    markMsgIdRead(const std::string &mid, bool bUnreadByUser);
    bool    setMsgFlag(const std::string &mid, uint32_t flag, uint32_t mask);
    bool    getMsgParentId(const std::string &msgId, std::string &msgParentId);
    // msgParentId == 0 --> remove
    bool    setMsgParentId(uint32_t msgId, uint32_t msgParentId);

    bool    MessageSend(Rs::Msgs::MessageInfo &info);
    bool    SystemMessage(const std::string &title, const std::string &message, uint32_t systemFlag);
    bool    MessageToDraft(Rs::Msgs::MessageInfo &info, const std::string &msgParentId);
    bool    MessageToTrash(const std::string &mid, bool bTrash);

    bool 	getMessageTagTypes(Rs::Msgs::MsgTagType& tags);
    bool  	setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color);
    bool    removeMessageTagType(uint32_t tagId);

    bool 	getMessageTag(const std::string &msgId, Rs::Msgs::MsgTagInfo& info);
    /* set == false && tagId == 0 --> remove all */
    bool 	setMessageTag(const std::string &msgId, uint32_t tagId, bool set);

    bool    resetMessageStandardTagTypes(Rs::Msgs::MsgTagType& tags);

    void    loadWelcomeMsg(); /* startup message */


    //std::list<RsMsgItem *> &getMsgList();
    //std::list<RsMsgItem *> &getMsgOutList();

    int	tick();
    int	status();

    /*** Overloaded from p3Config ****/
    virtual RsSerialiser *setupSerialiser();
    virtual bool saveList(bool& cleanup, std::list<RsItem*>&);
    virtual bool loadList(std::list<RsItem*>& load);
    virtual void saveDone();
    /*** Overloaded from p3Config ****/

    /*** Overloaded from pqiMonitor ***/
    virtual void    statusChange(const std::list<pqiServicePeer> &plist);
    int     checkOutgoingMessages();
    /*** Overloaded from pqiMonitor ***/

    /*** overloaded from p3turtle   ***/

    virtual void connectToGlobalRouter(p3GRouter *) ;

    struct DistantMessengingInvite
    {
	    time_t time_of_validity ;
    };
    struct DistantMessengingContact
    {
	    time_t last_hit_time ;
	    RsPeerId virtual_peer_id ;
	    uint32_t status ;
	    bool pending_messages ;
    };
    void enableDistantMessaging(bool b) ;
    bool distantMessagingEnabled() ;

    void setDistantMessagingPermissionFlags(uint32_t flags) ;
    uint32_t getDistantMessagingPermissionFlags() ;

private:
    void sendDistantMsgItem(RsMsgItem *msgitem) ;

    // This contains the ongoing tunnel handling contacts.
    // The map is indexed by the hash
    //
    std::map<GRouterMsgPropagationId,uint32_t> _ongoing_messages ;

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
    std::map<Sha1CheckSum,uint32_t> mRecentlyReceivedDistantMessageHashes;

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
};

#endif // MESSAGE_SERVICE_HEADER
