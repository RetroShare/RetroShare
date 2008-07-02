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

class ftController: public CacheTransfer, public RsThread, public pqiMonitor, public p3Config
{
	public:

	/* Setup */
	ftController(std::string configDir);

void	setFtSearch(ftSearch *);

virtual void run();

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool 	FileRequest(std::string fname, std::string hash, 
			uint32_t size, std::string dest, uint32_t flags);

bool 	FileCancel(std::string hash);
bool 	FileControl(std::string hash, uint32_t flags);
bool 	FileClearCompleted();

	/* get Details of File Transfers */
bool 	FileDownloads(std::list<std::string> &hashs);

	/* Directory Handling */
void 	setDownloadDirectory(std::string path);
void 	setPartialsDirectory(std::string path);
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

	/* pointers to other components */

	ftSearch *mSearch; 

	RsMutex ctrlMutex;

	std::list<FileDetails> incomingQueue;
	std::map<std::string, FileDetails> mCompleted;

	std::map<std::string, ftTransferModule *> mTransfers;
	std::map<std::string, ftFileCreator *> mFileCreators;

	std::string mConfigPath;
	std::string mDownloadPath;
	std::string mPartialPath;
};

#endif
