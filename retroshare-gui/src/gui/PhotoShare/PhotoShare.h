#ifndef PHOTOSHARE_H
#define PHOTOSHARE_H

#include <QWidget>

namespace Ui {
    class PhotoShare;
}

class PhotoShare : public QWidget
{
    Q_OBJECT

public:
    explicit PhotoShare(QWidget *parent = 0);
    ~PhotoShare();

private:
    Ui::PhotoShare *ui;
};

#endif // PHOTOSHARE_H
