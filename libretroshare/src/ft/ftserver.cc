/*
 * libretroshare/src/ft: ftserver.cc
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

	/* Setup */
ftServer::ftServer(CacheStrapper *cStrapper, p3ConnectMgr *connMgr)
	:mCacheStrapper(cStrapper), mConnMgr(connMgr)
{

	/* Final Setup (once everything is assigned) */
ftServer::SetupFtServer()
{
	/* make Controller */
	mFtController = new ftController();
	NotifyBase *cb = getNotify();

	/* setup FiStore/Monitor */
	std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";
	std::string ownId = mConnMgr->getOwnId();

	mFiStore = new FileIndexStore(mCacheStrapper, mFtController, cb, ownId, remotecachedir);
        mFiMon = new FileIndexMonitor(mCacheStrapper, localcachedir, ownId);

	/* now add the set to the cachestrapper */
	CachePair cp(mFiMon, mFiStore, CacheId(RS_SERVICE_TYPE_FILE_INDEX, 0));
	mCacheStrapper -> addCachePair(cp);

	/* extras List */
	mFtExtra = new ftExtraList();
	mFtSearch = new ftFileSearch(mCacheStrapper, mFtExtra, mFiMon, mFiStore);

	mFtController -> setFtSearch(mFtSearch);
	ftFiler -> setSaveBasePath(save_dir);
	
	return;
}

	/* Assign important variables */
void	setConfigDirectory(std::string path);

void	setPQInterface(PQInterface *pqi);

	/* Final Setup (once everything is assigned) */
void	SetupFtServer();

	/* add Config Items (Extra, Controller) */
void	addConfigComponents(p3ConfigMgr *mgr);


void	ftServer::setConfigDirectory(std::string path)
{
	mConfigPath = path;
}

void	ftServer::setPQInterface(PQInterface *pqi)

	/* Control Interface */

};


void    ftServer::StartupThreads()
{
	/* start up Controller thread */

	/* start own thread */
}

CacheStrapper *ftServer::getCacheStrapper()
{
	return mCacheStrapper;
}

CacheTransfer *ftServer::getCacheTransfer()
{
	return mFtController;
}

	/***************************************************************/
	/********************** RsFiles Interface **********************/
	/***************************************************************/


	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool ftServer::FileRequest(std::string fname, std::string hash, 
				uint32_t size, std::string dest, uint32_t flags)
{
	return mFtController->FileRequest(fname, hash, size, dest, flags);
}

bool ftServer::FileCancel(std::string hash);
{
	return mFtController->FileCancel(hash);
}

bool ftServer::FileControl(std::string hash, uint32_t flags);
{
	return mFtController->FileControl(hash, flags);
}

bool ftServer::FileClearCompleted();
{
	return mFtController->FileClearCompleted();
}

	/* get Details of File Transfers */
bool ftServer::FileDownloads(std::list<std::string> &hashs);
{
	return mFtController->FileDownloads(hashs);
}

	/* Directory Handling */
void ftServer::setDownloadDirectory(std::string path)
{
	return mFtController->setDownloadDirectory(path);
}

std::string ftServer::getDownloadDirectory()
{
	return mFtController->getDownloadDirectory();
}

void ftServer::setPartialsDirectory(std::string path);
{
	return mFtController->setPartialsDirectory(path);
}

void ftServer::getPartialsDirectory()
{
	return mFtController->getPartialsDirectory();
}


	/***************************************************************/
	/************************* Other Access ************************/
	/***************************************************************/

bool ftServer::FileUploads(std::list<std::string> &hashs);
{
	return mFtUploader->FileUploads(hashes);
}

bool ftServer::FileDetails(std::string hash, uint32_t hintflags, FileInfo &info);
{
	bool found = false;
	if (hintflags | DOWNLOADING)
	{
		found = mFtController->FileDetails(hash, info);
	}
	else if (hintflags | UPLOADING)
	{
		found = mFtUploader->FileDetails(hash, info);
	}

	if (!found)
	{
		mFtSearch->FileDetails(hash, hintflags, info);
	}
	return found;
}

	/***************************************************************/
	/******************* ExtraFileList Access **********************/
	/***************************************************************/

bool  ftServer::ExtraFileAdd(std::string fname, std::string hash, uint32_t size, 
						uint32_t period, uint32_t flags)
{
	return mFtExtra->addExtraFile(fname, hash, size, period, flags);
}

bool ftServer::ExtraFileRemove(std::string hash, uin32_t flags);
{
	return mFtExtra->removeExtraFile(hash, flags);
}

bool ftServer::ExtraFileHash(std::string localpath, uint32_t period, uint32_t flags);
{
	return mFtExtra->hashExtraFile(localpath, period, flags);
}

bool ftServer::ExtraFileStatus(std::string localpath, FileInfo &info);
{
	return mFtExtra->hashExtraFileDone(localpath, info);
}

	/***************************************************************/
	/******************** Directory Listing ************************/
	/***************************************************************/

int ftServer::RequestDirDetails(std::string uid, std::string path, DirDetails &details);
{
	return mFiStore->RequestDirDetails(uid, path, details);
}
	
int ftServer::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags);
{
	return mFiStore->RequestDirDetails(ref, details, flags);
}
	
	/***************************************************************/
	/******************** Search Interface *************************/
	/***************************************************************/


int ftServer::SearchKeywords(std::list<std::string> keywords, std::list<FileDetail> &results);
{
	return mFiStore->SearchKeywords(keywords, results);
}
	
int ftServer::SearchBoolExp(Expression * exp, std::list<FileDetail> &results);
{
	return mFiStore->searchBoolExp(exp, results);
}

	
	/***************************************************************/
	/*************** Local Shared Dir Interface ********************/
	/***************************************************************/
	
bool    ftServer::ConvertSharedFilePath(std::string path, std::string &fullpath)
{
	return mFiMon->convertSharedFilePath(path, fullpath);
}
	
void    ftServer::ForceDirectoryCheck()
{
	mFiMon->forceDirectoryCheck();
	return;
}
	
bool    ftServer::InDirectoryCheck()
{
	return mFtMon->inDirectoryCheck();
}
	
bool	ftServer::getSharedDirectories(std::list<std::string> &dirs)
{
	return mFtMon->getSharedDirectories(dirs);
}

bool 	ftServer::addSharedDirectory(std::string dir)
{
	return mFtMon->addSharedDirectory(dir);
}

bool 	ftServer::removeSharedDirectory(std::string dir)
{
	return mFtMon->removeSharedDirectory(dir);
}


	/***************************************************************/
	/****************** End of RsFiles Interface *******************/
	/***************************************************************/


	/***************************************************************/
	/**************** Config Interface *****************************/
	/***************************************************************/

	protected:
        /* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);

	private:
bool  loadConfigMap(std::map<std::string, std::string> &configMap);
        /******************* p3 Config Overload ************************/

};



#endif
