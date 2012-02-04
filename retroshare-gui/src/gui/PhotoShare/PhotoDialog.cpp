/*
 * Retroshare Photo Plugin.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "PhotoDialog.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsphoto.h>

#include <iostream>
#include <sstream>

#include <QTimer>

/******
 * #define PHOTO_DEBUG 1
 *****/


/****************************************************************
 * New Photo Display Widget.
 *
 * This has two 'lists'.
 * Top list shows Albums.
 * Lower list is photos from the selected Album.
 * 
 * Notes:
 *   Each Item will be an AlbumItem, which contains a thumbnail & random details.
 *   We will limit Items to < 100. With a 'Filter to see more message.
 * 
 *   Thumbnails will come from Service.
 *   Option to Share albums / pictures onward (if permissions allow).
 *   Option to Download the albums to a specified directory. (is this required if sharing an album?)
 *
 *   Will introduce a FullScreen SlideShow later... first get basics happening.
 */




/** Constructor */
PhotoDialog::PhotoDialog(QWidget *parent)
: MainPage(parent)
{
	ui.setupUi(this);

	mAddDialog = NULL;
	mAlbumSelected = NULL;
	mPhotoSelected = NULL;

	connect( ui.toolButton_NewAlbum, SIGNAL(clicked()), this, SLOT(OpenOrShowPhotoAddDialog()));

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);


}


void PhotoDialog::notifySelection(PhotoItem *item, int ptype)
{
        std::cerr << "PhotoDialog::notifySelection() from : " << ptype << " " << item;
        std::cerr << std::endl;

	switch(ptype)
	{
		default:
		case PHOTO_ITEM_TYPE_ALBUM:
			notifyAlbumSelection(item);
			break;
		case PHOTO_ITEM_TYPE_PHOTO:
			notifyPhotoSelection(item);
			break;
	}
}

void PhotoDialog::notifyAlbumSelection(PhotoItem *item)
{
	std::cerr << "PhotoDialog::notifyAlbumSelection() from : " << item;
	std::cerr << std::endl;
	
	if (mAlbumSelected)
	{
		std::cerr << "PhotoDialog::notifyAlbumSelection() unselecting old one : " << mAlbumSelected;
		std::cerr << std::endl;
	
		mAlbumSelected->setSelected(false);
	}
	
	mAlbumSelected = item;
	insertPhotosForSelectedAlbum();
}


void PhotoDialog::notifyPhotoSelection(PhotoItem *item)
{
	std::cerr << "PhotoDialog::notifyPhotoSelection() from : " << item;
	std::cerr << std::endl;
	
	if (mPhotoSelected)
	{
		std::cerr << "PhotoDialog::notifyPhotoSelection() unselecting old one : " << mPhotoSelected;
		std::cerr << std::endl;
	
		mPhotoSelected->setSelected(false);
	}
	
	mPhotoSelected = item;
}


void PhotoDialog::checkUpdate()
{
	/* update */
	if (!rsPhoto)
		return;

	if (rsPhoto->updated())
	{
		insertAlbums();
	}

	return;
}


/*************** New Photo Dialog ***************/

void PhotoDialog::OpenOrShowPhotoAddDialog()
{
	if (mAddDialog)
	{
		mAddDialog->show();
	}
	else
	{
		mAddDialog = new PhotoAddDialog(NULL);
		mAddDialog->show();
	}
}


bool PhotoDialog::matchesAlbumFilter(const RsPhotoAlbum &album)
{

	return true;
}

double PhotoDialog::AlbumScore(const RsPhotoAlbum &album)
{
	return 1;
}


bool PhotoDialog::matchesPhotoFilter(const RsPhotoPhoto &photo)
{

	return true;
}

double PhotoDialog::PhotoScore(const RsPhotoPhoto &photo)
{
	return 1;
}


bool PhotoDialog::FilterNSortAlbums(const std::list<std::string> &albumIds, std::list<std::string> &filteredAlbumIds, int count)
{
	std::multimap<double, std::string> sortedAlbums;
	std::multimap<double, std::string>::iterator sit;
	std::list<std::string>::const_iterator it;
	
	for(it = albumIds.begin(); it != albumIds.end(); it++)
	{
		RsPhotoAlbum album;
		rsPhoto->getAlbum(*it, album);

		if (matchesAlbumFilter(album))
		{
			double score = AlbumScore(album);

			sortedAlbums.insert(std::make_pair(score, *it));
		}
	}

	int i;
	for (sit = sortedAlbums.begin(), i = 0; (sit != sortedAlbums.end()) && (i < count); sit++, i++)
	{
		filteredAlbumIds.push_back(sit->second);
	}

	return true;
}


bool PhotoDialog::FilterNSortPhotos(const std::list<std::string> &photoIds, std::list<std::string> &filteredPhotoIds, int count)
{
	std::multimap<double, std::string> sortedPhotos;
	std::multimap<double, std::string>::iterator sit;
	std::list<std::string>::const_iterator it;

	int i = 0;
	for(it = photoIds.begin(); it != photoIds.end(); it++, i++)
	{
		RsPhotoPhoto photo;
		rsPhoto->getPhoto(*it, photo);

		if (matchesPhotoFilter(photo))
		{
			double score = i; //PhotoScore(album);
			sortedPhotos.insert(std::make_pair(score, *it));
		}
	}

	for (sit = sortedPhotos.begin(), i = 0; (sit != sortedPhotos.end()) && (i < count); sit++, i++)
	{
		filteredPhotoIds.push_back(sit->second);
	}
	return true;
}



