/*
 * "$Id: ftfiler.cc,v 1.13 2007-02-19 20:08:30 rmf24 Exp $"
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




#include "server/ftfiler.h"
#include "util/rsdir.h"

#include "pqi/pqidebug.h"
#include "pqi/pqinotify.h"

#include <errno.h>
#include <sstream>

const int ftfilerzone = 86539;

/* 
 * PQI Filer
 *
 * This managers the file transfers.
 *
 *
 * TODO: add trickle transfers.
 *
 */

const uint32_t PQIFILE_OFFLINE_CHECK  = 120; /* check every 2 minutes */
const uint32_t PQIFILE_DOWNLOAD_TIMEOUT  = 60; /* time it out, -> offline after 60 secs */
const uint32_t PQIFILE_DOWNLOAD_CHECK    = 10; /* desired delta = 10 secs */
const uint32_t PQIFILE_DOWNLOAD_TOO_FAST = 8; /* 8 secs */
const uint32_t PQIFILE_DOWNLOAD_TOO_SLOW = 12; /* 12 secs */
const uint32_t PQIFILE_DOWNLOAD_MIN_DELTA = 5; /* 5 secs */

const float TRANSFER_MODE_TRICKLE_RATE = 1000;   /* 1   kbyte limit */
const float TRANSFER_MODE_NORMAL_RATE  = 500000; /* 500 kbyte limit - everyone uses this one for now */
const float TRANSFER_MODE_FAST_RATE    = 500000; /* 500 kbyte limit */

const uint32_t TRANSFER_START_MIN = 500;  /* 500 byte  min limit */
const uint32_t TRANSFER_START_MAX = 10000; /* 10000 byte max limit */

void printFtFileStatus(ftFileStatus *s, std::ostream &out);

/************* Local File Interface ****************************
 *
 * virtual bool    getCacheFile(std::string id, std::string path, std::string hash) = 0;
 * virtual int     getFile(std::string name, std::string hash,
 *                                 int size, std::string destpath) = 0;
 *
 * virtual int     cancelFile(std::string hash) = 0;
 *
 * int	ftfiler::clearFailedTransfers();
 *
 * * Worker Fns.
 * ftFileStatus *ftfiler::findRecvFileItem(PQFileItem *in);
 */


int     ftfiler::getFile(std::string name, std::string hash, 
			uint64_t size, std::string destpath)
{
	/* add to local queue */
	{
		std::ostringstream out;
		out << "ftfiler::getFile(): ";
		out << " name: " << name << " hash: " << hash;
		out << " path: " << destpath << " size: " << size;
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}

	/* check for duplicates */
	ftFileStatus *state = findRecvFileItem(hash);
	if (state)
	{
		std::ostringstream out;
		out << "ftfiler::getFile() - duplicate, giving push!";
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

		/* if duplicate - give download a push! */
		/* do this by flagging last transfer at 0.
		 */
		/* and also set the request stuff, so it'll 
		 * generate a new request
		 * - we might lose the current transfer - but 
		 *   that's the idiots fault for redownloading.
		 */

		resetFileTransfer(state);
		return 1;
	}

	// HANDLE destpath - TODO!
	// state = new ftFileStatus(name, hash, size, destpath, FT_MODE_STD);
	state = new ftFileStatus(name, hash, size, "", FT_MODE_STD);
	if (initiateFileTransfer(state))
	{
		std::ostringstream out;
		out << "ftfiler::getFile() ";
		out << "adding to recvFiles queue";
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

		recvFiles.push_back(state);
	}
	return 1;
}

bool    ftfiler::RequestCacheFile(std::string id, std::string destpath, std::string hash, uint64_t size)
{
	/* add to local queue */
	{
		std::ostringstream out;
		out << "ftfiler::getCacheFile(): ";
		out << " id: " << id << " hash: " << hash;
		out << " path: " << destpath;
		out << " size: " << size;
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}

	/* check for duplicates */
	ftFileStatus *state = findRecvFileItem(hash);
	if (state)
	{
		std::ostringstream out;
		out << "ftfiler::getFile() - duplicate, giving push!";
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

		/* if duplicate - give download a push! */
		/* do this by flagging last transfer at 0.
		 */
		/* and also set the request stuff, so it'll 
		 * generate a new request
		 * - we might lose the current transfer - but 
		 *   that's the idiots fault for redownloading.
		 */

		resetFileTransfer(state);
		return 1;
	}

	state = new ftFileStatus(id, hash, hash, size, destpath, FT_MODE_CACHE);
	if (initiateFileTransfer(state))
	{
		std::ostringstream out;
		out << "ftfiler::getFile() ";
		out << "adding to recvFiles queue";
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

		recvFiles.push_back(state);
	}
	return 1;
}

