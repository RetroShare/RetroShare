#ifndef SHAREKEY_H
#define SHAREKEY_H

#include <QDialog>

#include "ui_GxsGroupShareKey.h"

#define CHANNEL_KEY_SHARE 0x00000001
#define FORUM_KEY_SHARE	  0x00000002
#define POSTED_KEY_SHARE  0x00000003

class GroupShareKey : public QDialog
{
	Q_OBJECT

public:
	/*
	 *@param chanId The channel id to send request for
	 */
    GroupShareKey(QWidget *parent = 0, const RsGxsGroupId& grpId = RsGxsGroupId(), int grpType = 0);
    ~GroupShareKey();

protected:
	void changeEvent(QEvent *e);

private slots:
	void shareKey();
  void setTyp();
private:
    RsGxsGroupId mGrpId;
	int mGrpType;

	Ui::ShareKey *ui;
};

#endif // SHAREKEY_H
