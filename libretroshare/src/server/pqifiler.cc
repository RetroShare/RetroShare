/*
 * "$Id: pqifiler.cc,v 1.13 2007-02-19 20:08:30 rmf24 Exp $"
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




#include "server/pqifiler.h"

#include "pqi/pqidebug.h"
#include <errno.h>

#include <sstream>

const int pqifilerzone = 86539;

/* 
 * PQI Filer
 *
 * This managers the file transfers.
 *
 *
 * TODO: add trickle transfers.
 *
 */

const int PQIFILE_OFFLINE_CHECK  = 120; /* check every 2 minutes */
const int PQIFILE_DOWNLOAD_TIMEOUT  = 60; /* time it out, -> offline after 60 secs */
const int PQIFILE_DOWNLOAD_CHECK    = 10; /* desired delta = 10 secs */
const int PQIFILE_DOWNLOAD_TOO_FAST = 8; /* 8 secs */
const int PQIFILE_DOWNLOAD_TOO_SLOW = 12; /* 12 secs */
const int PQIFILE_DOWNLOAD_MIN_DELTA = 5; /* 5 secs */

const float TRANSFER_MODE_TRICKLE_RATE = 1000;   /* 1   kbyte limit */
const float TRANSFER_MODE_NORMAL_RATE  = 500000; /* 500 kbyte limit - everyone uses this one for now */
const float TRANSFER_MODE_FAST_RATE    = 500000; /* 500 kbyte limit */

const int TRANSFER_START_MIN = 500;  /* 500 byte  min limit */
const int TRANSFER_START_MAX = 2000; /* 2000 byte max limit */

#ifdef USE_FILELOOK
pqifiler::pqifiler(fileLook *fd)
	:fileIndex(fd) { return; }
#else
pqifiler::pqifiler(filedex *fd)
	:fileIndex(fd) { return; }
#endif


/************* Local File Interface ****************************
 *
 * int	pqifiler::getFile(PQFileItem *in);
 * int	pqifiler::cancelFile(PQFileItem *i);
 * int	pqifiler::clearFailedTransfers();
 *
 * * Worker Fns.
 * PQFileStatus *pqifiler::findRecvFileItem(PQFileItem *in);
 */

int	pqifiler::getFile(PQFileItem *in)
{
	/* add to local queue */
	{
		std::ostringstream out;
		out << "pqifiler::getFile(): " << std::endl;
		in -> print(out);
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
	}



	/* check for duplicates */
	PQFileStatus *state = findRecvFileItem(in);
	if (state)
	{
		std::ostringstream out;
		out << "pqifiler::getFile() - duplicate, giving push!";
		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());

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

	state = new PQFileStatus(in);
	if (initiateFileTransfer(state))
	{
		std::ostringstream out;
		out << "pqifiler::getFile() ";
		out << "adding to recvFiles queue";
		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());

		recvFiles.push_back(state);
	}
	return 1;
}

PQFileStatus *pqifiler::findRecvFileItem(PQFileItem *in)
{
	{
		std::ostringstream out;
		out << "pqifiler::findRecvFileItem() Looking for: " << in->name;
		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
	}

	/* iterate through fileItems and check for this one */
	std::list<PQFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end(); it++)
	{
		if ((in->name==(*it)->fileItem->name) &&
		   	(in->hash==(*it)->fileItem->hash))
		{
			std::ostringstream out;
			out << "pqifiler::findRecvFileItem() ";
			out << "Match PQFileStatus: " << in -> name;
			pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
			/* same */
			return (*it);
		}
	}
	return NULL;
}




int	pqifiler::cancelFile(PQFileItem *i)
{
	/* flag as cancelled */
	/* iterate through fileItems and check for this one */
	{
		std::ostringstream out;
		out << "pqifiler::cancelFile(): " << std::endl;
		i -> print(out);
		out << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
	}

	std::list<PQFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end(); it++)
	{
		if ((i->name==(*it)->fileItem->name) &&
		   	(i->hash==(*it)->fileItem->hash))
		{
			std::ostringstream out;
			out << "pqifiler::cancelFile() ";
			out << "Found file: " << i -> name;
			pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());

			/* same */
			(*it)->status = (PQIFILE_FAIL | PQIFILE_FAIL_CANCEL);
			return 1;
		}
	}

	{
		std::ostringstream out;
		out << "pqifiler::cancelFile() ";
		out << "couldn't match PQFileStatus!";
		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
	}
	return 0;
}

