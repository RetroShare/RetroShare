/*
 * "$Id: filedexserver.cc,v 1.24 2007-05-05 16:10:06 rmf24 Exp $"
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




/* So this is a test pqi server.....
 *
 * the idea is that this holds some
 * random data....., and responds to 
 * requests of a pqihandler.
 *
 */

// as it is a simple test...
// don't make anything hard to do.
//

#include "server/filedexserver.h"
#include <fstream>
#include <time.h>

#include "pqi/pqibin.h"
#include "pqi/pqiarchive.h"
#include "pqi/pqidebug.h"

#include "util/rsdir.h"
#include <sstream>
#include <iomanip>


/* New FileCache Stuff */
#include "pqi/pqimon.h"
#include "server/ftfiler.h"
#include "dbase/cachestrapper.h"
#include "dbase/fimonitor.h"
#include "dbase/fistore.h"


#include <sstream>

const int fldxsrvrzone = 47659;


/* Another little hack ..... unique message Ids
 * will be handled in this class.....
 * These are unique within this run of the server, 
 * and are not stored long term....
 *
 * Only 3 entry points:
 * (1) from network....
 * (2) from local send
 * (3) from storage...
 */

static unsigned int msgUniqueId = 1;
unsigned int getNewUniqueMsgId()
{
	return msgUniqueId++;
}



filedexserver::filedexserver()
	:pqisi(NULL), sslr(NULL), 

	ftFiler(NULL), 
	
	save_dir("."), 
	msgChanged(1), msgMajorChanged(1), chatChanged(1)
#ifdef PQI_USE_CHANNELS
	,channelsChanged(1)
#endif
{
#ifdef PQI_USE_CHANNELS
	p3chan = NULL;
#endif
	initialiseFileStore();

}

int	filedexserver::setSearchInterface(P3Interface *si, sslroot *sr)
{
	pqisi = si;
	sslr = sr;
	return 1;
}


std::list<FileTransferItem *> filedexserver::getTransfers()
{
	return ftFiler->getStatus();
}






int	filedexserver::tick()
{
	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::tick()");

	/* the new Cache Hack() */
	FileStoreTick();

	if (pqisi == NULL)
	{
		std::ostringstream out;
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
			"filedexserver::tick() Invalid Interface()");

		return 1;
	}

	int moreToTick = 0;

	if (0 < pqisi -> tick())
	{
		moreToTick = 1;
	}

	if (0 < handleInputQueues())
	{
		moreToTick = 1;
	}

	if (0 < handleOutputQueues())
	{
		moreToTick = 1;
	}

	if (0 < getChat())
	{
		moreToTick = 1;
	}
	checkOutgoingMessages(); /* don't worry about increasing tick rate! */

	return moreToTick;
}


int	filedexserver::status()
{
	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::status()");

	if (pqisi == NULL)
	{
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
			"filedexserver::status() Invalid Interface()");
		return 1;
	}

	pqisi -> status();

	return 1;
}

std::string	filedexserver::getSaveDir()
{
	return save_dir;
}

void	filedexserver::setSaveDir(std::string d)
{
	save_dir = d;
	ftFiler -> setSaveBasePath(save_dir);
}


bool 	filedexserver::getSaveIncSearch()
{
	return save_inc;
}

void	filedexserver::setSaveIncSearch(bool v)
{
	save_inc = v;
}


/***************** Chat Stuff **********************/

int     filedexserver::sendChat(std::string msg)
{
	// make chat item....
	ChatItem *ci = new ChatItem();
	ci -> sid = getPQIsearchId();
	ci -> p = sslr -> getOwnCert();
	ci -> flags = 0;

	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::sendChat()");

	ci -> msg = msg;

	/* will come back to us anyway (for now) */
	//ichat.push_back(ci -> clone());
	//chatChanged.IndicateChanged();

	{
	  std::ostringstream out;
	  out << "Chat Item we are sending:" << std::endl;
	  ci -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());
	}

	/* to global .... */
	pqisi -> SendGlobalMsg(ci);
	return 1;
}

int     filedexserver::sendPrivateChat(ChatItem *ci)
{
	// make chat item....
	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::sendPrivateChat()");

	{
	  std::ostringstream out;
	  out << "Private Chat Item we are sending:" << std::endl;
	  ci -> print(out);
	  pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());
	}

	/* to global .... */
	pqisi -> SendMsg(ci);
	return 1;
}


