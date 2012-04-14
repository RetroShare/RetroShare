/*
 * libretroshare/src/services p3ranking.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "services/p3photoservice.h"

#include "pqi/pqibin.h"
#include "pqi/authssl.h"

#include "util/rsdir.h"

#include <sstream>

std::string generateRandomShowId();

/****
 * #define PHOTO_DEBUG 1
 ****/


PhotoSet::PhotoSet()
{
	return;
}

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3PhotoService::p3PhotoService(uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir)
	:CacheSource(type, true, cs, sourcedir), 
	CacheStore(type, true, cs, cft, storedir), mPhotoMtx("p3PhotoService"),
	mUpdated(true)
{

     { 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

        mOwnId = AuthSSL::getAuthSSL()->OwnId();
     } 	

//	createDummyData();
	return;
}


void	p3PhotoService::tick()
{
	bool repub = false;
	{
     		RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
		repub = mRepublish;
	}

	if (repub)
	{
		//publishPhotos();

     		RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
		mRepublish = false;
	}
}


/********************************************************************************/
/******************* Cache Interaction ******************************************/
/********************************************************************************/

bool    p3PhotoService::loadLocalCache(const CacheData &data)
{
	std::string filename = data.path + '/' + data.name;
	std::string hash = data.hash;
	//uint64_t size = data.size;
	std::string source = data.pid;

#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::loadLocalCache()";
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

	if (data.cid.subid == 0)
	{
		loadPhotoIndex(filename, hash, source);
	}
	else
	{
		availablePhoto(filename, hash, source);
	}

	{
          RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	  mRepublish = false;
	  mUpdated = true;
	}

	if (data.size > 0) /* don't refresh zero sized caches */
	{
		refreshCache(data);
	}
	return true;
}

int    p3PhotoService::loadCache(const CacheData &data)
{
	std::string filename = data.path + '/' + data.name;
	std::string hash = data.hash;
	//uint64_t size = data.size;
	std::string source = data.pid;

#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::loadCache()";
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

	if (data.cid.subid == 0)
	{
		loadPhotoIndex(filename, hash, source);
	}
	else
	{
		availablePhoto(filename, hash, source);
	}

	CacheStore::lockData();   /*****   LOCK ****/
	locked_storeCacheEntry(data); 
	CacheStore::unlockData(); /***** UNLOCK ****/

	return 1;
}


void p3PhotoService::loadPhotoIndex(std::string filename, std::string hash, std::string src)
{
	/* remove unused parameter warnings */
	(void) hash;

	/* create the serialiser to load info */
	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsPhotoSerialiser());
	
	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE;
	BinInterface *bio = new BinFileInterface(filename.c_str(), bioflags);
	pqistreamer *stream = new pqistreamer(rsSerialiser, src, bio, 0);
	
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::loadPhotoIndex()";
	std::cerr << std::endl;
	std::cerr << "\tSource: " << src;
	std::cerr << std::endl;
	std::cerr << "\tHash: " << hash;
	std::cerr << std::endl;
	std::cerr << "\tFilename: " << filename;
	std::cerr << std::endl;
#endif

	RsItem *item;
	RsPhotoItem *pitem;
	RsPhotoShowItem *sitem;

	stream->tick(); /* Tick to read! */
	while(NULL != (item = stream->GetItem()))
	{

#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::loadPhotoIndex() Got Item:";
		std::cerr << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif

		pitem = dynamic_cast<RsPhotoItem *>(item);
		sitem = dynamic_cast<RsPhotoShowItem *>(item);

		if (pitem)
		{
			/* photo item */
#ifdef PHOTO_DEBUG
			std::cerr << "p3PhotoService::loadPhotoIndex() Loading Photo Item";
			std::cerr << std::endl;
#endif
			loadPhotoItem(pitem);

		}
		else if (sitem)
		{
			/* show item */
#ifdef PHOTO_DEBUG
			std::cerr << "p3PhotoService::loadPhotoIndex() Loading Show Item";
			std::cerr << std::endl;
#endif
			loadPhotoShowItem(sitem);
		}
		else
		{
			/* unknown */
#ifdef PHOTO_DEBUG
			std::cerr << "p3PhotoService::loadRankFile() Unknown Item (deleting):";
			std::cerr << std::endl;
#endif

		}
		delete item;
		stream->tick(); /* Tick to read! */
	}

	delete stream;
}


bool p3PhotoService::loadPhotoItem(RsPhotoItem *item)
{
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::loadPhotoItem()";
	std::cerr << std::endl;
	item->print(std::cerr);
	std::cerr << std::endl;
#endif

 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	PhotoSet &pset = locked_getPhotoSet(item->PeerId());
	pset.photos[item->photoId] = item;

	mUpdated = true;

	return true;


}


