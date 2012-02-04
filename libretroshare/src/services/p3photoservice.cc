/*
 * libretroshare/src/services p3photoservice.cc
 *
 * Phot Service for RetroShare.
 *
 * Copyright 2007-2012 by Robert Fernie.
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
#include "util/rsrandom.h"

/****
 * #define PHOTO_DEBUG 1
 ****/

RsPhoto *rsPhoto = NULL;



PhotoAlbum::PhotoAlbum()
{
	return;
}

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3PhotoService::p3PhotoService(uint16_t type)
	:p3Service(type), mPhotoMtx("p3PhotoService"), mUpdated(true)
{
     	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	return;
}


int	p3PhotoService::tick()
{
	std::cerr << "p3PhotoService::tick()";
	std::cerr << std::endl;
	
	return 0;
}

bool p3PhotoService::updated()
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	if (mUpdated)
	{
		mUpdated = false;
		return true;
	}
	return false;
}

bool p3PhotoService::getAlbumList(std::list<std::string> &album)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, RsPhotoAlbum>::iterator it;
	for(it = mAlbums.begin(); it != mAlbums.end(); it++)
	{
		album.push_back(it->second.mAlbumId);
	}
	
	return false;
}

bool p3PhotoService::getAlbum(const std::string &albumid, RsPhotoAlbum &album)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, RsPhotoAlbum>::iterator it;
	it = mAlbums.find(albumid);
	if (it == mAlbums.end())
	{
		return false;
	}
	
	album = it->second;
	return true;
}

bool p3PhotoService::getPhoto(const std::string &photoid, RsPhotoPhoto &photo)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, RsPhotoPhoto>::iterator it;
	it = mPhotos.find(photoid);
	if (it == mPhotos.end())
	{
		return false;
	}
	
	photo = it->second;
	return true;
}

bool p3PhotoService::getPhotoList(const std::string &albumid, std::list<std::string> &photoIds)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, std::list<std::string> >::iterator it;
	it = mAlbumToPhotos.find(albumid);
	if (it == mAlbumToPhotos.end())
	{
		return false;
	}
	
	std::list<std::string>::iterator lit;
	for(lit = it->second.begin(); lit != it->second.end(); lit++)
	{
		photoIds.push_back(*lit);
	}	
	return true;
}

bool p3PhotoService::getPhotoThumbnail(const std::string &photoid, RsPhotoThumbnail &thumbnail)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, RsPhotoThumbnail *>::iterator it;
	it = mPhotoThumbnails.find(photoid);
	if (it == mPhotoThumbnails.end())
	{
		return false;
	}
	
	// shallow copy??? dangerous!
	thumbnail = *(it->second);
	return true;
}

bool p3PhotoService::getAlbumThumbnail(const std::string &albumid, RsPhotoThumbnail &thumbnail)
{
	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/
	
	std::map<std::string, RsPhotoThumbnail *>::iterator it;
	it = mAlbumThumbnails.find(albumid);
	if (it == mAlbumThumbnails.end())
	{
		return false;
	}
	
	// shallow copy??? dangerous!
	thumbnail = *(it->second);
	return true;
}


/* details are updated in album - to choose Album ID, and storage path */
bool p3PhotoService::submitAlbumDetails(RsPhotoAlbum &album, const RsPhotoThumbnail &thumbnail)
{
	/* check if its a modification or a new album */


	/* add to database */

	/* check if its a mod or new photo */
	if (album.mId.empty())
	{
		/* new photo */

		/* generate a temp id */
		album.mAlbumId = genRandomId();
	}

	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	/* add / modify */
	mAlbums[album.mAlbumId] = album;

	/* must fix this up later! */
	RsPhotoThumbnail *thumb = new RsPhotoThumbnail();
	thumb->copyFrom(thumbnail);
	mAlbumThumbnails[album.mAlbumId] = thumb;
	
	return true;
}


bool p3PhotoService::submitPhoto(RsPhotoPhoto &photo, const RsPhotoThumbnail &thumbnail)
{
	if (photo.mAlbumId.empty())
	{
		/* new photo */
		std::cerr << "p3PhotoService::submitPhoto() Missing AlbumID: ERROR";
		std::cerr << std::endl;
		return false;
	}
	
	/* check if its a mod or new photo */
	if (photo.mId.empty())
	{
		/* new photo */

		/* generate a temp id */
		photo.mId = genRandomId();
	}

	RsStackMutex stack(mPhotoMtx); /********** STACK LOCKED MTX ******/

	mUpdated = true;

	std::map<std::string, std::list<std::string> >::iterator it;
	it = mAlbumToPhotos.find(photo.mAlbumId);
	if (it == mAlbumToPhotos.end())
	{
		std::list<std::string> emptyList;
		mAlbumToPhotos[photo.mAlbumId] = emptyList;

		it = mAlbumToPhotos.find(photo.mAlbumId);
	}
	
	it->second.push_back(photo.mId);
		
	/* add / modify */
	mPhotos[photo.mId] = photo;

	/* must fix this up later! */
	RsPhotoThumbnail *thumb = new RsPhotoThumbnail();
	thumb->copyFrom(thumbnail);
	mPhotoThumbnails[photo.mId] = thumb;
	
	return true;
}
	
std::string p3PhotoService::genRandomId()
{
	std::string randomId;
	for(int i = 0; i < 20; i++)
	{
		randomId += (char) ('a' + (RSRandom::random_u32() % 26));
	}
	
	return randomId;
}
	
	
bool RsPhotoThumbnail::copyFrom(const RsPhotoThumbnail &nail)
{
	if (data)
	{
		free(data);
		size = 0;
	}

	if ((!nail.data) || (nail.size == 0))
	{
		return false;
	}

	size = nail.size;
	type = nail.type;
	data = (uint8_t *) malloc(size);
	memcpy(data, nail.data, size);

	return true;
}