bool    ftfiler::CancelCacheFile(RsPeerId id, std::string path, 
				std::string hash, uint64_t size)
{
	/* clean up old transfer - just remove it (no callback) */
	{
		std::ostringstream out;
		out << "ftfiler::CancelCacheFile() Looking for: " << hash;
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}

	/* iterate through fileItems and check for this one */
	std::list<ftFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end(); it++)
	{
		if  ((hash==(*it)->hash) && 
			(size==(*it)->size) &&
			((*it)->ftMode == FT_MODE_CACHE))
		{
			std::ostringstream out;
			out << "ftfiler::CancelCacheFile() ";
			out << "Match ftFileStatus: " << hash;
			pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
			/* same */

			std::cerr << "Clearing Failed Cache Transfer: " << (*it)->name;
			std::cerr << std::endl;
			delete (*it);
			it = recvFiles.erase(it);

			return true;
		}
	}

	std::cerr << "************* ERROR *****************";
	std::cerr << std::endl;
	std::cerr << "ftfiler::CancelCacheFile() Failed to Find: " << hash;
	std::cerr << std::endl;
	return false;
}



ftFileStatus *ftfiler::findRecvFileItem(std::string hash)
{
	{
		std::ostringstream out;
		out << "ftfiler::findRecvFileItem() Looking for: " << hash;
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}

	/* iterate through fileItems and check for this one */
	std::list<ftFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end(); it++)
	{
		if  (hash==(*it)->hash)
		{
			std::ostringstream out;
			out << "ftfiler::findRecvFileItem() ";
			out << "Match ftFileStatus: " << hash;
			pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
			/* same */
			return (*it);
		}
	}
	return NULL;
}




int     ftfiler::cancelFile(std::string hash)
{
	/* flag as cancelled */
	/* iterate through fileItems and check for this one */
	{
		std::ostringstream out;
		out << "ftfiler::cancelFile() hash: " << hash << std::endl;
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}

	std::list<ftFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end(); it++)
	{
		if (hash==(*it)->hash)
		{
			std::ostringstream out;
			out << "ftfiler::cancelFile() ";
			out << "Found file: " << hash;
			pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

			/* same */
			(*it)->status = (PQIFILE_FAIL | PQIFILE_FAIL_CANCEL);
			return 1;
		}
	}

	{
		std::ostringstream out;
		out << "ftfiler::cancelFile() ";
		out << "couldn't match ftFileStatus!";
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}
	return 0;
}

int	ftfiler::clearFailedTransfers()
{
	/* remove all the failed items */
	/* iterate through fileItems and check for this one */
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::clearFailedTransfers()");

	std::list<ftFileStatus *>::iterator it;
	int cleared = 0;
	for(it = recvFiles.begin(); it != recvFiles.end(); /* done in loop */)
	{
		if ((*it)->status & PQIFILE_FAIL)
		{
			std::ostringstream out;
			out << "ftfiler::clearFailedTransfers() ";
			out << "removing item: " << (*it) -> name;
			pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

			/* same */
			ftFileStatus *cfile = (*it);
			it = recvFiles.erase(it);
			delete cfile;
			cleared++;
		}
		else if ((*it)->status & PQIFILE_COMPLETE)
		{
			std::ostringstream out;
			out << "ftfiler::clearFailedTransfers() ";
			out << "removing Completed item: ";
			out << (*it) -> name;
			pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

			/* same */
			ftFileStatus *cfile = (*it);
			it = recvFiles.erase(it);
			delete cfile;
			cleared++;
		}
		else
		{
			it++;
		}
	}

	{
		std::ostringstream out;
		out << "ftfiler::clearFailedTransfers() cleared: ";
		out << cleared;
		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}

	return 1;
}

