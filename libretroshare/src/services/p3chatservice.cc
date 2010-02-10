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
#include "pqi/pqistore.h"

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
	:p3Service(RS_SERVICE_TYPE_CHAT), p3Config(CONFIG_TYPE_CHAT), mConnMgr(cm) 
{
	addSerialType(new RsChatSerialiser());

	_own_avatar = NULL ;
	_custom_status_string = "" ;
}

int	p3ChatService::tick()
{
	return 0;
}

int	p3ChatService::status()
{
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

class p3ChatService::StateStringInfo
{
   public: 
	  StateStringInfo() 
	  {
		  _custom_status_string = "" ;	// the custom status string of the peer
		  _peer_is_new = false ;			// true when the peer has a new avatar
		  _own_is_new = false ;				// true when I myself a new avatar to send to this peer.
	  }

	  std::string _custom_status_string ;
	  int _peer_is_new ;			// true when the peer has a new avatar
	  int _own_is_new ;			// true when I myself a new avatar to send to this peer.
};
class p3ChatService::AvatarInfo
{
   public: 
	  AvatarInfo() 
	  {
		  _image_size = 0 ;
		  _image_data = NULL ;
		  _peer_is_new = false ;			// true when the peer has a new avatar
		  _own_is_new = false ;				// true when I myself a new avatar to send to this peer.
	  }

	  ~AvatarInfo()
	  {
		  delete[] _image_data ;
		  _image_data = NULL ;
		  _image_size = 0 ;
	  }

	  AvatarInfo(const AvatarInfo& ai)
	  {
		  init(ai._image_data,ai._image_size) ;
	  }

	  void init(const unsigned char *jpeg_data,int size)
	  {
		  _image_size = size ;
		  _image_data = new unsigned char[size] ;
		  memcpy(_image_data,jpeg_data,size) ;
	  }
	  AvatarInfo(const unsigned char *jpeg_data,int size)
	  {
		  init(jpeg_data,size) ;
	  }

	  void toUnsignedChar(unsigned char *& data,uint32_t& size) const
	  {
		  data = new unsigned char[_image_size] ;
		  size = _image_size ;
		  memcpy(data,_image_data,size*sizeof(unsigned char)) ;
	  }

	  uint32_t _image_size ;
	  unsigned char *_image_data ;
	  int _peer_is_new ;			// true when the peer has a new avatar
	  int _own_is_new ;			// true when I myself a new avatar to send to this peer.
};

void p3ChatService::sendGroupChatStatusString(const std::string& status_string)
{
	std::list<std::string> ids;
	mConnMgr->getOnlineList(ids);

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sendChat(): sending group chat status string: " << status_string << std::endl ;
	std::cerr << std::endl;
#endif

	for(std::list<std::string>::iterator it = ids.begin(); it != ids.end(); ++it)
	{
		RsChatStatusItem *cs = new RsChatStatusItem ;

		cs->status_string = status_string ;
		cs->flags = RS_CHAT_FLAG_PUBLIC ;

		cs->PeerId(*it);

		sendItem(cs);
	}
}

void p3ChatService::sendStatusString( const std::string& id , const std::string& status_string)
{
	RsChatStatusItem *cs = new RsChatStatusItem ;

	cs->status_string = status_string ;
	cs->flags = RS_CHAT_FLAG_PRIVATE ;
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

	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
		std::map<std::string,AvatarInfo*>::iterator it = _avatars.find(id) ; 

		if(it == _avatars.end())
		{
			_avatars[id] = new AvatarInfo ;
			it = _avatars.find(id) ;
		}
		if(it->second->_own_is_new)
		{
#ifdef CHAT_DEBUG
			std::cerr << "p3ChatService::sendPrivateChat: new avatar never sent to peer " << id << ". Setting <new> flag to packet." << std::endl; 
#endif

			ci->chatFlags |= RS_CHAT_FLAG_AVATAR_AVAILABLE ;
			it->second->_own_is_new = false ;
		}
	}

#ifdef CHAT_DEBUG
	std::cerr << "Sending msg to peer " << id << ", flags = " << ci->chatFlags << std::endl ;
	std::cerr << "p3ChatService::sendPrivateChat() Item:";
	std::cerr << std::endl;
	ci->print(std::cerr);
	std::cerr << std::endl;
#endif

	sendItem(ci);

	// Check if custom state string has changed, in which case it should be sent to the peer.
	bool should_send_state_string = false ;
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

		std::map<std::string,StateStringInfo>::iterator it = _state_strings.find(id) ; 

		if(it == _state_strings.end())
		{
			_state_strings[id] = StateStringInfo() ;
			it = _state_strings.find(id) ;
			it->second._own_is_new = true ;
		}
		if(it->second._own_is_new)
		{
			should_send_state_string = true ;
			it->second._own_is_new = false ;
		}
	}

	if(should_send_state_string)
	{
#ifdef CHAT_DEBUG
		std::cerr << "own status string is new for peer " << id << ": sending it." << std::endl ;
#endif
		RsChatStatusItem *cs = makeOwnCustomStateStringItem() ;
		cs->PeerId(id) ;
		sendItem(cs) ;
	}

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

		if(ci != NULL)	// real chat message
		{
#ifdef CHAT_DEBUG
			std::cerr << "p3ChatService::getChatQueue() Item:";
			std::cerr << std::endl;
			ci->print(std::cerr);
			std::cerr << std::endl;
			std::cerr << "Got msg. Flags = " << ci->chatFlags << std::endl ;
#endif

			if(ci->chatFlags & RS_CHAT_FLAG_REQUESTS_AVATAR)		// no msg here. Just an avatar request.
			{
				sendAvatarJpegData(ci->PeerId()) ;
				delete item ;
			}
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

                                #ifdef CHAT_DEBUG
				std::cerr << "p3chatservice:: avatar requested from above. " << std::endl ;
                                #endif
				// has avatar. Return it strait away.
				//
				if(it!=_avatars.end() && it->second->_peer_is_new)
				{
					std::cerr << "Adatar is new for peer. ending info above" << std::endl ;
					ci->chatFlags |= RS_CHAT_FLAG_AVATAR_AVAILABLE ;
				}

				ci->recvTime = now;
				ilist.push_back(ci);	// don't delete the item !!
			}
			continue ;
		}

		RsChatStatusItem *cs = dynamic_cast<RsChatStatusItem*>(item) ;

		if(cs != NULL)
		{
			// we should notify for a status string for the current peer.
#ifdef CHAT_DEBUG
			std::cerr << "Received status string \"" << cs->status_string << "\"" << std::endl ;
#endif
			if(cs->flags & RS_CHAT_FLAG_PRIVATE)
				rsicontrol->getNotify().notifyChatStatus(cs->PeerId(),cs->status_string,true) ;

			if(cs->flags & RS_CHAT_FLAG_PUBLIC)
				rsicontrol->getNotify().notifyChatStatus(cs->PeerId(),cs->status_string,false) ;

			if(cs->flags & RS_CHAT_FLAG_CUSTOM_STATE)
			{
#ifdef CHAT_DEBUG
				std::cout << "Received custom status string packet from peer " << cs->PeerId() << ": " << cs->status_string << ". Storing it and notifying." << std::endl ;
#endif
				receiveStateString(cs->PeerId(),cs->status_string) ;	// store it
				rsicontrol->getNotify().notifyCustomState(cs->PeerId()) ;
			}

			delete item ;
			continue ;
		}

		RsChatAvatarItem *ca = dynamic_cast<RsChatAvatarItem*>(item) ;

		if(ca != NULL)
		{
			receiveAvatarJpegData(ca) ;

#ifdef CHAT_DEBUG
			std::cerr << "Received avatar data for peer " << ca->PeerId() << ". Notifying." << std::endl ;
#endif
			rsicontrol->getNotify().notifyPeerHasNewAvatar(ca->PeerId()) ;

			delete item ;
			continue ;
		}
	}

	return ilist;
}

