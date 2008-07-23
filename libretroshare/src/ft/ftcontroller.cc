/*
 * libretroshare/src/ft: ftcontroller.cc
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
	ftController::ftController(std::string configDir);

void	ftController::setFtSearch(ftSearch *search)
{
	mSearch = search;
}


virtual void ftController::run()
{
	/* check the queues */
}



/* Called every 10 seconds or so */
void ftController::checkDownloadQueue()
{
	/* */


}

bool ftController::completeFile(std::string hash)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string, ftFileControl>::iterator it;
	it = mDownloads.find(hash);
	if (it == mDownloads.end())
	{
		return false;
	}

	/* check if finished */
	if (!(it->second).mCreator->finished())
	{
		/* not done! */
		return false;
	}

	ftFileControl *fc = &(it->second);

	/* done - cleanup */
	fc->mTransfer->done();
	mClientModule->removeTransferModule(fc->mTransfer);
	mServerModule->removeFileCreator(fc->mCreator);
	
	delete fc->mTransfer;
	fc->mTransfer = NULL;

	delete fc->mCreator;
	fc->mCreator = NULL;

	fc->state = COMPLETE;

	/* switch map */
	mCompleted[fc->hash] = *fc;
	mDownloads.erase(it);

	return true;
}

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool 	ftController::FileRequest(std::string fname, std::string hash, 
			uint64_t size, std::string dest, uint32_t flags, 
			std::list<std::string> srcIds)
{
	/* check if we have the file */

	if (ftSearch->findFile(LOCAL))
	{
		/* have it already */
		/* add in as completed transfer */
		return true;
	}

	/* do a source search - for any extra sources */
	if (ftSearch->findFile(REMOTE))
	{

	}

	std::map<std::string, ftTransferModule *> mTransfers;
	std::map<std::string, ftFileCreator *> mFileCreators;

	/* add in new item for download */
	std::string savepath = mDownloadPath + "/" + fname;
	std::string chunker = "";
	ftFileCreator *fc = new ftFileCreator(savepath, size, hash, chunker);
	ftTransferModule = *tm = new ftTransferModule(fc, mClientModule);

	/* add into maps */
	ftFileControl ftfc(fname, size, hash, flags, fc, tm);

	/* add to ClientModule */
	mClientModule->addTransferModule(tm);

	/* now add source peers (and their current state) */
	tm->setFileSources(srcIds);

	/* get current state for transfer module */
	for(it = srcIds.begin(); it != srcIds.end(); it++)
	{
		if (mConnMgr->isOnline(*it))
		{
			tm->setPeer(*it, TRICKLE | ONLINE);
		}
		else
		{
			tm->setPeer(*it, OFFLINE);
		}
	}

	/* only need to lock before to fiddle with own variables */
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	mDownloads[hash] = ftfc;
	mSlowQueue.push_back(hash);

}


bool 	ftController::FileCancel(std::string hash);
bool 	ftController::FileControl(std::string hash, uint32_t flags);
bool 	ftController::FileClearCompleted();

	/* get Details of File Transfers */
bool 	ftController::FileDownloads(std::list<std::string> &hashs)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string, ftFileControl>::iterator it;
	for(it = mDownloads.begin(); it != mDownloads.end(); it++)
	{
		hashes.push_back(it->second.hash);
	}
	return true;
}


	/* Directory Handling */
bool 	ftController::setDownloadDirectory(std::string path)
{
	/* check if it exists */
        if (RsDirUtil::checkCreateDirectory(path))
	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		mDownloadPath = path;
		return true;
	}
	return false;
}

bool 	ftController::setPartialsDirectory(std::string path);
{
	/* check it is not a subdir of download / shared directories (BAD) - TODO */

	/* check if it exists */

        if (RsDirUtil::checkCreateDirectory(path))
	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		mPartialPath = path;

		/* move all existing files! */
		std::map<std::string, ftFileControl>::iterator it;
		for(it = mDownloads.begin(); it != mDownloads.end(); it++)
		{
			(it->second).mCreator->changePartialDirectory(mPartialPath);
		}
		return true;
	}
	return false;
}

std::string ftController::getDownloadDirectory()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	return mDownloadPath;
}

std::string ftController::getPartialsDirectory()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	return mPartialPath;
}

bool 	ftController::FileDetails(std::string hash, FileInfo &info)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string, ftFileControl>::iterator it;
	it = mDownloads.find(hash);
	if (it == mDownloads.end())
	{
		return false;
	}

	/* extract details */

	return true;



}


	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

	/* pqiMonitor callback:
	 * Used to tell TransferModules new available peers
	 */
void    ftController::statusChange(const std::list<pqipeer> &plist)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	/* add online to all downloads */
	std::map<std::string, ftFileControl>::iterator it;
	std::list<pqipeer>::const_iterator pit;

	for(it = mDownloads.begin(); it != mDownloads.end(); it++)
	{
		for(pit = plist.begin(); pit != plist.end(); pit++)
		{
			if (pit->actions | RS_PEER_CONNECTED)
			{
				((it->second).tm)->setPeer(ONLINE | TRICKLE);
			}
			else if (pit->actions | RS_PEER_DISCONNECTED)
			{
				((it->second).tm)->setPeer(OFFLINE);
			}
		}
	}

	/* modify my list of peers */
	for(pit = plist.begin(); pit != plist.end(); pit++)
	{
		if (pit->actions | RS_PEER_CONNECTED)
		{
			/* add in */

			((it->second).tm)->setPeer(ONLINE | TRICKLE);
		}
		else if (pit->actions | RS_PEER_DISCONNECTED)
		{
			((it->second).tm)->setPeer(OFFLINE);
		}
	}

}



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

class ftFileControl
{
	public:

	ftFileControl(std::string fname, uint64_t size, std::string hash, 
			uint32_t flags, ftFileCreator *fc, ftTransferModule *tm);

	std::string mName, 
	uint64_t    mSize;
	std::string mHash, 
	uint32_t    mFlags;
	ftFileCreator *mCreator;
	ftTransferModule *mTransfer;
};

class ftPeerControl
{
	std::string peerId;
	std::map<std::string, uint32_t> priority;
	uint32_t currentBandwidth;
	uint32_t maxBandwidth;
};

	std::map<std::string, ftFileControl> 	mDownloads;

	/* A Bunch of Queues */
	std::map<std::string, std::string>  mStreamQueue;
	std::map<std::string, std::string>  mFastQueue;
	std::list<std::string>		mSlowQueue;
	std::list<std::string>		mTrickleQueue;

	std::string mConfigPath;
	std::string mDownloadPath;
	std::string mPartialPath;
};

#endif
