/*******************************************************************************
 * libretroshare/src/pqi: p3notify.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2007-2008  Robert Fernie                                      *
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
#include "pqi/p3notify.h"
#include <stdint.h>
#include <algorithm>

RsNotify *rsNotify = NULL ;

/* Output for retroshare-gui */
bool p3Notify::NotifySysMessage(uint32_t &sysid, uint32_t &type, 
					std::string &title, std::string &msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	if (pendingSysMsgs.size() > 0)
	{
		p3NotifySysMsg smsg = pendingSysMsgs.front();
		pendingSysMsgs.pop_front();

		sysid = smsg.sysid;
		type = smsg.type;
		title = smsg.title;
		msg = smsg.msg;

		return true;
	}

	return false;
}

	/* Output for retroshare-gui */
bool p3Notify::NotifyLogMessage(uint32_t &sysid, uint32_t &type,
					std::string &title, std::string &msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	if (pendingLogMsgs.size() > 0)
	{
		p3NotifyLogMsg smsg = pendingLogMsgs.front();
		pendingLogMsgs.pop_front();

		sysid = smsg.sysid;
		type = smsg.type;
		title = smsg.title;
		msg = smsg.msg;

		return true;
	}

	return false;
}


bool p3Notify::NotifyPopupMessage(uint32_t &ptype, std::string &name, std::string &title, std::string &msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	if (pendingPopupMsgs.size() > 0)
	{
		p3NotifyPopupMsg pmsg = pendingPopupMsgs.front();
		pendingPopupMsgs.pop_front();

		ptype = pmsg.type;
		name = pmsg.name;
                title = pmsg.title;
		msg = pmsg.msg;

		return true;
	}

	return false;
}


	/* Control over Messages */
bool p3Notify::GetSysMessageList(std::map<uint32_t, std::string> &list)
{
	(void) list; /* suppress unused parameter warning */	
	return false;
}

bool p3Notify::GetPopupMessageList(std::map<uint32_t, std::string> &list)
{
	(void) list; /* suppress unused parameter warning */	
	return false;
}


bool p3Notify::SetSysMessageMode(uint32_t sysid, uint32_t mode)
{
	(void) sysid; /* suppress unused parameter warning */	
	(void) mode; /* suppress unused parameter warning */	
	return false;
}

bool p3Notify::SetPopupMessageMode(uint32_t ptype, uint32_t mode)
{
	(void) ptype; /* suppress unused parameter warning */	
	(void) mode; /* suppress unused parameter warning */	
	return false;
}


	/* Input from libretroshare */
bool p3Notify::AddPopupMessage(uint32_t ptype, const std::string& name, const std::string& title, const std::string& msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/

	p3NotifyPopupMsg pmsg;

	pmsg.type = ptype;
	pmsg.name = name;
	pmsg.title = title;
	pmsg.msg = msg;

	pendingPopupMsgs.push_back(pmsg);

	return true;
}


bool p3Notify::AddSysMessage(uint32_t sysid, uint32_t type, const std::string& title, const std::string& msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/

	p3NotifySysMsg smsg;

	smsg.sysid = sysid;
	smsg.type = type;
	smsg.title = title;
	smsg.msg = msg;

	pendingSysMsgs.push_back(smsg);

	return true;
}

bool p3Notify::AddLogMessage(uint32_t sysid, uint32_t type, const std::string& title, const std::string& msg)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/

	p3NotifyLogMsg smsg;

	smsg.sysid = sysid;
	smsg.type = type;
	smsg.title = title;
	smsg.msg = msg;

	pendingLogMsgs.push_back(smsg);

	return true;
}


bool p3Notify::GetFeedItem(RsFeedItem &item)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	if (pendingNewsFeed.size() > 0)
	{
		item = pendingNewsFeed.front();
		pendingNewsFeed.pop_front();

		return true;
	}

	return false;
}


bool p3Notify::AddFeedItem(uint32_t type, const std::string& id1, const std::string& id2, const std::string& id3, const std::string& id4, uint32_t result1)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/
	pendingNewsFeed.push_back(RsFeedItem(type, id1, id2, id3, id4, result1));

	return true;
}

bool p3Notify::ClearFeedItems(uint32_t type)
{
	RsStackMutex stack(noteMtx); /************* LOCK MUTEX ************/

	std::list<RsFeedItem>::iterator it;
	for(it = pendingNewsFeed.begin(); it != pendingNewsFeed.end(); )
	{
		if (it->mType == type)
		{
			it = pendingNewsFeed.erase(it);
		}
		else
		{
			++it;
		}
	}
	return true;
}

#define FOR_ALL_NOTIFY_CLIENTS for(std::list<NotifyClient*>::const_iterator it(notifyClients.begin());it!=notifyClients.end();++it)

void p3Notify::notifyChatLobbyEvent(uint64_t lobby_id, uint32_t event_type,const RsGxsId& nickname,const std::string& any_string) { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyChatLobbyEvent(lobby_id,event_type,nickname,any_string) ; }

void p3Notify::notifyListPreChange(int list, int type) { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyListPreChange(list,type) ; }
void p3Notify::notifyListChange   (int list, int type) { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyListChange   (list,type) ; }

