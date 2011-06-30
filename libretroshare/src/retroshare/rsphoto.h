#ifndef RETROSHARE_PHOTO_GUI_INTERFACE_H
#define RETROSHARE_PHOTO_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsphoto.h
 *
 * RetroShare C++ Interface.
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

#include <inttypes.h>
#include <string>
#include <list>

/* The Main Interface Class - for information about your Peers */
class RsPhoto;
extern RsPhoto *rsPhoto;

class RsPhotoDetails;
class RsPhotoShowDetails;

class RsPhotoShowInfo
{
	public:

	std::string photoId;
	std::wstring altComment;
	uint32_t    deltaT; /* in 100ths of sec? */
};
	
class RsPhotoShowDetails
{
	public:

	RsPhotoShowDetails();

	std::string id;
	std::string showid;

	std::string name;
	std::wstring location;
	std::wstring comment;
	std::string date;
	std::list<RsPhotoShowInfo> photos;
};

/* Details class */
class RsPhotoDetails
{
	public:

	RsPhotoDetails();

	std::string id;
	std::string srcid;

	std::string hash;
	uint64_t size;

	std::string name; 
	std::wstring comment;

	std::string location;
	std::string date;

	uint32_t format;

	bool isAvailable;
	std::string path;
};

std::ostream &operator<<(std::ostream &out, const RsPhotoShowDetails &detail);
std::ostream &operator<<(std::ostream &out, const RsPhotoDetails &detail);

class RsPhoto
{
	public:

	RsPhoto()  { return; }
virtual ~RsPhoto() { return; }

	/* changed? */
virtual bool updated() = 0;

	/* access data */
virtual bool getPhotoList(std::string id, std::list<std::string> &hashs) 	= 0;
virtual bool getShowList(std::string id,  std::list<std::string> &showIds) 	= 0;
virtual bool getShowDetails(std::string id, std::string showId, RsPhotoShowDetails &detail) = 0;
virtual bool getPhotoDetails(std::string id, std::string photoId, RsPhotoDetails &detail) = 0;

	/* add / delete */
virtual std::string createShow(std::string name) 				= 0; 
virtual bool deleteShow(std::string showId)					= 0;
virtual bool addPhotoToShow(std::string showId, std::string photoId, int16_t index) = 0;
virtual bool movePhotoInShow(std::string showId, std::string photoId, int16_t index) = 0;
virtual bool removePhotoFromShow(std::string showId, std::string photoId) 	= 0;

virtual std::string addPhoto(std::string path) = 0; /* add from file */
virtual bool addPhoto(std::string srcId, std::string photoId) = 0; /* add from peers photos */
virtual bool deletePhoto(std::string photoId) = 0;

	/* modify properties (TODO) */
virtual bool modifyShow(std::string showId, std::wstring name, std::wstring comment) = 0;
virtual bool modifyPhoto(std::string photoId, std::wstring name, std::wstring comment) = 0;
virtual bool modifyShowComment(std::string showId, std::string photoId, std::wstring comment) = 0;

};

#endif
