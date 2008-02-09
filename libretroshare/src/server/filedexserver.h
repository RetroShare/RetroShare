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

#include "pqi/pqi.h"
#include "pqi/pqiindic.h"
#include "serialiser/rsconfigitems.h"
#include <map>
#include <deque>
#include <list>
#include <map>
#include <iostream>

#include "rsiface/rsiface.h"

#include "pqi/p3cfgmgr.h"

class p3ConnectMgr;
class p3AuthMgr;

class CacheStrapper;
class ftfiler;
class FileIndexStore;
class FileIndexMonitor;

class ftFileRequest;
class ftFileData;

#define MAX_RESULTS 100 // nice balance between results and traffic.

class filedexserver: public p3Config
{
	public:
	filedexserver();

void    loadWelcomeMsg(); /* startup message */

int	setSearchInterface(P3Interface *si, p3AuthMgr *am, p3ConnectMgr *cm);

std::list<RsFileTransfer *> getTransfers();

void 	saveFileTransferStatus();
int     getFile(std::string fname, std::string hash,
                        uint32_t size, std::string dest);
void 	clear_old_transfers();
void 	cancelTransfer(std::string fname, std::string hash, uint32_t size);

// access to search info is also required.

std::list<std::string> &getSearchDirectories();
int 	addSearchDirectory(std::string dir);
int 	removeSearchDirectory(std::string dir);
int 	reScanDirs();
int 	check_dBUpdate();

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

	P3Interface *pqisi;
	p3AuthMgr    *mAuthMgr;
	p3ConnectMgr *mConnMgr;

std::string config_dir;
std::string save_dir;
bool	save_inc; // is savedir include in share list.

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

/*************************** p3 Config Overload ********************/
	protected:
        /* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);

	private:
bool  loadConfigMap(std::map<std::string, std::string> &configMap);

/*************************** p3 Config Overload ********************/

	/* new FileCache stuff */
	public:

int 	FileStoreTick();
int 	FileCacheSave();

	/* Setup */
void 	initialiseFileStore();
void    setFileCallback(std::string ownId, CacheStrapper *strapper, 
				ftfiler *ft, NotifyBase *cb);
void    StartupMonitor();

	/* Controls */
int RequestDirDetails(std::string uid, std::string path, DirDetails &details);
int RequestDirDetails(void *ref, DirDetails &details, uint32_t flags);

int SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results);
int SearchBoolExp(Expression * exp, std::list<FileDetail> &results);

	private:

void 	SendFileRequest(ftFileRequest *ftr, std::string pid);
void 	SendFileData(ftFileData *ftd, std::string pid);

	CacheStrapper *mCacheStrapper;
	ftfiler 	*ftFiler;
        FileIndexStore *fiStore;
	FileIndexMonitor *fimon;

	/* Temp Transfer List (for loading config) */
	std::list<RsFileTransfer *> mResumeTransferList;
};

#endif // MRK_PQI_FILEDEX_SERVER_HEADER