int	pqifiler::clearFailedTransfers()
{
	/* remove all the failed items */
	/* iterate through fileItems and check for this one */
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::clearFailedTransfers()");

	std::list<PQFileStatus *>::iterator it;
	int cleared = 0;
	for(it = recvFiles.begin(); it != recvFiles.end(); /* done in loop */)
	{
		if ((*it)->status & PQIFILE_FAIL)
		{
			std::ostringstream out;
			out << "pqifiler::clearFailedTransfers() ";
			out << "removing item: " << (*it) -> fileItem -> name;
			pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());

			/* same */
			PQFileStatus *cfile = (*it);
			it = recvFiles.erase(it);
			delete cfile;
			cleared++;
		}
		else if ((*it)->status & PQIFILE_COMPLETE)
		{
			std::ostringstream out;
			out << "pqifiler::clearFailedTransfers() ";
			out << "removing Completed item: ";
			out << (*it) -> fileItem -> name;
			pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());

			/* same */
			PQFileStatus *cfile = (*it);
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
		out << "pqifiler::clearFailedTransfers() cleared: ";
		out << cleared;
		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
	}

	return 1;
}


std::list<FileTransferItem *> pqifiler::getStatus()
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::getTransferStatus()");
	std::list<FileTransferItem *>  stateList;

	/* iterate through all files to recv */
	std::list<PQFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end(); it++)
	{
		FileTransferItem *fti = new FileTransferItem();
		fti -> PQFileItem::copy((*it)->fileItem);

		/* Fill in rate and State */
		fti -> transferred = (*it)->recv_size;
		fti -> crate = (*it)->rate / 1000.0; // kbytes.
		fti -> trate = (*it)->rate / 1000.0; // kbytes.
		fti -> lrate = (*it)->rate / 1000.0; // kbytes.
		fti -> ltransfer = (*it)->req_size;
		fti -> in = true;

		/* get inactive period */
		if ((*it) -> status == PQIFILE_NOT_ONLINE)
		{
			fti -> crate = 0;
			fti -> trate = 0;
			fti -> lrate = 0;
			fti -> ltransfer = 0;
			fti -> state = FT_STATE_OKAY;
		}
		else if ((*it) -> status & PQIFILE_FAIL)
		{
			fti -> crate = 0;
			fti -> trate = 0;
			fti -> lrate = 0;
			fti -> ltransfer = 0;
			fti -> state = FT_STATE_FAILED;

		}
		else if ((*it) -> status == PQIFILE_COMPLETE)
		{
			fti -> state = FT_STATE_COMPLETE;
		}
		else if ((*it) -> status == PQIFILE_DOWNLOADING)
		{
			fti -> state = FT_STATE_OKAY;
		}
		else
		{
			fti -> state = FT_STATE_FAILED;
		}
		stateList.push_back(fti);
	}
	return stateList;
}


/************* Incoming FileItems ******************************
 *
 * PQFileItem *pqifiler::sendPQFileItem()
 * int	pqifiler::recvPQFileItem(PQFileItem *in)
 *
 * * Worker Fns.
 * int pqifiler::handleFileNotOnline(PQFileItem *in)
 * int pqifiler::handleFileNotOnline(PQFileItem *in)
 * int pqifiler::handleFileNotAvailable(PQFileItem *in)
 * int pqifiler::handleFileData(PQFileItem *in)
 * int pqifiler::handleFileRequest(PQFileItem *in)
 * int pqifiler::handleFileCacheRequest(PQFileItem *req)
 *
 */

PQItem *pqifiler::sendPQFileItem()
{
	if (out_queue.size() < 1)
	{
		return NULL;
	}
	PQItem *i = out_queue.front();
	out_queue.pop_front();
	{
		std::ostringstream out;
		out << "pqifiler::sendPQFileItem() ";
		out << "returning: " << std::endl;
		i -> print(out);
		pqioutput(PQL_DEBUG_ALL, pqifilerzone, out.str());
	}
	return i;
}

