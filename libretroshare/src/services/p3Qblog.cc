/*
 * libretroshare/src/services: p3Qblog.cc
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Chris Evi-Parker
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

#include "serialiser/rsqblogitems.h"
#include "services/p3Qblog.h"
#include <utility>
#include <ctime>
#include <iomanip>
#include "pqi/pqibin.h"
#include "pqi/p3authmgr.h"

const uint32_t RANK_MAX_FWD_OFFSET = (60 * 60 * 24 * 2); /* 2 Days */

const uint32_t FRIEND_QBLOG_REPOST_PERIOD = 60; /* every minute for testing */

#define QBLOG_DEBUG 1

p3Qblog::p3Qblog(p3ConnectMgr *connMgr, 
		uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir, 
		uint32_t storePeriod)
	:CacheSource(type, true, cs, sourcedir), 
	CacheStore(type, true, cs, cft, storedir), 
	p3Config(CONFIG_TYPE_QBLOG), mConnMgr(connMgr), mFilterSwitch(false),
	mStorePeriod(storePeriod), mUpdated(true)
{
	{	
		RsStackMutex stack(mBlogMtx);
		mOwnId = mConnMgr->getOwnId(); // get your own usr Id
		loadDummy(); // load dummy data
		
		#ifdef QBLOG_DEBUG
		/*if(!ok)
			std::cerr << "initialization failed: could not retrieve friend list";	*/
		#endif
		
	}
	
	return;
}

bool p3Qblog::loadLocalCache(const CacheData &data)
{
	std::string filename = data.path + '/' + data.name;
	std::string hash = data.hash;
	std::string source = data.pid;
	
	#ifdef QBLOG_DEBUG
	std::cerr << "p3Ranking::loadLocalCache()";
	std::cerr << std::endl;
	std::cerr << "\tSource: " << source;
	std::cerr << std::endl;
	std::cerr << "\tFilename: " << filename;
	std::cerr << std::endl;
	std::cerr << "\tHash: " << hash;
	std::cerr << std::endl;
	std::cerr << "\tSize: " << data.size;
	std::cerr << std::endl;
	#endif

	loadBlogFile(filename, source);
	
	{
		RsStackMutex stack(mBlogMtx);
		mRepost = false; // there is nothing to tick/repost (i.e. comes from cache)
	}

	if (data.size > 0) /* don't refresh zero sized caches */
	{
		refreshCache(data);
	}
	
	return true;
}


int p3Qblog::loadCache(const CacheData &data)
{
	std::string filename = data.path + '/' + data.name;
	std::string hash = data.hash;
	std::string source = data.pid;

	#ifdef QBLOG_DEBUG
	std::cerr << "p3Qblog::loadCache()";
	std::cerr << std::endl;
	std::cerr << "\tSource: " << source;
	std::cerr << std::endl;
	std::cerr << "\tFilename: " << filename;
	std::cerr << std::endl;
	std::cerr << "\tHash: " << hash;
	std::cerr << std::endl;
	std::cerr << "\tSize: " << data.size;
	std::cerr << std::endl;
	#endif

	loadBlogFile(filename, source);


	CacheStore::lockData();   /*****   LOCK ****/
	locked_storeCacheEntry(data); 
	CacheStore::unlockData(); /***** UNLOCK ****/

	return 1;
}

