/*
 * libretroshare/src/rsserver: p3photo.cc
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

#include "rsserver/p3photo.h"
#include "services/p3photoservice.h"

RsPhoto *rsPhoto = NULL;


RsPhotoDetails::RsPhotoDetails()
{
	return;
}

RsPhotoShowDetails::RsPhotoShowDetails()
{
	return;
}

p3Photo::p3Photo(p3PhotoService *p3ps)
	:mPhoto(p3ps)
{ 
	return; 
}

p3Photo::~p3Photo() 
{ 
	return; 
}

	/* access data */
bool p3Photo::getPhotoList(std::string id, std::list<std::string> hashs)
{
	return mPhoto->getPhotoList(id, hashs);
}

bool p3Photo::getShowList(std::string id, std::list<std::string> showIds)
{
	return mPhoto -> getShowList(id, showIds);
}


bool p3Photo::getShowDetails(std::string id, std::string showId, RsPhotoShowDetails &detail)
{
	return mPhoto -> getShowDetails(id, showId, detail);
}


bool p3Photo::getPhotoDetails(std::string id, std::string photoId, RsPhotoDetails &detail)
{
	return mPhoto -> getPhotoDetails(id, photoId, detail);
}


	/* add / delete */
std::string p3Photo::createShow(std::string name) 
{
	return mPhoto -> createShow(name); 
}

bool p3Photo::deleteShow(std::string showId)
{
	return mPhoto -> deleteShow(showId);
}

bool p3Photo::addPhotoToShow(std::string showId, std::string photoId, int16_t index)
{
	return mPhoto -> addPhotoToShow(showId, photoId, index);
}

bool p3Photo::movePhotoInShow(std::string showId, std::string photoId, int16_t index)
{
	return mPhoto -> movePhotoInShow(showId, photoId, index);
}

bool p3Photo::removePhotoFromShow(std::string showId, std::string photoId)
{
	return mPhoto -> removePhotoFromShow(showId, photoId);
}


std::string p3Photo::addPhoto(std::string path) /* add from file */
{
	return mPhoto -> addPhoto(path); /* add from file */
}

bool p3Photo::addPhoto(std::string srcId, std::string photoId) /* add from peers photos */
{
	return mPhoto -> addPhoto(srcId, photoId); /* add from peers photos */

}

bool p3Photo::deletePhoto(std::string photoId)
{
	return mPhoto -> deletePhoto(photoId);
}


	/* modify properties (TODO) */
bool p3Photo::modifyShow(std::string showId, std::wstring name, std::wstring comment)
{
	return mPhoto -> modifyShow(showId, name, comment);
}

bool p3Photo::modifyPhoto(std::string photoId, std::wstring name, std::wstring comment)
{
	return mPhoto -> modifyPhoto(photoId, name, comment);
}

bool p3Photo::modifyShowComment(std::string showId, std::string photoId, std::wstring comment)
{
	return mPhoto -> modifyShowComment(showId, photoId, comment);
}


