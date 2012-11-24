#ifndef PHOTOITEM_H
#define PHOTOITEM_H

#include <QWidget>
#include <QLabel>
#include "PhotoShareItemHolder.h"
#include "retroshare/rsphoto.h"

namespace Ui {
    class PhotoItem;
}

class PhotoItem : public QWidget, public PhotoShareItem
{
    Q_OBJECT

public:

    PhotoItem(PhotoShareItemHolder *holder, const RsPhotoPhoto& photo, QWidget* parent = 0);
    PhotoItem(PhotoShareItemHolder *holder, const QString& path,  QWidget* parent = 0); // for new photos.
    ~PhotoItem();
    void setSelected(bool selected);
    bool isSelected(){ return mSelected; }
    const RsPhotoPhoto& getPhotoDetails();
    bool getPhotoThumbnail(RsPhotoThumbnail &nail);

protected:
        void mousePressEvent(QMouseEvent *event);

private:
        void updateImage(const RsPhotoThumbnail &thumbnail);
        void setUp();

    private slots:
        void setTitle();
        void setPhotoGrapher();

private:
    Ui::PhotoItem *ui;

    QPixmap mThumbNail;

    QPixmap getPixmap() { return mThumbNail; }

    bool mSelected;
    RsPhotoPhoto mPhotoDetails;
    PhotoShareItemHolder* mHolder;

    QLabel *mTitleLabel, *mPhotoGrapherLabel;
};

#endif // PHOTOITEM_H
