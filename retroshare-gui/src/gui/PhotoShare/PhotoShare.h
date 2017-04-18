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

namespace Ui {
    class PhotoShare;
}

class PhotoShare : public MainPage, public TokenResponse, public PhotoShareItemHolder
{
  Q_OBJECT

public:
        explicit PhotoShare(QWidget *parent = 0);
        ~PhotoShare();

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