int 	filedexserver::getChat()
{
	ChatItem *ci;
	int i = 0;
	while((ci = pqisi -> GetMsg()) != NULL)
	{
		++i;
		ci -> epoch = time(NULL);
		std::string mesg;

		if (ci -> subtype == PQI_MI_SUBTYPE_MSG)
		{
			// then a message..... push_back.
			MsgItem *mi = (MsgItem *) ci;
			if (mi -> p == sslr->getOwnCert())
			{
				/* from the loopback device */
				mi -> msgflags = PQI_MI_FLAGS_OUTGOING;
			}
			else
			{
				/* from a peer */
				mi -> msgflags = 0;
			}

			/* new as well! */
			mi -> msgflags |= PQI_MI_FLAGS_NEW;
			/* STORE MsgID */
			mi -> sid = getNewUniqueMsgId();

			imsg.push_back(mi);
			//nmsg.push_back(mi);
			msgChanged.IndicateChanged();
		}
		else
		{
			// push out the chat instead.
			ichat.push_back(ci);
			chatChanged.IndicateChanged();
		}
	}

	if (i > 0)
	{
		return 1;
	}
	return 0;
}
	

std::list<ChatItem *> filedexserver::getChatQueue()
{
	std::list<ChatItem *> ilist = ichat;
	ichat.clear();
	return ilist;
}


std::list<MsgItem *> &filedexserver::getMsgList()
{
	return imsg;
}

std::list<MsgItem *> &filedexserver::getMsgOutList()
{
	return msgOutgoing;
}



std::list<MsgItem *> filedexserver::getNewMsgs()
{
	std::list<MsgItem *> tmplist = nmsg;
	nmsg.clear();
	return tmplist;
}

/* remove based on the unique mid (stored in sid) */
int     filedexserver::removeMsgId(unsigned long mid)
{
	std::list<MsgItem *>::iterator it;

	for(it = imsg.begin(); it != imsg.end(); it++)
	{
		if ((*it)->sid == mid)
		{
			MsgItem *mi = (*it);
			imsg.erase(it);
			delete mi;
			msgChanged.IndicateChanged();
			msgMajorChanged.IndicateChanged();
			return 1;
		}
	}

	/* try with outgoing messages otherwise */
	for(it = msgOutgoing.begin(); it != msgOutgoing.end(); it++)
	{
		if ((*it)->sid == mid)
		{
			MsgItem *mi = (*it);
			msgOutgoing.erase(it);
			delete mi;
			msgChanged.IndicateChanged();
			msgMajorChanged.IndicateChanged();
			return 1;
		}
	}

	return 0;
}

int     filedexserver::markMsgIdRead(unsigned long mid)
{
	std::list<MsgItem *>::iterator it;

	for(it = imsg.begin(); it != imsg.end(); it++)
	{
		if ((*it)->sid == mid)
		{
			MsgItem *mi = (*it);
			mi -> msgflags &= ~(PQI_MI_FLAGS_NEW);
			msgChanged.IndicateChanged();
			return 1;
		}
	}
	return 0;
}

int     filedexserver::removeMsgItem(int itemnum)
{
	std::list<MsgItem *>::iterator it;
	int i;

	for(i = 1, it = imsg.begin(); (i != itemnum) && (it != imsg.end()); it++, i++);
	if (it != imsg.end())
	{
		MsgItem *mi = (*it);
		imsg.erase(it);
		delete mi;
		msgChanged.IndicateChanged();
		msgMajorChanged.IndicateChanged();
		return 1;
	}
	return 0;
}


int     filedexserver::removeMsgItem(MsgItem *mi)
{
	std::list<MsgItem *>::iterator it;
	for(it = imsg.begin(); (mi != *it) && (it != imsg.end()); it++);
	if (it != imsg.end())
	{
		imsg.erase(it);
		delete mi;
		msgChanged.IndicateChanged();
		msgMajorChanged.IndicateChanged();
		return 1;
	}
	return 0;
}

/* This is the old fltkgui send recommend....
 * can only handle one single file....
 * will maintain this fn.... but it is deprecated.
 */

int     filedexserver::sendRecommend(PQFileItem *fi, std::string msg)
{
	// make chat item....
	MsgItem *mi = new MsgItem();
	mi -> sid = getPQIsearchId();

	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::sendRecommend()");

	mi -> msg = msg;

	if (fi != NULL)
	{
		MsgFileItem mfi;
		mfi.name = fi -> name;
		mfi.hash = fi -> hash;
		mfi.size = fi -> size;

		mi -> files.push_back(mfi);
	}
	else
	{
		/* nuffink */
	}

	pqisi -> SendMsg(mi);
	return 1;
}