int	pqifiler::recvPQFileItem(PQItem *item)
{
	/* check what type */
	PQFileItem *in;

	{
		std::ostringstream out;
		out << "pqifiler::recvPQFileItem() ";
		out << "input: " << std::endl;
		item -> print(out);
		pqioutput(PQL_DEBUG_ALL, pqifilerzone, out.str());
	}

	if (NULL == (in = dynamic_cast<PQFileItem *>(item)))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::recvPQFileItem() Error Not PQFileItem");
		delete item;
		return 0;
	}


	switch(in -> subtype)
	{
		case PQI_FI_SUBTYPE_ERROR: /* not currently connected */
		 	handleFileError(in);
			break;
		case PQI_FI_SUBTYPE_DATA: /* received some data */
			handleFileData(in);
			break;
		case PQI_FI_SUBTYPE_REQUEST:
			handleFileRequest(in);
			break;
		default:
			/* what ? */
			break;
	}

	/* clean up */
	delete in;
	return 1;
}



int pqifiler::handleFileError(PQFileItem *in)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::handleFileError()");
	/* get out the error */
	if (in->fileoffset | PQIFILE_NOT_ONLINE)
	{
		return handleFileNotOnline(in);
	}
	if (in->fileoffset & PQIFILE_FAIL)
	{
		return handleFileNotAvailable(in);
	}
	return 0;
}

int pqifiler::handleFileNotOnline(PQFileItem *in)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::handleFileNotOnline()");
	/* flag recvFile item as not Online */
	PQFileStatus *s = findRecvFileItem(in);
	if ((!s) || (s -> status & PQIFILE_FAIL))
	{
		return 0;
	}

	s -> status = PQIFILE_NOT_ONLINE;

	return 1;
}


int pqifiler::handleFileNotAvailable(PQFileItem *in)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::handleFileNotAvailable()");
	/* error - flag recvFile item with FAILED */
	PQFileStatus *s = findRecvFileItem(in);
	if (!s)
	{
		return 0;
	}

	s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_AVAIL);
	return 1;
}


int pqifiler::handleFileData(PQFileItem *in)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::handleFileData()");
	/* find the right PQFileStatus */
	PQFileStatus *recv = findRecvFileItem(in);
	if (!recv)
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::handleFileData() no matching PQFileStatus (current download)");
		return 0;
	}

	if(recv->status & PQIFILE_FAIL)
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::handleFileData() marked as FAIL");
		return 0;
	}

	PQFileData *dta;
	if (NULL == (dta = dynamic_cast<PQFileData *>(in)))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::handleFileData() not PQFileData");
		return 0;
	}

	/* first check the cid matches, so we can get it right for later */
	if (0 != pqicid_cmp(&(in->cid), &(recv->fileItem->cid)))
	{
		/* not matched */
        	pqioutput(PQL_WARNING, pqifilerzone,
	       		       "pqifiler::handleFileData() correcting fileItem->cid");
		pqicid_copy(&(in->cid), &(recv->fileItem->cid));

                std::ostringstream out;
                out << "pqifiler::handleFileData() in->cid != recv->fileItem->cid";
                out << std::endl;
                out << "in -> CID   [" << in->cid.route[0];
                for(int i = 0; i < 10; i++)
                {
                        out << ":" << in->cid.route[i];
		}
	        out << "]" << std::endl;

		out << "recv -> CID [" << recv->fileItem->cid.route[0];
	        for(int i = 0; i < 10; i++)
	        {
			out << ":" << recv->fileItem->cid.route[i];
		}
	        out << "]" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, pqifilerzone,out.str());
	}


	/* add to file */
 	addFileData(recv, dta->fileoffset, dta->data, dta->datalen);

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

int pqifiler::handleFileRequest(PQFileItem *in)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::handleFileRequest()");
	/* see if in cache */
	/* if yes send out chunk */
	if (handleFileCacheRequest(in))
	{
		return 1;
	}

	/* if not in cache - find file */
	PQFileStatus *new_file = createFileCache(in);
	if (!new_file)
	{
		/* bad file */
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	        	      "pqifiler::handleFileRequest() Failed to Load File-sendNotAvail");
		return sendFileNotAvail(in);
	}

	fileCache.push_back(new_file);

	return handleFileCacheRequest(in);
}

