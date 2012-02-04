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

#ifndef MRK_PHOTO_DIALOG_H
#define MRK_PHOTO_DIALOG_H

#include "gui/mainpage.h"
#include "ui_PhotoDialog.h"

#include <retroshare/rsphoto.h>

#include <map>

#include "gui/PhotoShare/PhotoItem.h"
#include "gui/PhotoShare/PhotoAddDialog.h"

class PhotoDialog : public MainPage, public PhotoHolder 
{
  Q_OBJECT

public:
	PhotoDialog(QWidget *parent = 0);

virtual void deletePhotoItem(PhotoItem *, uint32_t type);
virtual void notifySelection(PhotoItem *item, int ptype);

	void notifyAlbumSelection(PhotoItem *item);
	void notifyPhotoSelection(PhotoItem *item);

private slots:

	void checkUpdate();
	void OpenOrShowPhotoAddDialog();

private:



	/* TODO: These functions must be filled in for proper filtering to work 
	 * and tied to the GUI input
	 */

	bool matchesAlbumFilter(const RsPhotoAlbum &album);
	double AlbumScore(const RsPhotoAlbum &album);
	bool matchesPhotoFilter(const RsPhotoPhoto &photo);
	double PhotoScore(const RsPhotoPhoto &photo);

	/* Grunt work of setting up the GUI */

	bool FilterNSortAlbums(const std::list<std::string> &albumIds, std::list<std::string> &filteredAlbumIds, int count);
	bool FilterNSortPhotos(const std::list<std::string> &photoIds, std::list<std::string> &filteredPhotoIds, int count);
	void insertAlbums();
	void insertPhotosForAlbum(const std::list<std::string> &albumIds);
	void insertPhotosForSelectedAlbum();

	void addAlbum(const std::string &id);
	void addPhoto(const std::string &id);

	void clearAlbums();
	void clearPhotos();

	PhotoAddDialog *mAddDialog;

	PhotoItem *mAlbumSelected;
	PhotoItem *mPhotoSelected;

	/* UI - from Designer */
	Ui::PhotoDialog ui;

};

#endif