std::list<RsFileTransfer *> ftfiler::getStatus()
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::getTransferStatus()");
	std::list<RsFileTransfer *>  stateList;

	/* iterate through all files to recv */
	std::list<ftFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end(); it++)
	{
		RsFileTransfer *rft = new RsFileTransfer();
		rft -> in = true;

		/* Ids: current and all */
		std::list<std::string>::iterator pit;
		for(pit = (*it)->sources.begin(); 
				pit != (*it)->sources.end(); pit++)
		{
			rft->allPeerIds.ids.push_back(*pit);
		}
		rft -> cPeerId = (*it)->id;

		/* file details */
		rft -> file.name = (*it)->name;
		rft -> file.filesize = (*it)->size;
		rft -> file.path = "";
		rft -> file.pop = 0;
		rft -> file.age = 0;

		/* hack to store 'Real' Transfers (Cache have blank hash)*/
		if ((*it)->ftMode != FT_MODE_CACHE)
			rft -> file.hash = (*it)->hash;
		else
			rft -> file.hash = "";

		/* Fill in rate and State */
		rft -> transferred = (*it)->recv_size;
		rft -> crate = (*it)->rate; // bytes.
		rft -> trate = (*it)->rate; // bytes.
		rft -> lrate = (*it)->rate; // bytes.
		rft -> ltransfer = (*it)->req_size;

		/* get inactive period */
		if ((*it) -> status == PQIFILE_NOT_ONLINE)
		{
			rft -> crate = 0;
			rft -> trate = 0;
			rft -> lrate = 0;
			rft -> ltransfer = 0;
			rft -> state = FT_STATE_WAITING;
		}
		else if ((*it) -> status & PQIFILE_FAIL)
		{
			rft -> crate = 0;
			rft -> trate = 0;
			rft -> lrate = 0;
			rft -> ltransfer = 0;
			rft -> state = FT_STATE_FAILED;

		}
		else if ((*it) -> status == PQIFILE_COMPLETE)
		{
			rft -> state = FT_STATE_COMPLETE;
		}
		else if ((*it) -> status == PQIFILE_DOWNLOADING)
		{
			rft -> state = FT_STATE_DOWNLOADING;
		}
		else
		{
			rft -> state = FT_STATE_FAILED;
		}
		stateList.push_back(rft);
	}

	/* outgoing files */
	for(it = fileCache.begin(); it != fileCache.end(); it++)
	{
		RsFileTransfer *rft = new RsFileTransfer();
		rft -> in = false;

		/* Ids: current and all */
		std::list<std::string>::iterator pit;
		for(pit = (*it)->sources.begin(); 
				pit != (*it)->sources.end(); pit++)
		{
			rft->allPeerIds.ids.push_back(*pit);
		}
		rft -> cPeerId = (*it)->id;

		/* file details */
		rft -> file.name = (*it)->name;
		rft -> file.filesize = (*it)->size;
		rft -> file.path = "";
		rft -> file.pop = 0;
		rft -> file.age = 0;

		/* hack to store 'Real' Transfers (Cache have blank hash)*/
		if ((*it)->ftMode != FT_MODE_CACHE)
			rft -> file.hash = (*it)->hash;
		else
			rft -> file.hash = "";

		/* Fill in rate and State */
		rft -> transferred = (*it)->recv_size;
		rft -> crate = (*it)->rate; // bytes.
		rft -> trate = (*it)->rate; // bytes.
		rft -> lrate = (*it)->rate; // bytes.
		rft -> ltransfer = (*it)->req_size;

		/* get inactive period */
		if ((*it) -> status == PQIFILE_NOT_ONLINE)
		{
			rft -> crate = 0;
			rft -> trate = 0;
			rft -> lrate = 0;
			rft -> ltransfer = 0;
			rft -> state = FT_STATE_WAITING;
		}
		else if ((*it) -> status & PQIFILE_FAIL)
		{
			rft -> crate = 0;
			rft -> trate = 0;
			rft -> lrate = 0;
			rft -> ltransfer = 0;
			rft -> state = FT_STATE_FAILED;

		}
		else if ((*it) -> status == PQIFILE_COMPLETE)
		{
			rft -> state = FT_STATE_COMPLETE;
		}
		else if ((*it) -> status == PQIFILE_DOWNLOADING)
		{
			rft -> state = FT_STATE_DOWNLOADING;
		}
		else
		{
			rft -> state = FT_STATE_FAILED;
		}
		stateList.push_back(rft);
	}
	return stateList;
}


/************* Incoming FileItems ******************************
 *
 * PQFileItem *ftfiler::sendPQFileItem()
 * int	ftfiler::recvPQFileItem(PQFileItem *in)
 *
 * * Worker Fns.
 * int ftfiler::handleFileNotOnline(PQFileItem *in)
 * int ftfiler::handleFileNotOnline(PQFileItem *in)
 * int ftfiler::handleFileNotAvailable(PQFileItem *in)
 * int ftfiler::handleFileData(PQFileItem *in)
 * int ftfiler::handleFileRequest(PQFileItem *in)
 * int ftfiler::handleFileCacheRequest(PQFileItem *req)
 *
 */

ftFileRequest *ftfiler::sendFileInfo()
{
	if (out_queue.size() < 1)
	{
		return NULL;
	}
	ftFileRequest *i = out_queue.front();
	out_queue.pop_front();
	return i;
}

int 	ftfiler::recvFileInfo(ftFileRequest *in)
{

	/* decide if it is a fileData or Request */
	ftFileData *dta = dynamic_cast<ftFileData *>(in);
	if (dta)
	{
		handleFileData(dta->hash, dta->offset, dta->data, dta->chunk);
	}
	else
	{
		handleFileRequest(in->id, in->hash, in->offset, in->chunk);
	}
	/* cleanup */
	delete in;

	return 1;
}


int ftfiler::handleFileError(std::string hash, uint32_t err)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::handleFileError()");
	/* get out the error */
	if (err & PQIFILE_NOT_ONLINE)
	{
		return handleFileNotOnline(hash);
	}
	if (err & PQIFILE_FAIL)
	{
		return handleFileNotAvailable(hash);
	}
	return 0;
}

int ftfiler::handleFileNotOnline(std::string hash)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::handleFileNotOnline()");
	/* flag recvFile item as not Online */
	ftFileStatus *s = findRecvFileItem(hash);
	if ((!s) || (s -> status & PQIFILE_FAIL))
	{
		return 0;
	}

	s -> status = PQIFILE_NOT_ONLINE;

	return 1;
}


int ftfiler::handleFileNotAvailable(std::string hash)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::handleFileNotAvailable()");
	/* error - flag recvFile item with FAILED */
	ftFileStatus *s = findRecvFileItem(hash);
	if (!s)
	{
		return 0;
	}

	s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_AVAIL);
	return 1;
}