bool p3Qblog::loadBlogFile(std::string filename, std::string src)
{
	/* create the serialiser to load info */
	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsQblogSerialiser());
	
	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE;
	BinInterface *bio = new BinFileInterface(filename.c_str(), bioflags); 
	pqistreamer *stream = new pqistreamer(rsSerialiser, src, bio, 0);
	
	#ifdef QBLOG_DEBUG
	
	std::cerr << "p3Qblog::loadBlogFile()";
	std::cerr << std::endl;
	std::cerr << "\tSource: " << src;
	std::cerr << std::endl;
	std::cerr << "\tFilename: " << filename;
	std::cerr << std::endl;
	
	#endif
	
	/* will load file info to these items */
	RsItem *item;
	RsQblogItem *newBlog;
	
	stream->tick(); // tick to read
	
	time_t now = time(NULL);
	time_t min, max;
	
	{ 	 /********** STACK LOCKED MTX ******/
		RsStackMutex stack(mBlogMtx);
		min = now - mStorePeriod;
		max = now + RANK_MAX_FWD_OFFSET;
     } 	/********** STACK LOCKED MTX ******/
	
	while(NULL != (item = stream->GetItem()))
	{
		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::loadBlogFile() Got Item:";
		std::cerr << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
		#endif
		
		if (NULL == (newBlog = dynamic_cast<RsQblogItem *>(item)))
		{
			#ifdef QBLOG_DEBUG
			std::cerr << "p3Qblog::loadBlogFile() Item not Blog (deleting):";
			std::cerr << std::endl;
			#endif

			delete item;
		}
					/* check timestamp */
		else if (((time_t) newBlog->blogMsg.first < min) || 
				((time_t) newBlog->blogMsg.first > max))
		{
			#ifdef QBLOG_DEBUG
			std::cerr << "p3Qblog::loadBlogFile() Outside TimeRange (deleting):";
			std::cerr << std::endl;
			#endif
			/* if outside  time range remove */
			delete newBlog;
		}
		else
		{
			#ifdef QBLOG_DEBUG
			std::cerr << "p3Qblog::loadBlogFile() Loading Item";
			std::cerr << std::endl;
			#endif

			addBlog(newBlog); // add received blog to list
		}
		
		stream->tick();	// tick to read
	}
		
	delete stream; // stream finished with/return resource
	return true;
}

bool p3Qblog::addBlog(RsQblogItem *newBlog)
{
	#ifdef QBLOG_DEBUG
	std::cerr << "p3Ranking::addBlog() Item:";
	std::cerr << std::endl;
	newBlog->print(std::cerr, 10); // check whats in the blog item
	std::cerr << std::endl;
	#endif
	
	{
		RsStackMutex Stack(mBlogMtx);
		
		mPeerSongSet[newBlog->pid] = newBlog->favSong;
		mPeerStatusSet[newBlog->pid] = newBlog->status;
		mUsrBlogSet[newBlog->pid].insert(newBlog->blogMsg);
		
		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::addBlog()";
		std::cerr << std::endl;
		std::cerr << "pid" << newBlog->pid;
		std::cerr << std::endl;
		std::cerr << "\tmPeerSongset" << mPeerSongSet[newBlog->pid];
		std::cerr << std::endl;
		std::cerr << "\tmPeerStatusSet" << mPeerStatusSet[newBlog->pid];
		std::cerr << std::endl;
		std::cerr << "\tmUsrBlogSet: time" << newBlog->blogMsg.first;
		std::cerr << std::endl;
		std::cerr << "\tmUsrBlogSet: blog" << newBlog->blogMsg.second;
		std::cerr << std::endl;
		#endif 
		mUpdated = true; // might be useless
		mRepost = true;
	}	
	
	return true;
}
	
