/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoShare.h                              *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#ifndef PHOTOSHARE_H
#define PHOTOSHARE_H

#include <QWidget>
#include <map>
#include "ui_PhotoShare.h"

#include "retroshare/rsphoto.h"
#include "retroshare-gui/mainpage.h"

#include "AlbumCreateDialog.h"
#include "AlbumDialog.h"
#include "PhotoDialog.h"

#include "AlbumItem.h"
#include "PhotoItem.h"
#include "PhotoSlideShow.h"

#include "util/TokenQueue.h"
#include "PhotoShareItemHolder.h"

#define IMAGE_PHOTO              ":/icons/png/photo.png"

namespace Ui {
    class PhotoShare;
}

class PhotoShare : public MainPage, public TokenResponse, public PhotoShareItemHolder
{
  Q_OBJECT

public:
        PhotoShare(QWidget *parent = 0);
        ~PhotoShare();

        virtual QIcon iconPixmap() const { return QIcon(IMAGE_PHOTO) ; }
        virtual QString pageName() const { return tr("Photo Albums") ; }
        virtual QString helpText() const { return ""; }

        void notifySelection(PhotoShareItem* selection);

private slots:
        void checkUpdate();
        void createAlbum();
        void OpenAlbumDialog();
        void OpenPhotoDialog();
        void OpenSlideShow();
        void updateAlbums();
        void subscribeToAlbum();
        void deleteAlbum(const RsGxsGroupId&);

private:
        /* Request Response Functions for loading data */
        void requestAlbumList(std::list<RsGxsGroupId> &ids);
        void requestAlbumData(std::list<RsGxsGroupId> &ids);

        /*!
         * request data for all groups
         */
        void requestAlbumData();
        void requestPhotoList(GxsMsgReq &albumIds);
        void requestPhotoList(const RsGxsGroupId &albumId);
        void requestPhotoData(GxsMsgReq &photoIds);
        void requestPhotoData(const std::list<RsGxsGroupId> &grpIds);

        void loadAlbumList(const uint32_t &token);
        bool loadAlbumData(const uint32_t &token);
        void loadPhotoList(const uint32_t &token);
        void loadPhotoData(const uint32_t &token);

        void loadRequest(const TokenQueue *queue, const TokenRequest &req);

        void acknowledgeGroup(const uint32_t &token);
        void acknowledgeMessage(const uint32_t &token);

        /* Grunt work of setting up the GUI */

        void addAlbum(const RsPhotoAlbum &album);
        void addPhoto(const RsPhotoPhoto &photo);

        void clearAlbums();
        void clearPhotos();
        void deleteAlbums();
        /*!
         * Fills up photo ui with photos held in mPhotoItems (current groups photos)
         */
        void updatePhotos();

private:
        AlbumItem* mAlbumSelected;
        PhotoItem* mPhotoSelected;


        TokenQueue *mPhotoQueue;

        /* UI - from Designer */
        Ui::PhotoShare ui;

        QSet<AlbumItem*> mAlbumItems;
        QSet<PhotoItem*> mPhotoItems; // the current album selected

};

#endif // PHOTOSHARE_H