int ftfiler::handleFileData(std::string hash, uint64_t offset, 
				void *data, uint32_t datalen)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::handleFileData()");
	/* find the right ftFileStatus */
	ftFileStatus *recv = findRecvFileItem(hash);
	if (!recv)
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::handleFileData() no matching ftFileStatus (current download)");
		return 0;
	}

	if(recv->status & PQIFILE_FAIL)
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::handleFileData() marked as FAIL");
		return 0;
	}

	/* add to file */
 	addFileData(recv, offset, data, datalen);

	if (recv->status == PQIFILE_NOT_ONLINE)
	{
		/* switch to active */
		recv->status = PQIFILE_DOWNLOADING;
	}

	/* if we have recieved all data - request some more */
	if ((recv->recv_size == recv->req_loc + recv->req_size) &&
		(recv->status != PQIFILE_COMPLETE)) 
	{
		requestData(recv);
	}
	return 1;
}

int ftfiler::handleFileRequest(std::string id, std::string hash, uint64_t offset, uint32_t chunk)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::handleFileRequest()");
	/* see if in cache */
	/* if yes send out chunk */
	if (handleFileCacheRequest(id, hash, offset, chunk))
	{
		return 1;
	}

	/* if not in cache - find file */
	ftFileStatus *new_file = createFileCache(hash);
	if (!new_file)
	{
		/* bad file */
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	        	      "ftfiler::handleFileRequest() Failed to Load File-sendNotAvail");
		return 0;
		//sendFileNotAvail(in);
	}

	fileCache.push_back(new_file);

	return handleFileCacheRequest(id, hash, offset, chunk);
}

int ftfiler::handleFileCacheRequest(std::string id, std::string hash, uint64_t offset, uint32_t chunk)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::handleFileCacheRequest()");
	/* check if in cache */
	bool found = false;
	ftFileStatus *s;
	std::list<ftFileStatus *>::iterator it;
	for(it = fileCache.begin(); it != fileCache.end(); it++)
	{
		if (hash==(*it)->hash)
		{
			found = true;
			s = (*it);
			break;
		}
	}
	if (!found)
		return 0;

	/* push to out queue */
	return generateFileData(s, id, offset, chunk);
}

/************* Outgoing FileItems ******************************
 *
 * PQFileItem *ftfiler::sendPQFileItem()
 *
 * * Worker Fns.
 * int	ftfiler::tick();
 * void	ftfiler::queryInactive()
 *
 */


int	ftfiler::tick()
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::tick()");
	/* check the status of recved files */
	queryInactive();

	/* this doesn't matter much how often it's ticked...
	 * if it generates Items, they will be detected other places.
	 * so we can return 0 (waiting)
	 */
	return 0;
}


void	ftfiler::queryInactive()
{
	std::ostringstream out;

	out << "ftfiler::queryInactive()";
	out << std::endl;



	/* iterate through all files to recv */
	int ts = time(NULL);
	std::list<ftFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end();) /* increment at end of loop */
	{
		/* get inactive period */
		switch((*it) -> status)
		{
		case PQIFILE_NOT_ONLINE:
			out << "File: " << (*it)->name << " Not Online: ";
			out << "Delta: " << (ts - (*it)->lastTS) << std::endl;
			out << " LDelta: " << (*it)->lastDelta;
			out << " Recved: " << (*it)->recv_size;
			out << " Total: " << (*it)->total_size;
			out << " LChunk: " << (*it)->req_size;
			out << std::endl;

			if (ts - ((*it)->lastTS) > PQIFILE_OFFLINE_CHECK)
			{
				resetFileTransfer(*it);
				requestData(*it);
			}
			break;
		case PQIFILE_DOWNLOADING:
			out << "File: " << (*it)->name << " Downloading: ";
			out << " Delta: " << (ts - (*it)->lastTS) << std::endl;
			out << " LDelta: " << (*it)->lastDelta;
			out << " Recved: " << (*it)->recv_size;
			out << " Total: " << (*it)->total_size;
			out << " LChunk: " << (*it)->req_size;
			out << std::endl;

			if (ts - ((*it)->lastTS) > PQIFILE_DOWNLOAD_CHECK)
			{
				requestData(*it); /* give it a push */
			}
			break;
		default:
			out << "File: " << (*it)->name << " Other mode: " << (*it)->status;
			out << " Delta: " << (ts - (*it)->lastTS) << std::endl;
			out << " LDelta: " << (*it)->lastDelta;
			out << " Recved: " << (*it)->recv_size;
			out << " Total: " << (*it)->total_size;
			out << " LChunk: " << (*it)->req_size;
			out << std::endl;
			/* nothing */
			break;
		}

		/* remove/increment */
		if (((*it) -> status == PQIFILE_COMPLETE) && ((*it)->ftMode == FT_MODE_CACHE))
		{
			std::cerr << "Clearing Completed Cache File: " << (*it)->name;
			std::cerr << std::endl;
			delete (*it);
			it = recvFiles.erase(it);
		}
		else
		{
			it++;
		}

	}
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
}


