/*
 * libretroshare/src/services: p3photoservice.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#include "services/p3service.h"
#include "retroshare/rsphoto.h"

#include <map>
#include <string>

/* 
 * Photo Service
 *
 * This is an example service for the new cache system.
 * For the moment, it will only hold data passed to it from the GUI.
 * and spew that back when asked....
 *
 * We are doing it like this - so we can check the required interface functionality.
 *
 * Expect it won't take long before it'll be properly linked into the backend!
 *
 * This will be transformed into a Plugin Service, once the basics have been worked out.
 *
 */

class PhotoAlbum
{
	public:

	PhotoAlbum();
        std::string albumid;

	RsPhotoAlbum mAlbum;
	RsPhotoThumbnail mAlbumThumbnail;

        std::map<std::string, RsPhotoPhoto> mPhotos;
        std::map<std::string, RsPhotoThumbnail> mNails;
};



class p3PhotoService: public p3Service, public RsPhoto
{
	public:

	p3PhotoService(uint16_t type);

virtual int	tick();

	public:

// NEW INTERFACE.
/************* Extern Interface *******/

virtual bool updated();
virtual bool getAlbumList(std::list<std::string> &album);

virtual bool getAlbum(const std::string &albumid, RsPhotoAlbum &album);
virtual bool getPhoto(const std::string &photoid, RsPhotoPhoto &photo);
virtual bool getPhotoList(const std::string &albumid, std::list<std::string> &photoIds);
virtual bool getPhotoThumbnail(const std::string &photoid, RsPhotoThumbnail &thumbnail);
virtual bool getAlbumThumbnail(const std::string &albumid, RsPhotoThumbnail &thumbnail);

/* details are updated in album - to choose Album ID, and storage path */
virtual bool submitAlbumDetails(RsPhotoAlbum &album, const RsPhotoThumbnail &thumbnail);
virtual bool submitPhoto(RsPhotoPhoto &photo, const RsPhotoThumbnail &thumbnail);

	private:

std::string genRandomId();

	RsMutex mPhotoMtx;

	/***** below here is locked *****/

	bool mUpdated;

	std::map<std::string, std::list<std::string > > mAlbumToPhotos;
	std::map<std::string, RsPhotoPhoto> mPhotos;
	std::map<std::string, RsPhotoThumbnail *> mPhotoThumbnails;

	std::map<std::string, RsPhotoAlbum> mAlbums;
	std::map<std::string, RsPhotoThumbnail *> mAlbumThumbnails;

};

#endif 
