/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 by Thunder
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

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
	: QWidget(NULL), mFeedReader(feedReader), mNotify(notify), mParent(parent), ui(new Ui::FeedReaderFeedItem)
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

	ui->titleLabel->setText(QString::fromUtf8(feedInfo.name.c_str()));
	ui->msgTitleLabel->setText(QString::fromUtf8(msgInfo.title.c_str()));
	ui->descriptionLabel->setText(QString::fromUtf8(msgInfo.description.c_str()));

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
	mParent->lockLayout(this, true);

	if (ui->expandFrame->isHidden()) {
		ui->expandFrame->show();
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
		ui->expandButton->setToolTip(tr("Hide"));

		setMsgRead();
	} else {
		ui->expandFrame->hide();
		ui->expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	mParent->lockLayout(this, false);
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
