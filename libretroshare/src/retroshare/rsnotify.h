/*******************************************************************************
 * libretroshare/src/retroshare: rsnotify.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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

#include <map>
#include <list>
#include <iostream>
#include <string>
#include <stdint.h>

#include "rsturtle.h"
#include "rsgxsifacetypes.h"
#include "util/rsdeprecate.h"

class ChatId;
class ChatMessage;
struct RsGxsChanges;

class RsNotify;
extern RsNotify   *rsNotify;

const uint32_t RS_SYS_ERROR 	= 0x0001;
const uint32_t RS_SYS_WARNING 	= 0x0002;
const uint32_t RS_SYS_INFO 	    = 0x0004;

const uint32_t RS_POPUP_MSG             = 0x0001;
const uint32_t RS_POPUP_CHAT            = 0x0002;
//const uint32_t RS_POPUP_CALL          = 0x0004;
const uint32_t RS_POPUP_CONNECT         = 0x0008;
const uint32_t RS_SYSTRAY_GROUP_MSG     = 0x0010;
const uint32_t RS_POPUP_DOWNLOAD        = 0x0020;
const uint32_t RS_POPUP_GROUPCHAT       = 0x0040;
const uint32_t RS_POPUP_CHATLOBBY       = 0x0080;
const uint32_t RS_POPUP_CONNECT_ATTEMPT = 0x0100;
const uint32_t RS_POPUP_ENCRYPTED_MSG   = 0x0200;

/* CHAT flags are here - so they are in the same place as 
 * other Notify flags... not used by libretroshare though
 */
const uint32_t RS_CHAT_OPEN          = 0x0001;
//const uint32_t free                = 0x0002;
const uint32_t RS_CHAT_FOCUS         = 0x0004;
const uint32_t RS_CHAT_TABBED_WINDOW = 0x0008;
const uint32_t RS_CHAT_BLINK         = 0x0010;

const uint32_t RS_FEED_TYPE_PEER     = 0x0010;
const uint32_t RS_FEED_TYPE_CHANNEL  = 0x0020;
const uint32_t RS_FEED_TYPE_FORUM    = 0x0040;
//const uint32_t RS_FEED_TYPE_BLOG     = 0x0080;
const uint32_t RS_FEED_TYPE_CHAT     = 0x0100;
const uint32_t RS_FEED_TYPE_MSG      = 0x0200;
const uint32_t RS_FEED_TYPE_FILES    = 0x0400;
const uint32_t RS_FEED_TYPE_SECURITY = 0x0800;
const uint32_t RS_FEED_TYPE_POSTED   = 0x1000;
const uint32_t RS_FEED_TYPE_SECURITY_IP = 0x2000;
const uint32_t RS_FEED_TYPE_CIRCLE   = 0x4000;

const uint32_t RS_FEED_ITEM_PEER_CONNECT            = RS_FEED_TYPE_PEER  | 0x0001;
const uint32_t RS_FEED_ITEM_PEER_DISCONNECT         = RS_FEED_TYPE_PEER  | 0x0002;
const uint32_t RS_FEED_ITEM_PEER_HELLO              = RS_FEED_TYPE_PEER  | 0x0003;
const uint32_t RS_FEED_ITEM_PEER_NEW                = RS_FEED_TYPE_PEER  | 0x0004;
const uint32_t RS_FEED_ITEM_PEER_OFFSET             = RS_FEED_TYPE_PEER  | 0x0005;
const uint32_t RS_FEED_ITEM_PEER_DENIES_CONNEXION   = RS_FEED_TYPE_PEER  | 0x0006;

