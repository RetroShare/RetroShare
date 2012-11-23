#ifndef ALBUMCREATEDIALOG_H
#define ALBUMCREATEDIALOG_H

#include <QDialog>
#include "util/TokenQueue.h"
#include "retroshare/rsphotoV2.h"
#include "PhotoShareItemHolder.h"
#include "PhotoItem.h"
#include "PhotoDrop.h"

namespace Ui {
    class AlbumCreateDialog;
}


class AlbumCreateDialog : public QDialog, public PhotoShareItemHolder
{
    Q_OBJECT

public:
    explicit AlbumCreateDialog(TokenQueue* photoQueue, RsPhotoV2* rs_photo, QWidget *parent = 0);
    ~AlbumCreateDialog();
    
    void notifySelection(PhotoShareItem* selection);
    

private slots:
    void publishAlbum();
    void publishPhotos();
    void addAlbumThumbnail();
	void changePage();
	void backPage();    
    

private:

    bool getAlbumThumbnail(RsPhotoThumbnail &nail);
private:
    Ui::AlbumCreateDialog *ui;

    TokenQueue* mPhotoQueue;
    RsPhotoV2* mRsPhoto;
    QPixmap mThumbNail;
    PhotoDrop* mPhotoDrop;
    PhotoItem* mPhotoSelected;
};



#endif // ALBUMCREATEDIALOG_H