int     filedexserver::sendMessage(MsgItem *item)
{
	item -> sid = getPQIsearchId();

	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"filedexserver::sendMessage()");

	if (item -> p)
	{
		/* add pending flag */
		item->msgflags |= 
			(PQI_MI_FLAGS_OUTGOING | 
			 PQI_MI_FLAGS_PENDING);
		/* STORE MsgID */
		item -> sid = getNewUniqueMsgId();
		msgOutgoing.push_back(item);
	}
	else
	{
		delete item;
	}
	return 1;
}

int     filedexserver::checkOutgoingMessages()
{
	/* iterate through the outgoing queue 
	 *
	 * if online, send
	 */

	std::list<MsgItem *>::iterator it;
	for(it = msgOutgoing.begin(); it != msgOutgoing.end();)
	{
	 	/* if online, send it */
		if (((*it) -> p -> Status() & PERSON_STATUS_CONNECTED) 
		    || ((*it) -> p == sslr->getOwnCert()))
		{
			/* send msg */
			pqioutput(PQL_ALERT, fldxsrvrzone, 
				"filedexserver::checkOutGoingMessages() Sending out message");
			/* remove the pending flag */
			(*it)->msgflags &= ~PQI_MI_FLAGS_PENDING;

			pqisi -> SendMsg(*it);
			it = msgOutgoing.erase(it);
		}
		else
		{
			pqioutput(PQL_ALERT, fldxsrvrzone, 
				"filedexserver::checkOutGoingMessages() Delaying until available...");
			it++;
		}
	}
	return 0;
}


int     filedexserver::addSearchDirectory(std::string dir)
{
	dbase_dirs.push_back(dir);
	reScanDirs();
	return 1;
}

int     filedexserver::removeSearchDirectory(std::string dir)
{
	std::list<std::string>::iterator it;
	for(it = dbase_dirs.begin(); (it != dbase_dirs.end()) 
			&& (dir != *it); it++);
	if (it != dbase_dirs.end())
	{
		dbase_dirs.erase(it);
	}

	reScanDirs();
	return 1;
}

std::list<std::string> &filedexserver::getSearchDirectories()
{
	return dbase_dirs;
}


int     filedexserver::reScanDirs()
{
	fimon->setSharedDirectories(dbase_dirs);

	return 1;
}

static const std::string fdex_dir("FDEX_DIR");
static const std::string save_dir_ss("SAVE_DIR");
static const std::string save_inc_ss("SAVE_INC");
static const std::string NETWORK_ss("NET_PARAM");

int     filedexserver::save_config()
{
	std::list<std::string>::iterator it;
	std::string empty("");

	pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, 
		"fildexserver::save_config()");

	/* basic control parameters */
	{
	  std::ostringstream out;
	  out << getDHTEnabled() << " " << getUPnPEnabled();
	  sslr -> setSetting(NETWORK_ss, out.str());
	}


	sslr -> setSetting(save_dir_ss, getSaveDir());
	if (getSaveIncSearch())
	{
		sslr -> setSetting(save_inc_ss, "true");
	}
	else
	{
		sslr -> setSetting(save_inc_ss, "false");
	}

	int i;
	for(it = dbase_dirs.begin(), i = 0; (it != dbase_dirs.end()) 
		&& (i < 1000); it++, i++)
	{
		std::string name = fdex_dir;
		int d1, d2, d3;
		d1 = i / 100;
		d2 = (i - d1 * 100) / 10;
		d3 = i - d1 * 100 - d2 * 10;

		name += '0'+d1;
		name += '0'+d2;
		name += '0'+d3;

		sslr -> setSetting(name, (*it));
	}
	// blank other ones.
	bool done = false;
	for(; (i < 1000) && (!done); i++)
	{
		std::string name = fdex_dir;
		int d1, d2, d3;
		d1 = i / 100;
		d2 = (i - d1 * 100) / 10;
		d3 = i - d1 * 100 - d2 * 10;

		name += '0'+d1;
		name += '0'+d2;
		name += '0'+d3;

		if (empty == sslr -> getSetting(name))
		{
			done = true;
		}
		else
		{
			sslr -> setSetting(name, empty);

			std::ostringstream out;
			out << "Blanking Setting(" << name;
			out << ")" << std::endl;
			pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());
		}
	}


	/* save the local cache file name */
	FileCacheSave();

	/* now we create a pqiarchive, and stream all the msgs/fts
	 * into it
	 */

	std::string statelog = config_dir + "/state.rst";

	BinFileInterface *out = new BinFileInterface((char *) statelog.c_str(), BIN_FLAGS_WRITEABLE);
        pqiarchive *pa_out = new pqiarchive(out, BIN_FLAGS_WRITEABLE, sslr);
	bool written = false;

	std::list<MsgItem *>::iterator mit;
	for(mit = imsg.begin(); mit != imsg.end(); mit++)
	{
		MsgItem *mi = (*mit)->clone();
		if (pa_out -> SendItem(mi))
		{
			written = true;
		}
		
	}

	for(mit = msgOutgoing.begin(); mit != msgOutgoing.end(); mit++)
	{
		MsgItem *mi = (*mit)->clone();
		mi -> msgflags |= PQI_MI_FLAGS_PENDING;
		if (pa_out -> SendItem(mi))
		{
			written = true;
		}
		
	}

	std::list<FileTransferItem *>::iterator fit;
	std::list<FileTransferItem *> ftlist = ftFiler -> getStatus();
	for(fit = ftlist.begin(); fit != ftlist.end(); fit++)
	{
		/* only write out the okay/uncompleted (with hash) files */
		if (((*fit)->state != FT_STATE_OKAY) || ((*fit)->hash == ""))
		{
			delete(*fit);
		}
		else if (pa_out -> SendItem(*fit))
		{
			written = true;
		}
	}

	if (!written)
	{
		/* need to push something out to overwrite old data! (For WINDOWS ONLY) */
	}

	delete pa_out;	
	return 1;
}

