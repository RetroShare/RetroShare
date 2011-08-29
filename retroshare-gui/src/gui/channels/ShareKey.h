#ifndef SHAREKEY_H
#define SHAREKEY_H

#include <QDialog>

#include "ui_ShareKey.h"

#define CHANNEL_KEY_SHARE 0x00000001
#define FORUM_KEY_SHARE	  0x00000002

class ShareKey : public QDialog {
    Q_OBJECT
public:
    /*
     *@param chanId The channel id to send request for
     */
    ShareKey(QWidget *parent = 0, Qt::WFlags flags = 0,
    		std::string grpId = "", int grpType = 0);
    ~ShareKey();



protected:
    void changeEvent(QEvent *e);
    void closeEvent (QCloseEvent * event);

private:

    void setShareList();

    Ui::ShareKey *ui;

    std::string mGrpId;
    std::list<std::string> mShareList;
    int mGrpType;

private slots:

    void shareKey();
    void cancel();
    void togglePersonItem(QTreeWidgetItem* item, int col);


};

#endif // SHAREKEY_H
