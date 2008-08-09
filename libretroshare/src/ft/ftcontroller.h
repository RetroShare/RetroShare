/*
 * libretroshare/src/ft: ftcontroller.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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

#ifndef FT_CONTROLLER_HEADER
#define FT_CONTROLLER_HEADER

/* 
 * ftController
 *
 * Top level download controller.
 *
 * inherits configuration (save downloading files)
 * inherits pqiMonitor (knows which peers are online).
 * inherits CacheTransfer (transfers cache files too)
 * inherits RsThread (to control transfers)
 *
 */

class ftFileCreator;
class ftTransferModule;
class ftFileProvider;
class ftSearch;
class ftDataMultiplex;

#include "dbase/cachestrapper.h"
#include "util/rsthreads.h"
#include "pqi/pqimonitor.h"
#include "pqi/p3cfgmgr.h"

#include "rsiface/rsfiles.h"

#include <map>

class ftFileControl
{
	public:

	ftFileControl();
        ftFileControl(std::string fname, uint64_t size, std::string hash, 
		uint32_t flags, ftFileCreator *fc, ftTransferModule *tm);

	ftTransferModule * mTransfer;
	ftFileCreator *    mCreator;
	uint32_t	   mState;
	std::string	   mHash;
	std::string	   mName;
	uint64_t	   mSize;
};


class ftController: public CacheTransfer, public RsThread, public pqiMonitor, public p3Config
{
	public:

	/* Setup */
	ftController(CacheStrapper *cs, ftDataMultiplex *dm, std::string configDir);

void	setFtSearch(ftSearch *);

virtual void run();

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool 	FileRequest(std::string fname, std::string hash, 
			uint64_t size, std::string dest, uint32_t flags, 
			std::list<std::string> &sourceIds);

bool 	FileCancel(std::string hash);
bool 	FileControl(std::string hash, uint32_t flags);
bool 	FileClearCompleted();

	/* get Details of File Transfers */
bool 	FileDownloads(std::list<std::string> &hashs);

	/* Directory Handling */
bool 	setDownloadDirectory(std::string path);
bool 	setPartialsDirectory(std::string path);
std::string getDownloadDirectory();
std::string getPartialsDirectory();
bool 	FileDetails(std::string hash, FileInfo &info);

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

	/* pqiMonitor callback (also provided mConnMgr pointer!) */
	public:
virtual void    statusChange(const std::list<pqipeer> &plist);


	/* p3Config Interface */
        protected:
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);

	private:

	/* RunTime Functions */
void 	checkDownloadQueue();
bool 	completeFile(std::string hash);

	/* pointers to other components */

	ftSearch *mSearch; 
	ftDataMultiplex *mDataplex;

	RsMutex ctrlMutex;

	std::list<FileInfo> incomingQueue;
	std::map<std::string, FileInfo> mCompleted;


        std::map<std::string, ftFileControl> mDownloads;

	//std::map<std::string, ftTransferModule *> mTransfers;
	//std::map<std::string, ftFileCreator *> mFileCreators;

	std::string mConfigPath;
	std::string mDownloadPath;
	std::string mPartialPath;
};

#endif