void p3ChatService::setOwnCustomStateString(const std::string& s)
{
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

#ifdef CHAT_DEBUG
		std::cerr << "p3chatservice: Setting own state string to new value : " << s << std::endl ;
#endif
		_custom_status_string = s ;

		for(std::map<std::string,StateStringInfo>::iterator it(_state_strings.begin());it!=_state_strings.end();++it)
			it->second._own_is_new = true ;
	}

	rsicontrol->getNotify().notifyOwnStatusMessageChanged() ;
	IndicateConfigChanged();
}

void p3ChatService::setOwnAvatarJpegData(const unsigned char *data,int size)
{
	{
		RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
#ifdef CHAT_DEBUG
		std::cerr << "p3chatservice: Setting own avatar to new image." << std::endl ;
#endif

		if(_own_avatar != NULL)
			delete _own_avatar ;

		_own_avatar = new AvatarInfo(data,size) ;

		// set the info that our avatar is new, for all peers
		for(std::map<std::string,AvatarInfo *>::iterator it(_avatars.begin());it!=_avatars.end();++it)
			it->second->_own_is_new = true ;
	}
	IndicateConfigChanged();

	rsicontrol->getNotify().notifyOwnAvatarChanged() ;

#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:setOwnAvatarJpegData() done." << std::endl ;
#endif

}

