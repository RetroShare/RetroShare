#ifndef USERNOTIFY_H
#define USERNOTIFY_H

#include <QObject>
#include <QSystemTrayIcon>

class QAction;

class UserNotify : public QObject
{
	Q_OBJECT

public:
	UserNotify(QObject *parent = 0);

	void initialize(QAction *mainAction);
	void createIcons(QMenu *notifyMenu);

	virtual bool hasSetting(QString &/*name*/) { return false; }
	virtual bool notifyEnabled() { return false; }
	virtual bool notifyCombined() { return false; }
	virtual void setNotifyEnabled(bool /*enabled*/, bool /*combined*/) {}

signals:
	void countChanged();

public slots:
	void updateIcon();

private slots:
	void trayIconClicked(QSystemTrayIcon::ActivationReason e = QSystemTrayIcon::Trigger);

private:
	virtual QIcon getIcon() { return QIcon(); }
	virtual QIcon getMainIcon(bool /*hasNew*/) { return QIcon(); }
	virtual unsigned int getNewCount() { return 0; }

	virtual QString getTrayMessage(bool plural);
	virtual QString getNotifyMessage(bool plural);

	virtual void iconClicked() {}

	QAction *mMainIcon;
	QSystemTrayIcon *mTrayIcon;
	QAction *mNotifyIcon;
	unsigned int newCount;
};

#endif // USERNOTIFY_H
