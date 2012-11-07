#ifndef POSTEDCREATECOMMENTDIALOG_H
#define POSTEDCREATECOMMENTDIALOG_H

#include <QDialog>

namespace Ui {
    class PostedCreateCommentDialog;
}

class PostedCreateCommentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PostedCreateCommentDialog(QWidget *parent = 0);
    ~PostedCreateCommentDialog();

private:
    Ui::PostedCreateCommentDialog *ui;
};

#endif // POSTEDCREATECOMMENTDIALOG_H
