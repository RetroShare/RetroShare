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

#ifndef MRK_PHOTO_ADD_DIALOG_H
#define MRK_PHOTO_ADD_DIALOG_H

#include "ui_PhotoAddDialog.h"

#include <retroshare/rsphotoV2.h>
#include "util/TokenQueueV2.h"

class PhotoDetailsDialog;

class PhotoAddDialog : public QWidget, public TokenResponseV2
{
  Q_OBJECT

public:
	PhotoAddDialog(QWidget *parent = 0);

	void loadAlbum(const std::string &albumId);
virtual	void loadRequest(const TokenQueueV2 *queue, const TokenRequestV2 &req);

	void clearDialog();

private slots:
	void showPhotoDetails();
	void showAlbumDetails();
	void editingStageDone();

        // From PhotoDrops...
        void albumImageChanged();
        void photoImageChanged();
	void updateMoveButtons(uint32_t status);

	void publishAlbum();

	void deleteAlbum();
	void deletePhoto();
private:

	void publishPhotos(std::string albumId);

	bool updateAlbumDetails(const RsPhotoAlbum &album);
	bool setAlbumDataToPhotos();
	bool loadPhotoData(const uint32_t &token);
	bool loadAlbumData(const uint32_t &token);
	bool loadCreatedAlbum(const uint32_t &token);

        TokenQueueV2 *mPhotoQueue;
protected:

	bool mAlbumEdit; // Editing or New.
	bool mEditingModeAlbum; // Changing Album or Photo Details.
	RsPhotoAlbum mAlbumData; 
	PhotoDetailsDialog *mPhotoDetails;
	Ui::PhotoAddDialog ui;

};

#endif

