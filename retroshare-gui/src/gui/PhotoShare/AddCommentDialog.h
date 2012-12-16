#ifndef ADDCOMMENTDIALOG_H
#define ADDCOMMENTDIALOG_H

#include <QDialog>

namespace Ui {
    class AddCommentDialog;
}

class AddCommentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCommentDialog(QWidget *parent = 0);
    ~AddCommentDialog();
    QString getComment() const;

private:
    Ui::AddCommentDialog *ui;
};

#endif // ADDCOMMENTDIALOG_H
