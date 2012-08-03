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
#include "util/TokenQueueV2.h"

class PhotoSlideShow : public QWidget, public TokenResponseV2
{
  Q_OBJECT

public:
	PhotoSlideShow(QWidget *parent = 0);
        virtual ~PhotoSlideShow();

	void loadAlbum(const std::string &albumId);
virtual	void loadRequest(const TokenQueueV2 *queue, const TokenRequestV2 &req);

	void clearDialog();

private slots:
	void showPhotoDetails();
	void moveLeft();
	void moveRight();
	void StartStop();
	void timerEvent();
	void closeShow();


private:

	void loadImage();
	void updateMoveButtons(uint32_t status);

	bool loadPhotoData(const uint32_t &token);
	bool loadAlbumData(const uint32_t &token);

//protected:

	std::map<std::string, RsPhotoPhoto *> mPhotos;
	std::map<int, std::string> mPhotoOrder;

	bool mRunning;
	int mImageIdx;
	bool mShotActive;

        TokenQueueV2 *mPhotoQueue;

	Ui::PhotoSlideShow ui;
};

#endif

















