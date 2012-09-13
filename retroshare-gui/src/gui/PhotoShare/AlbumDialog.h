#ifndef ALBUMDIALOG_H
#define ALBUMDIALOG_H

#include <QWidget>
#include "retroshare/rsphotoV2.h"
#include "util/TokenQueueV2.h"
#include "PhotoShareItemHolder.h"
#include "PhotoItem.h"
#include "PhotoDrop.h"

namespace Ui {
    class AlbumDialog;
}

class AlbumDialog : public QWidget, public PhotoShareItemHolder
{
    Q_OBJECT

public:
    explicit AlbumDialog(const RsPhotoAlbum& album, TokenQueueV2* photoQueue, RsPhotoV2* rs_Photo, QWidget *parent = 0);
    ~AlbumDialog();

    void notifySelection(PhotoShareItem* selection);

private:

    void setUp();

private slots:

    void updateAlbumPhotos();
    void deletePhoto();
    void editPhoto();
private:
    Ui::AlbumDialog *ui;
    RsPhotoV2* mRsPhoto;
    TokenQueueV2* mPhotoQueue;
    RsPhotoAlbum mAlbum;
    PhotoDrop* mPhotoDrop;
    PhotoItem* mPhotoSelected;
};

#endif // ALBUMDIALOG_H
