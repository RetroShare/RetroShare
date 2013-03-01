#ifndef RETROSHARE_GUI_INTERFACE_H
#define RETROSHARE_GUI_INTERFACE_H

/*
 * "$Id: rsiface.h,v 1.9 2007-04-21 19:08:51 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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


#include "rstypes.h"

#include <map>

class NotifyBase;
class RsIface;
class RsControl;
class RsInit;
struct TurtleFileInfo ;

/* declare single RsIface for everyone to use! */

extern RsIface   *rsiface;
extern RsControl *rsicontrol;

/* RsInit -> Configuration Parameters for RetroShare Startup
 */

RsInit *InitRsConfig();
/* extract various options for GUI */
const char   *RsConfigDirectory(RsInit *config);
bool    RsConfigStartMinimised(RsInit *config);
void    CleanupRsConfig(RsInit *);


// Called First... (handles comandline options) 
int InitRetroShare(int argc, char **argv, RsInit *config);

// This Functions are used for Login.
bool ValidateCertificate(RsInit *config, std::string &userName);
bool ValidateTrustedUser(RsInit *config, std::string fname, std::string &userName);
bool LoadPassword(RsInit *config, std::string passwd);
bool RsGenerateCertificate(RsInit *config, std::string name, std::string org,
                        std::string loc, std::string country, std::string passwd, std::string &errString);

/* Auto Login Fns */
bool  RsTryAutoLogin(RsInit *config);
bool  RsStoreAutoLogin(RsInit *config);
bool  RsClearAutoLogin(std::string basedir);

// Handle actual Login.
int LoadCertificates(RsInit *config, bool autoLoginNT);

RsIface   *createRsIface  (NotifyBase &notify);
RsControl *createRsControl(RsIface &iface, NotifyBase &notify);


class RsIface /* The Main Interface Class - create a single one! */
{
public:
	RsIface(NotifyBase &callback)
	:cb(callback) { return; }
	virtual ~RsIface() { return; }

/****************************************/

	/* Stubs for Very Important Fns -> Locking Functions */
virtual	void lockData() = 0;
virtual	void unlockData() = 0;

	const RsConfig &getConfig()
		{ return mConfig; }
/****************************************/


	/* Flags to indicate used or not */
	enum DataFlags
	{
		Neighbour = 0,
		Friend = 1,
		DirLocal = 2,  /* Not Used - QModel instead */
		DirRemote = 3, /* Not Used - QModel instead */
		Transfer = 4,
		Message = 5,
		Channel = 6,
		Chat = 7,
		Recommend = 8,
		Config = 9,
		NumOfFlags = 10
	};


	/* 
	 * Operations for flags
	 */

bool	setChanged(DataFlags set); /* set to true */
bool	getChanged(DataFlags set); /* leaves it */
bool	hasChanged(DataFlags set); /* resets it */

	private:

void	fillLists(); /* create some dummy data to display */

		/* Internals */
	bool mChanged[NumOfFlags];

	RsConfig mConfig;

	NotifyBase &cb;

	/* Classes which can update the Lists! */
	friend class RsControl;
	friend class RsServer; 
};


class RsControl /* The Main Interface Class - for controlling the server */
{
	public:

		RsControl(RsIface &i, NotifyBase &callback)
			:cb(callback), rsIface(i) { return; }

		virtual ~RsControl() { return; }

		/* Real Startup Fn */
		virtual int StartupRetroShare() = 0;

		/****************************************/

		/* Flagging Persons / Channels / Files in or out of a set (CheckLists) */
		virtual int 	SetInBroadcast(std::string id, bool in) = 0;	/* channel : channel broadcast */
		virtual int 	SetInSubscribe(std::string id, bool in) = 0;	/* channel : subscribed channels */
		virtual int 	ClearInBroadcast() = 0;
		virtual int 	ClearInSubscribe() = 0;

		/****************************************/
		/* Config */

		virtual	int 	ConfigSetBootPrompt( bool on ) = 0;
		virtual void    ConfigFinalSave( ) 			   = 0;
		virtual void 	rsGlobalShutDown( )			   = 0;

		/****************************************/

		NotifyBase &getNotify() { return cb; }
		RsIface    &getIface()  { return rsIface; }

	private:
		NotifyBase &cb;
		RsIface    &rsIface;
};


/********************** Overload this Class for callback *****************/


class NotifyBase
{
	public:
	NotifyBase() { return; }
	virtual ~NotifyBase() { return; }
	virtual void notifyListPreChange(int list, int type) { (void) list; (void) type; return; }
	virtual void notifyListChange(int list, int type) { (void) list; (void) type; return; }
	virtual void notifyErrorMsg(int list, int sev, std::string msg) { (void) list; (void) sev; (void) msg; return; }
	virtual void notifyChatStatus(const std::string& /* peer_id */, const std::string& /* status_string */ ,bool /* is_private */) {}
	virtual void notifyChatLobbyEvent(uint64_t /* lobby id */,uint32_t /* event type */,const std::string& /* nickname */,const std::string& /* any string */) {}
	virtual void notifyCustomState(const std::string& /* peer_id */, const std::string& /* status_string */) {}
	virtual void notifyHashingInfo(uint32_t type, const std::string& fileinfo) { (void) type; (void)fileinfo; }
	virtual void notifyTurtleSearchResult(uint32_t /* search_id */ ,const std::list<TurtleFileInfo>& files) { (void)files; }
	virtual void notifyPeerHasNewAvatar(std::string peer_id) { (void)peer_id; }
	virtual void notifyOwnAvatarChanged() {}
	virtual void notifyOwnStatusMessageChanged() {}
	virtual void notifyDiskFull(uint32_t /* location */,uint32_t /* size limit in MB */) {}
	/* peer has changed the status */
	virtual void notifyPeerStatusChanged(const std::string& /* peer_id */, uint32_t /* status */) {}
	/* one or more peers has changed the states */
	virtual void notifyPeerStatusChangedSummary() {}
	virtual void notifyForumMsgReadSatusChanged(const std::string& /* channelId */, const std::string& /* msgId */, uint32_t /* status */) {}
	virtual void notifyChannelMsgReadSatusChanged(const std::string& /* channelId */, const std::string& /* msgId */, uint32_t /* status */) {}
	virtual void notifyDiscInfoChanged() {}
	virtual void notifyDownloadComplete(const std::string& /* fileHash */) {};
	virtual void notifyDownloadCompleteCount(uint32_t /* count */) {};
	virtual void notifyHistoryChanged(uint32_t /* msgId */, int /* type */) {}

	virtual bool askForPassword(const std::string& /* key_details */, bool /* prev_is_bad */, std::string& /* password */ ) { return false ;}
	virtual bool askForPluginConfirmation(const std::string& /* plugin_filename */, const std::string& /* plugin_file_hash */) { return false ;}
	virtual bool askForDeferredSelfSignature(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,int& signature_result) { signature_result = false ;return true; }

};

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

#endif
