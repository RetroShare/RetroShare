/*******************************************************************************
 * gui/TheWire/WireDialog.cpp                                                  *
 *                                                                             *
 * Copyright (c) 2012 Robert Fernie   <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "WireDialog.h"

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
WireDialog::WireDialog(QWidget *parent)
: MainPage(parent)
{
	ui.setupUi(this);

	mAddDialog = NULL;
	mPulseSelected = NULL;

	connect( ui.pushButton_Post, SIGNAL(clicked()), this, SLOT(OpenOrShowPulseAddDialog()));
	//connect( ui.pushButton_Accounts, SIGNAL(clicked()), this, SLOT(OpenOrShowAccountDialog()));

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);


}


void WireDialog::notifySelection(PulseItem *item, int ptype)
{
        std::cerr << "WireDialog::notifySelection() from : " << ptype << " " << item;
        std::cerr << std::endl;

	notifyPulseSelection(item);

#if 0
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
#endif
}

void WireDialog::notifyPulseSelection(PulseItem *item)
{
	std::cerr << "WireDialog::notifyPulseSelection() from : " << item;
	std::cerr << std::endl;
	
	if (mPulseSelected)
	{
		std::cerr << "WireDialog::notifyPulseSelection() unselecting old one : " << mPulseSelected;
		std::cerr << std::endl;
	
		mPulseSelected->setSelected(false);
	}
	
	mPulseSelected = item;
}


void WireDialog::checkUpdate()
{
#if 0
	/* update */
	if (!rsWire)
		return;

	if (rsWire->updated())
	{
		insertAlbums();
	}
#endif
	return;
}


/*************** New Photo Dialog ***************/

void WireDialog::OpenOrShowPulseAddDialog()
{
	if (mAddDialog)
	{
		mAddDialog->show();
	}
	else
	{
		mAddDialog = new PulseAddDialog(NULL);
		mAddDialog->show();
	}
}


bool WireDialog::matchesAlbumFilter(const RsPhotoAlbum &album)
{

	return true;
}

double WireDialog::AlbumScore(const RsPhotoAlbum &album)
{
	return 1;
}


bool WireDialog::matchesPhotoFilter(const RsPhotoPhoto &photo)
{

	return true;
}

double WireDialog::PhotoScore(const RsPhotoPhoto &photo)
{
	return 1;
}


bool WireDialog::FilterNSortAlbums(const std::list<std::string> &albumIds, std::list<std::string> &filteredAlbumIds, int count)
{
#if 0
	std::multimap<double, std::string> sortedAlbums;
	std::multimap<double, std::string>::iterator sit;
	std::list<std::string>::const_iterator it;
	
	for(it = albumIds.begin(); it != albumIds.end(); ++it)
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
	for (sit = sortedAlbums.begin(), i = 0; (sit != sortedAlbums.end()) && (i < count); ++sit, ++i)
	{
		filteredAlbumIds.push_back(sit->second);
	}
#endif
	return true;
}


bool WireDialog::FilterNSortPhotos(const std::list<std::string> &photoIds, std::list<std::string> &filteredPhotoIds, int count)
{
#if 0
	std::multimap<double, std::string> sortedPhotos;
	std::multimap<double, std::string>::iterator sit;
	std::list<std::string>::const_iterator it;

	int i = 0;
	for(it = photoIds.begin(); it != photoIds.end(); ++it, ++i)
	{
		RsPhotoPhoto photo;
		rsPhoto->getPhoto(*it, photo);

		if (matchesPhotoFilter(photo))
		{
			double score = i; //PhotoScore(album);
			sortedPhotos.insert(std::make_pair(score, *it));
		}
	}

	for (sit = sortedPhotos.begin(), i = 0; (sit != sortedPhotos.end()) && (i < count); ++sit, ++i)
	{
		filteredPhotoIds.push_back(sit->second);
	}
#endif
	return true;
}



void WireDialog::insertAlbums()
{
#if 0
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

	for(it = filteredAlbumIds.begin(); it != filteredAlbumIds.end(); ++it)
	{
		addAlbum(*it);
	}

	insertPhotosForAlbum(filteredAlbumIds);
#endif
}

void WireDialog::insertPhotosForSelectedAlbum()
{
#if 0
	std::cerr << "WireDialog::insertPhotosForSelectedAlbum()";
	std::cerr << std::endl;

	clearPhotos();

	std::list<std::string> albumIds;
	if (mAlbumSelected)
	{
		albumIds.push_back(mAlbumSelected->mDetails.mAlbumId);

		std::cerr << "WireDialog::insertPhotosForSelectedAlbum() AlbumId: " << mAlbumSelected->mDetails.mAlbumId;
		std::cerr << std::endl;
	}

	insertPhotosForAlbum(albumIds);
#endif
}