const uint32_t RS_FEED_ITEM_SEC_CONNECT_ATTEMPT     = RS_FEED_TYPE_SECURITY  | 0x0001;
const uint32_t RS_FEED_ITEM_SEC_AUTH_DENIED         = RS_FEED_TYPE_SECURITY  | 0x0002;	// locally denied connection
const uint32_t RS_FEED_ITEM_SEC_UNKNOWN_IN          = RS_FEED_TYPE_SECURITY  | 0x0003;
const uint32_t RS_FEED_ITEM_SEC_UNKNOWN_OUT         = RS_FEED_TYPE_SECURITY  | 0x0004;
const uint32_t RS_FEED_ITEM_SEC_WRONG_SIGNATURE     = RS_FEED_TYPE_SECURITY  | 0x0005;
const uint32_t RS_FEED_ITEM_SEC_BAD_CERTIFICATE     = RS_FEED_TYPE_SECURITY  | 0x0006;
const uint32_t RS_FEED_ITEM_SEC_INTERNAL_ERROR      = RS_FEED_TYPE_SECURITY  | 0x0007;
const uint32_t RS_FEED_ITEM_SEC_MISSING_CERTIFICATE = RS_FEED_TYPE_SECURITY  | 0x0008;

const uint32_t RS_FEED_ITEM_SEC_IP_BLACKLISTED      = RS_FEED_TYPE_SECURITY_IP  | 0x0001;
const uint32_t RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED = RS_FEED_TYPE_SECURITY_IP  | 0x0002;

const uint32_t RS_FEED_ITEM_CHANNEL_NEW      = RS_FEED_TYPE_CHANNEL  | 0x0001;
//const uint32_t RS_FEED_ITEM_CHANNEL_UPDATE   = RS_FEED_TYPE_CHANNEL  | 0x0002;
const uint32_t RS_FEED_ITEM_CHANNEL_MSG      = RS_FEED_TYPE_CHANNEL  | 0x0003;
const uint32_t RS_FEED_ITEM_CHANNEL_PUBLISHKEY = RS_FEED_TYPE_CHANNEL  | 0x0004;

const uint32_t RS_FEED_ITEM_FORUM_NEW        = RS_FEED_TYPE_FORUM | 0x0001;
//const uint32_t RS_FEED_ITEM_FORUM_UPDATE     = RS_FEED_TYPE_FORUM | 0x0002;
const uint32_t RS_FEED_ITEM_FORUM_MSG        = RS_FEED_TYPE_FORUM | 0x0003;
const uint32_t RS_FEED_ITEM_FORUM_PUBLISHKEY = RS_FEED_TYPE_FORUM | 0x0004;

//const uint32_t RS_FEED_ITEM_BLOG_NEW         = RS_FEED_TYPE_BLOG  | 0x0001;
//const uint32_t RS_FEED_ITEM_BLOG_UPDATE      = RS_FEED_TYPE_BLOG  | 0x0002;
//const uint32_t RS_FEED_ITEM_BLOG_MSG         = RS_FEED_TYPE_BLOG  | 0x0003;

const uint32_t RS_FEED_ITEM_POSTED_NEW       = RS_FEED_TYPE_POSTED  | 0x0001;
//const uint32_t RS_FEED_ITEM_POSTED_UPDATE    = RS_FEED_TYPE_POSTED  | 0x0002;
const uint32_t RS_FEED_ITEM_POSTED_MSG       = RS_FEED_TYPE_POSTED  | 0x0003;

const uint32_t RS_FEED_ITEM_CHAT_NEW         = RS_FEED_TYPE_CHAT  | 0x0001;
const uint32_t RS_FEED_ITEM_MESSAGE          = RS_FEED_TYPE_MSG   | 0x0001;
const uint32_t RS_FEED_ITEM_FILES_NEW        = RS_FEED_TYPE_FILES | 0x0001;

