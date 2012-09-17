#ifndef PHOTOCOMMENTITEM_H
#define PHOTOCOMMENTITEM_H

#include <QWidget>

namespace Ui {
    class PhotoCommentItem;
}

class PhotoCommentItem : public QWidget
{
    Q_OBJECT

public:
    explicit PhotoCommentItem(QWidget *parent = 0);
    ~PhotoCommentItem();

private:
    Ui::PhotoCommentItem *ui;
};

#endif // PHOTOCOMMENTITEM_H