bool p3PhotoService::loadPhotoShowItem(RsPhotoShowItem *item)
{

#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::loadPhotoShowItem()";
	std::cerr << std::endl;
	item->print(std::cerr);
	std::cerr << std::endl;
#endif

	/* add to set */
 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	PhotoSet &pset = locked_getPhotoSet(item->PeerId());
	pset.shows[item->showId] = item;

	//mRepublish = true;
	mUpdated = true;

	return true;
}
	
void p3PhotoService::availablePhoto(std::string /*filename*/, std::string /*hash*/, std::string /*src*/)
{
	/* TODO */
	return;
}


void p3PhotoService::publishPhotos()
{

#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::publishPhotos()";
	std::cerr << std::endl;
#endif

	/* determine filename */

	std::string path = CacheSource::getCacheDir();
	std::ostringstream out;
	out << "photo-index-" << time(NULL) << ".pdx";

	std::string tmpname = out.str();
	std::string fname = path + "/" + tmpname;

#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::publishPhotos() Storing to: " << fname;
	std::cerr << std::endl;
#endif


	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsPhotoSerialiser());
	
	uint32_t bioflags = BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE;
	BinInterface *bio = new BinFileInterface(fname.c_str(), bioflags);
	pqistreamer *stream = new pqistreamer(rsSerialiser, mOwnId, bio,
					BIN_FLAGS_NO_DELETE);


     { 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	PhotoSet &pset = locked_getPhotoSet(mOwnId);

        std::map<std::string, RsPhotoItem *>::iterator pit;
	for(pit = pset.photos.begin(); pit != pset.photos.end(); pit++)
	{
		RsPhotoItem *pitem = pit->second;
#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::publishPhotos() Storing Photo Item:";
		std::cerr << std::endl;
		pitem->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
		stream->SendItem(pitem);
		stream->tick(); /* Tick to write! */

	}

        std::map<std::string, RsPhotoShowItem *>::iterator sit;
	for(sit = pset.shows.begin(); sit != pset.shows.end(); sit++)
	{
		RsPhotoShowItem *sitem = sit->second;
#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::publishPhotos() Storing PhotoShow Item:";
		std::cerr << std::endl;
		sitem->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
		stream->SendItem(sitem);
		stream->tick(); /* Tick to write! */

	}
     } 	/********** STACK LOCKED MTX ******/
	stream->tick(); /* Tick for final write! */

	/* flag as new info */
	CacheData data;

     { 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	data.pid = mOwnId;
     } 	/********** STACK LOCKED MTX ******/

	data.cid = CacheId(CacheSource::getCacheType(), 1);

	data.path = path;
	data.name = tmpname;

	data.hash = bio->gethash();
	data.size = bio->bytecount();
	data.recvd = time(NULL);
	
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::publishPhotos() refreshing Cache";
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
	if (data.size > 0) /* don't refresh zero sized caches */
	{
		refreshCache(data);
	}

	delete stream;

	/* TO HERE 
	 * update Cache (new Photos (all Photos?))
	 * */

}


/********************************************************************************/
/******************* External Interface *****************************************/
/********************************************************************************/

bool    p3PhotoService::updated()
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/ 
	
	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}

bool p3PhotoService::getPhotoList(std::string id, std::list<std::string> &photoIds)
{
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::getPhotoList() pid: " << id;
	std::cerr << std::endl;
#endif

 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	PhotoSet &pset = locked_getPhotoSet(id);

	/* get the list of photoids */
	std::map<std::string, RsPhotoItem *>::iterator pit;
	for(pit = pset.photos.begin(); pit != pset.photos.end(); pit++)
	{
#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::getPhotoList() PhotoId: " << pit->first;
		std::cerr << std::endl;
#endif
		photoIds.push_back(pit->first);
	}

	return true;
}


bool p3PhotoService::getShowList(std::string id, std::list<std::string> &showIds)
{
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::getShowList() pid: " << id;
	std::cerr << std::endl;
#endif

 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	PhotoSet &pset = locked_getPhotoSet(id);

	/* get the list of showIds */
        std::map<std::string, RsPhotoShowItem *>::iterator sit;
	for(sit = pset.shows.begin(); sit != pset.shows.end(); sit++)
	{
#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::getShowList() ShowId: " << sit->first;
		std::cerr << std::endl;
#endif
		showIds.push_back(sit->first);
	}

	return true;
}