const uint32_t RS_FEED_ITEM_CIRCLE_MEMB_REQ      = RS_FEED_TYPE_CIRCLE  | 0x0001;
const uint32_t RS_FEED_ITEM_CIRCLE_INVIT_REC     = RS_FEED_TYPE_CIRCLE  | 0x0002;
const uint32_t RS_FEED_ITEM_CIRCLE_MEMB_LEAVE    = RS_FEED_TYPE_CIRCLE  | 0x0003;
const uint32_t RS_FEED_ITEM_CIRCLE_MEMB_JOIN     = RS_FEED_TYPE_CIRCLE  | 0x0004;
const uint32_t RS_FEED_ITEM_CIRCLE_MEMB_REVOKED  = RS_FEED_TYPE_CIRCLE  | 0x0005;

const uint32_t RS_MESSAGE_CONNECT_ATTEMPT    = 0x0001;

const int NOTIFY_LIST_NEIGHBOURS             = 1;
const int NOTIFY_LIST_FRIENDS                = 2;
const int NOTIFY_LIST_SEARCHLIST             = 4;
const int NOTIFY_LIST_MESSAGELIST            = 5;
const int NOTIFY_LIST_CHANNELLIST            = 6;
const int NOTIFY_LIST_TRANSFERLIST           = 7;
const int NOTIFY_LIST_CONFIG                 = 8;
const int NOTIFY_LIST_DIRLIST_LOCAL          = 9;
const int NOTIFY_LIST_DIRLIST_FRIENDS        = 10;
const int NOTIFY_LIST_FORUMLIST_LOCKED       = 11; // use connect with Qt::QueuedConnection
const int NOTIFY_LIST_MESSAGE_TAGS           = 12;
const int NOTIFY_LIST_PUBLIC_CHAT            = 13;
const int NOTIFY_LIST_PRIVATE_INCOMING_CHAT  = 14;
const int NOTIFY_LIST_PRIVATE_OUTGOING_CHAT  = 15;
const int NOTIFY_LIST_GROUPLIST              = 16;
const int NOTIFY_LIST_CHANNELLIST_LOCKED     = 17; // use connect with Qt::QueuedConnection
const int NOTIFY_LIST_CHAT_LOBBY_INVITATION  = 18;
const int NOTIFY_LIST_CHAT_LOBBY_LIST        = 19;

const int NOTIFY_TYPE_SAME   = 0x01;
const int NOTIFY_TYPE_MOD    = 0x02; /* general purpose, check all */
const int NOTIFY_TYPE_ADD    = 0x04; /* flagged additions */
const int NOTIFY_TYPE_DEL    = 0x08; /* flagged deletions */

const uint32_t NOTIFY_HASHTYPE_EXAMINING_FILES = 1; /* Examining shared files */
const uint32_t NOTIFY_HASHTYPE_FINISH          = 2; /* Finish */
const uint32_t NOTIFY_HASHTYPE_HASH_FILE       = 3; /* Hashing file */
const uint32_t NOTIFY_HASHTYPE_SAVE_FILE_INDEX = 4; /* Hashing file */

class RS_DEPRECATED RsFeedItem
{
	public:
		RsFeedItem(uint32_t type, const std::string& id1, const std::string& id2, const std::string& id3, const std::string& id4, uint32_t result1)
			:mType(type), mId1(id1), mId2(id2), mId3(id3), mId4(id4), mResult1(result1) {}

		RsFeedItem() :mType(0), mResult1(0) { return; }

		uint32_t mType;
		std::string mId1, mId2, mId3, mId4;
		uint32_t mResult1;
};

// This class implements a generic notify client. To have your own components being notified by
// the Retroshare library, sub-class NotifyClient, and overload the methods you want to make use of
// (The other methods will just ignore the call), and register your ownclient into RsNotify, as:
//
//       	myNotifyClient: public NotifyClient
//       	{
//       		public:
//       			virtual void void notifyPeerHasNewAvatar(std::string peer_id) 
//       			{
//       				doMyOwnThing() ;
//       			}
//       	}
//      
//       	myNotifyClient *client = new myNotifyClient() ;
//      
//       	rsNotify->registerNotifyClient(client) ;
//
// This mechanism can be used in plugins, new services, etc.
//	

