/*******************************************************************************
 * libretroshare/src/chat: distantchat.h                                       *
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

#pragma once

#include <chat/rschatitems.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsgxstunnel.h>

class RsGixs ;

static const uint32_t DISTANT_CHAT_AES_KEY_SIZE = 16 ;

class DistantChatService: public RsGxsTunnelService::RsGxsTunnelClientService
{
public:
    DistantChatService() ;

    virtual void triggerConfigSave()=0 ;
    bool processLoadListItem(const RsItem *item) ;
    void addToSaveList(std::list<RsItem*>& list) const;
    
	/**
	 * Creates the invite if the public key of the distant peer is available.
	 * On success, stores the invite in the map above, so that we can respond
	 * to tunnel requests. */
	bool initiateDistantChatConnexion( const RsGxsId& to_gxs_id,
	                                   const RsGxsId &from_gxs_id,
	                                   DistantChatPeerId& dcpid,
	                                   uint32_t &error_code,
	                                   bool notify = true );
    bool closeDistantChatConnexion(const DistantChatPeerId &tunnel_id) ;
    
    // Sets flags to only allow connexion from some people.
    
    uint32_t getDistantChatPermissionFlags() ;
    bool setDistantChatPermissionFlags(uint32_t flags) ;
    
    // Returns the status of a distant chat contact. The contact is defined by the tunnel id (turned into a DistantChatPeerId) because
    // each pair of talking GXS id needs to be treated separately
    
    virtual bool getDistantChatStatus(const DistantChatPeerId &tunnel_id, DistantChatPeerInfo& cinfo) ;

    // derived in p3ChatService, so as to pass down some info
    virtual void handleIncomingItem(RsItem *) = 0;
    virtual bool handleRecvChatMsgItem(RsChatMsgItem *& ci)=0 ;

    bool handleOutgoingItem(RsChatItem *) ;
    bool handleRecvItem(RsChatItem *) ;
    void handleRecvChatStatusItem(RsChatStatusItem *cs) ;

private:
    struct DistantChatContact
    {
        RsGxsId from_id ;
        RsGxsId to_id ;
    };
    // This maps contains the current peers to talk to with distant chat.
    //
    std::map<DistantChatPeerId, DistantChatContact> 	mDistantChatContacts ;		// current peers we can talk to

    // Permission handling
    
    uint32_t mDistantChatPermissions ;

    // Overloaded from RsGxsTunnelClientService
    
public:
    virtual void connectToGxsTunnelService(RsGxsTunnelService *tunnel_service) ;
    
private:
	virtual bool acceptDataFromPeer(
	        const RsGxsId& gxs_id, const RsGxsTunnelId& tunnel_id,
	        bool am_I_client_side);
	virtual void notifyTunnelStatus(
	        const RsGxsTunnelId& tunnel_id, uint32_t tunnel_status);
	virtual void receiveData(
	        const RsGxsTunnelId& id, unsigned char* data, uint32_t data_size );

    // Utility functions.
    
    void markDistantChatAsClosed(const DistantChatPeerId& dcpid) ;

    RsGxsTunnelService *mGxsTunnels ;
    RsMutex mDistantChatMtx ;
};