void p3ChatService::receiveStateString(const std::string& id,const std::string& s)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
#ifdef CHAT_DEBUG
   std::cerr << "p3chatservice: received avatar jpeg data for peer " << id << ". Storing it." << std::endl ;
#endif

   bool new_peer = (_state_strings.find(id) == _state_strings.end()) ;

   _state_strings[id]._custom_status_string = s ;
   _state_strings[id]._peer_is_new = true ;
   _state_strings[id]._own_is_new = new_peer ;
}
void p3ChatService::receiveAvatarJpegData(RsChatAvatarItem *ci)
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
#ifdef CHAT_DEBUG
   std::cerr << "p3chatservice: received avatar jpeg data for peer " << ci->PeerId() << ". Storing it." << std::endl ;
#endif

   bool new_peer = (_avatars.find(ci->PeerId()) == _avatars.end()) ;

   _avatars[ci->PeerId()] = new AvatarInfo(ci->image_data,ci->image_size) ; 
   _avatars[ci->PeerId()]->_peer_is_new = true ;
   _avatars[ci->PeerId()]->_own_is_new = new_peer ;
}

std::string p3ChatService::getOwnCustomStateString() 
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	return _custom_status_string ;
}
void p3ChatService::getOwnAvatarJpegData(unsigned char *& data,int& size) 
{
	// should be a Mutex here.
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	uint32_t s = 0 ;
#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:: own avatar requested from above. " << std::endl ;
#endif
	// has avatar. Return it strait away.
	//
	if(_own_avatar != NULL)
	{
	   _own_avatar->toUnsignedChar(data,s) ;
		size = s ;
	}
	else
	{
		data=NULL ;
		size=0 ;
	}
}

std::string p3ChatService::getCustomStateString(const std::string& peer_id) 
{
	// should be a Mutex here.
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string,StateStringInfo>::iterator it = _state_strings.find(peer_id) ; 

#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:: status string for peer " << peer_id << " requested from above. " << std::endl ;
#endif
	// has it. Return it strait away.
	//
	if(it!=_state_strings.end())
	{
	   it->second._peer_is_new = false ;
#ifdef CHAT_DEBUG
	   std::cerr << "Already has status string. Returning it" << std::endl ;
#endif
	   return it->second._custom_status_string ;
	}
	else
	{
#ifdef CHAT_DEBUG
		std::cerr << "No status string for this peer. Not requesting it." << std::endl ;
#endif
		return std::string() ;
	}
}

