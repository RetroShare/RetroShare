#ifndef PHOTOCOMMENTITEM_H
#define PHOTOCOMMENTITEM_H

#include <QWidget>
#include "retroshare/rsphoto.h"

namespace Ui {
    class PhotoCommentItem;
}

class PhotoCommentItem : public QWidget
{
    Q_OBJECT

public:
    explicit PhotoCommentItem(const RsPhotoComment& comment, QWidget *parent = 0);
    ~PhotoCommentItem();

    const RsPhotoComment& getComment();

private:

    void setUp();
private:
    Ui::PhotoCommentItem *ui;
    RsPhotoComment mComment;
};

#endif // PHOTOCOMMENTITEM_H
