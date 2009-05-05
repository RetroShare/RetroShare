/*
 * "$Id: p3ChatService.cc,v 1.24 2007-05-05 16:10:06 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

#include "util/rsdir.h"
#include "rsiface/rsiface.h"
#include "pqi/pqibin.h"
#include "pqi/pqinotify.h"
#include "pqi/pqiarchive.h"

#include "services/p3chatservice.h"

/****
 * #define CHAT_DEBUG 1
 ****/

/************ NOTE *********************************
 * This Service is so simple that there is no
 * mutex protection required!
 *
 */

p3ChatService::p3ChatService(p3ConnectMgr *cm)
	:p3Service(RS_SERVICE_TYPE_CHAT), pqiConfig(CONFIG_TYPE_CHAT), mConnMgr(cm) 
{
	addSerialType(new RsChatSerialiser());

	_own_avatar = NULL ;
}

int	p3ChatService::tick()
{

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::tick()";
	std::cerr << std::endl;
#endif

	return 0;
}

int	p3ChatService::status()
{

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::status()";
	std::cerr << std::endl;
#endif

	return 1;
}

/***************** Chat Stuff **********************/

int     p3ChatService::sendChat(std::wstring msg)
{
	/* go through all the peers */

	std::list<std::string> ids;
	std::list<std::string>::iterator it;
	mConnMgr->getOnlineList(ids);

	/* add in own id -> so get reflection */
	ids.push_back(mConnMgr->getOwnId());

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendChat()";
	std::cerr << std::endl;
#endif

	for(it = ids.begin(); it != ids.end(); it++)
	{
		RsChatMsgItem *ci = new RsChatMsgItem();

		ci->PeerId(*it);
		ci->chatFlags = 0;
		ci->sendTime = time(NULL);
		ci->message = msg;
	
#ifdef CHAT_DEBUG
		std::cerr << "p3ChatService::sendChat() Item:";
		std::cerr << std::endl;
		ci->print(std::cerr);
		std::cerr << std::endl;
#endif

		sendItem(ci);
	}

	return 1;
}

class p3ChatService::AvatarInfo
{
   public: 
	  AvatarInfo() 
	  {
		  _peer_is_new = false ;			// true when the peer has a new avatar
		  _own_is_new = false ;				// true when I myself a new avatar to send to this peer.
	  }

	  AvatarInfo(const unsigned char *jpeg_data,int size)
	  {
		 int n_c = size ;
		 int p   = 2 ;// minimum value for sizeof(wchar_t) over win/mac/linux ;
		 int n   = n_c/p + 1 ;

		 _jpeg_wstring = std::wstring(n,0) ;

		 for(int i=0;i<n;++i)
		 {
			wchar_t h = jpeg_data[p*i+p-1] ;

			for(int j=p-2;j>=0;--j)
			{
			   h = h << 8 ;
			   h += jpeg_data[p*i+j] ;
			}
			_jpeg_wstring[i] = h ;
		 }
	  }
	  AvatarInfo(const std::wstring& s) : _jpeg_wstring(s) {}

	  const std::wstring& toStdWString() const { return _jpeg_wstring; }

	  void toUnsignedChar(unsigned char *& data,int& size) const
	  {
		 int p 	= 2 ;// minimum value for sizeof(wchar_t) over win/mac/linux ;
		 int n 	= _jpeg_wstring.size() ;
		 int n_c	= p*n ;

		 data = new unsigned char[n_c] ;
		 size = n_c ;

		 for(int i=0;i<n;++i)
		 {
			wchar_t h = _jpeg_wstring[i] ;

			for(int j=0;j<p;++j)
			{
			   data[p*i+j] = (unsigned char)(h & 0xff) ;
			   h = h >> 8 ;
			}
		 }
	  }

	  std::wstring _jpeg_wstring;
	  int _peer_is_new ;			// true when the peer has a new avatar
	  int _own_is_new ;				// true when I myself a new avatar to send to this peer.
};


void    p3ChatService::sendStatusString( const std::string& id , const std::string& status_string)
{
	RsChatStatusItem *cs = new RsChatStatusItem ;

	cs->status_string = status_string ;
	cs->PeerId(id);

#ifdef CHAT_DEBUG
	std::cerr  << "sending chat status packet:" << std::endl ;
	cs->print(std::cerr) ;
#endif
	sendItem(cs);
}