int     filedexserver::load_config()
{
	std::list<std::string>::iterator it;

	int i;
	std::string empty("");
	std::string dir("notempty");
	std::string str_true("true");

	std::string snet = sslr -> getSetting(NETWORK_ss);
	{
	  int a, b;

	  // on by default
	  setDHTEnabled(true);
	  setUPnPEnabled(true); /* UPnP false -> until its totally happy */

	  if (2 == sscanf(snet.c_str(), "%d %d", &a, &b))
	  {
	  	setDHTEnabled(a);
		setUPnPEnabled(b);
	  }
	}


	std::string sdir = sslr -> getSetting(save_dir_ss);
	if (sdir != empty)
	{
		setSaveDir(sdir);
	}

	std::string sinc = sslr -> getSetting(save_inc_ss);
	if (sdir != empty)
	{
		setSaveIncSearch(sdir == str_true);
	}

	dbase_dirs.clear();

	for(i = 0; (i < 1000) && (dir != empty); i++)
	{
		std::string name = fdex_dir;
		int d1, d2, d3;
		d1 = i / 100;
		d2 = (i - d1 * 100) / 10;
		d3 = i - d1 * 100 - d2 * 10;

		name += '0'+d1;
		name += '0'+d2;
		name += '0'+d3;

		dir = sslr -> getSetting(name);
		if (dir != empty)
		{
			dbase_dirs.push_back(dir);
		}
	}
	if (dbase_dirs.size() > 0)
	{
		std::ostringstream out;
		out << "Loading " << dbase_dirs.size();
		out << " Directories" << std::endl;
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());

		reScanDirs();
	}

	/* load msg/ft */
	std::string statelog = config_dir + "/state.rst";

	// XXX Fix Interface!
	BinFileInterface *in = new BinFileInterface((char *) statelog.c_str(), BIN_FLAGS_READABLE);
        pqiarchive *pa_in = new pqiarchive(in, BIN_FLAGS_READABLE, sslr);
	PQItem *item;
	MsgItem *mitem;
	PQFileItem *fitem;

	while((item = pa_in -> GetItem()))
	{
		switch(item->type)
		{
			case PQI_ITEM_TYPE_FILEITEM:
				/* add to ft queue */
				if (NULL != (fitem = dynamic_cast<PQFileItem *>(item)))
				{
					/* only add in ones which have a hash (filters old versions) */
					if (fitem->hash != "")
					{
						ftFiler -> getFile(fitem->name, fitem->hash, 
								fitem->size, "");
					}
				}
				delete item;
				break;
			case PQI_ITEM_TYPE_CHATITEM:
				if (NULL != (mitem = dynamic_cast<MsgItem *>(item)))
				{
					/* switch depending on the PENDING 
					 * flags
					 */
					/* STORE MsgID */
					mitem->sid = getNewUniqueMsgId();
					if (mitem -> msgflags & PQI_MI_FLAGS_PENDING)
					{
						std::cerr << "MSG_PENDING";
						std::cerr << std::endl;
						mitem->print(std::cerr);
						msgOutgoing.push_back(mitem);
					}
					else
					{
						imsg.push_back(mitem);
					}
				}
				else
				{
					delete item;
				}
				/* add to chat queue */
				break;
			default:
				/* unexpected */
				break;
		}
	}

	delete pa_in;	

	return 1;
}


