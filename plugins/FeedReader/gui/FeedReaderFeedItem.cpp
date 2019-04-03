/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderFeedItem.cpp                               *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2012 by Thunder                                               *
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
#include <QUrl>
#include <QClipboard>
#include <QDesktopServices>

#include "FeedReaderFeedItem.h"
#include "ui_FeedReaderFeedItem.h"

#include "FeedReaderNotify.h"

#include "util/DateTime.h"
#include "gui/feeds/FeedHolder.h"

/** Constructor */
FeedReaderFeedItem::FeedReaderFeedItem(RsFeedReader *feedReader, FeedReaderNotify *notify, FeedHolder *parent, const FeedInfo &feedInfo, const FeedMsgInfo &msgInfo)
    : FeedItem(NULL), mFeedReader(feedReader), mNotify(notify), mParent(parent), ui(new Ui::FeedReaderFeedItem)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	connect(ui->expandButton, SIGNAL(clicked(void)), this, SLOT(toggle(void)));
	connect(ui->clearButton, SIGNAL(clicked(void)), this, SLOT(removeItem(void)));
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
	connect(ui->linkButton, SIGNAL(clicked()), this, SLOT(openLink()));

	connect(mNotify, SIGNAL(msgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)), Qt::QueuedConnection);

	ui->expandFrame->hide();

	mFeedId = feedInfo.feedId;
	mMsgId = msgInfo.msgId;

	if (feedInfo.icon.empty()) {
		ui->feedIconLabel->hide();
	} else {
		/* use icon from feed */
		QPixmap pixmap;
		if (pixmap.loadFromData(QByteArray::fromBase64(feedInfo.icon.c_str()))) {
			ui->feedIconLabel->setPixmap(pixmap.scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		} else {
			ui->feedIconLabel->hide();
		}
	}

	ui->titleLabel->setText(QString::fromUtf8(feedInfo.name.c_str()));
	ui->msgTitleLabel->setText(QString::fromUtf8(msgInfo.title.c_str()));
	ui->descriptionLabel->setText(QString::fromUtf8((msgInfo.descriptionTransformed.empty() ? msgInfo.description : msgInfo.descriptionTransformed).c_str()));

	ui->dateTimeLabel->setText(DateTime::formatLongDateTime(msgInfo.pubDate));

	/* build menu for link button */
	mLink = QString::fromUtf8(msgInfo.link.c_str());
	if (mLink.isEmpty()) {
		ui->linkButton->setEnabled(false);
	} else {
		QMenu *menu = new QMenu(this);
		QAction *action = menu->addAction(tr("Open link in browser"), this, SLOT(openLink()));
		menu->addAction(tr("Copy link to clipboard"), this, SLOT(copyLink()));

		QFont font = action->font();
		font.setBold(true);
		action->setFont(font);

		ui->linkButton->setMenu(menu);
	}
}

FeedReaderFeedItem::~FeedReaderFeedItem()
{
	delete(ui);
}

void FeedReaderFeedItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

void FeedReaderFeedItem::doExpand(bool open)
{
	if (mParent) {
		mParent->lockLayout(this, true);
	}

	if (open) {
		ui->expandFrame->show();
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		ui->expandButton->setToolTip(tr("Hide"));

		setMsgRead();
	} else {
		ui->expandFrame->hide();
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mParent) {
		mParent->lockLayout(this, false);
	}
}

void FeedReaderFeedItem::removeItem()
{
	mParent->lockLayout(this, true);
	hide();
	mParent->lockLayout(this, false);

	if (mParent) {
		mParent->deleteFeedItem(this, 0);
	}
}

/*********** SPECIFIC FUNCTIONS ***********************/

void FeedReaderFeedItem::readAndClearItem()
{
	setMsgRead();
	removeItem();
}

void FeedReaderFeedItem::setMsgRead()
{
	disconnect(mNotify, SIGNAL(msgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)));
	mFeedReader->setMessageRead(mFeedId, mMsgId, true);
	connect(mNotify, SIGNAL(msgChanged(QString,QString,int)), this, SLOT(msgChanged(QString,QString,int)), Qt::QueuedConnection);
}

void FeedReaderFeedItem::msgChanged(const QString &feedId, const QString &msgId, int /*type*/)
{
	if (feedId.toStdString() != mFeedId) {
		return;
	}

	if (msgId.toStdString() != mMsgId) {
		return;
	}

	FeedMsgInfo msgInfo;
	if (!mFeedReader->getMsgInfo(mFeedId, mMsgId, msgInfo)) {
		return;
	}

	if (!msgInfo.flag.isnew) {
		close();
		return;
	}
}

void FeedReaderFeedItem::copyLink()
{
	if (mLink.isEmpty()) {
		return;
	}

	QApplication::clipboard()->setText(mLink);
}

void FeedReaderFeedItem::openLink()
{
	if (mLink.isEmpty()) {
		return;
	}

	QDesktopServices::openUrl(QUrl(mLink));
}
