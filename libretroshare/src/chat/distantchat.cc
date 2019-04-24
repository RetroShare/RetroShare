/*******************************************************************************
 * libretroshare/src/chat: distantchat.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include <unistd.h>
#include <sstream>

#include "openssl/rand.h"
#include "openssl/dh.h"
#include "openssl/err.h"

#include "crypto/rsaes.h"
#include "util/rsmemory.h"
#include "util/rsprint.h"

#include "rsitems/rsmsgitems.h"

#include "retroshare/rsmsgs.h"
#include "retroshare/rsidentity.h"
#include "retroshare/rsiface.h"

#include "rsserver/p3face.h"
#include "services/p3idservice.h"
#include "gxs/gxssecurity.h"
#include "turtle/p3turtle.h"
#include "retroshare/rsids.h"
#include "distantchat.h"

//#define DEBUG_DISTANT_CHAT

#ifdef DEBUG_DISTANT_CHAT

#include <sys/time.h>

uint32_t msecs_of_day()
{
	timeval tv ;
	gettimeofday(&tv,NULL) ;
	return tv.tv_usec / 1000 ;
}
#define DISTANT_CHAT_DEBUG() std::cerr << time(NULL) << "." << std::setfill('0') << std::setw(3) << msecs_of_day() << " : DISTANT_CHAT : " << __FUNCTION__ << " : "
#endif

//static const uint32_t DISTANT_CHAT_KEEP_ALIVE_TIMEOUT = 6 ; // send keep alive packet so as to avoid tunnel breaks.

//static const uint32_t RS_DISTANT_CHAT_DH_STATUS_UNINITIALIZED = 0x0000 ;
//static const uint32_t RS_DISTANT_CHAT_DH_STATUS_HALF_KEY_DONE = 0x0001 ;
//static const uint32_t RS_DISTANT_CHAT_DH_STATUS_KEY_AVAILABLE = 0x0002 ;

static const uint32_t DISTANT_CHAT_GXS_TUNNEL_SERVICE_ID = 0xa0001 ;

DistantChatService::DistantChatService() :
    // default: accept everyone
    mDistantChatPermissions(RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_NONE),
    mGxsTunnels(nullptr), mDistantChatMtx("distant chat") {}

void DistantChatService::connectToGxsTunnelService(RsGxsTunnelService *tr)
{
	mGxsTunnels = tr ;
	tr->registerClientService(DISTANT_CHAT_GXS_TUNNEL_SERVICE_ID,this) ;
}

bool DistantChatService::handleOutgoingItem(RsChatItem *item)
{
    RsGxsTunnelId tunnel_id ;
    
    {
        RS_STACK_MUTEX(mDistantChatMtx) ;

        std::map<DistantChatPeerId,DistantChatContact>::const_iterator it=mDistantChatContacts.find(DistantChatPeerId(item->PeerId()));

        if(it == mDistantChatContacts.end())
            return false ;
    }

#ifdef CHAT_DEBUG
    DISTANT_CHAT_DEBUG() << "p3ChatService::handleOutgoingItem(): sending to " << item->PeerId() << ": interpreted as a distant chat virtual peer id." << std::endl;
#endif
    
    uint32_t size = RsChatSerialiser().size(item) ;
    RsTemporaryMemory mem(size) ;
    
    if(!RsChatSerialiser().serialise(item,mem,&size))
    {
        std::cerr << "(EE) serialisation error. Something's really wrong!" << std::endl;
        return false;
    }
#ifdef DEBUG_DISTANT_CHAT    
    DISTANT_CHAT_DEBUG() << "  sending: " << RsUtil::BinToHex(mem,size,100) << std::endl;
    DISTANT_CHAT_DEBUG() << "  size: " << std::dec << size << std::endl;
	DISTANT_CHAT_DEBUG() << "  hash: " << RsDirUtil::sha1sum(mem,size) << std::endl;
#endif
    
    mGxsTunnels->sendData( RsGxsTunnelId(item->PeerId()),DISTANT_CHAT_GXS_TUNNEL_SERVICE_ID,mem,size);
    return true;
}

void DistantChatService::handleRecvChatStatusItem(RsChatStatusItem *cs)
{
    if(cs->flags & RS_CHAT_FLAG_CONNEXION_REFUSED)
    {
#ifdef DEBUG_DISTANT_CHAT    
        DISTANT_CHAT_DEBUG() << "(II) Distant chat: received notification that peer refuses conversation." << std::endl;
#endif
        RsServer::notify()->notifyChatStatus(ChatId(DistantChatPeerId(cs->PeerId())),"Connexion refused by distant peer!") ;
    }

    if(cs->flags & RS_CHAT_FLAG_CLOSING_DISTANT_CONNECTION)
        markDistantChatAsClosed(DistantChatPeerId(cs->PeerId())) ;

    // nothing more to do, because the decryption routing will update the last_contact time when decrypting.

    if(cs->flags & RS_CHAT_FLAG_KEEP_ALIVE)
        std::cerr << "DistantChatService::handleRecvChatStatusItem(): received keep alive packet for inactive chat! peerId=" << cs->PeerId() << std::endl;
}

bool DistantChatService::acceptDataFromPeer(const RsGxsId& gxs_id,const RsGxsTunnelId& tunnel_id,bool am_I_client_side)
{
    if(am_I_client_side)	// always accept distant chat when we're the client side.
        return true ;
    
    bool res = true ;

    if(mDistantChatPermissions & RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_NON_CONTACTS)
        res = (rsIdentity!=NULL) && rsIdentity->isARegularContact(gxs_id) ;
    
    if(mDistantChatPermissions & RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_EVERYBODY)
        res = false ;
    
    if(!res)
    {
#ifdef DEBUG_DISTANT_CHAT    
	    DISTANT_CHAT_DEBUG() << "(II) refusing distant chat from peer " << gxs_id << ". Sending a notification back to tunnel " << tunnel_id << std::endl;
#endif
	    RsChatStatusItem *item = new RsChatStatusItem ;
	    item->flags = RS_CHAT_FLAG_CONNEXION_REFUSED ;
	    item->status_string.clear() ; // is not used yet! But could be set in GUI to some message (??).
	    item->PeerId(RsPeerId(tunnel_id)) ;

            // we do not use handleOutGoingItem() because there's no distant chat contact, as the chat is refused.
            
	    uint32_t size = RsChatSerialiser().size(item) ;
	    RsTemporaryMemory mem(size) ;

	    if(!RsChatSerialiser().serialise(item,mem,&size))
	    {
		    std::cerr << "(EE) serialisation error. Something's really wrong!" << std::endl;
		    return false;
	    }

#ifdef DEBUG_DISTANT_CHAT
	    DISTANT_CHAT_DEBUG() << "  sending: " << RsUtil::BinToHex(mem,size,100) << std::endl;
		DISTANT_CHAT_DEBUG() << "  size: " << std::dec << std::endl;
		DISTANT_CHAT_DEBUG() << "  hash: " << RsDirUtil::sha1sum(mem,size) << std::endl;
#endif

	    mGxsTunnels->sendData( RsGxsTunnelId(item->PeerId()),DISTANT_CHAT_GXS_TUNNEL_SERVICE_ID,mem,size);
    }
    
    return res ;
}

void DistantChatService::notifyTunnelStatus(
        const RsGxsTunnelId& tunnel_id, uint32_t tunnel_status )
{
#ifdef DEBUG_DISTANT_CHAT    
    DISTANT_CHAT_DEBUG() << "DistantChatService::notifyTunnelStatus(): got notification " << std::hex << tunnel_status << std::dec << " for tunnel " << tunnel_id << std::endl;
#endif
    
    switch(tunnel_status)
    {
    default:
    case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_UNKNOWN: 		std::cerr << "(EE) don't know how to handle RS_GXS_TUNNEL_STATUS_UNKNOWN !" << std::endl;
        								break ;
        
    case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_CAN_TALK:    	RsServer::notify()->notifyChatStatus(ChatId(DistantChatPeerId(tunnel_id)),"Tunnel is secured. You can talk!") ;
        								RsServer::notify()->notifyPeerStatusChanged(tunnel_id.toStdString(),RS_STATUS_ONLINE) ;
                            						break ;
                            
    case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_TUNNEL_DN:    	RsServer::notify()->notifyChatStatus(ChatId(DistantChatPeerId(tunnel_id)),"tunnel is down...") ;
			        					RsServer::notify()->notifyPeerStatusChanged(tunnel_id.toStdString(),RS_STATUS_OFFLINE) ;
        								break ;
        
    case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_REMOTELY_CLOSED:	RsServer::notify()->notifyChatStatus(ChatId(DistantChatPeerId(tunnel_id)),"tunnel is down...") ;
        								RsServer::notify()->notifyPeerStatusChanged(tunnel_id.toStdString(),RS_STATUS_OFFLINE) ;
                            						break ;
    }
}

void DistantChatService::receiveData(
        const RsGxsTunnelId& tunnel_id, unsigned char* data, uint32_t data_size)
{
#ifdef DEBUG_DISTANT_CHAT    
    DISTANT_CHAT_DEBUG() << "DistantChatService::receiveData(): got data of size " << std::dec << data_size << " for tunnel " << tunnel_id << std::endl;
    DISTANT_CHAT_DEBUG() << "  received: " << RsUtil::BinToHex(data,data_size,100) << std::endl;
    DISTANT_CHAT_DEBUG() << "  hash: " << RsDirUtil::sha1sum(data,data_size) << std::endl;
    DISTANT_CHAT_DEBUG() << "  deserialising..." << std::endl;
#endif

    // always make the contact up to date. This is useful for server side, which doesn't know about the chat until it
    // receives the first item.
    {
	    RS_STACK_MUTEX(mDistantChatMtx) ;

	    RsGxsTunnelService::GxsTunnelInfo tinfo;
	    if(!mGxsTunnels->getTunnelInfo(tunnel_id,tinfo))
		    return ;

	    // Check if the data is accepted. We cannot prevent the creation of tunnels at the level of p3GxsTunnels, since tunnels are shared between services.
	    // however, 

	    DistantChatContact& contact(mDistantChatContacts[DistantChatPeerId(tunnel_id)]) ;

	    contact.to_id = tinfo.destination_gxs_id ;
	    contact.from_id = tinfo.source_gxs_id ;
    }

    RsItem *item = RsChatSerialiser().deserialise(data,&data_size) ;

    if(item != NULL)
    {
	    item->PeerId(RsPeerId(tunnel_id)) ;	// just in case, but normally this is already done.

	    handleIncomingItem(item) ;
	    RsServer::notify()->notifyListChange(NOTIFY_LIST_PRIVATE_INCOMING_CHAT, NOTIFY_TYPE_ADD);
    }
    else
	    std::cerr << "  (EE) item could not be deserialised!" << std::endl;
}

void DistantChatService::markDistantChatAsClosed(const DistantChatPeerId& dcpid)
{
	mGxsTunnels->closeExistingTunnel(
	            RsGxsTunnelId(dcpid), DISTANT_CHAT_GXS_TUNNEL_SERVICE_ID );
    
    RS_STACK_MUTEX(mDistantChatMtx) ;
    
    std::map<DistantChatPeerId,DistantChatContact>::iterator it = mDistantChatContacts.find(dcpid) ;
    
    if(it != mDistantChatContacts.end())
        mDistantChatContacts.erase(it) ;
}

bool DistantChatService::initiateDistantChatConnexion(
        const RsGxsId& to_gxs_id, const RsGxsId& from_gxs_id,
        DistantChatPeerId& dcpid, uint32_t& error_code, bool notify )
{
    RsGxsTunnelId tunnel_id ;

    if(!mGxsTunnels->requestSecuredTunnel(to_gxs_id,from_gxs_id,tunnel_id,DISTANT_CHAT_GXS_TUNNEL_SERVICE_ID,error_code))
	    return false ;

    dcpid = DistantChatPeerId(tunnel_id) ;

    DistantChatContact& dc_contact(mDistantChatContacts[dcpid]) ;

    dc_contact.from_id = from_gxs_id ;
    dc_contact.to_id = to_gxs_id ;

    error_code = RS_DISTANT_CHAT_ERROR_NO_ERROR ;

	if(notify)
	{
		// Make a self message to raise the chat window
		RsChatMsgItem *item = new RsChatMsgItem;
		item->message = "[Starting distant chat. Please wait for secure tunnel";
		item->message += " to be established]";
		item->chatFlags = RS_CHAT_FLAG_PRIVATE;
		item->sendTime = time(NULL);
		item->PeerId(RsPeerId(tunnel_id));
		handleRecvChatMsgItem(item);
		delete item ;
	}

    return true ;
}

bool DistantChatService::getDistantChatStatus(const DistantChatPeerId& tunnel_id, DistantChatPeerInfo& cinfo) 
{
	RS_STACK_MUTEX(mDistantChatMtx);

	RsGxsTunnelService::GxsTunnelInfo tinfo;

	if(!mGxsTunnels->getTunnelInfo(RsGxsTunnelId(tunnel_id),tinfo)) return false;

	cinfo.to_id  = tinfo.destination_gxs_id;
	cinfo.own_id = tinfo.source_gxs_id;
	cinfo.peer_id = tunnel_id;

	switch(tinfo.tunnel_status)
	{
	case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_CAN_TALK :
		cinfo.status = RS_DISTANT_CHAT_STATUS_CAN_TALK; break;
	case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_TUNNEL_DN:
		cinfo.status = RS_DISTANT_CHAT_STATUS_TUNNEL_DN; break;
	case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_REMOTELY_CLOSED:
		cinfo.status = RS_DISTANT_CHAT_STATUS_REMOTELY_CLOSED; break;
	case RsGxsTunnelService::RS_GXS_TUNNEL_STATUS_UNKNOWN:
	default:
		cinfo.status = RS_DISTANT_CHAT_STATUS_UNKNOWN; break;
	}

	return true;
}

bool DistantChatService::closeDistantChatConnexion(const DistantChatPeerId &tunnel_id)
{
    mGxsTunnels->closeExistingTunnel(RsGxsTunnelId(tunnel_id), DISTANT_CHAT_GXS_TUNNEL_SERVICE_ID) ;
    
    // also remove contact. Or do we wait for the notification?
    
    return true ;
}

uint32_t DistantChatService::getDistantChatPermissionFlags()
{
    return mDistantChatPermissions ;
}
bool DistantChatService::setDistantChatPermissionFlags(uint32_t flags)
{
    if(mDistantChatPermissions != flags)
    {
    mDistantChatPermissions = flags ;
#ifdef DEBUG_DISTANT_CHAT    
    DISTANT_CHAT_DEBUG() << "(II) Changing distant chat permissions to " << flags << ". Existing openned chats will however remain active until closed" << std::endl;
#endif
    triggerConfigSave() ;
    }
    
    return true ;
}

void DistantChatService::addToSaveList(std::list<RsItem*>& list) const
{
	/* Save permission flags */

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;
	kv.key = "DISTANT_CHAT_PERMISSION_FLAGS" ;
	kv.value = RsUtil::NumberToString(mDistantChatPermissions) ;
    
	vitem->tlvkvs.pairs.push_back(kv) ;

	list.push_back(vitem) ;
}
bool DistantChatService::processLoadListItem(const RsItem *item)
{
	const RsConfigKeyValueSet *vitem = NULL ;

	if(NULL != (vitem = dynamic_cast<const RsConfigKeyValueSet*>(item)))
		for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) 
			if(kit->key == "DISTANT_CHAT_PERMISSION_FLAGS")
			{
#ifdef DEBUG_DISTANT_CHAT
				DISTANT_CHAT_DEBUG() << "Loaded distant chat permission flags: " << kit->value << std::endl ;
#endif
				if (!kit->value.empty())
				{
					std::istringstream is(kit->value) ;

					uint32_t tmp ;
					is >> tmp ;

					if(tmp < 3)
						mDistantChatPermissions = tmp ;
					else
						std::cerr << "(EE) Invalid value read for DistantChatPermission flags in config: " << tmp << std::endl;
				}

				return true;
			}

	return false ;
}

