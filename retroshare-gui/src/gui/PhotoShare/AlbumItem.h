#ifndef ALBUMITEM_H
#define ALBUMITEM_H

#include <QWidget>
#include "string.h"
#include "retroshare/rsphotoV2.h"

namespace Ui {
    class AlbumItem;
}

class AlbumItem : public QWidget
{
    Q_OBJECT

public:
    explicit AlbumItem(const RsPhotoAlbum& album, QWidget *parent = 0);
    ~AlbumItem();

    RsPhotoAlbum getAlbum();

private:
    void setUp();
private:
    Ui::AlbumItem *ui;
    RsPhotoAlbum mAlbum;
};

#endif // ALBUMITEM_H