bool p3Qblog::postBlogs(void)
{
	
	#ifdef QBLOG_DEBUG
	std::cerr << "p3Qblog::postBlogs()";
	std::cerr << std::endl;
	#endif

	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	uint16_t subid = 1;
	
	out << "qblogs-" << time(NULL) << ".rsqb"; // create blog file name based on time posted
	
	/* determine filename */
	std::string tmpname = out.str();
	std::string fname = path + "/" + tmpname;
	
	#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::publishMsgs() Storing to: " << fname;
	std::cerr << std::endl;
	#endif

	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsQblogSerialiser());
	
	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE;
	BinInterface *bio = new BinFileInterface(fname.c_str(), bioflags);
	pqistreamer *stream = new pqistreamer(rsSerialiser, mOwnId, bio,
					BIN_FLAGS_NO_DELETE);
					
	{ 	
		RsStackMutex stack(mBlogMtx); /********** STACK LOCKED MTX ******/
		
			/* iterate through list */
		std::list<RsQblogItem*>::iterator it; 
		
		for(it = mBlogItems.begin(); it != mBlogItems.end();  it++)
		{
			/* only post own blogs */
			if(mOwnId == (*it)->pid)
			{
				/*write to serialiser*/
				RsItem *item = *it;
				
				#ifdef QBLOG_DEBUG
				std::cerr << "p3Qblog::postBlogs() Storing Item:";
				std::cerr << std::endl;
				item->print(std::cerr, 10);
				std::cerr << std::endl;
				#endif
				
				stream->SendItem(item);
				stream->tick(); /* tick to write */
			}
			else /* if blogs belong to a friend */
			{
				#ifdef QBLOG_DEBUG
				std::cerr << "p3Ranking::postBlogs() Skipping friend blog items:";
				std::cerr << std::endl;
				#endif
				
				continue;
			}		
		}

	}
		
			
	stream->tick(); /* Tick for final write! */
		
	/* flag as new info */
	CacheData data;
		
	{ 	/********** STACK LOCKED MTX ******/
		RsStackMutex stack(mBlogMtx); 
		data.pid = mOwnId;
	} 	/********** STACK LOCKED MTX ******/
		
	data.cid = CacheId(CacheSource::getCacheType(), subid);
		
	data.path = path;
	data.name = tmpname;
		
	data.hash = bio->gethash();
	data.size = bio->bytecount();
	data.recvd = time(NULL);
			
	#ifdef RANK_DEBUG
	std::cerr << "p3Ranking::postBlogs() refreshing Cache";
	std::cerr << std::endl;
	std::cerr << "\tCache Path: " << data.path;
	std::cerr << std::endl;
	std::cerr << "\tCache Name: " << data.name;
	std::cerr << std::endl;
	std::cerr << "\tCache Hash: " << data.hash;
	std::cerr << std::endl;
	std::cerr << "\tCache Size: " << data.size;
	std::cerr << std::endl;
	#endif
			
	if(data.size > 0) /* don't refresh zero sized caches */
		{
			refreshCache(data);
		}
		
	delete stream;
	return true;		
}
		
		
	 

p3Qblog::~p3Qblog()
{
	return;
}


bool p3Qblog::setStatus(const std::string &status)
{
	{
		RsStackMutex stack(mBlogMtx);
		mPeerStatusSet[mOwnId] = status;
		
		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::getStatus() mPeerStatus[mOwnId]" << mPeerStatusSet[mOwnId];
		std::cerr << std::endl;
		#endif
	}
	
	return true;
}

bool p3Qblog::getFilterSwitch(void)
{
	{ 
		RsStackMutex stack(mBlogMtx); // might be pointless
		return mFilterSwitch; 
	}
}

bool p3Qblog::setFilterSwitch(bool &filterSwitch)
{
	{
		RsStackMutex stack(mBlogMtx);
		mFilterSwitch = filterSwitch;
		
		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::setFilterSwitch() " << mFilterSwitch;
		std::cerr << std::endl;
		#endif
	}	
	return true;
}

bool p3Qblog::getStatus(std::map<std::string, std::string> &usrStatus)
{
	{
		RsStackMutex stack(mBlogMtx);
		
		#ifdef QBLOG_DEBUG
		if(mPeerStatusSet.empty())
		{
			std::cerr << "p3Qblog::getStatus() mPeerStatusSet empty! ";
			std::cerr << std::endl;
			return false;
		}
		#endif
		
		usrStatus = mPeerStatusSet;
	}
		
	return true;
}

bool p3Qblog::removeFiltFriend(std::string &usrId)
{
	std::list<std::string>::iterator it;
	
	{
		RsStackMutex stack(mBlogMtx);
		
		/* search through list to remove friend */
		for(it = mFilterList.begin(); it != mFilterList.end(); it++)
		{
			if(it->compare(usrId))
			{
				#ifdef QBLOG_DEBUG
				std::cerr << "p3Qblog::removeFilterFriend() it " << *it;
				std::cerr << std::endl;
				#endif
				mFilterList.erase(it); // remove friend from list
				return true; 
			}
		}
	}
	
	#ifdef QBLOG_DEBUG
	std::cerr << "usr could not be found!" << std::endl;
	#endif
	
	return false; // could not find friend 
}

