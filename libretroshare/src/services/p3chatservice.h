/*
 * libretroshare/src/services chatservice.h
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


#ifndef SERVICE_CHAT_HEADER
#define SERVICE_CHAT_HEADER



#include <list>
#include <string>

#include "serialiser/rsmsgitems.h"
#include "services/p3service.h"
#include "pqi/p3connmgr.h"


//!The basic Chat service.
 /**
  *
  * Can be used to send and receive chats, immediate status (using notify), avatars, and custom status
  * This service uses rsnotify (callbacks librs clients (e.g. rs-gui))
  * @see NotifyBase
  */
class p3ChatService: public p3Service, public p3Config
{
	public:
		p3ChatService(p3ConnectMgr *cm);

		/* overloaded */
		virtual int   tick();
		virtual int   status();

		/*!
		 * public chat sent to all peers
		 */
		int	sendChat(std::wstring msg);

		/*!
		 * chat is sent to specifc peer
		 * @param id peer to send caht msg to
		 */
		int	sendPrivateChat(std::wstring msg, std::string id);

		/*!
		 * can be used to send 'immediate' status msgs, these status updates are meant for immediate use by peer (not saved by rs)
		 * e.g currently used to update user when a peer 'is typing' during a chat
		 */
		void  sendStatusString(const std::string& peer_id,const std::string& status_str) ;

		/*!
		 * send to all peers online
		 *@see sendStatusString()
		 */
		void  sendGroupChatStatusString(const std::string& status_str) ;

		/*!
		 * this retrieves custom status for a peers, generate a requests to the peer
		 * @param peer_id the id of the peer you want status string for
		 */
		std::string getCustomStateString(const std::string& peer_id) ;

		/*!
		 * sets the client's custom status, generates 'status available' item sent to all online peers
		 */
		void  setOwnCustomStateString(const std::string&) ;

		/*!
		 * @return client's custom string
		 */
		std::string getOwnCustomStateString() ;

		/*! gets the peer's avatar in jpeg format, if available. Null otherwise. Also asks the peer to send
		* its avatar, if not already available. Creates a new unsigned char array. It's the caller's
		* responsibility to delete this ones used.
		*/
		void getAvatarJpegData(const std::string& peer_id,unsigned char *& data,int& size) ;

		/*!
		 * Sets the avatar data and size for client's account
		 * @param data is copied, so should be destroyed by the caller
		 */
		void setOwnAvatarJpegData(const unsigned char *data,int size) ;

		/*!
		 * Gets the avatar data for clients account
		 * data is in jpeg format
		 */
		void getOwnAvatarJpegData(unsigned char *& data,int& size) ;


		/*!
		 * This retrieves all chat msg items and also (important!)
		 * processes chat-status items that are in service item queue. chat msg item requests are also processed and not returned
		 * (important! also) notifications sent to notify base  on receipt avatar, immediate status and custom status
		 * : notifyCustomState, notifyChatStatus, notifyPeerHasNewAvatar
		 * @see NotifyBase
		 */
		std::list<RsChatMsgItem *> getChatQueue(); 

		/************* from p3Config *******************/
		virtual RsSerialiser *setupSerialiser() ;

		/*!
		 * chat msg items and custom status are saved
		 */
		virtual std::list<RsItem*> saveList(bool& cleanup) ;
		virtual bool loadList(std::list<RsItem*> load) ;

	private:
		RsMutex mChatMtx;

		class AvatarInfo ;
		class StateStringInfo ;

		/// Send avatar info to peer in jpeg format.
		void sendAvatarJpegData(const std::string& peer_id) ;

		/// Send custom state info to peer
		void sendCustomState(const std::string& peer_id);

		/// Receive the avatar in a chat item, with RS_CHAT_RECEIVE_AVATAR flag.
		void receiveAvatarJpegData(RsChatAvatarItem *ci) ;	// new method
		void receiveStateString(const std::string& id,const std::string& s) ;

		/// Sends a request for an avatar to the peer of given id
		void sendAvatarRequest(const std::string& peer_id) ;

		/// Send a request for custom status string
		void sendCustomStateRequest(const std::string& peer_id);

		RsChatAvatarItem *makeOwnAvatarItem() ;
		RsChatStatusItem *makeOwnCustomStateStringItem() ;

		p3ConnectMgr *mConnMgr;

		AvatarInfo *_own_avatar ;
		std::map<std::string,AvatarInfo *> _avatars ;

		std::string _custom_status_string ;
		std::map<std::string,StateStringInfo> _state_strings ;
};

#endif // SERVICE_CHAT_HEADER