int 	ftfiler::requestData(ftFileStatus *item)
{

	/* formulate a request for the next desired data chunk */
	/* this handles the key throttling. This algorithm can
	 * be quite crude, as the tcp / and pqistreamer throttle as 
	 * well.
	 */

	std::ostringstream out;
	out << "ftfiler::requestData()" << std::endl;

	/* get the time since last request */
	int tm = time(NULL);
	float delta = tm - item -> lastTS;

	if (item->id == "") /* no possible source */
	{
		/* flag as handled for now so it doesn't repeat to fast */
		item->lastTS = tm; 
		return 0;
	}

	/* decide on transfer mode */
	float max_rate = TRANSFER_MODE_NORMAL_RATE;
	switch(item->mode)
	{
		case TRANSFER_MODE_TRICKLE:
			max_rate = TRANSFER_MODE_TRICKLE_RATE;
			break;
		case TRANSFER_MODE_NORMAL:
			max_rate = TRANSFER_MODE_NORMAL_RATE;
			break;
		case TRANSFER_MODE_FAST:
			max_rate = TRANSFER_MODE_FAST_RATE;
			break;
		default:
			break;
	}
	out << "max rate: " << max_rate;
	out << std::endl;

	/* not finished */
	if (item->recv_size < item->req_loc + item->req_size)
	{
	  	if (delta > PQIFILE_DOWNLOAD_TIMEOUT)
		{
			/* we have timed out ... switch to 
			 * offline
			 */
			/* start again slowly */
			item->req_size = (int) (0.1 * max_rate);
			out << "Timeout: switching to Offline.";
			out << std::endl;
			item->status = PQIFILE_NOT_ONLINE;
		}
		else
		{
			out << "Pause: Not Finished";
			out << std::endl;
			/* pause */
		}
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
		return 0;
	}


	if (delta <= PQIFILE_DOWNLOAD_MIN_DELTA)
	{
		/* pause */
		out << "Small Delta -> Pause";
		out << std::endl;
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
		return 0;
	}

	/* From this point - we will continue ... so handle rate now! */
	/* calc rate */
	float bytes_psec = item -> req_size / delta;
	item -> rate = 0.7 * item -> rate + 0.3 * bytes_psec;
	out << "delta: " << delta << " bytes: " << bytes_psec << " rate: " << item -> rate;
	out << std::endl;

	if (item->lastDelta <= PQIFILE_DOWNLOAD_TOO_FAST)
	{
		/* increase 0.75 of the calculated extra that could be transmitted
		 * in the timeframe
		 */

		float data_tf = item -> req_size;
		float ldelta_f = item->lastDelta + 0.5; // 0.5 for extra space (+ dont / 0.0)
		float tf_p_sec = data_tf / ldelta_f;
		float extra_tf = tf_p_sec * (PQIFILE_DOWNLOAD_CHECK - ldelta_f);

		item -> req_size = item->req_size + (int) (0.75 * extra_tf);

		if (item->req_size > max_rate * PQIFILE_DOWNLOAD_CHECK)
			item->req_size = (int) (max_rate * PQIFILE_DOWNLOAD_CHECK);

		out << "Small Delta: " << ldelta_f << " (sec), rate: " << tf_p_sec;
		out << std::endl;
		out << "Small Delta Incrementing req_size from: " << data_tf;
		out << " to :" <<  item->req_size;
		out << std::endl;

	}
	else if (item->lastDelta > PQIFILE_DOWNLOAD_TOO_SLOW)
	{
		/* similarly decrease rate by 1.5 of extra time */

		float data_tf = item -> req_size;
		float ldelta_f = item->lastDelta + 0.5; // 0.5 for extra space (+ dont / 0.0)
		float tf_p_sec = data_tf / ldelta_f;
		float extra_tf = tf_p_sec * (ldelta_f - PQIFILE_DOWNLOAD_CHECK);

		item -> req_size -= (int) (1.25 * extra_tf);

		out << "Long Delta: " << ldelta_f << " (sec), rate: " << tf_p_sec;
		out << std::endl;
		out << "Long Delta Decrementing req_size from: " << data_tf;
		out << " to :" <<  item->req_size;
		out << std::endl;
	}

	/* make the packet */

	item->req_loc = item->recv_size;
	/* req_size already calculated (unless NULL) */
	if (item->req_size < TRANSFER_START_MIN)
	{
		/* start again slowly 
		 * added an extra limiter. 
		 * - make this dependent on number of transfers ... */
		item->req_size = (int) (max_rate * (0.01 + 0.10 / recvFiles.size()));
		if (item->req_size < TRANSFER_START_MIN)
		{
			item->req_size = TRANSFER_START_MIN;
		}
		else if (item->req_size > TRANSFER_START_MAX) 
		{
			item->req_size = TRANSFER_START_MAX;
		}
	}

	out << "Making Packet: offset: " << item->req_loc << " size: " << item->req_size;
	out << std::endl;
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

	ftFileRequest *req = generateFileRequest(item);
	out_queue.push_back(req);

	return 1;
}

/************* PQIFILEITEM Generator ***************************
 *
 * PQFileItem *ftfiler::generatePQFileRequest(ftFileStatus *s);
 * int ftfiler::generateFileData(ftFileStatus *s, PQFileItem *req);
 * int ftfiler::sendFileNotAvail(PQFileItem *req)
 *
 */