void PhotoDialog::insertAlbums()
{
	/* clear it all */
	clearAlbums();
	//ui.albumLayout->clear();

	/* create a list of albums */


	std::list<std::string> albumIds;
	std::list<std::string> filteredAlbumIds;
	std::list<std::string>::iterator it;

	rsPhoto->getAlbumList(albumIds);

	/* Filter Albums */ /* Sort Albums */
#define MAX_ALBUMS 50

	int count = MAX_ALBUMS;
	FilterNSortAlbums(albumIds, filteredAlbumIds, count);

	for(it = filteredAlbumIds.begin(); it != filteredAlbumIds.end(); it++)
	{
		addAlbum(*it);
	}

	insertPhotosForAlbum(filteredAlbumIds);
}

void PhotoDialog::insertPhotosForSelectedAlbum()
{
	std::cerr << "PhotoDialog::insertPhotosForSelectedAlbum()";
	std::cerr << std::endl;

	clearPhotos();

	std::list<std::string> albumIds;
	if (mAlbumSelected)
	{
		albumIds.push_back(mAlbumSelected->mDetails.mAlbumId);

		std::cerr << "PhotoDialog::insertPhotosForSelectedAlbum() AlbumId: " << mAlbumSelected->mDetails.mAlbumId;
		std::cerr << std::endl;
	}

	insertPhotosForAlbum(albumIds);
}


void PhotoDialog::addAlbum(const std::string &id)
{

	RsPhotoAlbum album;
	rsPhoto->getAlbum(id, album);


	RsPhotoThumbnail thumbnail;
	rsPhoto->getAlbumThumbnail(id, thumbnail);

	std::cerr << " PhotoDialog::addAlbum() AlbumId: " << album.mAlbumId << std::endl;

	PhotoItem *item = new PhotoItem(this, album, thumbnail);
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	alayout->addWidget(item);
}

void PhotoDialog::clearAlbums()
{
	std::cerr << "PhotoDialog::clearAlbums()" << std::endl;

	std::list<PhotoItem *> photoItems;
	std::list<PhotoItem *>::iterator pit;
	
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
        int count = alayout->count();
	for(int i = 0; i < count; i++)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "PhotoDialog::clearAlbums() missing litem";
			std::cerr << std::endl;
			continue;
		}
		
		PhotoItem *item = dynamic_cast<PhotoItem *>(litem->widget());
		if (item)
		{
			std::cerr << "PhotoDialog::clearAlbums() item: " << item;
			std::cerr << std::endl;
		
			photoItems.push_back(item);
		}
		else
		{
			std::cerr << "PhotoDialog::clearAlbums() Found Child, which is not a PhotoItem???";
			std::cerr << std::endl;
		}
	}
	
	for(pit = photoItems.begin(); pit != photoItems.end(); pit++)
	{
		PhotoItem *item = *pit;
		alayout->removeWidget(item);
		delete item;
	}
	mAlbumSelected = NULL;

}

void PhotoDialog::clearPhotos()
{
	std::cerr << "PhotoDialog::clearPhotos()" << std::endl;

	std::list<PhotoItem *> photoItems;
	std::list<PhotoItem *>::iterator pit;
	
	QLayout *alayout = ui.scrollAreaWidgetContents_2->layout();
        int count = alayout->count();
	for(int i = 0; i < count; i++)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "PhotoDialog::clearPhotos() missing litem";
			std::cerr << std::endl;
			continue;
		}
		
		PhotoItem *item = dynamic_cast<PhotoItem *>(litem->widget());
		if (item)
		{
			std::cerr << "PhotoDialog::clearPhotos() item: " << item;
			std::cerr << std::endl;
		
			photoItems.push_back(item);
		}
		else
		{
			std::cerr << "PhotoDialog::clearPhotos() Found Child, which is not a PhotoItem???";
			std::cerr << std::endl;
		}
	}
	
	for(pit = photoItems.begin(); pit != photoItems.end(); pit++)
	{
		PhotoItem *item = *pit;
		alayout->removeWidget(item);
		delete item;
	}
	
	mPhotoSelected = NULL;
	
	
}


void PhotoDialog::insertPhotosForAlbum(const std::list<std::string> &albumIds)
{
	/* clear it all */
	clearPhotos();
	//ui.photoLayout->clear();

	/* create a list of albums */

	std::list<std::string> ids;
	std::list<std::string> photoIds;
	std::list<std::string> filteredPhotoIds;
	std::list<std::string>::const_iterator it;

	for(it = albumIds.begin(); it != albumIds.end(); it++)
	{
		rsPhoto->getPhotoList(*it, photoIds);
	}

	/* Filter Albums */ /* Sort Albums */
#define MAX_PHOTOS 50
	
	int count = MAX_PHOTOS;

	FilterNSortPhotos(photoIds, filteredPhotoIds, MAX_PHOTOS);

	for(it = filteredPhotoIds.begin(); it != filteredPhotoIds.end(); it++)
	{
		addPhoto(*it);
	}
}


void PhotoDialog::addPhoto(const std::string &id)
{

	RsPhotoPhoto photo;
	rsPhoto->getPhoto(id,photo);

	RsPhotoThumbnail thumbnail;
	rsPhoto->getPhotoThumbnail(id, thumbnail);

	std::cerr << "PhotoDialog::addPhoto() AlbumId: " << photo.mAlbumId;
	std::cerr << " PhotoId: " << photo.mId;
	std::cerr << std::endl;

	PhotoItem *item = new PhotoItem(this, photo, thumbnail);
	QLayout *alayout = ui.scrollAreaWidgetContents_2->layout();
	alayout->addWidget(item);

}


void PhotoDialog::deletePhotoItem(PhotoItem *item, uint32_t type)
{


	return;
}



