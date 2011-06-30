#ifndef SHAREKEY_H
#define SHAREKEY_H

#include <QDialog>

#include "ui_ShareKey.h"


class ShareKey : public QDialog {
    Q_OBJECT
public:
    /*
     *@param chanId The channel id to send request for
     */
    ShareKey(QWidget *parent = 0, Qt::WFlags flags = 0, std::string chanId = "");
    ~ShareKey();



protected:
    void changeEvent(QEvent *e);
    void closeEvent (QCloseEvent * event);

private:

    void setShareList();

    Ui::ShareKey *ui;

    std::string mChannelId;
    std::list<std::string> mShareList;


private slots:

    void shareKey();
    void cancel();
    void togglePersonItem(QTreeWidgetItem* item, int col);


};

#endif // SHAREKEY_H