int	ftfiler::generateFileData(ftFileStatus *s, std::string id, uint64_t offset, uint32_t chunk)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::generateFileData()");

	if ((!s) || (!s->fd) || (s->status & PQIFILE_FAIL))
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::generateFileData() Bad Status");
		if (!s)
		{
        		pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::generateFileData() Bad Status (!s)");
		}
		if (!s->fd)
		{
        		pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::generateFileData() Bad Status (!s->fd)");
		}
		if (s->status & PQIFILE_FAIL)
		{
			std::ostringstream out;
	       		out << "ftfiler::generateFileData() Bad Status (s->status): " << s->status;
        		pqioutput(PQL_DEBUG_BASIC, ftfilerzone,out.str());
		}

		/* return an error */
		return 0;
		//sendFileNotAvail(req);
	}

	/* make the packets */
	int tosend    = chunk;
	long base_loc = offset;

	if (base_loc + tosend > s -> total_size)
	{
		tosend = s -> total_size - base_loc;
	}

	{
		std::ostringstream out;
		out << "ftfiler::generateFileData() Sending " << tosend;
		out << " bytes from offset: " << base_loc << std::endl;
		out << "\tFrom File:" << s -> name;
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}

	if (tosend > 0)
	{
		/* seek for base_loc */
		fseek(s->fd, base_loc, SEEK_SET);

		void *data = malloc(tosend);
		/* read the data */
		if (1 != fread(data, tosend, 1, s->fd))
		{
			std::ostringstream out;

			out << "ftfiler::generateFileData() Failed to get data!";

        		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

			free(data);
			return 0;
		}

                // make a FileData type.
		ftFileData *fd = new ftFileData(id, s->hash, s->size, offset, tosend, data, 0);

		/* send off the packet */
		out_queue.push_back(fd);
	}
	return 1;
}


ftFileRequest *ftfiler::generateFileRequest(ftFileStatus *s)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::generatePQFileRequest()");

	ftFileRequest *fr = new ftFileRequest(s->id, s->hash, 
				s->size, s->req_loc, s->req_size);

	std::ostringstream out;

	out << "ftfiler::generateFileRequest() for: " << s->name << std::endl;
	out << "ftfiler::generateFileRequest() offset: " << fr->offset << " chunksize: ";
	out << fr->chunk << std::endl;

 	pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());

	// timestamp request.
	s->lastTS = time(NULL);

	return fr;
}

/************* FILE DATA HANDLING ******************************
 *
 * std::string ftfiler::determineTmpFilePath(ftFileStatus *s);
 * std::string ftfiler::determineDestFilePath(ftFileStatus *s)
 * int ftfiler::initiateFileTransfer(ftFileStatus *s);
 * int ftfiler::resetFileTransfer(ftFileStatus *s);
 * int ftfiler::addFileData(ftFileStatus *s, long idx, void *data, int size);
 *
 */

const std::string PARTIAL_DIR = "partials";

std::string ftfiler::determineTmpFilePath(ftFileStatus *s)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::determineTmpFilePath()");

	/* get the download path */
	// savePath = ".";
	std::string filePath = saveBasePath;
	filePath += "/";
	filePath += PARTIAL_DIR;
	filePath += "/";
	filePath += s->hash;

	return filePath;


}

std::string ftfiler::determineDestFilePath(ftFileStatus *s)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::determineDestFilePath()");

	/* should be three different options here:
	 * (1) relative to baseSavePath (default)
	 * (2) Abs (for Cache Files)
	 * (3) relative to shared dirs (TODO)
	 *
	 * XXX TODO.
	 */

	std::string filePath;
	
	if (s->destpath == "")
	{
		filePath = saveBasePath;
	}
	else
	{
		filePath = s->destpath;
	}

	/* get the download path */
	filePath += "/";
	filePath += s->name;

	return filePath;


}

/******
 * NOTES:
 *
 * This is called to start the Transfer - from GetFile() or GetCacheFile()
 *
 * we need to determine the destination.
 *
 *
 *
 */

