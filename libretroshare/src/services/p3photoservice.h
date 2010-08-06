/*
 * libretroshare/src/services: p3photoservice.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2008-2008 by Robert Fernie.
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

#ifndef P3_PHOTO_SERVICE_HEADER
#define P3_PHOTO_SERVICE_HEADER

#include "dbase/cachestrapper.h"
#include "pqi/pqiservice.h"
#include "pqi/pqistreamer.h"

#include "serialiser/rsserial.h"
#include "serialiser/rsphotoitems.h"

#include "retroshare/rsphoto.h"

/* 
 * Photo Service
 */

class PhotoSet
{
	public:

	PhotoSet();
        std::string pid;

        std::map<std::string, RsPhotoItem *> photos;
        std::map<std::string, RsPhotoShowItem *> shows;
};



class p3PhotoService: public CacheSource, public CacheStore
{
	public:

	p3PhotoService(uint16_t type, CacheStrapper *cs, CacheTransfer *cft,
		std::string sourcedir, std::string storedir);

void	tick();

/******************************* CACHE SOURCE / STORE Interface *********************/

	/* overloaded functions from Cache Source */
virtual bool    loadLocalCache(const CacheData &data);

	/* overloaded functions from Cache Store */
virtual int    loadCache(const CacheData &data);

/******************************* CACHE SOURCE / STORE Interface *********************/

	public:

/************* Extern Interface *******/

	/* things changed */
bool updated();

        /* access data */
bool getPhotoList(std::string id, std::list<std::string> &hashs);
bool getShowList(std::string id, std::list<std::string> &showIds);
bool getShowDetails(std::string id, std::string showId, RsPhotoShowDetails &detail);
bool getPhotoDetails(std::string id, std::string photoId, RsPhotoDetails &detail);

	        /* add / delete */
std::string createShow(std::string name);
bool deleteShow(std::string showId);
bool addPhotoToShow(std::string showId, std::string photoId, int16_t index);
bool movePhotoInShow(std::string showId, std::string photoId, int16_t index);
bool removePhotoFromShow(std::string showId, std::string photoId);

std::string addPhoto(std::string path); /* add from file */
bool addPhoto(std::string srcId, std::string photoId); /* add from peers photos */
bool deletePhoto(std::string photoId);

	        /* modify properties (TODO) */
bool modifyShow(std::string showId, std::wstring name, std::wstring comment);
bool modifyPhoto(std::string photoId, std::wstring name, std::wstring comment);
bool modifyShowComment(std::string showId, std::string photoId, std::wstring comment);



	private:	

	/* cache processing */

void loadPhotoIndex(std::string filename, std::string hash, std::string src);
void availablePhoto(std::string filename, std::string hash, std::string src);

bool loadPhotoItem(RsPhotoItem *item);
bool loadPhotoShowItem(RsPhotoShowItem *item);
void publishPhotos();


	/* locate info */

PhotoSet 	&locked_getPhotoSet(std::string id);
RsPhotoItem 	*locked_getPhoto(std::string id, std::string photoId);
RsPhotoShowItem *locked_getShow(std::string id, std::string showId);


	/* test functions */
void		createDummyData();



	RsMutex mPhotoMtx;

	/***** below here is locked *****/

	bool mRepublish;
	std::string mOwnId;

	bool mUpdated;

	std::map<std::string, PhotoSet> mPhotos;


};

#endif 
