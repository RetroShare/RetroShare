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

/* 
 * The basic Chat service.
 *
 */

#include <list>
#include <string>

#include "serialiser/rsmsgitems.h"
#include "services/p3service.h"
#include "pqi/p3connmgr.h"

class p3ChatService: public p3Service
{
	public:
		p3ChatService(p3ConnectMgr *cm);

		/* overloaded */
		virtual int   tick();
		virtual int   status();

		int	sendChat(std::wstring msg);
		int	sendPrivateChat(std::wstring msg, std::string id);

		/// gets the peer's avatar in jpeg format, if available. Null otherwise. Also asks the peer to send
		/// its avatar, if not already available. Creates a new unsigned char array. It's the caller's
		/// responsibility to delete this ones used.
		///
		void getAvatarJpegData(const std::string& peer_id,unsigned char *& data,int& size) ;

		/// Sets the avatar data and size. Data is copied, so should be destroyed by the caller.
		void setOwnAvatarJpegData(const unsigned char *data,int size) ;
		void getOwnAvatarJpegData(unsigned char *& data,int& size) ;

		std::list<RsChatItem *> getChatQueue(); 

	private:
		class AvatarInfo ;

		/// Send avatar info to peer in jpeg format.
		void sendAvatarJpegData(const std::string& peer_id) ;

		/// Receive the avatar in a chat item, with RS_CHAT_RECEIVE_AVATAR flag.
		void receiveAvatarJpegData(RsChatItem *ci) ;

		/// Sends a request for an avatar to the peer of given id
		void sendAvatarRequest(const std::string& peer_id) ;

		p3ConnectMgr *mConnMgr;

		AvatarInfo *_own_avatar ;
		std::map<std::string,AvatarInfo *> _avatars ;
};

#endif // SERVICE_CHAT_HEADER

