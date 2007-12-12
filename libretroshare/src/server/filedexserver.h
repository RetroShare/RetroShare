/*
 * "$Id: filedexserver.h,v 1.18 2007-05-05 16:10:06 rmf24 Exp $"
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



#ifndef MRK_PQI_FILEDEX_SERVER_HEADER
#define MRK_PQI_FILEDEX_SERVER_HEADER

/* 
 * Slightly more complete server....
 * has a filedex pointer, which manages the local indexing/searching.
 *
 */

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/pqi.h"
#include "pqi/pqiindic.h"
#include "serialiser/rsconfigitems.h"
#include <map>
#include <deque>
#include <list>
#include <map>
#include <iostream>

#include "rsiface/rsiface.h"
class pqimonitor;
class CacheStrapper;
class ftfiler;
class FileIndexStore;
class FileIndexMonitor;

class ftFileRequest;
class ftFileData;


#ifdef PQI_USE_CHANNELS
	#include "pqi/pqichannel.h"
	#include "pqi/p3channel.h"
#endif
	
#define MAX_RESULTS 100 // nice balance between results and traffic.


class filedexserver
{
	public:
	filedexserver();

void    loadWelcomeMsg(); /* startup message */

int	setSearchInterface(P3Interface *si, sslroot *sr);
//int	additem(SearchItem *item, char *fname);

//int	sendChat(std::string msg);
//int	sendPrivateChat(ChatItem *ci);

//int	sendRecommend(PQFileItem *fi, std::string msg);
//int     sendMessage(MsgItem *item);
//int     checkOutgoingMessages();

//int 	getChat();
/* std::deque<std::string> &getChatQueue(); */
//std::list<ChatItem *> getChatQueue(); 

//std::list<MsgItem *> &getMsgList();
//std::list<MsgItem *> &getMsgOutList();
//std::list<MsgItem *> getNewMsgs();


std::list<RsFileTransfer *> getTransfers();

	// get files (obsolete?)
int     getFile(std::string fname, std::string hash,
                        uint32_t size, std::string dest);
void 	clear_old_transfers();
void 	cancelTransfer(std::string fname, std::string hash, uint32_t size);

	// cleaning up....
//int	removeMsgItem(int itemnum);
	// alternative versions.
//int	removeMsgItem(MsgItem *mi);
	// third versions.
//int     removeMsgId(unsigned long mid); /* id stored in sid */
//int     markMsgIdRead(unsigned long mid);

// access to search info is also required.

std::list<std::string> &getSearchDirectories();
int 	addSearchDirectory(std::string dir);
int 	removeSearchDirectory(std::string dir);
int 	reScanDirs();
int 	check_dBUpdate();

int	load_config();
int	save_config();

std::string	getSaveDir();
void		setSaveDir(std::string d);
void		setConfigDir(std::string d) { config_dir = d; }
bool		getSaveIncSearch();
void		setSaveIncSearch(bool v);

int	tick();
int	status();


	private:

int	handleInputQueues();
int	handleOutputQueues();

std::list<std::string> dbase_dirs;

	sslroot	*sslr;

	P3Interface *pqisi;

std::string config_dir;
std::string save_dir;
bool	save_inc; // is savedir include in share list.

// bool state flags.
	public:
//	Indicator msgChanged;
//	Indicator msgMajorChanged;
//	Indicator chatChanged;

#ifdef PQI_USE_CHANNELS

	public:
	// Channel stuff.
//void	setP3Channel(p3channel *p3c);
//int     getAvailableChannels(std::list<pqichannel *> &chans);
//int     getChannelMsgList(channelSign s, std::list<chanMsgSummary> &summary);
//channelMsg *getChannelMsg(channelSign s, MsgHash mh);

//	Indicator channelsChanged; // everything!

	private:
//p3channel *p3chan;

#endif

	public:
	/* some more switches (here for uniform saving) */
int	getDHTEnabled()
	{
		return DHTState;
	}

int 	getUPnPEnabled()
	{
		return UPnPState;
	}

void	setDHTEnabled(int i)
	{
		DHTState = i;
	}

void 	setUPnPEnabled(int i)
	{
		UPnPState = i;
	}

	private:
	int DHTState;
	int UPnPState;


	/* new FileCache stuff */
	public:

int 	FileStoreTick();
int 	FileCacheSave();

void 	initialiseFileStore();
void    setFileCallback(NotifyBase *cb);

int RequestDirDetails(std::string uid, std::string path, DirDetails &details);
int RequestDirDetails(void *ref, DirDetails &details, uint32_t flags);

int SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results);
int SearchBoolExp(Expression * exp, std::list<FileDetail> &results);

	private:

void 	SendFileRequest(ftFileRequest *ftr, cert *peer);
void 	SendFileData(ftFileData *ftd, cert *peer);

	pqimonitor *peerMonitor;
	CacheStrapper *cacheStrapper;
	ftfiler 	*ftFiler;
        FileIndexStore *fiStore;
	FileIndexMonitor *fimon;
};

#endif // MRK_PQI_FILEDEX_SERVER_HEADER
