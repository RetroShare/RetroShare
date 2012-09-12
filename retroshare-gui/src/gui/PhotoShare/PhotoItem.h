#ifndef PHOTOITEM_H
#define PHOTOITEM_H

#include <QWidget>

namespace Ui {
    class PhotoItem;
}

class PhotoItem : public QWidget
{
    Q_OBJECT

public:
    explicit PhotoItem(QWidget *parent = 0);
    ~PhotoItem();

private:
    Ui::PhotoItem *ui;
};

#endif // PHOTOITEM_H
