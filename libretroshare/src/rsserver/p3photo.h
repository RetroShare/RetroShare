#ifndef RETROSHARE_P3_PHOTO_INTERFACE_H
#define RETROSHARE_P3_PHOTO_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3photo.h
 *
 * RetroShare C++ Interface.
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

#include "retroshare/rsphoto.h"
#include "services/p3photoservice.h"

class p3Photo: public RsPhoto
{
	public:

	p3Photo(p3PhotoService *p3ps);
virtual ~p3Photo();

	/* changed? */
virtual bool updated();

	/* access data */
virtual bool getPhotoList(std::string id, std::list<std::string> &hashs);
virtual bool getShowList(std::string id, std::list<std::string> &showIds);
virtual bool getShowDetails(std::string id, std::string showId, RsPhotoShowDetails &detail);
virtual bool getPhotoDetails(std::string id, std::string photoId, RsPhotoDetails &detail);

	/* add / delete */
virtual std::string createShow(std::string name); 
virtual bool deleteShow(std::string showId);
virtual bool addPhotoToShow(std::string showId, std::string photoId, int16_t index);
virtual bool movePhotoInShow(std::string showId, std::string photoId, int16_t index);
virtual bool removePhotoFromShow(std::string showId, std::string photoId);

virtual std::string addPhoto(std::string path); /* add from file */
virtual bool addPhoto(std::string srcId, std::string photoId); /* add from peers photos */
virtual bool deletePhoto(std::string photoId);

	/* modify properties (TODO) */
virtual bool modifyShow(std::string showId, std::wstring name, std::wstring comment);
virtual bool modifyPhoto(std::string photoId, std::wstring name, std::wstring comment);
virtual bool modifyShowComment(std::string showId, std::string photoId, std::wstring comment);

	private:

	p3PhotoService *mPhoto;
};

#endif