int pqifiler::handleFileCacheRequest(PQFileItem *req)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::handleFileCacheRequest()");
	/* check if in cache */
	bool found = false;
	PQFileStatus *s;
	std::list<PQFileStatus *>::iterator it;
	for(it = fileCache.begin(); (!found) && (it != fileCache.end()); it++)
	{
		if ((req->name==(*it)->fileItem->name) &&
		   	(req->hash==(*it)->fileItem->hash))
		{
			found = true;
			s = (*it);
		}
	}
	if (!found)
		return 0;

	/* push to out queue */
	return generateFileData(s, req);
}

/************* Outgoing FileItems ******************************
 *
 * PQFileItem *pqifiler::sendPQFileItem()
 *
 * * Worker Fns.
 * int	pqifiler::tick();
 * void	pqifiler::queryInactive()
 *
 */


int	pqifiler::tick()
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::tick()");
	/* check the status of recved files */
	queryInactive();

	/* this doesn't matter much how often it's ticked...
	 * if it generates Items, they will be detected other places.
	 * so we can return 0 (waiting)
	 */
	return 0;
}


void	pqifiler::queryInactive()
{
	std::ostringstream out;

	out << "pqifiler::queryInactive()";
	out << std::endl;



	/* iterate through all files to recv */
	int ts = time(NULL);
	std::list<PQFileStatus *>::iterator it;
	for(it = recvFiles.begin(); it != recvFiles.end(); it++)
	{
		/* get inactive period */
		switch((*it) -> status)
		{
		case PQIFILE_NOT_ONLINE:
			out << "File: " << (*it)->fileItem->name << " Not Online: ";
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
			out << "File: " << (*it)->fileItem->name << " Downloading: ";
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
			out << "File: " << (*it)->fileItem->name << " Other mode: " << (*it)->status;
			out << " Delta: " << (ts - (*it)->lastTS) << std::endl;
			out << " LDelta: " << (*it)->lastDelta;
			out << " Recved: " << (*it)->recv_size;
			out << " Total: " << (*it)->total_size;
			out << " LChunk: " << (*it)->req_size;
			out << std::endl;
			/* nothing */
			break;
		}
	}
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
}


int 	pqifiler::requestData(PQFileStatus *item)
{

	/* formulate a request for the next desired data chunk */
	/* this handles the key throttling. This algorithm can
	 * be quite crude, as the tcp / and pqistreamer throttle as 
	 * well.
	 */

	std::ostringstream out;
	out << "pqifiler::requestData()" << std::endl;

	/* get the time since last request */
	int tm = time(NULL);
	float delta = tm - item -> lastTS;




	/* decide on transfer mode */
	float max_rate;
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
			max_rate = TRANSFER_MODE_NORMAL_RATE;
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
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
		return 0;
	}


	if (delta <= PQIFILE_DOWNLOAD_MIN_DELTA)
	{
		/* pause */
		out << "Small Delta -> Pause";
		out << std::endl;
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
		return 0;
	}

	/* From this point - we will continue ... so handle rate now! */
	/* calc rate */
	float bytes_psec = item -> req_size / delta;
	item -> rate = 0.9 * item -> rate + 0.1 * bytes_psec;
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
		 * added an extra limiter. */
		item->req_size = (int) (0.01 * max_rate);
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
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());

	PQFileItem *req = generatePQFileRequest(item);
	out_queue.push_back(req);

	return 1;
}

/************* PQIFILEITEM Generator ***************************
 *
 * PQFileItem *pqifiler::generatePQFileRequest(PQFileStatus *s);
 * int pqifiler::generateFileData(PQFileStatus *s, PQFileItem *req);
 * int pqifiler::sendFileNotAvail(PQFileItem *req)
 *
 */


