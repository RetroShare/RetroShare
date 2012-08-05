#include <QMenu>
#include "UserNotify.h"

UserNotify::UserNotify(QObject *parent) :
	QObject(parent)
{
	mMainIcon = NULL;
	mTrayIcon = NULL;
	mNotifyIcon = NULL;
	newCount = 0;
}

void UserNotify::initialize(QAction *mainAction)
{
	mMainIcon = mainAction;
}

void UserNotify::createIcons(QMenu *notifyMenu)
{
#define DELETE_OBJECT(x) if (x) { delete(x); x = NULL; }

	/* Create systray icons or actions */
	if (notifyEnabled()) {
		if (notifyCombined()) {
			DELETE_OBJECT(mTrayIcon);

			if (mNotifyIcon == NULL) {
				mNotifyIcon = notifyMenu->addAction(getIcon(), "", this, SLOT(trayIconClicked()));
				mNotifyIcon->setVisible(false);
			}
		} else if (mTrayIcon == NULL) {
			/* Create the tray icon for messages */
			mTrayIcon = new QSystemTrayIcon(this);
			mTrayIcon->setIcon(getIcon());
			connect(mTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));
		}
	} else {
		DELETE_OBJECT(mTrayIcon);
		DELETE_OBJECT(mNotifyIcon);
	}

#undef DELETE_OBJECT
}

void UserNotify::updateIcon()
{
	unsigned int count = getNewCount();

	if (mMainIcon) {
		mMainIcon->setIcon(getMainIcon(count > 0));
	}

	if (mTrayIcon) {
		if (count) {
			mTrayIcon->setToolTip("RetroShare\n" + getTrayMessage(count > 1).arg(count));
			mTrayIcon->show();
		} else {
			mTrayIcon->hide();
		}
	}

	if (mNotifyIcon) {
		mNotifyIcon->setData(count);
		if (count) {
			mNotifyIcon->setText(getNotifyMessage(count > 1).arg(count));
			mNotifyIcon->setVisible(true);
		} else {
			mNotifyIcon->setVisible(false);
		}
	}

	if (newCount != count) {
		emit countChanged();
	}

	count = newCount;
}

QString UserNotify::getTrayMessage(bool plural)
{
	return plural ? tr("You have %1 new messages") : tr("You have %1 new message");
}

QString UserNotify::getNotifyMessage(bool plural)
{
	return plural ? tr("%1 new messages") : tr("%1 new message");
}

void UserNotify::trayIconClicked(QSystemTrayIcon::ActivationReason e)
{
	if (e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick) {
		iconClicked();
	}
}
