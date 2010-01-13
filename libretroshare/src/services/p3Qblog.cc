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
#include "p3Qblog.h"
#include <utility>
#include <ctime>
#include <iomanip>
#include "pqi/pqistore.h"
#include "pqi/pqibin.h"

const uint32_t BLOG_MAX_FWD_OFFSET = (60 * 60 * 24 * 2); /* 2 Days */
const uint32_t FRIEND_QBLOG_REPOST_PERIOD = 60; /* every minute for testing */


#define QBLOG_DEBUG 1


p3Qblog::p3Qblog(p3ConnectMgr *connMgr,
		uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir,
		uint32_t storePeriod)
	:CacheSource(type, true, cs, sourcedir),
	CacheStore(type, true, cs, cft, storedir),
	mConnMgr(connMgr),
	mStorePeriod(storePeriod), mPostsUpdated(false), mProfileUpdated(false)
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

std::ostream &operator<<(std::ostream& out, const std::wstring wstr)
{
	std::string str(wstr.begin(), wstr.end());
	out << str;

	return out;
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
		mPostsUpdated = true; // there is nothing to tick/repost (i.e. comes from cache)
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
	rsSerialiser->addSerialType(new RsQblogMsgSerialiser());

	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE;
	BinInterface *bio = new BinFileInterface(filename.c_str(), bioflags);
	pqistore *store = new pqistore(rsSerialiser, src, bio, 0);

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
	RsQblogMsg *newBlog;

	time_t now = time(NULL);
	time_t min, max;

	{ 	 /********** STACK LOCKED MTX ******/
		RsStackMutex stack(mBlogMtx);
		min = now - mStorePeriod;
		max = now + BLOG_MAX_FWD_OFFSET;
     } 	/********** STACK LOCKED MTX ******/

	while(NULL != (item = store->GetItem()))
	{
		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::loadBlogFile() Got Item:";
		std::cerr << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
		#endif

		if (NULL == (newBlog = dynamic_cast<RsQblogMsg *>(item)))
		{
			#ifdef QBLOG_DEBUG
			std::cerr << "p3Qblog::loadBlogFile() Item not Blog (deleting):";
			std::cerr << std::endl;
			#endif

			delete item;
		}
					/* check timestamp: delete if here longer than store period or more than two days */
		else if (((time_t) newBlog->sendTime < min) ||
				((time_t) newBlog->sendTime > max))
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
	}

	delete store; // store finished with/return resource
	return true;
}

