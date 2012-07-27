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

#ifndef MRK_WIRE_DIALOG_H
#define MRK_WIRE_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_WireDialog.h"

#include <retroshare/rsphoto.h>

#include <map>

#include "gui/TheWire/PulseItem.h"
#include "gui/TheWire/PulseAddDialog.h"

class WireDialog : public MainPage, public PulseHolder 
{
  Q_OBJECT

public:
	WireDialog(QWidget *parent = 0);

virtual void deletePulseItem(PulseItem *, uint32_t type);
virtual void notifySelection(PulseItem *item, int ptype);

	void notifyPulseSelection(PulseItem *item);

private slots:

	void checkUpdate();
	void OpenOrShowPulseAddDialog();

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

	PulseAddDialog *mAddDialog;

	PulseItem *mPulseSelected;

	/* UI - from Designer */
	Ui::WireDialog ui;

};

#endif

