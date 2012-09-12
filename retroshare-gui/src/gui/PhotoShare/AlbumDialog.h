#ifndef ALBUMDIALOG_H
#define ALBUMDIALOG_H

#include <QWidget>

namespace Ui {
    class AlbumDialog;
}

class AlbumDialog : public QWidget
{
    Q_OBJECT

public:
    explicit AlbumDialog(QWidget *parent = 0);
    ~AlbumDialog();

private:
    Ui::AlbumDialog *ui;
};

#endif // ALBUMDIALOG_H
