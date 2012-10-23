#ifndef ALBUMDIALOG_H
#define ALBUMDIALOG_H

#include <QDialog>
#include "retroshare/rsphotoV2.h"
#include "util/TokenQueue.h"
#include "PhotoShareItemHolder.h"
#include "PhotoItem.h"
#include "PhotoDrop.h"

namespace Ui {
    class AlbumDialog;
}

class AlbumDialog : public QDialog, public PhotoShareItemHolder
{
    Q_OBJECT

public:
    explicit AlbumDialog(const RsPhotoAlbum& album, TokenQueue* photoQueue, RsPhotoV2* rs_Photo, QWidget *parent = 0);
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
    TokenQueue* mPhotoQueue;
    RsPhotoAlbum mAlbum;
    PhotoDrop* mPhotoDrop;
    PhotoItem* mPhotoSelected;
};

#endif // ALBUMDIALOG_H
