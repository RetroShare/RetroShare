#ifndef ALBUMCREATEDIALOG_H
#define ALBUMCREATEDIALOG_H

#include <QDialog>
#include "util/TokenQueue.h"
#include "retroshare/rsphotoV2.h"

namespace Ui {
    class AlbumCreateDialog;
}


class AlbumCreateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AlbumCreateDialog(TokenQueue* photoQueue, RsPhotoV2* rs_photo, QWidget *parent = 0);
    ~AlbumCreateDialog();

private slots:
    void publishAlbum();
    void addAlbumThumbnail();

private:

    bool getAlbumThumbnail(RsPhotoThumbnail &nail);
private:
    Ui::AlbumCreateDialog *ui;

    TokenQueue* mPhotoQueue;
    RsPhotoV2* mRsPhoto;
    QPixmap mThumbNail;
};



#endif // ALBUMCREATEDIALOG_H