#ifdef PQI_USE_CHANNELS

        // Channel stuff.
void    filedexserver::setP3Channel(p3channel *p3c)
{
	p3chan = p3c;
	return;
}

int 	filedexserver::getAvailableChannels(std::list<pqichannel *> &chans)
{
	if (!p3chan)
	{
		pqioutput(PQL_ALERT, fldxsrvrzone, 
			"fildexserver::getAvailableChannels() p3chan == NULL");
		return 0;
	}
	return p3chan->getChannelList(chans);
}


int 	filedexserver::getChannelMsgList(channelSign s, std::list<chanMsgSummary> &summary)
{
	if (!p3chan)
	{
		pqioutput(PQL_ALERT, fldxsrvrzone, 
			"fildexserver::getChannelMsgList() p3chan == NULL");
		return 0;
	}
	pqichannel *chan = p3chan -> findChannel(s);
	if (!chan)
	{
		pqioutput(PQL_ALERT, fldxsrvrzone, 
			"fildexserver::getChannelMsgList() Channel don't exist!");
		return 0;
	}
	return chan -> getMsgSummary(summary);
}


channelMsg *filedexserver::getChannelMsg(channelSign s, MsgHash mh)
{
	if (!p3chan)
	{
		pqioutput(PQL_ALERT, fldxsrvrzone, 
			"fildexserver::getChannelMsg() p3chan == NULL");
		return NULL;
	}
	pqichannel *chan = p3chan -> findChannel(s);
	if (!chan)
	{
		pqioutput(PQL_ALERT, fldxsrvrzone, 
			"fildexserver::getChannelMsg() Channel don't exist!");
		return NULL;
	}
	channelMsg *msg = chan -> findMsg(mh);
	if (!msg)
	{
		pqioutput(PQL_ALERT, fldxsrvrzone, 
			"fildexserver::getChannelMsg() Msg don't exist!");
		return NULL;
	}
	return msg;
}


	
#endif


void filedexserver::loadWelcomeMsg()
{
	/* Load Welcome Message */
	MsgItem *msg = new MsgItem();

	msg -> p = getSSLRoot() -> getOwnCert();

	msg -> title  = "Welcome to Retroshare";
	msg -> header = "Basic Instructions";
	msg -> sendTime = 0;

	msg -> msg    = "Send and receive messages\n"; 
	msg -> msg   += "with your friends...\n\n";

	msg -> msg   += "These can hold recommendations\n";
	msg -> msg   += "from your local shared files\n\n";

	msg -> msg   += "Add recommendations through\n";
	msg -> msg   += "the Local Files Dialog\n\n";

	msg -> msg   += "Enjoy.\n";

	imsg.push_back(msg);	
}

/*************************************** NEW File Cache Stuff ****************************/

void filedexserver::initialiseFileStore()
{

}

const std::string LOCAL_CACHE_FILE_KEY = "LCF_NAME";
const std::string LOCAL_CACHE_HASH_KEY = "LCF_HASH";
const std::string LOCAL_CACHE_SIZE_KEY = "LCF_SIZE";

