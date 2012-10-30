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

#ifndef MRK_PHOTO_SLIDE_SHOW_H
#define MRK_PHOTO_SLIDE_SHOW_H

#include "ui_PhotoSlideShow.h"

#include <retroshare/rsphotoV2.h>
#include "util/TokenQueue.h"
#include "AlbumItem.h"

class PhotoSlideShow : public QWidget, public TokenResponse
{
  Q_OBJECT

public:
        PhotoSlideShow(const RsPhotoAlbum& mAlbum, QWidget *parent = 0);
        virtual ~PhotoSlideShow();

        void loadRequest(const TokenQueue *queue, const TokenRequest &req);

private slots:
	void showPhotoDetails();
	void moveLeft();
	void moveRight();
	void StartStop();
	void timerEvent();
	void closeShow();
	void setFullScreen();

private:

        void requestPhotos();
	void loadImage();
	void updateMoveButtons(uint32_t status);

        bool loadPhotoData(const uint32_t &token);

private:

	std::map<std::string, RsPhotoPhoto *> mPhotos;
	std::map<int, std::string> mPhotoOrder;

	bool mRunning;
	int mImageIdx;
	bool mShotActive;

        RsPhotoAlbum mAlbum;

        TokenQueue *mPhotoQueue;

	Ui::PhotoSlideShow ui;
};

#endif

