int	pqifiler::generateFileData(PQFileStatus *s, PQFileItem *req)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::generateFileData()");

	if ((!s) || (!s->fd) || (s->status & PQIFILE_FAIL))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::generateFileData() Bad Status");
		if (!s)
		{
        		pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::generateFileData() Bad Status (!s)");
		}
		if (!s->fd)
		{
        		pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::generateFileData() Bad Status (!s->fd)");
		}
		if (s->status & PQIFILE_FAIL)
		{
			std::ostringstream out;
	       		out << "pqifiler::generateFileData() Bad Status (s->status): " << s->status;
        		pqioutput(PQL_DEBUG_BASIC, pqifilerzone,out.str());
		}

		/* return an error */
		return sendFileNotAvail(req);
	}

	/* make the packets */
	int tosend    = req -> chunksize;
	long base_loc = req -> fileoffset;

	if (base_loc + tosend > s -> total_size)
	{
		tosend = s -> total_size - base_loc;
	}

	{
		std::ostringstream out;
		out << "pqifiler::generateFileData() Sending " << tosend;
		out << " bytes from offset: " << base_loc << std::endl;
		out << "\tFrom File:" << req -> name;
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
	}

		
	while(tosend > 0)
	{
		int pktsize = 5 * 1024;
		if (pktsize > tosend)
			pktsize = tosend;

		/* seek for base_loc */
		fseek(s->fd, base_loc, SEEK_SET);

                // make a FileData type.
		PQFileData *fd = new PQFileData();

		// Copy details from the Request.
		fd -> PQFileItem::copy(req);

		// PQItem  
		fd -> sid = req -> sid;
		fd -> type = PQI_ITEM_TYPE_FILEITEM;
		fd -> subtype = PQI_FI_SUBTYPE_DATA;

		// PQFileITem
        	fd -> size = s->fileItem->size;   // total size of file.
		fd -> fileoffset = base_loc;
		fd -> chunksize = pktsize;

	        // data
		fd -> datalen = pktsize;
		fd -> data = malloc(fd -> datalen);
		fd -> fileflags = 0;
		{
			std::ostringstream out;
			out << "pqifiler::generateFileData() pkt:" << std::endl;
			//fd -> print(out);
        		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
		}

		/* read the data */
		if (1 != fread(fd -> data, fd -> datalen, 1, s->fd))
		{
			std::ostringstream out;

			out << "pqifiler::generateFileData() Read only: ";
			out << fd->datalen << "/" << pktsize << " bytes of data - Discarding";

        		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());

			//free(fd->data);
			//fd->data = NULL;
			delete fd;
			return 0;
		}

		/* decrement sizes */
		base_loc += pktsize;
		tosend   -= pktsize;

		/* send off the packet */
		out_queue.push_back(fd);
	}
	return 1;
}


PQFileItem *pqifiler::generatePQFileRequest(PQFileStatus *s)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::generatePQFileRequest()");

	PQFileItem *fi = s->fileItem->clone();

	/* set req_loc, and req_size */

	// PQItem  
	fi -> sid = getPQIsearchId();
	fi -> type = PQI_ITEM_TYPE_FILEITEM;
	fi -> subtype = PQI_FI_SUBTYPE_REQUEST;

	// PQFileITem
        fi -> size = s->fileItem->size;   // total size of file.
	fi -> fileoffset = s->req_loc;
	fi -> chunksize =  s->req_size;

	std::ostringstream out;

	out << "pqifiler::generatePQFileRequest() for: " << s->fileItem->name << std::endl;
	out << "pqifiler::generatePQFileRequest() offset: " << fi->fileoffset << " chunksize: ";
	out << fi->chunksize << std::endl;

	//out << "s->fileItem:" << std::endl;
	//s->fileItem->print(out);
	//out << "DataRequest:" << std::endl;
	//fi->print(out);

 	pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());

	// timestamp request.
	s->lastTS = time(NULL);

	return fi;
}

int pqifiler::sendFileNotAvail(PQFileItem *req)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::sendFileNotAvail()");
	PQFileItem *fi = req -> clone();

	/* set error code */
	fi -> subtype = PQI_FI_SUBTYPE_ERROR;
	fi -> fileoffset = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_AVAIL);

	/* send out */
	out_queue.push_back(fi);
	return 1;
}


/************* FILE DATA HANDLING ******************************
 *
 * std::string pqifiler::determineFilePath(PQFileItem *item);
 * int pqifiler::initiateFileTransfer(PQFileStatus *s);
 * int pqifiler::resetFileTransfer(PQFileStatus *s);
 * int pqifiler::addFileData(PQFileStatus *s, long idx, void *data, int size);
 *
 */

std::string pqifiler::determineFilePath(PQFileItem *item)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::determineFilePath()");

	/* get the download path */
	// savePath = ".";
	std::string filePath = savePath;
	filePath += "/";
	filePath += item->name;
	return filePath;
}