class RS_DEPRECATED NotifyClient;

class RS_DEPRECATED_FOR(RsEvents) RsNotify
{
	public:
		/* registration of notifies clients */
		virtual void registerNotifyClient(NotifyClient *nc) = 0;
        /* returns true if NotifyClient was found */
        virtual bool unregisterNotifyClient(NotifyClient *nc) = 0;

		/* Pull methods for retroshare-gui                   */
		/* this should probably go into a different service. */
		 
		virtual bool NotifySysMessage(uint32_t &sysid, uint32_t &type, std::string &title, std::string &msg) = 0;
		virtual bool NotifyPopupMessage(uint32_t &ptype, std::string &name, std::string &title, std::string &msg) = 0;
		virtual bool NotifyLogMessage(uint32_t &sysid, uint32_t &type, std::string &title, std::string &msg) = 0;

		virtual bool GetFeedItem(RsFeedItem &item) = 0;

		virtual bool cachePgpPassphrase (const std::string& /* pgp_passphrase */) { return false ; }
		virtual bool clearPgpPassphrase () { return false ; }

		virtual bool setDisableAskPassword (const bool /*bValue*/) { return false ; }
};

class RS_DEPRECATED NotifyClient
{
public:
	NotifyClient() {}
	virtual ~NotifyClient() {}

	virtual void notifyListPreChange              (int /* list */, int /* type */) {}
	virtual void notifyListChange                 (int /* list */, int /* type */) {}
	virtual void notifyErrorMsg                   (int /* list */, int /* sev  */, std::string /* msg */) {}
	virtual void notifyChatMessage                (const ChatMessage& /* msg      */) {}
	virtual void notifyChatStatus                 (const ChatId&      /* chat_id  */, const std::string& /* status_string */) {}
	virtual void notifyChatCleared                (const ChatId&      /* chat_id  */) {}
	virtual void notifyChatLobbyEvent             (uint64_t           /* lobby id */, uint32_t           /* event type    */ ,const RsGxsId& /* nickname */,const std::string& /* any string */) {}
	virtual void notifyChatLobbyTimeShift         (int                /* time_shift*/) {}
	virtual void notifyCustomState                (const std::string& /* peer_id   */, const std::string&               /* status_string */) {}
	virtual void notifyHashingInfo                (uint32_t           /* type      */, const std::string&               /* fileinfo      */) {}
	virtual void notifyTurtleSearchResult         (const RsPeerId&    /* pid       */, uint32_t                         /* search_id     */, const std::list<TurtleFileInfo>& /* files         */) {}
	virtual void notifyPeerHasNewAvatar           (std::string        /* peer_id   */) {}
	virtual void notifyOwnAvatarChanged           () {}
	virtual void notifyOwnStatusMessageChanged    () {}
	virtual void notifyDiskFull                   (uint32_t           /* location  */, uint32_t                         /* size limit in MB */) {}
	virtual void notifyPeerStatusChanged          (const std::string& /* peer_id   */, uint32_t                         /* status           */) {}

	/* one or more peers has changed the states */
	virtual void notifyPeerStatusChangedSummary   () {}
	virtual void notifyDiscInfoChanged            () {}

	virtual void notifyDownloadComplete           (const std::string& /* fileHash  */) {}
	virtual void notifyDownloadCompleteCount      (uint32_t           /* count     */) {}
	virtual void notifyHistoryChanged             (uint32_t           /* msgId     */, int /* type */) {}

	virtual bool askForPassword                   (const std::string& /* title     */, const std::string& /* key_details     */, bool               /* prev_is_bad */, std::string& /* password */,bool& /* cancelled */ ) { return false ;}
	virtual bool askForPluginConfirmation         (const std::string& /* plugin_filename */, const std::string& /* plugin_file_hash */,bool /* first_time */) { return false ;}
};