int     p3ChatService::sendPrivateChat( std::wstring msg, std::string id)
{
	// make chat item....
#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendPrivateChat()";
	std::cerr << std::endl;
#endif

	RsChatMsgItem *ci = new RsChatMsgItem();

	ci->PeerId(id);
	ci->chatFlags = RS_CHAT_FLAG_PRIVATE;
	ci->sendTime = time(NULL);
	ci->message = msg;

	std::map<std::string,AvatarInfo*>::iterator it = _avatars.find(id) ; 

	if(it == _avatars.end())
	{
		_avatars[id] = new AvatarInfo ;
		it = _avatars.find(id) ;
	}
	if(it != _avatars.end() && it->second->_own_is_new)
	{
#ifdef CHAT_DEBUG
	   std::cerr << "p3ChatService::sendPrivateChat: new avatar never sent to peer " << id << ". Setting <new> flag to packet." << std::endl; 
#endif

	   ci->chatFlags |= RS_CHAT_FLAG_AVATAR_AVAILABLE ;
	   it->second->_own_is_new = false ;
	}

#ifdef CHAT_DEBUG
	std::cerr << "Sending msg to peer " << id << ", flags = " << ci->chatFlags << std::endl ;
	std::cerr << "p3ChatService::sendPrivateChat() Item:";
	std::cerr << std::endl;
	ci->print(std::cerr);
	std::cerr << std::endl;
#endif

	sendItem(ci);

	return 1;
}

std::list<RsChatMsgItem *> p3ChatService::getChatQueue()
{
	time_t now = time(NULL);

	RsItem *item ;
	std::list<RsChatMsgItem *> ilist;

	while(NULL != (item=recvItem()))
	{
#ifdef CHAT_DEBUG
		std::cerr << "p3ChatService::getChatQueue() Item:" << (void*)item << std::endl ;
#endif
		RsChatMsgItem *ci = dynamic_cast<RsChatMsgItem*>(item) ;

		if(ci != NULL)
		{
#ifdef CHAT_DEBUG
			std::cerr << "p3ChatService::getChatQueue() Item:";
			std::cerr << std::endl;
			ci->print(std::cerr);
			std::cerr << std::endl;
			std::cerr << "Got msg. Flags = " << ci->chatFlags << std::endl ;
#endif

			if(ci->chatFlags & RS_CHAT_FLAG_CONTAINS_AVATAR)			// no msg here. Just an avatar.
				receiveAvatarJpegData(ci) ;
			else if(ci->chatFlags & RS_CHAT_FLAG_REQUESTS_AVATAR)		// no msg here. Just an avatar request.
				sendAvatarJpegData(ci->PeerId()) ;
			else														// normal msg. Return it normally.
			{
				// Check if new avatar is available at peer's. If so, send a request to get the avatar.
				if(ci->chatFlags & RS_CHAT_FLAG_AVATAR_AVAILABLE) 
				{
					std::cerr << "New avatar is available for peer " << ci->PeerId() << ", sending request" << std::endl ;
					sendAvatarRequest(ci->PeerId()) ;
					ci->chatFlags &= ~RS_CHAT_FLAG_AVATAR_AVAILABLE ;
				}

				std::map<std::string,AvatarInfo *>::const_iterator it = _avatars.find(ci->PeerId()) ; 

				std::cerr << "p3chatservice:: avatar requested from above. " << std::endl ;
				// has avatar. Return it strait away.
				//
				if(it!=_avatars.end() && it->second->_peer_is_new)
				{
					std::cerr << "Adatar is new for peer. ending info above" << std::endl ;
					ci->chatFlags |= RS_CHAT_FLAG_AVATAR_AVAILABLE ;
				}

				ci->recvTime = now;
				ilist.push_back(ci);
			}
		}
		RsChatStatusItem *cs = dynamic_cast<RsChatStatusItem*>(item) ;

		if(cs != NULL)
		{
			// we should notify for a status string for the current peer.
#ifdef CHAT_DEBUG
			std::cerr << "Received status string \"" << cs->status_string << "\"" << std::endl ;
#endif
			rsicontrol->getNotify().notifyChatStatus(cs->PeerId(),cs->status_string) ;

			delete item ;
		}
	}

	return ilist;
}

void p3ChatService::setOwnAvatarJpegData(const unsigned char *data,int size)
{
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		std::cerr << "p3chatservice: Setting own avatar to new image." << std::endl ;

		if(_own_avatar != NULL)
			delete _own_avatar ;

		_own_avatar = new AvatarInfo(data,size) ;

		// set the info that our avatar is new, for all peers
		for(std::map<std::string,AvatarInfo *>::iterator it(_avatars.begin());it!=_avatars.end();++it)
			it->second->_own_is_new = true ;
	}

	IndicateConfigChanged();
}

void p3ChatService::receiveAvatarJpegData(RsChatMsgItem *ci)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
   std::cerr << "p3chatservice: received avatar jpeg data for peer " << ci->PeerId() << ". Storing it." << std::endl ;

   bool new_peer = (_avatars.find(ci->PeerId()) == _avatars.end()) ;

   _avatars[ci->PeerId()] = new AvatarInfo(ci->message) ; 
   _avatars[ci->PeerId()]->_peer_is_new = true ;
   _avatars[ci->PeerId()]->_own_is_new = new_peer ;
}

