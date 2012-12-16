#ifndef ALBUMITEM_H
#define ALBUMITEM_H

#include <QWidget>
#include "string.h"
#include "retroshare/rsphoto.h"
#include "PhotoShareItemHolder.h"

namespace Ui {
    class AlbumItem;
}

class AlbumItem : public QWidget, public PhotoShareItem
{
    Q_OBJECT

public:
    explicit AlbumItem(const RsPhotoAlbum& album, PhotoShareItemHolder* albumHolder, QWidget *parent = 0);
    virtual ~AlbumItem();

    const RsPhotoAlbum& getAlbum();

    bool isSelected() {  return mSelected ;}
    void setSelected(bool selected);

protected:
    void mousePressEvent(QMouseEvent *event);

private:
    void setUp();
private:
    Ui::AlbumItem *ui;
    RsPhotoAlbum mAlbum;
    PhotoShareItemHolder* mAlbumHolder;
    bool mSelected;
};

#endif // ALBUMITEM_H