int pqifiler::initiateFileTransfer(PQFileStatus *s)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::initiateFileTransfer()");

	/* check if the file exists */
	if (s->file_name.length() < 1)
	{
		s->file_name = determineFilePath(s->fileItem);
	}

	{
		std::ostringstream out;
		out << "pqifiler::initiateFileTransfer() Filename: ";
		out << s->file_name;
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
	}

	/* attempt to open file */
	FILE *fd = fopen(s->file_name.c_str(), "r+b");
	if (!fd)
	{
		{
		std::ostringstream out;
		out << "pqifiler::initiateFileTransfer() Failed to open: ";
		out << s->file_name << " Error:" << errno;
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
		}

		/* open in writing mode */
		fd = fopen(s->file_name.c_str(), "w+b");
		if (!fd)
		{
			std::ostringstream out;
			out << "pqifiler::initiateFileTransfer() Failed to open: ";
			out << s->file_name << " Error:" << errno;
        		pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
		
			/* failed to open the file */
			s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_OPEN);
			return 0;
		}

	}


	/* if it opened, find it's length */
	/* move to the end */
	if (0 != fseek(fd, 0L, SEEK_END))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::initiateFileTransfer() Seek Failed");
		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_SEEK);
		return 0;
	}

	s->recv_size  = ftell(fd); /* get the file length */
	s->total_size = s->fileItem->size; /* save the total length */
	s->fd = fd;

	resetFileTransfer(s);
	return 1;
}


int pqifiler::resetFileTransfer(PQFileStatus *state)
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
	}

	return 1;
}



int pqifiler::addFileData(PQFileStatus *s, long idx, void *data, int size)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::addFileData()");

	/* check the status */
	if ((!s) || (!s->fd) || (s->status & PQIFILE_FAIL))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::addFileData() Bad Status");
		return 0;
	}

	/* check its at the correct location */
	if ((idx != s->recv_size) || (s->recv_size + size > s->total_size))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::addFileData() Bad Data Location");
		return 0;
	}

	/* go to the end of the file */
	if (0 != fseek(s->fd, 0L, SEEK_END))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::addFileData() Bad fseek");
		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_SEEK);
		return 0;
	}

	/* add the data */
	if (1 != fwrite(data, size, 1, s->fd))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::addFileData() Bad fwrite");
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
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	       		       "pqifiler::addFileData() File Complete!");
		s->status = PQIFILE_COMPLETE;
		fclose(s->fd);
		// re-open in read mode (for transfers?)
		s->fd = fopen(s->file_name.c_str(), "r+b");
	}

	return 1;
}


PQFileStatus *pqifiler::createFileCache(PQFileItem *in)
{
        pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::createFileCache()");

	PQFileStatus *s = new PQFileStatus(in->clone());

	/* request from fileindex */
	bool found = false;

	/* so here we will work with
	 */

#ifdef USE_FILELOOK
	PQFileItem *real = fileIndex -> findFileEntry(in);
	if (real)
	{
		s->file_name = real -> path + "/" + real -> name;
		found = true;
	}

#else /*************************************************************************/
	std::list<fdex *> flist = fileIndex -> findfilename(in->name);
	std::list<fdex *>::iterator it;
	for(it = flist.begin(); (!found) && (it != flist.end()); it++)
	{
		if (in -> size == (*it) -> len)
		{
			found = true;
			s->file_name = (*it) -> path;
		}
	}
#endif

	if ((!found) || (s->file_name.length() < 1))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::createFileCache() Failed to Find File");
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
		out << "pqifiler::createFileCache() Failed to Open the File" << std::endl;
		out << "\tFull Path:" << s->file_name.c_str() << std::endl;
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone, out.str());
		/* failed to open the file */
		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_OPEN);
		delete s;
		return NULL;
	}

	/* if it opened, find it's length */
	/* move to the end */
	if (0 != fseek(fd, 0L, SEEK_END))
	{
        	pqioutput(PQL_DEBUG_BASIC, pqifilerzone,
	              "pqifiler::createFileCache() Fseek Failed");

		s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_OPEN);
		delete s;
		return NULL;
	}

	s->recv_size  = ftell(fd); /* get the file length */
	s->total_size = s->fileItem->size; /* save the total length */
	s->req_loc  = 0;	/* no request */
	s->req_size = 0;

	/* we are now ready for transfers */
	s->fd = fd;
	s->lastTS = 0;
	s->status = PQIFILE_DOWNLOADING;
	return s;
}
