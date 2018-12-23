/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoSlideShow.h                          *
 *                                                                             *
 * Copyright (C) 2012 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#ifndef MRK_PHOTO_SLIDE_SHOW_H
#define MRK_PHOTO_SLIDE_SHOW_H

#include "ui_PhotoSlideShow.h"

#include <retroshare/rsphoto.h>
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

    std::map<RsGxsMessageId, RsPhotoPhoto *> mPhotos;
    std::map<int, RsGxsMessageId> mPhotoOrder;

	bool mRunning;
	int mImageIdx;
	bool mShotActive;

        RsPhotoAlbum mAlbum;

        TokenQueue *mPhotoQueue;

	Ui::PhotoSlideShow ui;
};

#endif

