bool p3PhotoService::getShowDetails(std::string id, std::string showId, RsPhotoShowDetails &/*detail*/)
{
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::getShowDetails() pid: " << id;
	std::cerr << "showId: " << showId;
	std::cerr << std::endl;
#endif

 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	RsPhotoShowItem *item = locked_getShow(id, showId);
	if (!item)
	{
		return false;
	}

	/* extract Show details */

	return true;
}


bool p3PhotoService::getPhotoDetails(std::string id, std::string photoId, RsPhotoDetails &detail)
{
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::getPhotoDetails() pid: " << id;
	std::cerr << " photoId: " << photoId;
	std::cerr << std::endl;
#endif

 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	RsPhotoItem *item = locked_getPhoto(id, photoId);
	if (!item)
	{
		return false;
	}

	/* extract Photo details */
	detail.id = item->PeerId();
	detail.srcid = item->srcId;
	detail.hash = item->photoId;
	detail.size = item->size;
	detail.name = item->name;
	detail.location = item->location;
	detail.comment = item->comment;
	detail.date = item->date;
	detail.format = 0;
	detail.isAvailable = item->isAvailable;
	detail.path = item->path;

	return true;
}

/* add / delete */
std::string p3PhotoService::createShow(std::string name)
{
	std::string showId = generateRandomShowId();
	RsPhotoShowItem *newShow = new RsPhotoShowItem();


	newShow->showId = showId;
	newShow->name = name;

	/* add to set */
 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	PhotoSet &pset = locked_getPhotoSet(mOwnId);
	pset.shows[showId] = newShow;

	mRepublish = true;

	return showId;
}


bool p3PhotoService::deleteShow(std::string showId)
{
	/* add to set */
 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	PhotoSet &pset = locked_getPhotoSet(mOwnId);

        std::map<std::string, RsPhotoShowItem *>::iterator sit;
	sit = pset.shows.find(showId);
	if (sit == pset.shows.end())
	{
#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::deleteShow() no existant";
		std::cerr << std::endl;
#endif
		return false;
	}

	
	RsPhotoShowItem *oldShow = sit->second;
	pset.shows.erase(sit);
	delete oldShow;

	mRepublish = true;

	return true;
}


