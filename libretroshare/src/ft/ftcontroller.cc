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


#define CONTROL_DEBUG 1

#warning CONFIG_FT_CONTROL Not defined in p3cfgmgr.h

const uint32_t CONFIG_FT_CONTROL  = 1;

ftFileControl::ftFileControl()
	:mTransfer(NULL), mCreator(NULL), 
	 mState(0), mSize(0), mFlags(0)
{
	return;
}

ftFileControl::ftFileControl(std::string fname, uint64_t size, 
		std::string hash, uint32_t flags, 
		ftFileCreator *fc, ftTransferModule *tm)
	:mTransfer(tm), mCreator(fc), mState(0), mHash(hash),
	 mName(fname), mSize(size), mFlags(0)
{
	return;
}

ftController::ftController(CacheStrapper *cs, ftDataMultiplex *dm, std::string configDir)
	:CacheTransfer(cs), p3Config(CONFIG_FT_CONTROL), mDataplex(dm)
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

	while(1)
	{
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif

		std::cerr << "ftController::run()";
		std::cerr << std::endl;

		/* tick the transferModules */
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		std::map<std::string, ftFileControl>::iterator it;
		for(it = mDownloads.begin(); it != mDownloads.end(); it++)
		{
			std::cerr << "\tTicking: " << it->first;
			std::cerr << std::endl;

			(it->second.mTransfer)->tick();
		}
	}
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
	/* check if we have the file */
	FileInfo info;
	std::list<std::string>::iterator it;

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileRequest(" << fname << ",";
	std::cerr << hash << "," << size << "," << dest << ",";
	std::cerr << flags << ",<";

	for(it = srcIds.begin(); it != srcIds.end(); it++)
	{
		std::cerr << *it << ",";
	}
	std::cerr << ">)";
	std::cerr << std::endl;
#endif

	if (flags | RS_FILE_HINTS_NO_SEARCH)
	{
		/* no search */

	}
	else 
	{
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
	}

	std::map<std::string, ftTransferModule *> mTransfers;
	std::map<std::string, ftFileCreator *> mFileCreators;

	/* add in new item for download */
	std::string savepath = mDownloadPath + "/" + fname;
	std::string chunker = "default";
	ftFileCreator *fc = new ftFileCreator(savepath, size, hash, chunker);
	ftTransferModule *tm = new ftTransferModule(fc, mDataplex,this);

	/* add into maps */
	ftFileControl ftfc(fname, size, hash, flags, fc, tm);

	/* add to ClientModule */
	mDataplex->addTransferModule(tm, fc);

	/* now add source peers (and their current state) */
	tm->setFileSources(srcIds);

	/* get current state for transfer module */
	std::string ownId = mConnMgr->getOwnId();
	for(it = srcIds.begin(); it != srcIds.end(); it++)
	{
		if (*it == ownId)
		{
#ifdef CONTROL_DEBUG
			std::cerr << "ftController::FileRequest()";
			std::cerr << *it << " is Self - set high rate";
			std::cerr << std::endl;
#endif
			//tm->setPeerState(*it, RS_FILE_RATE_FAST | 
			//			RS_FILE_PEER_ONLINE, 100000);
			tm->setPeerState(*it, PQIPEER_IDLE, 10000);
		}
		else if (mConnMgr->isOnline(*it))
		{
#ifdef CONTROL_DEBUG
			std::cerr << "ftController::FileRequest()";
			std::cerr << *it << " is Online";
			std::cerr << std::endl;
#endif
			//tm->setPeerState(*it, RS_FILE_RATE_TRICKLE | 
			//			RS_FILE_PEER_ONLINE, 10000);
			tm->setPeerState(*it, PQIPEER_IDLE, 10000);
		}
		else
		{
#ifdef CONTROL_DEBUG
			std::cerr << "ftController::FileRequest()";
			std::cerr << *it << " is Offline";
			std::cerr << std::endl;
#endif
			//tm->setPeerState(*it, RS_FILE_PEER_OFFLINE,  10000);
			tm->setPeerState(*it, PQIPEER_NOT_ONLINE,  10000);
		}
	}

	/* only need to lock before to fiddle with own variables */
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	mDownloads[hash] = ftfc;
	mSlowQueue.push_back(hash);

}


bool 	ftController::FileCancel(std::string hash)
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileCancel" << std::endl;
#endif
	/*check if the file in the download map*/
	std::map<std::string,ftFileControl>::iterator mit;
	mit=mDownloads.find(hash);
	if (mit==mDownloads.end())
	{
#ifdef CONTROL_DEBUG
		std::cerr<<"ftController::FileCancel file is not found in mDownloads"<<std::endl;
#endif
		return false;
	}

	/*find the point to transfer module*/
	ftTransferModule* ft=(mit->second).mTransfer;
	ft->cancelTransfer();
	return true;
}

bool 	ftController::FileControl(std::string hash, uint32_t flags)
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileControl(" << hash << ",";
	std::cerr << flags << ")"<<std::endl;
#endif
	/*check if the file in the download map*/
	std::map<std::string,ftFileControl>::iterator mit;
	mit=mDownloads.find(hash);
	if (mit==mDownloads.end())
	{
#ifdef CONTROL_DEBUG
		std::cerr<<"ftController::FileControl file is not found in mDownloads"<<std::endl;
#endif
		return false;
	}

	/*find the point to transfer module*/
	ftTransferModule* ft=(mit->second).mTransfer;
	switch (flags)
	{
		case RS_FILE_CTRL_PAUSE:
			ft->pauseTransfer();
			break;
		case RS_FILE_CTRL_START:
			ft->resumeTransfer();
			break;
		default:
			return false;
	}
	return true;
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
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::setDownloadDirectory(" << path << ")";
	std::cerr << std::endl;
#endif
	/* check if it exists */
        if (RsDirUtil::checkCreateDirectory(path))
	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		mDownloadPath = path;
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::setDownloadDirectory() Okay!";
		std::cerr << std::endl;
#endif
		return true;
	}

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::setDownloadDirectory() Failed";
	std::cerr << std::endl;
#endif
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


	/* Cache Interface */
bool ftController::RequestCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size)
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::RequestCacheFile(" << id << ",";
	std::cerr << path << "," << hash << "," << size << ")";
	std::cerr << std::endl;
#endif

	/* Request File */
	std::list<std::string> ids;
	ids.push_back(id);

	FileRequest(hash, hash, size, path, 
		RS_FILE_HINTS_CACHE | RS_FILE_HINTS_NO_SEARCH, ids);
}


bool ftController::CancelCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size)
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::CancelCacheFile(" << id << ",";
	std::cerr << path << "," << hash << "," << size << ")";
	std::cerr << std::endl;
#endif


}