bool p3Qblog::addToFilter(std::string& usrId)
{
	{
		RsStackMutex stack(mBlogMtx);
		std::list<std::string>::iterator it;
		
		/* search through list in case friend already exist */
		for(it = mFilterList.begin(); it != mFilterList.end(); it++)
		{
			if(it->compare(usrId))
			{
				#ifdef QBLOG_DEBUG
				std::cerr << "usr already in list!" << *it;
				std::cerr << std::endl;
				#endif
				
				return false; // user already in list, not added
			}
		}
	
		mFilterList.push_back(usrId);
	}
	
	return true;
}
	
bool p3Qblog::getBlogs(std::map< std::string, std::multimap<long int, std:: string> > &blogs)
{
	{	
		RsStackMutex stack(mBlogMtx);
		
		if(mUsrBlogSet.empty()) // return error blogs are empty
		{
			std::cerr << "usr blog set empty!" << std::endl;
			return false;
		}
		
		blogs = mUsrBlogSet;
		return true;
		
		#ifdef QBOG_DEBUG
		std::cerr << "p3Qblog::getBlogs() number of blogs: " << mUsrBlogSet.size();
		std::cerr << std::endl;
		#endif
	}
}
	
bool p3Qblog::sendBlog(const std::string &msg)
{
	time_t blogTimeStamp;
	
	{ 
		RsStackMutex stack(mBlogMtx);
		mUsrBlogSet[mOwnId].insert(std::make_pair(blogTimeStamp, msg));
	
		RsQblogItem *blogItem = new RsQblogItem();
		blogItem->clear();
		
		blogItem->pid = mOwnId;
		blogItem->blogMsg.first = blogTimeStamp;
		blogItem->blogMsg.second = msg;
		blogItem->favSong = mPeerSongSet[mOwnId];
		blogItem->status = mPeerStatusSet[mOwnId];
		
		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::sendBlogFile()";
		std::cerr << std::endl;
		std::cerr << "\tblogItem->pid" << blogItem->blogMsg.first;
		std::cerr << std::endl;
		std::cerr << "\tblogItem->blogMsg.first" << blogItem->blogMsg.first;
		std::cerr << std::endl;
		std::cerr << "\tblogItem->blogMsg.second" << blogItem->blogMsg.second;
		std::cerr << std::endl;
		std::cerr << "\tblogItem->favSong" << blogItem->favSong;
		std::cerr << std::endl;
		std::cerr << "\tblogItem->status" << blogItem->status;
		std::cerr << std::endl; 
		#endif
		
		mBlogItems.push_back(blogItem);
	}	
	return true;
}

bool p3Qblog::getProfile(std::map<std::string, std::string> &profile)
{	
	{
		RsStackMutex stack(mBlogMtx);
	
		/* return error is set empty */
		if(mPeerSongSet.empty())
		{
			std::cerr << "friend song set empty!" << std::endl;
			return false;
		} 
	
		profile = mPeerSongSet;
	}
	
	return true;
}

bool p3Qblog::setProfile(const std::string &favSong)
{
	{
		RsStackMutex stack(mBlogMtx);
		mPeerSongSet[mOwnId] = favSong;
	}
	
	return true;
} 