void    filedexserver::setFileCallback(NotifyBase *cb)
{
	/* work out our id! */
	cert *own = getSSLRoot()->getOwnCert();
	certsign sign;
	getSSLRoot()->getcertsign(own, sign);
	RsPeerId ownId = convert_to_str(sign);

	uint32_t queryPeriod = 60; /* query every 1 minutes -> change later to 600+ */

	/* setup the pqimonitor */
	peerMonitor = new pqimonitor();

	cacheStrapper = new CacheStrapper(ownId, queryPeriod);
	/* fiFiler is also CacheTransfer */
	ftFiler = new ftfiler(cacheStrapper);

	/* setup FiStore/Monitor */
	std::string localcachedir = config_dir + "/cache/local";
	std::string remotecachedir = config_dir + "/cache/remote";
	fiStore = new FileIndexStore(ftFiler, cb, ownId, remotecachedir);

	/* now setup the FiMon */
	fimon = new FileIndexMonitor(localcachedir, ownId);

	/* setup ftFiler
	 * to find peer info / savedir 
	 */
	FileHashSearch *fhs = new FileHashSearch(fiStore, fimon);
	ftFiler -> setFileHashSearch(fhs);
	ftFiler -> setSaveBasePath(save_dir);

	/* now add the set to the cachestrapper */

	CachePair cp(fimon, fiStore, CacheId(CACHE_TYPE_FILE_INDEX, 0));
	cacheStrapper -> addCachePair(cp);

	/* add to peermonitor */
	peerMonitor -> addClient(cacheStrapper);


	/* now we can load the cache configuration */
	//std::string cacheconfig = config_dir + "/caches.cfg";
	//cacheStrapper -> loadCaches(cacheconfig);

	/************ TMP HACK LOAD until new serialiser is finished */
	/* get filename and hash from configuration */
	std::string localCacheFile = getSSLRoot()->getSetting(LOCAL_CACHE_FILE_KEY);
	std::string localCacheHash = getSSLRoot()->getSetting(LOCAL_CACHE_HASH_KEY);
	std::string localCacheSize = getSSLRoot()->getSetting(LOCAL_CACHE_SIZE_KEY);

	std::list<std::string> saveLocalCaches;
	std::list<std::string> saveRemoteCaches;

	if ((localCacheFile != "") && 
		(localCacheHash != "") && 
		(localCacheSize != ""))
	{
		/* load it up! */
		std::string loadCacheFile = localcachedir + "/" + localCacheFile;
		CacheData cd;
		cd.pid = ownId;
		cd.cid = CacheId(CACHE_TYPE_FILE_INDEX, 0);
		cd.name = localCacheFile;
		cd.path = localcachedir;
		cd.hash = localCacheHash;
		cd.size = atoi(localCacheSize.c_str());
		fimon -> loadCache(cd);

		saveLocalCaches.push_back(cd.name);
	}

	/* cleanup cache directories */
	RsDirUtil::cleanupDirectory(localcachedir, saveLocalCaches);
	RsDirUtil::cleanupDirectory(remotecachedir, saveRemoteCaches); /* clean up all */

	/************ TMP HACK LOAD until new serialiser is finished */


	/* startup the FileMonitor (after cache load) */
	fimon->setPeriod(600); /* 10 minutes */
	/* start it up */
	fimon->setSharedDirectories(dbase_dirs);
	fimon->start();

}

int filedexserver::FileCacheSave()
{
	/************ TMP HACK SAVE until new serialiser is finished */

	RsPeerId pid;
	std::map<CacheId, CacheData> ids;
	std::map<CacheId, CacheData>::iterator it;

	std::cerr << "filedexserver::FileCacheSave() listCaches:" << std::endl;
	fimon->listCaches(std::cerr);
	fimon->cachesAvailable(pid, ids);

	std::string localCacheFile;
	std::string localCacheHash;
	std::string localCacheSize;

	if (ids.size() == 1)
	{
		it = ids.begin();
		localCacheFile = (it->second).name;
		localCacheHash = (it->second).hash;
		std::ostringstream out;
		out << (it->second).size;
		localCacheSize = out.str();
	}

	/* extract the details of the local cache */
	getSSLRoot()->setSetting(LOCAL_CACHE_FILE_KEY, localCacheFile);
	getSSLRoot()->setSetting(LOCAL_CACHE_HASH_KEY, localCacheHash);
	getSSLRoot()->setSetting(LOCAL_CACHE_SIZE_KEY, localCacheSize);

	/************ TMP HACK SAVE until new serialiser is finished */
	return 1;
}


// Transfer control.
int filedexserver::getFile(std::string fname, std::string hash,
                        uint32_t size, std::string dest)

{
	// send to filer.
	return ftFiler -> getFile(fname, hash, size, dest);
}

void filedexserver::clear_old_transfers()
{
	ftFiler -> clearFailedTransfers();
}

void filedexserver::cancelTransfer(std::string fname, std::string hash, uint32_t size)
{
	ftFiler -> cancelFile(hash);
}


int filedexserver::RequestDirDetails(std::string uid, std::string path,
                                        DirDetails &details)
{
	return fiStore->RequestDirDetails(uid, path, details);
}

int filedexserver::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags)
{
	return fiStore->RequestDirDetails(ref, details, flags);
}

int filedexserver::SearchKeywords(std::list<std::string> keywords, 
					std::list<FileDetail> &results)
{
	return fiStore->SearchKeywords(keywords, results);
}

int filedexserver::SearchBoolExp(Expression * exp, std::list<FileDetail> &results)
{
	return fiStore->searchBoolExp(exp, results);
}