void WireDialog::addAlbum(const std::string &id)
{
#if 0
	RsPhotoAlbum album;
	rsPhoto->getAlbum(id, album);


	RsPhotoThumbnail thumbnail;
	rsPhoto->getAlbumThumbnail(id, thumbnail);

	std::cerr << " WireDialog::addAlbum() AlbumId: " << album.mAlbumId << std::endl;

	PulseItem *item = new PulseItem(this, album, thumbnail);
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	alayout->addWidget(item);
#endif
}

void WireDialog::clearAlbums()
{
#if 0
	std::cerr << "WireDialog::clearAlbums()" << std::endl;

	std::list<PulseItem *> photoItems;
	std::list<PulseItem *>::iterator pit;
	
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
        int count = alayout->count();
	for(int i = 0; i < count; ++i)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "WireDialog::clearAlbums() missing litem";
			std::cerr << std::endl;
			continue;
		}
		
		PulseItem *item = dynamic_cast<PulseItem *>(litem->widget());
		if (item)
		{
			std::cerr << "WireDialog::clearAlbums() item: " << item;
			std::cerr << std::endl;
		
			photoItems.push_back(item);
		}
		else
		{
			std::cerr << "WireDialog::clearAlbums() Found Child, which is not a PulseItem???";
			std::cerr << std::endl;
		}
	}
	
	for(pit = photoItems.begin(); pit != photoItems.end(); ++pit)
	{
		PulseItem *item = *pit;
		alayout->removeWidget(item);
		delete item;
	}
	mAlbumSelected = NULL;
#endif
}

void WireDialog::clearPhotos()
{
#if 0
	std::cerr << "WireDialog::clearPhotos()" << std::endl;

	std::list<PulseItem *> photoItems;
	std::list<PulseItem *>::iterator pit;
	
	QLayout *alayout = ui.scrollAreaWidgetContents_2->layout();
        int count = alayout->count();
	for(int i = 0; i < count; ++i)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "WireDialog::clearPhotos() missing litem";
			std::cerr << std::endl;
			continue;
		}
		
		PulseItem *item = dynamic_cast<PulseItem *>(litem->widget());
		if (item)
		{
			std::cerr << "WireDialog::clearPhotos() item: " << item;
			std::cerr << std::endl;
		
			photoItems.push_back(item);
		}
		else
		{
			std::cerr << "WireDialog::clearPhotos() Found Child, which is not a PulseItem???";
			std::cerr << std::endl;
		}
	}
	
	for(pit = photoItems.begin(); pit != photoItems.end(); ++pit)
	{
		PulseItem *item = *pit;
		alayout->removeWidget(item);
		delete item;
	}
	
	mPhotoSelected = NULL;
#endif	
	
}


void WireDialog::insertPhotosForAlbum(const std::list<std::string> &albumIds)
{
#if 0
	/* clear it all */
	clearPhotos();
	//ui.photoLayout->clear();

	/* create a list of albums */

	std::list<std::string> ids;
	std::list<std::string> photoIds;
	std::list<std::string> filteredPhotoIds;
	std::list<std::string>::const_iterator it;

	for(it = albumIds.begin(); it != albumIds.end(); ++it)
	{
		rsPhoto->getPhotoList(*it, photoIds);
	}

	/* Filter Albums */ /* Sort Albums */
#define MAX_PHOTOS 50
	
	int count = MAX_PHOTOS;

	FilterNSortPhotos(photoIds, filteredPhotoIds, MAX_PHOTOS);

	for(it = filteredPhotoIds.begin(); it != filteredPhotoIds.end(); ++it)
	{
		addPhoto(*it);
	}
#endif
}


void WireDialog::addPhoto(const std::string &id)
{
#if 0
	RsPhotoPhoto photo;
	rsPhoto->getPhoto(id,photo);

	RsPhotoThumbnail thumbnail;
	rsPhoto->getPhotoThumbnail(id, thumbnail);

	std::cerr << "WireDialog::addPhoto() AlbumId: " << photo.mAlbumId;
	std::cerr << " PhotoId: " << photo.mId;
	std::cerr << std::endl;

	PulseItem *item = new PulseItem(this, photo, thumbnail);
	QLayout *alayout = ui.scrollAreaWidgetContents_2->layout();
	alayout->addWidget(item);
#endif
}


void WireDialog::deletePulseItem(PulseItem *item, uint32_t type)
{


	return;
}