void p3ChatService::getAvatarJpegData(const std::string& peer_id,unsigned char *& data,int& size) 
{
	// should be a Mutex here.
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string,AvatarInfo *>::const_iterator it = _avatars.find(peer_id) ; 

#ifdef CHAT_DEBUG
	std::cerr << "p3chatservice:: avatar for peer " << peer_id << " requested from above. " << std::endl ;
#endif
	// has avatar. Return it strait away.
	//
	if(it!=_avatars.end())
	{
		uint32_t s=0 ;
	   it->second->toUnsignedChar(data,s) ;
		size = s ;
	   it->second->_peer_is_new = false ;
#ifdef CHAT_DEBUG
	   std::cerr << "Already has avatar. Returning it" << std::endl ;
#endif
	   return ;
       } else {
           #ifdef CHAT_DEBUG
	   std::cerr << "No avatar for this peer. Requesting it by sending request packet." << std::endl ;
           #endif
       }

        sendAvatarRequest(peer_id);
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

#ifdef CHAT_DEBUG
	std::cerr << "p3ChatService::sending request for avatar, to peer " << peer_id << std::endl ;
	std::cerr << std::endl;
#endif

	sendItem(ci);
}

RsChatStatusItem *p3ChatService::makeOwnCustomStateStringItem()
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	RsChatStatusItem *ci = new RsChatStatusItem();

	ci->flags = RS_CHAT_FLAG_CUSTOM_STATE ;
	ci->status_string = _custom_status_string ;

	return ci ;
}
RsChatAvatarItem *p3ChatService::makeOwnAvatarItem()
{
	RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/
	RsChatAvatarItem *ci = new RsChatAvatarItem();

	_own_avatar->toUnsignedChar(ci->image_data,ci->image_size) ;

	return ci ;
}


void p3ChatService::sendAvatarJpegData(const std::string& peer_id)
{
   #ifdef CHAT_DEBUG
   std::cerr << "p3chatservice: sending requested for peer " << peer_id << ", data=" << (void*)_own_avatar << std::endl ;
   #endif

   if(_own_avatar != NULL)
	{
		RsChatAvatarItem *ci = makeOwnAvatarItem();
		ci->PeerId(peer_id);

		// take avatar, and embed it into a std::wstring.
		//
		std::cerr << "p3ChatService::sending avatar image to peer" << peer_id << ", image size = " << ci->image_size << std::endl ;
		std::cerr << std::endl;

		sendItem(ci) ;
	}
   else {
        #ifdef CHAT_DEBUG
        std::cerr << "Doing nothing" << std::endl ;
        #endif
   }
}

bool p3ChatService::loadList(std::list<RsItem*> load)
{
	for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
	{
		RsChatAvatarItem *ai = NULL ;

		if(NULL != (ai = dynamic_cast<RsChatAvatarItem *>(*it)))
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			_own_avatar = new AvatarInfo(ai->image_data,ai->image_size) ;
		}

		RsChatStatusItem *mitem = NULL ;

		if(NULL != (mitem = dynamic_cast<RsChatStatusItem *>(*it)))
		{
			RsStackMutex stack(mChatMtx); /********** STACK LOCKED MTX ******/

			_custom_status_string = mitem->status_string ;
		}


		delete *it;
	}
	return true;
}

std::list<RsItem*> p3ChatService::saveList(bool& cleanup)
{
	cleanup = true ;
	/* now we create a pqistore, and stream all the msgs into it */

	std::list<RsItem*> list ;

	if(_own_avatar != NULL)
	{
		RsChatAvatarItem *ci = makeOwnAvatarItem() ;
		ci->PeerId(mConnMgr->getOwnId());

		list.push_back(ci) ;
	}

	RsChatStatusItem *di = new RsChatStatusItem ;
	di->status_string = _custom_status_string ;
	di->flags = RS_CHAT_FLAG_CUSTOM_STATE ;

	list.push_back(di) ;

	return list;
}

RsSerialiser *p3ChatService::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsChatSerialiser) ;

	return rss ;
}