void p3Notify::notifyErrorMsg      (int list, int sev, std::string msg)                                                         { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyErrorMsg(list,sev,msg) ; }
void p3Notify::notifyChatMessage   (const ChatMessage &msg)                                                                     { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyChatMessage(msg) ; }
void p3Notify::notifyChatStatus    (const ChatId&  chat_id, const std::string& status_string)                                   { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyChatStatus(chat_id,status_string) ; }
void p3Notify::notifyChatCleared   (const ChatId&  chat_id)                                                                     { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyChatCleared(chat_id) ; }

void p3Notify::notifyChatLobbyTimeShift     (int                time_shift)                                                     { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyChatLobbyTimeShift(time_shift) ; }
void p3Notify::notifyCustomState            (const std::string& peer_id   , const std::string&               status_string )    { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyCustomState       (peer_id,status_string) ; }
void p3Notify::notifyHashingInfo            (uint32_t           type      , const std::string&               fileinfo      )    { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyHashingInfo       (type,fileinfo) ; } 
void p3Notify::notifyTurtleSearchResult     (const RsPeerId& pid          , uint32_t search_id , const std::list<TurtleFileInfo>& files         )    { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyTurtleSearchResult(pid,search_id,files) ; }
#warning MISSING CODE HERE
//void p3Notify::notifyTurtleSearchResult     (uint32_t           search_id , const std::list<TurtleGxsInfo>&  groups        )    { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyTurtleSearchResult(search_id,groups) ; }
void p3Notify::notifyPeerHasNewAvatar       (std::string        peer_id   )                                                     { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyPeerHasNewAvatar(peer_id) ; }
void p3Notify::notifyOwnAvatarChanged       ()                                                                                  { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyOwnAvatarChanged() ; } 
void p3Notify::notifyOwnStatusMessageChanged()                                                                                  { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyOwnStatusMessageChanged() ; } 
void p3Notify::notifyDiskFull               (uint32_t           location  , uint32_t                         size_limit_in_MB ) { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyDiskFull          (location,size_limit_in_MB) ; }
void p3Notify::notifyPeerStatusChanged      (const std::string& peer_id   , uint32_t                         status           ) { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyPeerStatusChanged (peer_id,status) ; }

void p3Notify::notifyPeerStatusChangedSummary   ()                                                                              { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyPeerStatusChangedSummary() ; }
void p3Notify::notifyDiscInfoChanged            ()                                                                              { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyDiscInfoChanged         () ; } 

void p3Notify::notifyDownloadComplete           (const std::string& fileHash )                                                  { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyDownloadComplete           (fileHash) ; }
void p3Notify::notifyDownloadCompleteCount      (uint32_t           count    )                                                  { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyDownloadCompleteCount      (count) ; }
void p3Notify::notifyHistoryChanged             (uint32_t           msgId    , int type)                                        { FOR_ALL_NOTIFY_CLIENTS (*it)->notifyHistoryChanged             (msgId,type) ; }

bool p3Notify::cachePgpPassphrase(const std::string& s)
{
    clearPgpPassphrase() ;
    cached_pgp_passphrase = s ;

    std::cerr << "(WW) Caching PGP passphrase." << std::endl;
    return true ;
}
bool p3Notify::clearPgpPassphrase()
{
    std::cerr << "(WW) Clearing PGP passphrase." << std::endl;

    // Just whipe out the memory instead of just releasing it.

    for(uint32_t i=0;i<cached_pgp_passphrase.length();++i)
        cached_pgp_passphrase[i] = 0 ;

    cached_pgp_passphrase.clear();
    return true ;
}

bool p3Notify::setDisableAskPassword(const bool bValue)
{
	_disableAskPassword = bValue;
	return true;
}

bool p3Notify::askForPassword                   (const std::string& title    , const std::string& key_details    , bool               prev_is_bad , std::string& password,bool *cancelled)
{
    if(!prev_is_bad && !cached_pgp_passphrase.empty())
    {
        password = cached_pgp_passphrase ;
        if(cancelled)
			*cancelled = false ;
        return true ;
    }

	FOR_ALL_NOTIFY_CLIENTS
		if (!_disableAskPassword)
			if( (*it)->askForPassword(title,key_details,prev_is_bad,password,*cancelled) )
				return true;

	return false ;
}
bool p3Notify::askForPluginConfirmation         (const std::string& plugin_filename, const std::string& plugin_file_hash,bool first_time)
{
	FOR_ALL_NOTIFY_CLIENTS
		if( (*it)->askForPluginConfirmation(plugin_filename,plugin_file_hash,first_time))
			return true ;

	return false ;
}

void p3Notify::registerNotifyClient(NotifyClient *cl)
{
	notifyClients.push_back(cl) ;
}

bool p3Notify::unregisterNotifyClient(NotifyClient *nc)
{
    std::list<NotifyClient*>::iterator it = std::find(notifyClients.begin(), notifyClients.end(), nc);
    if(it != notifyClients.end())
    {
        notifyClients.erase(it);
        return true;
    }
    else
    {
        return false;
    }
}
