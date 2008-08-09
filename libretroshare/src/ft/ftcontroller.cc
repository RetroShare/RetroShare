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

#include "ft/ftcontroller.h"

#include "ft/ftfilecreator.h"
#include "ft/fttransfermodule.h"
#include "ft/ftsearch.h"
#include "ft/ftdatamultiplex.h"

#include "util/rsdir.h"

#include "pqi/p3connmgr.h"


#warning CONFIG_FT_CONTROL Not defined in p3cfgmgr.h

const uint32_t CONFIG_FT_CONTROL  = 1;

ftController::ftController(CacheStrapper *cs, ftDataMultiplex *dm, std::string configDir)
	:CacheTransfer(cs), p3Config(CONFIG_FT_CONTROL)
{
	/* TODO */
}

void ftController::setFtSearch(ftSearch *search)
{
	mSearch = search;
}

void ftController::run()
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

#if 0

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
	mDataplex->removeTransferModule(fc->mTransfer->hash());
	
	delete fc->mTransfer;
	fc->mTransfer = NULL;

	delete fc->mCreator;
	fc->mCreator = NULL;

	fc->mState = COMPLETE;

	/* switch map */
	mCompleted[fc->mHash] = *fc;
	mDownloads.erase(it);

	return true;

#endif
}

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool 	ftController::FileRequest(std::string fname, std::string hash, 
			uint64_t size, std::string dest, uint32_t flags, 
			std::list<std::string> &srcIds)
{

#if 0 /*** FIX ME !!!**************/

	/* check if we have the file */
	FileInfo info;

	if (mSearch->search(hash, size, 
		RS_FILE_HINTS_LOCAL | 
		RS_FILE_HINTS_EXTRA | 
		RS_FILE_HINTS_SPEC_ONLY, info))
	{
		/* have it already */
		/* add in as completed transfer */
		return true;
	}

	/* do a source search - for any extra sources */
	if (mSearch->search(hash, size, 
		RS_FILE_HINTS_REMOTE |
		RS_FILE_HINTS_SPEC_ONLY, info))
	{
		/* do something with results */
	}

	std::map<std::string, ftTransferModule *> mTransfers;
	std::map<std::string, ftFileCreator *> mFileCreators;

	/* add in new item for download */
	std::string savepath = mDownloadPath + "/" + fname;
	std::string chunker = "";
	ftFileCreator *fc = new ftFileCreator(savepath, size, hash, chunker);
	ftTransferModule *tm = new ftTransferModule(fc, mDataplex);

	/* add into maps */
	ftFileControl ftfc(fname, size, hash, flags, fc, tm);

	/* add to ClientModule */
	mDataplex->addTransferModule(tm, fc);

	/* now add source peers (and their current state) */
	tm->setFileSources(srcIds);

	/* get current state for transfer module */
	std::list<std::string>::iterator it;
	for(it = srcIds.begin(); it != srcIds.end(); it++)
	{
		if (mConnMgr->isOnline(*it))
		{
			tm->setPeer(*it, RS_FILE_RATE_TRICKLE | RS_FILE_PEER_ONLINE);
		}
		else
		{
			tm->setPeer(*it, RS_FILE_PEER_OFFLINE);
		}
	}

	/* only need to lock before to fiddle with own variables */
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	mDownloads[hash] = ftfc;
	mSlowQueue.push_back(hash);

#endif

}


bool 	ftController::FileCancel(std::string hash)
{
	/* TODO */
	return false;
}

bool 	ftController::FileControl(std::string hash, uint32_t flags)
{
	return false;
}

bool 	ftController::FileClearCompleted()
{
	return false;
}

	/* get Details of File Transfers */
bool 	ftController::FileDownloads(std::list<std::string> &hashs)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string, ftFileControl>::iterator it;
	for(it = mDownloads.begin(); it != mDownloads.end(); it++)
	{
		hashs.push_back(it->second.mHash);
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

bool 	ftController::setPartialsDirectory(std::string path)
{
#if 0 /*** FIX ME !!!**************/

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
#endif
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

#if 0 /*** FIX ME !!!**************/

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
				((it->second).mTransfer)->setPeer(RS_FILE_PEER_ONLINE | RS_FILE_RATE_TRICKLE);
			}
			else if (pit->actions | RS_PEER_DISCONNECTED)
			{
				((it->second).mTransfer)->setPeer(RS_FILE_PEER_OFFLINE);
			}
		}
	}

	/* modify my list of peers */
	for(pit = plist.begin(); pit != plist.end(); pit++)
	{
		if (pit->actions | RS_PEER_CONNECTED)
		{
			/* add in */

			((it->second).mTransfer)->setPeer(RS_FILE_PEER_ONLINE | RS_FILE_RATE_TRICKLE);
		}
		else if (pit->actions | RS_PEER_DISCONNECTED)
		{
			((it->second).mTransfer)->setPeer(RS_FILE_PEER_OFFLINE);
		}
	}

#endif

}
	/* p3Config Interface */
RsSerialiser *ftController::setupSerialiser()
{
	return NULL;
}

std::list<RsItem *> ftController::saveList(bool &cleanup)
{
	std::list<RsItem *> emptyList;
	return emptyList;
}

	
bool    ftController::loadList(std::list<RsItem *> load)
{
	return false;
}


