#ifndef GXSCREATEGROUPDIALOG_H
#define GXSCREATEGROUPDIALOG_H

#include <QDialog>

namespace Ui {
    class GxsCreateGroupDialog;
}

class GxsCreateGroupDialog : public QDialog {
    Q_OBJECT
public:
    GxsCreateGroupDialog(QWidget *parent = 0);
    ~GxsCreateGroupDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::GxsCreateGroupDialog *ui;
};

#endif // GXSCREATEGROUPDIALOG_H