void p3Qblog::loadDummy(void)
{
	std::list<std::string> peers;
	
	mConnMgr->getFriendList(peers); // retrieve peers list from core
	if(peers.empty())
	{
		for(int i = 0; i < 50; i++)
			std::cerr << "nothing in peer list!!!" << std::endl;
	}
	
	
	srand(60);
	long int now = time(NULL); // the present time
	std::multimap<long int, std::string> emptySet; // time/blog map
	
	std::string statusSet[5] = { "great", "rubbish", "ecstatic", "save me", "emo depression"};
	std::string songs[5] = { "broken spleen", "niobium", "ewe (a sheep)", "velvet stuff", "chun li kicks"};

	/* the usr dummy usr blogs */	
	std::string B1 = "I think we should eat more cheese";
	std::string B2 = "today was so cool, i got attacked by fifty ninja while buying a loaf so i used my paper bag to suffocate each of them to death at hyper speed";
	std::string B3 = "Nuthins up";
	std::string B4 = "stop bothering me";
	std::string B5 = "I'm really a boring person and having nothin interesting to say";
	std::string blogs[5] = {B1, B2, B3, B4, B5};
	
	/* fill up maps: first usrblogset with empty blogs */
	
	std::list<std::string>::iterator it; 
	
		mUsrBlogSet.insert(std::make_pair(mOwnId, emptySet));
		mPeerSongSet.insert(std::make_pair(mOwnId, songs[rand() % 5]));
		mPeerStatusSet.insert(std::make_pair(mOwnId, statusSet[rand() % 5]));
	
	for(it = peers.begin(); it!=peers.end();it++)
	{
		mUsrBlogSet.insert(std::make_pair(*it, emptySet));
		mPeerSongSet.insert(std::make_pair(*it, songs[rand() % 5]));
		mPeerStatusSet.insert(std::make_pair(*it, statusSet[rand() % 5]));
	}

	/* now fill up blog map */
	
	for(int i=0; i < 50 ; i++)
	{
		std::list<std::string>::iterator it = peers.begin();
		long int timeStamp;
		timeStamp = now + rand() % 2134223;
		mUsrBlogSet[mOwnId].insert(std::make_pair(timeStamp, blogs[rand() % 5]));
	
		for(;it!=peers.end(); it++)
		{
			timeStamp = now + rand() % 2134223; // a random time for each blog
			mUsrBlogSet[*it].insert(std::make_pair(timeStamp, blogs[rand() % 5]));
		}
	}
	
	return;
}	
	

RsSerialiser* p3Qblog::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsQblogSerialiser()); // add in the types we need! 
	return rss;
}

std::list<RsItem* > p3Qblog::saveList(bool &cleanup)
{
	std::list<RsItem* > saveData;
	
	mBlogMtx.lock(); // LOCK
	
	cleanup = false;
	
	/* create save data */
	
	std::list<RsQblogItem*>::iterator it;
	
	for(it = mBlogItems.begin(); it != mBlogItems.end() ; it++)
		saveData.push_back(*it);
	
	/*save data created */
	return saveData;
}

void p3Qblog::saveDone()
{
	mBlogMtx.unlock(); // UNLOCK
	return;
}
	
bool p3Qblog::loadList(std::list<RsItem* > load)
{
	std::list<RsItem*>::iterator it;
	RsQblogItem *blog;
	
	#ifdef SERVER_DEBUG 
	std::cerr << "p3Qblog::loadList() Item Count: " << load.size();
	std::cerr << std::endl;
	#endif

	time_t now = time(NULL);
	time_t min, max;

     { 	RsStackMutex stack(mBlogMtx); /********** STACK LOCKED MTX ******/

	min = now - mStorePeriod;
	max = now + RANK_MAX_FWD_OFFSET;

     } 	/********** STACK LOCKED MTX ******/

	for(it = load.begin(); it != load.end(); it++)
	{
		/* switch on type */
		if (NULL != (blog = dynamic_cast<RsQblogItem *>(*it)))
		{
			/* check date -> if old expire */
			if (((time_t) blog->blogMsg.first < min) || 
				((time_t) blog->blogMsg.first > max))
			{
				#ifdef QBLOG_DEBUG
				std::cerr << "p3Blog::loadList() Outside TimeRange :";
				std::cerr << std::endl;
				#endif
				
				/* if outside range -> remove */
				delete blog;
				continue;
			}
			
			#ifdef QBLOG_DEBUG
			std::cerr << "p3Ranking::loadList() Anon TimeRange ok";
			std::cerr << std::endl;
			#endif

			/* make a copy to add into standard map */
			addBlog(blog);
		}
		else
		{
			/* cleanup */
			delete (*it);
		}
	}

	return true;
}

bool p3Qblog::sort(void)
{
	// TODO sorts blog maps in time order
	return true;
}

void p3Qblog::tick()
{
	bool repost = false; // so stack mutex is not enabled during postblog call
	
	{
		RsStackMutex stack(mBlogMtx);
		repost = mRepost;
	}
	
	if(repost) // 
	{
		if(!postBlogs())
			std::cerr << "p3Qblog::tick():" << "tick failed!";
	}
}