bool p3Qblog::addBlog(RsQblogMsg *newBlog)
{
	#ifdef QBLOG_DEBUG
	std::cerr << "p3Ranking::addBlog() Item:";
	std::cerr << std::endl;
	newBlog->print(std::cerr, 10); // check whats in the blog item
	std::cerr << std::endl;
	#endif

	{
		RsStackMutex Stack(mBlogMtx);

		mUsrBlogSet[newBlog->PeerId()].insert(std::make_pair(newBlog->sendTime, newBlog->message));

		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::addBlog()";
		std::cerr << std::endl;
		std::cerr << "\tpeerId" << newBlog->PeerId();
		std::cerr << std::endl;
		std::cerr << "\tmUsrBlogSet: time" << newBlog->sendTime;
		std::cerr << std::endl;
		std::cerr << "\tmUsrBlogSet: blog" << newBlog->message;
		std::cerr << std::endl;
		#endif
		mPostsUpdated = false; // need to figure how this should work/ where this should be placed
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

	#ifdef QBLOG_DEBUG
	std::cerr << "p3Qblog::postBlogs() Storing to: " << fname;
	std::cerr << std::endl;
	#endif

	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsQblogMsgSerialiser());

	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE;
	BinInterface *bio = new BinFileInterface(fname.c_str(), bioflags);
	pqistore *store = new pqistore(rsSerialiser, mOwnId, bio,
					BIN_FLAGS_NO_DELETE);

	{
		RsStackMutex stack(mBlogMtx); /********** STACK LOCKED MTX ******/

			/* iterate through list */
		std::list<RsQblogMsg*>::iterator it;

		for(it = mBlogs.begin(); it != mBlogs.end();  it++)
		{
			/* only post own blogs */
			if(mOwnId == (*it)->PeerId())
			{
				/*write to serialiser*/
				RsItem *item = *it;

				#ifdef QBLOG_DEBUG
				std::cerr << "p3Qblog::postBlogs() Storing Item:";
				std::cerr << std::endl;
				item->print(std::cerr, 10);
				std::cerr << std::endl;
				#endif

				store->SendItem(item);
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

	#ifdef QBLOG_DEBUG
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

	delete store;
	return true;
}




p3Qblog::~p3Qblog()
{
	return;
}



bool p3Qblog::getBlogs(std::map< std::string, std::multimap<long int, std::wstring> > &blogs)
{
	{
		RsStackMutex stack(mBlogMtx);

		if(mUsrBlogSet.empty()) // return error blogs are empty
		{
			std::cerr << "usr blog set empty!" << std::endl;
			return false;
		}

		blogs = mUsrBlogSet;


		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::getBlogs() number of blogs: " << mUsrBlogSet.size();
		std::cerr << std::endl;
		#endif
	}

	return true;
}

bool p3Qblog::sendBlog(const std::wstring &msg)
{
	time_t blogTimeStamp;

	{
		RsStackMutex stack(mBlogMtx);
		mUsrBlogSet[mOwnId].insert(std::make_pair(blogTimeStamp, msg));

		RsQblogMsg *blog = new RsQblogMsg();
		blog->clear();

		blog->sendTime = blogTimeStamp;
		blog->message = msg;


		#ifdef QBLOG_DEBUG
		std::cerr << "p3Qblog::sendBlogFile()";
		std::cerr << std::endl;
		std::cerr << "\tblogItem->sendTime" << blog->sendTime;
		std::cerr << std::endl;
		std::cerr << "\tblogItem->message" << blog->message;
		std::cerr << std::endl;
		#endif

		mBlogs.push_back(blog);
	}
	return true;
}

void p3Qblog::loadDummy(void)
{
	std::list<std::string> peers;
	std::wstring cnv_wstr;
	mConnMgr->getFriendList(peers); // retrieve peers list from core
	if(peers.empty())
	{
		//for(int i = 0; i < 50; i++)
		//	std::cerr << "nothing in peer list!!!" << std::endl;
	}


	srand(60);
	long int now = time(NULL); // the present time
	std::multimap<long int, std::wstring> emptySet; // time/blog map

	//std::string statusSet[5] = { "great", "rubbish", "ecstatic", "save me", "emo depression"};
	//std::string songs[5] = { "broken spleen", "niobium", "ewe (a sheep)", "velvet stuff", "chun li kicks"};

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

	for(it = peers.begin(); it!=peers.end();it++)
	{
		mUsrBlogSet.insert(std::make_pair(*it, emptySet));
	}

	/* now fill up blog map */



	for(int i=0; i < 50 ; i++)
	{
		std::list<std::string>::iterator it = peers.begin();
		long int timeStamp;
		timeStamp = now + rand() % 2134223;
		int b = rand() % 5;
		cnv_wstr.assign(blogs[b].begin(), blogs[b].end());
		mUsrBlogSet[mOwnId].insert(std::make_pair(timeStamp, cnv_wstr )); // store a random blog

		for(;it!=peers.end(); it++)
		{
			timeStamp = now + rand() % 2134223; // a random time for each blog
			int c = rand() % 5;
			cnv_wstr.assign(blogs[c].begin(), blogs[c].end());
			mUsrBlogSet[*it].insert(std::make_pair(timeStamp, cnv_wstr)); // store a random blog
		}
	}

	return;
}

bool p3Qblog::getPeerLatestBlog(std::string id, uint32_t &ts, std::wstring &post)
{
	#ifdef QBLOG_DEBUG
	std::cerr << "p3Qblog::getPeerLatestBlog(): " << "not implemented yet" << std::endl;
	#endif
	return false;
}


bool p3Qblog::sort(void)
{
	// TODO sorts blog maps in time order
	return true;
}

void p3Qblog::tick()
{
	bool postUpdated = false; // so stack mutex is not enabled during postblog call

	{
		RsStackMutex stack(mBlogMtx);
		postUpdated = mPostsUpdated;
	}

	if(!postUpdated)
	{
		if(!postBlogs())
			std::cerr << "p3Qblog::tick():" << "tick failed!";

		/* drbob: added to stop infinite qblog output */
		RsStackMutex stack(mBlogMtx);
		mPostsUpdated = true;
	}
}