int ftfiler::initiateFileTransfer(ftFileStatus *s)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::initiateFileTransfer()");


	std::string partialpath = saveBasePath + "/";
	partialpath += PARTIAL_DIR;
	if (!RsDirUtil::checkCreateDirectory(partialpath))
	{
		{
		  std::ostringstream out;
		  out << "ftfiler::initiateFileTransfer() Cannot create partial directory: " << partialpath;
        	  pqioutput(PQL_ALERT, ftfilerzone, out.str());
		}

		std::string tmppath = mEmergencyIncomingDir;
		if (!RsDirUtil::checkCreateDirectory(tmppath))
		{
		  	std::ostringstream out;
			out << "ftfiler::initiateFileTransfer() Cannot create EmergencyIncomingDir: ";
			out << tmppath;
        		pqioutput(PQL_ALERT, ftfilerzone, out.str());
			exit(1);
		}

		/* Store new temp path */
		saveBasePath = tmppath;

		tmppath += "/";
		tmppath += PARTIAL_DIR;

		if (!RsDirUtil::checkCreateDirectory(tmppath))
		{
		  	std::ostringstream out;
			out << "ftfiler::initiateFileTransfer() Cannot create EmergencyIncomingPartialsDir: ";
			out << tmppath;
        		pqioutput(PQL_ALERT, ftfilerzone, out.str());
			exit(1);
		}

		{
		  std::ostringstream out;
		  out << "ftfiler::initiateFileTransfer() Using Emergency Download Directory: " << saveBasePath;
        	  pqioutput(PQL_ALERT, ftfilerzone, out.str());
		}

		pqiNotify *notify = getPqiNotify();
		if (notify)
		{
			std::string title =
			"Warning: Bad Incoming Directory";
			
			std::string msg;
			msg +=  "               **** WARNING ****     \n";
			msg +=  "Retroshare cannot create Incoming Partials Directory: ";
			msg +=  "\n";
			msg +=  partialpath;
			msg +=  "\n";
			msg +=  "\n";
			msg +=  "This is needed for normal operation.";
			msg +=  "\n";
			msg +=  "\n";
			msg +=  "The incoming directory has been temporarily changed to:";
			msg +=  "\n";
			msg +=  saveBasePath;
			msg +=  "\n";
			msg +=  "\n";
			msg +=  "Please select a new Downloads Directory ASAP Using:";
			msg +=  "\n";
			msg +=  "SideBar->Options->Directories";
			msg +=  "\n";
			
			notify->AddSysMessage(0, RS_SYS_WARNING, title, msg);
		}
		else
		{
			std::cerr << "ftfiler::initiateFileTransfer() Notify not exist!";
			std::cerr << std::endl;
			exit(1);
		}
	}

	/* check if the file exists */
	s->file_name = determineTmpFilePath(s);

	{
		std::ostringstream out;
		out << "ftfiler::initiateFileTransfer() Filename: ";
		out << s->file_name;
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
	}

	/* attempt to open file */
	FILE *fd = fopen(s->file_name.c_str(), "r+b");
	if (!fd)
	{
		{
		std::ostringstream out;
		out << "ftfiler::initiateFileTransfer() Failed to open (r+b): ";
		out << s->file_name << " Error: " << errno;
		out << " Will try to create file";
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
		}

		/* open in writing mode */
		fd = fopen(s->file_name.c_str(), "w+b");
		if (!fd)
		{
			std::ostringstream out;
			out << "ftfiler::initiateFileTransfer() Failed to open (w+b): ";
			out << s->file_name << " Error:" << errno;
        		pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
		
			/* failed to open the file */
			s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_OPEN);
			return 0;
		}

	}


	/* if it opened, find it's length */
	/* move to the end */
	if (0 != fseek(fd, 0L, SEEK_END))
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::initiateFileTransfer() Seek Failed");
		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_SEEK);
		return 0;
	}

	s->recv_size  = ftell(fd); /* get the file length */
	s->total_size = s->size; /* save the total length */
	s->fd = fd;

	/* now determine the sources */
	if (s->ftMode != FT_MODE_CACHE)
	{
	}

	resetFileTransfer(s);
	return 1;
}

int ftfiler::resetFileTransfer(ftFileStatus *state)
{
	// reset all the basic items ... so the transfer will continue.
	state->req_loc  = 0;
	state->req_size = 0;
	state->lastTS = 0;
	state->lastDelta = 0;
	state->status = PQIFILE_NOT_ONLINE;
	state->mode = TRANSFER_MODE_NORMAL;
	state->rate = 0;
	if (state->recv_size == state->total_size)
	{
		state->status = PQIFILE_COMPLETE;
		/* if we're kicking it again for some reason? */
		completeFileTransfer(state);
	}
	else if (state->ftMode != FT_MODE_CACHE)
	{
		/* lookup options */
		state->sources.clear();
		if (!lookupRemoteHash(state->hash, state->sources))
		{
        		pqioutput(PQL_WARNING, ftfilerzone,
	              		"ftfiler::resetFileTransfer() Failed to locate Peers");
		}
		if (state->sources.size() == 0)
		{
			state->id = "";
			return 0;
		}

		/* select a new source if possible */
		int idno = state->resetCount % state->sources.size();
		int i = 0;
		std::list<std::string>::const_iterator it;
		for(it = state->sources.begin(); (it != state->sources.end()) 
					&& (i < idno); it++, i++);

		if (it != state->sources.end())
		{
			state->id = (*it);
		}
	}
	state->resetCount++;
	return 1;
}



int ftfiler::addFileData(ftFileStatus *s, uint64_t idx, void *data, uint32_t size)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::addFileData()");

        //std::cerr << "ftfiler::addFileData() PreStatus" << std::endl;
	//printFtFileStatus(s, std::cerr);

	/* check the status */
	if ((!s) || (!s->fd) || (s->status & PQIFILE_FAIL))
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::addFileData() Bad Status");
		return 0;
	}

	/* check its at the correct location */
	if ((idx != s->recv_size) || (s->recv_size + size > s->total_size))
	{
		std::ostringstream out;
		out << "ftfiler::addFileData() Bad Data Location" << std::endl;
		out << " recv_size: " << s->recv_size << " offset: " << idx;
		out << " total_size: " << s->total_size << " size: " << size;
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
		return 0;
	}

	/* go to the end of the file */
	if (0 != fseek(s->fd, 0L, SEEK_END))
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::addFileData() Bad fseek");
		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_SEEK);
		return 0;
	}

	/* add the data */
	if (1 != fwrite(data, size, 1, s->fd))
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::addFileData() Bad fwrite");
		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_WRITE);
		return 0;
	}

	s->recv_size += size;


	/* if we've completed the request this time */
	if (s->req_loc + s->req_size == s->recv_size)
	{
		s->lastDelta = time(NULL) - s->lastTS;
	}

	if (s->recv_size == s->total_size)
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::addFileData() File Complete!");
		s->status = PQIFILE_COMPLETE;

		/* HANDLE COMPLETION HERE */
		completeFileTransfer(s);
	}

	return 1;
}


