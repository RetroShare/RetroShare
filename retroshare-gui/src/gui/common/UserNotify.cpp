/*******************************************************************************
 * retroshare-gui/src/gui/common/UserNotify.cpp                                *
 *                                                                             *
 * Copyright (c) 2012, RetroShare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QApplication>
#include <QIconEngine>
#include <QPainter>

#include "UserNotify.h"
#include "rshare.h"
#include "gui/settings/rsharesettings.h"

namespace
{
// Paint at the requested size so badges also follow toolbar size and DPI changes.
class NotificationIconEngine : public QIconEngine
{
public:
	NotificationIconEngine(const QIcon& icon, unsigned int count)
	    : mIcon(icon), mText(count > 999 ? QStringLiteral("999+") : QString::number(count)) {}

	QIconEngine* clone() const override { return new NotificationIconEngine(*this); }

	void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override
	{
		mIcon.paint(painter, rect, Qt::AlignCenter, mode, state);
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		const qreal size = qMin(rect.width(), rect.height());
		QFont font = QApplication::font();
		font.setBold(true);
		font.setPixelSize(qMax(1, qRound(size * 0.32)));
		painter->setFont(font);
		const QFontMetricsF metrics(font);
		qreal height = qMax(size * 0.50, metrics.height() + size * 0.04);
		const qreal width = qMax(height, metrics.boundingRect(mText).width() + size * 0.16);
		// Keep one- and two-digit badges circular, including with wider fonts.
		if (mText.size() <= 2) height = width;
		// The outline reaches the icon edge; only half the stroke is inset.
		const qreal border = size / 64.0;
		const QRectF badge(rect.x() + rect.width() - width - border / 2,
		                   rect.y() + rect.height() - height - border / 2,
		                   width, height);
		painter->setPen(QPen(Qt::white, border));
		painter->setBrush(QColor(QStringLiteral("#E21D3A")));
		painter->drawRoundedRect(badge, height / 2, height / 2);
		painter->setPen(Qt::white);
		painter->drawText(badge, Qt::AlignCenter, mText);
		painter->restore();
	}

	QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override
	{
		QPixmap result(size);
		result.fill(Qt::transparent);
		QPainter painter(&result);
		paint(&painter, QRect(QPoint(0, 0), size), mode, state);
		return result;
	}

private:
	QIcon mIcon;
	QString mText;
};
}

UserNotify::UserNotify(QObject *parent)
    : QObject(parent)
    , mMainToolButton(nullptr), mMainAction(nullptr)
    , mListItem(nullptr), mTrayIcon(nullptr)
    , mNotifyIcon(nullptr), mNewCount(0)
    , mLastBlinking(false)
{
	connect(rApp, SIGNAL(blink(bool)), this, SLOT(blink(bool)));
}

UserNotify::~UserNotify()
{
}

bool UserNotify::notifyEnabled()
{
	QString group;
	if (!hasSetting(NULL, &group) || group.isEmpty()) {
		return false;
	}

	return Settings->valueFromGroup(group, "TrayNotifyEnable", true).toBool();
}

bool UserNotify::notifyCombined()
{
	QString group;
	if (!hasSetting(NULL, &group) || group.isEmpty()) {
		return false;
	}

	return Settings->valueFromGroup(group, "TrayNotifyCombined", false).toBool();
}

bool UserNotify::notifyBlink()
{
	QString group;
	if (!hasSetting(NULL, &group) || group.isEmpty()) {
		return false;
	}

	return Settings->valueFromGroup(group, "TrayNotifyBlink", false).toBool();
}

void UserNotify::setNotifyEnabled(bool enabled, bool combined, bool blink)
{
	QString group;
	if (!hasSetting(NULL, &group) || group.isEmpty()) {
		return;
	}

	Settings->beginGroup(group);
	Settings->setValue("TrayNotifyEnable", enabled);
	Settings->setValue("TrayNotifyCombined", combined);
	Settings->setValue("TrayNotifyBlink", blink);
	Settings->endGroup();
}

void UserNotify::initialize(QToolBar *mainToolBar, QAction *mainAction, QListWidgetItem *listItem)
{
	mMainAction = mainAction;
	if (mMainAction) {
		mButtonText = mMainAction->text();
		if (mainToolBar) {
			mMainToolButton = dynamic_cast<QToolButton*>(mainToolBar->widgetForAction(mMainAction));
		}
	}
	mListItem = listItem;
	if (mListItem && mMainAction) {
		mButtonText = mMainAction->text();
	}
}

void UserNotify::createIcons(QMenu *notifyMenu)
{
#define DELETE_OBJECT(x) if (x) { delete(x); x = nullptr; }

	/* Create systray icons or actions */
	if (notifyEnabled()) {
		if (notifyCombined()) {
			DELETE_OBJECT(mTrayIcon);

			if (mNotifyIcon == nullptr) {
				mNotifyIcon = notifyMenu->addAction(getIcon(), "", this, SLOT(trayIconClicked()));
				mNotifyIcon->setVisible(false);
				connect(mNotifyIcon, SIGNAL(hovered()), this, SLOT(trayIconHovered()));
			}
		} else {
			DELETE_OBJECT(mNotifyIcon);

			if (mTrayIcon == NULL) {
				/* Create the tray icon for messages */
				mTrayIcon = new QSystemTrayIcon(this);
#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK (5, 10, 0)
				// Set tray icon visible to hide it again with ::hide.
				// ::hide or ::setVisible(false) does nothing when QSystemTrayIcon is hidden.
				mTrayIcon->show();
#endif
#endif
				mTrayIcon->setIcon(getIcon());
#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK (5, 10, 0)
				// Hide tray icon as workaround for Qt 5.10.0 (Windows only?)
				// QSystemTrayIcon is initially not visible but the icon gets visible after the call to ::setIcon.
				mTrayIcon->hide();
#endif
#endif
				connect(mTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));
			}
		}
	} else {
		DELETE_OBJECT(mTrayIcon);
		DELETE_OBJECT(mNotifyIcon);
	}