void p3ChatService::getOwnAvatarJpegData(unsigned char *& data,int& size) 
{
	// should be a Mutex here.
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::cerr << "p3chatservice:: own avatar requested from above. " << std::endl ;
	// has avatar. Return it strait away.
	//
	if(_own_avatar != NULL)
	   _own_avatar->toUnsignedChar(data,size) ;
	else
	{
		data=NULL ;
		size=0 ;
	}
}
void p3ChatService::getAvatarJpegData(const std::string& peer_id,unsigned char *& data,int& size) 
{
	// should be a Mutex here.
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string,AvatarInfo *>::const_iterator it = _avatars.find(peer_id) ; 

	std::cerr << "p3chatservice:: avatar requested from above. " << std::endl ;
	// has avatar. Return it strait away.
	//
	if(it!=_avatars.end())
	{
	   it->second->toUnsignedChar(data,size) ;
	   it->second->_peer_is_new = false ;
	   std::cerr << "Already has avatar. Returning it" << std::endl ;
	   return ;
	}
	else
	   std::cerr << "No avatar for this peer. Requesting it by sending request packet." << std::endl ;

	sendAvatarRequest(peer_id) ;
}

void p3ChatService::sendAvatarRequest(const std::string& peer_id)
{
	// Doesn't have avatar. Request it.
	//
	RsChatMsgItem *ci = new RsChatMsgItem();

	ci->PeerId(peer_id);
	ci->chatFlags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_REQUESTS_AVATAR ;
	ci->sendTime = time(NULL);
	ci->message = std::wstring() ;

	std::cerr << "p3ChatService::sending request for avatar, to peer " << peer_id << std::endl ;
	std::cerr << std::endl;

	sendItem(ci);
}

RsChatMsgItem *p3ChatService::makeOwnAvatarItem()
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	RsChatMsgItem *ci = new RsChatMsgItem();

	ci->chatFlags = RS_CHAT_FLAG_PRIVATE | RS_CHAT_FLAG_CONTAINS_AVATAR ;
	ci->sendTime = time(NULL);
	ci->message = _own_avatar->toStdWString() ;

	return ci ;
}


void p3ChatService::sendAvatarJpegData(const std::string& peer_id)
{
   std::cerr << "p3chatservice: sending requested for peer " << peer_id << ", data=" << (void*)_own_avatar << std::endl ;

   if(_own_avatar != NULL)
	{
		RsChatMsgItem *ci = makeOwnAvatarItem();
		ci->PeerId(peer_id);

		// take avatar, and embed it into a std::wstring.
		//
		std::cerr << "p3ChatService::sending avatar image to peer" << peer_id << ", string size = " << ci->message.size() << std::endl ;
		std::cerr << std::endl;

		sendItem(ci) ;
	}
   else
	  std::cerr << "Doing nothing" << std::endl ;
}

bool p3ChatService::loadConfiguration(std::string &loadHash)
{
	std::list<std::string>::iterator it;
	std::string msgfile = Filename();

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsChatSerialiser());

	BinFileInterface *in = new BinFileInterface(msgfile.c_str(), BIN_FLAGS_READABLE | BIN_FLAGS_HASH_DATA);
	pqiarchive *pa_in = new pqiarchive(rss, in, BIN_FLAGS_READABLE);
	RsItem *item;
	RsChatMsgItem *mitem;

	while((item = pa_in -> GetItem()))
	{
		if(NULL != (mitem = dynamic_cast<RsChatMsgItem *>(item)))
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			_own_avatar = new AvatarInfo(mitem->message) ;
		}

		delete item;
	}

	std::string hashin = in->gethash();
	delete pa_in;	

	if (hashin != loadHash)
	{
		/* big error message! */
		std::cerr << "p3ChatService::loadConfiguration() FAILED! avatar Tampered" << std::endl;
		std::string msgfileold = msgfile + ".failed";

		rename(msgfile.c_str(), msgfileold.c_str());

		std::cerr << "Moving Old file to: " << msgfileold << std::endl;
		std::cerr << "removing dodgey msgs" << std::endl;

		_own_avatar = NULL ;

		setHash("");
		return false;
	}

	setHash(hashin);

	return true;
}

bool p3ChatService::saveConfiguration()
{
	/* now we create a pqiarchive, and stream all the msgs into it */

	std::string msgfile = Filename();
	std::string msgfiletmp = Filename()+".tmp";

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsChatSerialiser());

	BinFileInterface *out = new BinFileInterface(msgfiletmp.c_str(), BIN_FLAGS_WRITEABLE | BIN_FLAGS_HASH_DATA);
	pqiarchive *pa_out = new pqiarchive(rss, out, BIN_FLAGS_WRITEABLE);

	if(_own_avatar != NULL)
	{
		std::cerr << "Saving avatar config to file " << msgfile << std::endl ;
		RsChatMsgItem *ci = makeOwnAvatarItem() ;
		ci->PeerId(mConnMgr->getOwnId());

		if(!pa_out -> SendItem(ci))
			return false ;
	}

	setHash(out->gethash());
	delete pa_out;	

	if(!RsDirUtil::renameFile(msgfiletmp,msgfile))
	{
		getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "File rename error", "Error while renaming file " + msgfile) ;
		return false ;
	}

	return true;
}



