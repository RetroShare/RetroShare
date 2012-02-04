#ifndef RETROSHARE_PHOTO_GUI_INTERFACE_H
#define RETROSHARE_PHOTO_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsphoto.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2008-2012 by Robert Fernie.
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

#include <inttypes.h>
#include <string>
#include <list>

/* The Main Interface Class - for information about your Peers */
class RsPhoto;
extern RsPhoto *rsPhoto;

/******************* NEW STUFF FOR NEW CACHE SYSTEM *********/

#define RSPHOTO_MODE_NEW	1
#define RSPHOTO_MODE_OWN	2
#define RSPHOTO_MODE_REMOTE	3

class RsPhotoPhoto
{
	public:

	std::string mAlbumId;
	std::string mId;

	std::string mTitle; // only used by Album.
	std::string mCaption;
	std::string mDescription;
	std::string mPhotographer;
	std::string mWhere;
	std::string mWhen;
	std::string mOther;
	std::string mCategory;

	std::string mHashTags;

	int mOrder;

	int mMode;
	std::string path; // if in Mode NEW.
};

class RsPhotoAlbumShare
{
	public:

	uint32_t mShareType;
	std::string mShareGroupId;
	std::string mPublishKey;
	uint32_t mCommentMode;
	uint32_t mResizeMode;
};

class RsPhotoAlbum: public RsPhotoPhoto
{
	public:

	std::string mPhotoPath;
	RsPhotoAlbumShare mShareOptions;
};

class RsPhotoThumbnail
{
	public:
		RsPhotoThumbnail()
		:data(NULL), size(0), type("N/A") { return; }

	bool copyFrom(const RsPhotoThumbnail &nail);

	// Holds Thumbnail image.
	uint8_t *data;
	int size;
	std::string type;
};


class RsPhoto
{
	public:

	RsPhoto()  { return; }
virtual ~RsPhoto() { return; }

	/* changed? */
virtual bool updated() = 0;

virtual bool getAlbumList(std::list<std::string> &album) = 0;

virtual bool getAlbum(const std::string &albumid, RsPhotoAlbum &album) = 0;
virtual bool getPhoto(const std::string &photoid, RsPhotoPhoto &photo) = 0;
virtual bool getPhotoList(const std::string &albumid, std::list<std::string> &photoIds) = 0;
virtual bool getPhotoThumbnail(const std::string &photoid, RsPhotoThumbnail &thumbnail) = 0;
virtual bool getAlbumThumbnail(const std::string &albumid, RsPhotoThumbnail &thumbnail) = 0;

/* details are updated in album - to choose Album ID, and storage path */
virtual bool submitAlbumDetails(RsPhotoAlbum &album, const RsPhotoThumbnail &thumbnail) = 0;
virtual bool submitPhoto(RsPhotoPhoto &photo, const RsPhotoThumbnail &thumbnail) = 0;

};


#endif