int ftfiler::completeFileTransfer(ftFileStatus *s)
{
	/* cleanup transfer */
	if (s->fd)
	{
		fclose(s->fd);
		s->fd = 0;

		// re-open in read mode (for transfers?)
		// don't bother ....
		// s->fd = fopen(s->file_name.c_str(), "r+b");

	}

	/* so now we move it to the expected destination */
	/* determine where it should go! */
	bool ok = true;

	std::string dest = determineDestFilePath(s);
	if (0 == rename(s->file_name.c_str(), dest.c_str()))
	{
		/* correct the file_name */
		s->file_name = dest;
	}
	else
	{
		ok = false;
	}

	/* do callback if CACHE */
	if (s->ftMode == FT_MODE_CACHE)
	{
		if (ok) 
		{
			CompletedCache(s->hash);
		}
		else
		{
			FailedCache(s->hash);
		}
	}
	return 1;
}


/***********************
 * Notes
 *
 * createFileCache is called: int ftfiler::handleFileRequest(PQFileItem *in) only.
 *
 * it should 
 * (1) create a ftFileStatus
 * (2) find it in the indices.
 * (3) load up the details.
 */


ftFileStatus *ftfiler::createFileCache(std::string hash)
{
        pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::createFileCache()");

	ftFileStatus *s = new ftFileStatus(hash, hash, 0, "", FT_MODE_UPLOAD);

	/* request from fileindex */
	bool found = false;

	/* so look it up! */
	std::string srcpath;
	uint64_t size;
	if (lookupLocalHash(s->hash, srcpath, size))
	{
		found = true;
		s->file_name = srcpath;
		s->size = size;
	}

	if ((!found) || (s->file_name.length() < 1))
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::createFileCache() Failed to Find File");
		/* failed to open the file */
		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_BAD_PATH);
		delete s;
		return NULL;
	}
		

	/* attempt to open file (readonly) */
	FILE *fd = fopen(s->file_name.c_str(), "rb");
	if (!fd)
	{
		std::stringstream out;
		out << "ftfiler::createFileCache() Failed to Open the File" << std::endl;
		out << "\tFull Path:" << s->file_name.c_str() << std::endl;
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone, out.str());
		/* failed to open the file */
		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_OPEN);
		delete s;
		return NULL;
	}

	/* if it opened, find it's length */
	/* move to the end */
	if (0 != fseek(fd, 0L, SEEK_END))
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	              "ftfiler::createFileCache() Fseek Failed");

		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_OPEN);
		delete s;
		return NULL;
	}

	s->recv_size  = ftell(fd); /* get the file length */
	s->total_size = s->size; /* save the total length */
	s->req_loc  = 0;	/* no request */
	s->req_size = 0;

	/* we are now ready for transfers */
	s->fd = fd;
	s->lastTS = 0;
	s->status = PQIFILE_DOWNLOADING;
	return s;
}




/**** 
 * NOTE this should move all temporary and cache files.
 * TODO!
 */

void    ftfiler::setSaveBasePath(std::string s)
{
	saveBasePath = s;
	return;
}


void    ftfiler::setEmergencyBasePath(std::string s)
{
	mEmergencyIncomingDir = s;
	return;
}



/***********************
 * Notes
 *
 * debugging functions.
 *
 */


void printFtFileStatus(ftFileStatus *s, std::ostream &out)
{
	/* main details */
	out << "FtFileStatus::Internals:" << std::endl;
	out << "name: " << s->name << std::endl;
	out << "hash: " << s->hash << std::endl;
	out << "destpath " << s->destpath << std::endl;
	// 

	out << "Source: " << s->id << std::endl;
	out << "Alt Srcs: ";
	std::list<std::string>::iterator it;
	for(it = s->sources.begin(); it != s->sources.end(); it++)
	{
		out << " " << (*it);
	}

	out << std::endl;
	out << " mode: " << s->mode;
	out << " ftMode: " << s->ftMode;
	out << " status: " << s->status;
	out << " resetCount: " << s->resetCount;
	out << std::endl;

	if (s->fd)
	{
		out << "FD Valid:   ";
	}
	else
	{
		out << "FD Invalid: ";
	}
	out << "file_name " << s->file_name << std::endl;

	out << " size " << s->size;
	out << " total_size " << s->total_size;
	out << " recv_size " << s->recv_size;
	out << " rate: " << s->rate;
	out << std::endl;
	out << " Req loc: " << s->req_loc;
	out << " Req size: " << s->req_size;
	out << std::endl;
	out << " last Delta: " << s->lastDelta;
	out << " last TS: " << s->lastTS;
	out << std::endl;
}