#undef DELETE_OBJECT
}

void UserNotify::updateIcon()
{
	startUpdate();
}

void UserNotify::startUpdate()
{
	update();
}

void UserNotify::update()
{
	unsigned int count = getNewCount();

	if (mMainAction) {
		const QIcon icon = getMainIcon(count > 0);
		mMainAction->setIcon(count > 0 ? QIcon(new NotificationIconEngine(icon, count)) : icon);
		mMainAction->setText(mButtonText);

		if(count > 0)
		{
			mMainAction->setToolTip(QString("%1 (%2)").arg(mButtonText, getNotifyMessage(count > 1).arg(count) ));
		}
		else
		{
			mMainAction->setText(mButtonText);
			mMainAction->setToolTip(mButtonText);
		}

		QFont font = mMainAction->font();
		font.setBold(false);
		mMainAction->setFont(font);
	}

	if (mListItem) {
		mListItem->setIcon(getMainIcon(count > 0));
		mListItem->setText((count > 0) ? QString("%1 (%2)").arg(mButtonText).arg(count) : mButtonText);

		QFont font = mListItem->font();
		font.setBold(count > 0);
		mListItem->setFont(font);
	}
	if (mMainToolButton) {
		QFont font = mMainToolButton->font();
		font.setBold(false);
		mMainToolButton->setFont(font);
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

	if (mNewCount != count) {
		emit countChanged();
	}

	mNewCount = count;
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
	if (e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick || e == QSystemTrayIcon::Context) {
		iconClicked();
	}
}

void UserNotify::trayIconHovered()
{
	iconHovered();
}

void UserNotify::blink(bool on)
{
	if (mTrayIcon) {
		bool blinking = notifyBlink();

		if (blinking) {
			/* blink icon */
			mTrayIcon->setIcon(on ? getIcon() : QIcon());
		} else {
			if (mLastBlinking) {
				/* reset icon */
				mTrayIcon->setIcon(getIcon());
			}
		}

		mLastBlinking = blinking;
	}
}