int filedexserver::FileStoreTick()
{
	peerMonitor -> tick();
	ftFiler -> tick();
	return 1;
}


// This function needs to be divided up.
int     filedexserver::handleInputQueues()
{
	// get all the incoming results.. and print to the screen.
	SearchItem *si;
	PQFileItem *fi;
	PQFileItem *pfi;
	// Loop through Search Results.
	int i = 0;
	int i_init = 0;

	//std::cerr << "filedexserver::handleInputQueues()" << std::endl;
	while((fi = pqisi -> GetSearchResult()) != NULL)
	{
		//std::cerr << "filedexserver::handleInputQueues() Recvd SearchResult (CacheResponse!)" << std::endl;
		std::ostringstream out;
		if (i++ == i_init)
		{
			out << "Recieved Search Results:" << std::endl;
		}
		fi -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());

		/* these go to the CacheStrapper! */
		CacheData data;
		data.cid = CacheId(fi->fileoffset, fi->chunksize);
		data.hash = fi->hash;
		data.size = fi->size;
		data.name = fi->name;
		data.path = fi->path;

		certsign sign;

		if (getSSLRoot() -> getcertsign((cert *) fi->p, sign))
		{
			data.pid = convert_to_str(sign);
			data.pname = fi->p->Name();
			cacheStrapper->recvCacheResponse(data, time(NULL));
		}
		else
		{
			std::cerr << "ERROR";
			exit(1);
		}

		delete fi;
	}

	// now requested Searches.
	i_init = i;
	while((si = pqisi -> RequestedSearch()) != NULL)
	{
		//std::cerr << "filedexserver::handleInputQueues() Recvd RequestedSearch (CacheQuery!)" << std::endl;
		std::ostringstream out;
		if (i++ == i_init)
		{
			out << "Requested Search:" << std::endl;
		}
		si -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());

		/* these go to the CacheStrapper (handled immediately) */
		std::map<CacheId, CacheData>::iterator it;
		std::map<CacheId, CacheData> answer;
		RsPeerId id;

		cacheStrapper->handleCacheQuery(id, answer);
		for(it = answer.begin(); it != answer.end(); it++)
		{
			//std::cerr << "filedexserver::handleInputQueues() Sending (CacheAnswer!)" << std::endl;
			/* construct reply */
			PQFileItem *fi = new PQFileItem();
	
			/* PQItem ones (from incoming) */
			fi -> p = si -> p;
			fi -> cid = si -> cid;
			/* type/subtype already done, flags/search id ignored */

			fi -> hash = (it->second).hash;
			fi -> name = (it->second).name;
			fi -> path = ""; // (it->second).path;
			fi -> size = (it->second).size;
			fi -> fileoffset = (it->second).cid.type;
			fi -> chunksize =  (it->second).cid.subid;

			std::ostringstream out2;
			out2 << "Outgoing CacheStrapper reply -> PQFileItem:" << std::endl;
			fi -> print(out2);
			pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out2.str());
			pqisi -> SendFileItem(fi);
		}

		delete si;
	}

	// now Cancelled Searches.
	i_init = i;
	while((si = pqisi -> CancelledSearch()) != NULL)
	{
		std::ostringstream out;
		if (i++ == i_init)
		{
			out << "Deleting Cancelled Search:" << std::endl;
		}
		si -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());
		delete si;
	}

	// now File Input.
	i_init = i;
	while((pfi = pqisi -> GetFileItem()) != NULL )
	{
		//std::cerr << "filedexserver::handleInputQueues() Recvd ftFiler Data" << std::endl;
		std::ostringstream out;
		if (i++ == i_init)
		{
			out << "Incoming(Net) File Item:" << std::endl;
		}
		pfi -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());
		certsign sign;
		if (!getSSLRoot() -> getcertsign((cert *) pfi->p, sign))
		{
			std::cerr << "ERROR FFR";
			exit(1);
		}
		std::string id = convert_to_str(sign);

		PQFileData *pfd = dynamic_cast<PQFileData *>(pfi);
		if (pfd)
		{
			/* incoming data */
			ftFileData *ffd = new ftFileData(id, pfd->hash, pfd->size, 
					pfd->fileoffset, pfd->chunksize, pfd->data);
			pfd -> data = NULL;
			ftFiler->recvFileInfo(ffd);
		}
		else
		{
			/* request */

			ftFileRequest *ffr = new ftFileRequest(id, pfi->hash, 
				pfi->size, pfi->fileoffset, pfi->chunksize);
			ftFiler->recvFileInfo(ffr);
		}
		delete pfi;
	}

	if (i > 0)
	{
		return 1;
	}
	return 0;
}