bool p3PhotoService::addPhotoToShow(std::string showId, std::string photoId, int16_t /*index*/)
{

#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::addPhotoToShow() showId: " << showId;
	std::cerr << " photoId: " << photoId;
	std::cerr << std::endl;
#endif

	/* add to set */
 	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	RsPhotoItem *photo = locked_getPhoto(mOwnId, photoId);
	RsPhotoShowItem *show = locked_getShow(mOwnId, showId);

	if ((!photo) || (!show))
	{
#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::addPhotoToShow() NULL data";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* can have duplicates so just add it in */
	RsPhotoRefItem ref;
	ref.photoId = photoId;

	/* add in at correct location! */
	//uint32_t i = 0;
	//for(it = show.photos.begin();
	//	(i < index) && (it != show.photos.end()); it++, i++);

	show->photos.push_back(ref);

	mRepublish = true;

	return true;
}

bool p3PhotoService::movePhotoInShow(std::string /*showId*/, std::string /*photoId*/, int16_t /*index*/)
{
	return false;
}

bool p3PhotoService::removePhotoFromShow(std::string /*showId*/, std::string /*photoId*/)
{
	return false;
}


std::string p3PhotoService::addPhoto(std::string path) /* add from file */
{
	/* check file exists */
	std::string hash;
	uint64_t    size;

	if (!RsDirUtil::getFileHash(path, hash, size))
	{
		return hash;
	}

	/* copy to outgoing directory TODO */

	/* create item */
	RsPhotoItem *item = new RsPhotoItem();

	{
 	  RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	  item->PeerId(mOwnId);
	}

	item->srcId = item->PeerId();
	item->photoId = hash;
	item->name = RsDirUtil::getTopDir(path);
	item->path = path;
	item->size = size;
	item->isAvailable = true;
	item->comment = L"No Comment Yet!";

	/* add in */
	loadPhotoItem(item);

	/* flag for republish */
	{
 	  RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	  mRepublish = true;
	}

	return hash;
}




bool p3PhotoService::addPhoto(std::string /*srcId*/, std::string /*photoId*/) /* add from peers photos */
{
	return false;
}

bool p3PhotoService::deletePhoto(std::string /*photoId*/)
{
	return false;
}


/* modify properties (TODO) */
bool p3PhotoService::modifyShow(std::string /*showId*/, std::wstring /*name*/, std::wstring /*comment*/)
{
	return false;
}

bool p3PhotoService::modifyPhoto(std::string /*photoId*/, std::wstring /*name*/, std::wstring /*comment*/)
{
	return false;
}

bool p3PhotoService::modifyShowComment(std::string /*showId*/, std::string /*photoId*/, std::wstring /*comment*/)
{
	return false;
}

/********************************************************************************/
/******************* Utility Functions ******************************************/
/********************************************************************************/

PhotoSet &p3PhotoService::locked_getPhotoSet(std::string id)
{
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::locked_getPhotoSet() pid: " << id;
	std::cerr << std::endl;
#endif

	std::map<std::string, PhotoSet>::iterator it;
	it = mPhotos.find(id);
	if (it == mPhotos.end())
	{
		/* missing group -> add it in */
		PhotoSet pset;
		pset.pid = id;
		mPhotos[id] = pset;

		it = mPhotos.find(id);
	}

	return (it->second);
}


RsPhotoItem *p3PhotoService::locked_getPhoto(std::string id, std::string photoId)
{
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::locked_getPhoto() pid: " << id;
	std::cerr << " photoId: " << photoId;
	std::cerr << std::endl;
#endif

	PhotoSet &pset = locked_getPhotoSet(id);

        std::map<std::string, RsPhotoItem *>::iterator pit;
	pit = pset.photos.find(photoId);
	if (pit == pset.photos.end())
	{
#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::getPhotoDetails() Failed - noPhoto";
		std::cerr << std::endl;
#endif
		return NULL;
	}
	return pit->second;
}


RsPhotoShowItem *p3PhotoService::locked_getShow(std::string id, std::string showId)
{
#ifdef PHOTO_DEBUG
	std::cerr << "p3PhotoService::locked_getShow() pid: " << id;
	std::cerr << " showId: " << showId;
	std::cerr << std::endl;
#endif

	PhotoSet &pset = locked_getPhotoSet(id);

        std::map<std::string, RsPhotoShowItem *>::iterator sit;
	sit = pset.shows.find(showId);
	if (sit == pset.shows.end())
	{
#ifdef PHOTO_DEBUG
		std::cerr << "p3PhotoService::locked_getShow() Failed - no Show";
		std::cerr << std::endl;
#endif
		return NULL;
	}
	return sit->second;
}




std::string generateRandomShowId()
{
	std::ostringstream out;
	out << std::hex;
	
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
        /* 4 bytes per random number: 4 x 4 = 16 bytes */
        for(int i = 0; i < 4; i++)
        {
                out << std::setw(8) << std::setfill('0');
                uint32_t rint = random();
                out << rint;
        }
#else
        srand(time(NULL));
        /* 2 bytes per random number: 8 x 2 = 16 bytes */
        for(int i = 0; i < 8; i++)
        {
                out << std::setw(4) << std::setfill('0');
                uint16_t rint = rand(); /* only gives 16 bits */
                out << rint;
        }
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	return out.str();
}
	

void	p3PhotoService::createDummyData()
{

#if 0
	RsRankLinkMsg *msg = new RsRankLinkMsg();

	time_t now = time(NULL);

	msg->PeerId(mOwnId);
	msg->rid = "0001";
	msg->title = L"Original Awesome Site!";
	msg->timestamp = now - 60 * 60 * 24 * 15;
	msg->link = L"http://www.retroshare.org";
	msg->comment = L"Retroshares Website";

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId(mOwnId);
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 123;
	msg->link = L"http://www.lunamutt.org";
	msg->comment = L"Lunamutt's Website";

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID");
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 60 * 60 * 24 * 29;
	msg->link = L"http://www.lunamutt.org";
	msg->comment = L"Lunamutt's Website (TWO) How Long can this comment be!\n";
	msg->comment += L"What happens to the second line?\n";
	msg->comment += L"And a 3rd!";

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID2");
	msg->rid = "0002";
	msg->title = L"Awesome Site!";
	msg->timestamp = now - 60 * 60 * 7;
	msg->link = L"http://www.lunamutt.org";
	msg->comment += L"A Short Comment";

	addRankMsg(msg);


	/***** Third one ****/

	msg = new RsRankLinkMsg();
	msg->PeerId(mOwnId);
	msg->rid = "0003";
	msg->title = L"Weird Site!";
	msg->timestamp = now - 60 * 60;
	msg->link = L"http://www.lunamutt.com";
	msg->comment = L"";

	addRankMsg(msg);

	msg = new RsRankLinkMsg();
	msg->PeerId("ALTID");
	msg->rid = "0003";
	msg->title = L"Weird Site!";
	msg->timestamp = now - 60 * 60 * 24 * 2;
	msg->link = L"http://www.lunamutt.com";
	msg->comment = L"";

	addRankMsg(msg);

#endif

}