// This function needs to be divided up.
int     filedexserver::handleOutputQueues()
{
	// get all the incoming results.. and print to the screen.
	//std::cerr << "filedexserver::handleOutputQueues()" << std::endl;
	int i = 0;

	std::list<RsPeerId> ids;
        std::list<RsPeerId>::iterator pit;

	cacheStrapper->sendCacheQuery(ids, time(NULL));

	for(pit = ids.begin(); pit != ids.end(); pit++)
	{
		//std::cerr << "filedexserver::handleOutputQueues() Cache Query for: " << (*pit) << std::endl;

		/* now create one! */
		SearchItem *si = new SearchItem();

		/* set it up */
		certsign sign;
		if (!convert_to_certsign(*pit, sign))
		{
			std::cerr << "CERTSIGN error!" << std::endl;
			exit(1);
		}

		/* look it up */
		cert *c = getSSLRoot() -> findcertsign(sign);
		if (c == NULL)
		{
			std::cerr << "CERTSIGN error! 2" << std::endl;
			exit(1);
		}

		si->p = c;
		si->cid = c->cid;

		std::ostringstream out;
		if (i++ == 0)
		{
			out << "Outgoing CacheStrapper -> SearchItem:" << std::endl;
		}
		si -> print(out);
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());

		/* send it off */
		pqisi -> SearchSpecific(si);
	}

	/* now see if the filer has any data */
	ftFileRequest *ftr;
	while((ftr = ftFiler -> sendFileInfo()) != NULL)
	{
		//std::cerr << "filedexserver::handleOutputQueues() ftFiler Data for: " << ftr->id << std::endl;

		/* work out who its going to */
		certsign sign;
		if (!convert_to_certsign(ftr->id, sign))
		{
			std::cerr << "CERTSIGN error! 3" << std::endl;
			exit(1);
		}

		/* look it up */
		cert *c = getSSLRoot() -> findcertsign(sign);
		if (c == NULL)
		{
			std::cerr << "CERTSIGN error! 4" << std::endl;
			exit(1);
		}

		/* decide if its data or request */
		ftFileData *ftd = dynamic_cast<ftFileData *>(ftr);
		if (ftd)
		{
			SendFileData(ftd, c);
		}
		else
		{
			SendFileRequest(ftr, c);
		}

		std::ostringstream out;
		if (i++ == 0)
		{
			out << "Outgoing filer -> PQFileItem:" << std::endl;
		}
		pqioutput(PQL_DEBUG_BASIC, fldxsrvrzone, out.str());

		/* clean up */
		delete ftr;
	}



	if (i > 0)
	{
		return 1;
	}
	return 0;
}

void filedexserver::SendFileRequest(ftFileRequest *ftr, cert *peer)
{
	/* send request */
	PQFileItem *fi = new PQFileItem();
	fi -> subtype = PQI_FI_SUBTYPE_REQUEST;

	fi -> p = peer;
	fi -> cid = peer->cid;
	/* type/subtype already done, flags/search id ignored */

	fi -> hash = ftr -> hash;
	/* name, path, ext are ignored */
	fi -> size = ftr -> size;
	fi -> fileoffset = ftr->offset;
	fi -> chunksize = ftr->chunk;

	pqisi -> SendFileItem(fi);
}

#define MAX_FT_CHUNK 4096

void filedexserver::SendFileData(ftFileData *ftd, cert *peer)
{
	uint32_t tosend = ftd->chunk;
	uint32_t baseoffset = ftd->offset;
	uint32_t offset = 0;
	uint32_t chunk;


	while(tosend > 0)
	{
		/* workout size */
		chunk = MAX_FT_CHUNK;
		if (chunk > tosend)
		{
			chunk = tosend;
		}

		/* send data! */
		PQFileData *fid = new PQFileData();

		fid -> p = peer;
		fid -> cid = peer->cid;
		fid -> hash = ftd -> hash;
		fid -> size = ftd -> size;

		fid -> data = malloc(chunk);
		memcpy(fid->data, &(((uint8_t *) ftd->data)[offset]), chunk);
		fid -> datalen = chunk;
		fid -> chunksize = chunk;
		fid -> fileoffset = baseoffset + offset;

		pqisi -> SendFileItem(fid);

		offset += chunk;
		tosend -= chunk;
	}
}


