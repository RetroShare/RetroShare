/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedCardView.cpp                            *
 *                                                                             *
 * Copyright (C) 2019  Retroshare Team       <retroshare.project@gmail.com>    *
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

#include <QDateTime>
#include <QMenu>
#include <QStyle>
#include <QTextDocument>

#include "rshare.h"
#include "PostedCardView.h"
#include "gui/feeds/FeedHolder.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"

#include "ui_PostedCardView.h"
#include "retroshare/rsposted.h"

#include <iostream>

#define LINK_IMAGE ":/images/thumb-link.png"

static QString formatDate(uint64_t seconds)
{
    QDateTime dt = DateTime::DateTimeFromTime_t(seconds);
    switch(Settings->getDateFormat()) {
        case RshareSettings::DateFormat_ISO:
            return dt.toString(Qt::ISODate).replace('T', ' ');
        case RshareSettings::DateFormat_Text:
            return QLocale::system().toString(dt, QLocale::LongFormat);
        case RshareSettings::DateFormat_System:
        default:
            return QLocale::system().toString(dt, QLocale::ShortFormat);
    }
}

/** Constructor */

PostedCardView::PostedCardView(FeedHolder *feedHolder, uint32_t feedId, const RsGroupMetaData &group_meta, const RsGxsMessageId &post_id, bool isHome, bool autoUpdate)
    : BasePostedItem(feedHolder, feedId, group_meta, post_id, isHome, autoUpdate)
{
    PostedCardView::setup();
}

PostedCardView::PostedCardView(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate)
    : BasePostedItem(feedHolder, feedId, groupId, post_id, isHome, autoUpdate)
{
    PostedCardView::setup();
    PostedCardView::loadGroup();
}

void PostedCardView::setCommentsSize(int comNb)
{
	QString sComButText = tr("Comment");
	if (comNb == 1)
		sComButText = sComButText.append("(1)");
	else if(comNb > 1)
		sComButText = tr("Comments ").append("(%1)").arg(comNb);

	ui->commentButton->setText(sComButText);
}

void PostedCardView::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, false);
}

void PostedCardView::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, true);
}

void PostedCardView::setReadStatus(bool isNew, bool isUnread)
{
	if (isUnread)
	{
		ui->readButton->setChecked(true);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
	}
	else
	{
		ui->readButton->setChecked(false);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png"));
	}

	ui->newLabel->setVisible(isNew);

	ui->feedFrame->setProperty("new", isNew);
	ui->feedFrame->style()->unpolish(ui->feedFrame);
	ui->feedFrame->style()->polish(  ui->feedFrame);
}

void PostedCardView::setComment(const RsGxsComment& /*cmt*/) {}

PostedCardView::~PostedCardView()
{
	delete(ui);
}

void PostedCardView::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::PostedCardView;
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	mInFill = false;

	/* clear ui */
	ui->titleLabel->setText(tr("Loading"));
	ui->dateLabel->clear();
	ui->fromLabel->clear();
	ui->siteLabel->clear();

	/* general ones */
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));

	connect(ui->commentButton, SIGNAL( clicked()), this, SLOT(loadComments()));
	connect(ui->voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
	connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT( makeDownVote()));
	connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));
	
	QAction *CopyLinkAction = new QAction(QIcon(""),tr("Copy RetroShare Link"), this);
	connect(CopyLinkAction, SIGNAL(triggered()), this, SLOT(copyMessageLink()));
	
	QAction *showInPeopleAct = new QAction(QIcon(), tr("Show author in people tab"), this);
	connect(showInPeopleAct, SIGNAL(triggered()), this, SLOT(showAuthorInPeople()));
	
	int S = QFontMetricsF(font()).height() ;
	
	ui->voteUpButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->voteDownButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->commentButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->readButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->shareButton->setIconSize(QSize(S*1.5,S*1.5));
	
	QMenu *menu = new QMenu();
	menu->addAction(CopyLinkAction);
	menu->addSeparator();
	menu->addAction(showInPeopleAct);
	ui->shareButton->setMenu(menu);

	ui->clearButton->hide();
	ui->readAndClearButton->hide();
}

void PostedCardView::fill()
{
	RsReputationLevel overall_reputation = rsReputations->overallReputationLevel(mPost.mMeta.mAuthorId);
	bool redacted = (overall_reputation == RsReputationLevel::LOCALLY_NEGATIVE);

	if(redacted) {
		ui->commentButton->setDisabled(true);
		ui->voteUpButton->setDisabled(true);
		ui->voteDownButton->setDisabled(true);
		ui->picture_frame->hide();
		ui->fromLabel->setId(mPost.mMeta.mAuthorId);
		ui->titleLabel->setText(tr( "<p><font color=\"#ff0000\"><b>The author of this message (with ID %1) is banned.</b>").arg(QString::fromStdString(mPost.mMeta.mAuthorId.toStdString()))) ;
		
        // [MODIFICATION] Use standardized helper for consistent Ban message date
		ui->dateLabel->setText(formatDate(mPost.mMeta.mPublishTs));
	} else {
		mInFill = true;

        // [MODIFICATION] Using the helper to respect format settings. 
        // Removed hardcoded relative time calls ("timestamp2").
		ui->dateLabel->setText(formatDate(mPost.mMeta.mPublishTs));
		ui->dateLabel->setToolTip(QLocale::system().toString(DateTime::DateTimeFromTime_t(mPost.mMeta.mPublishTs), QLocale::ShortFormat));

		ui->fromLabel->setId(mPost.mMeta.mAuthorId);

		QByteArray urlarray(mPost.mLink.c_str());
		QUrl url = QUrl::fromEncoded(urlarray.trimmed());
		QString sitestr = "Invalid Link";

		bool urlOkay = url.isValid();
		if (urlOkay && (url.scheme() == "https" || url.scheme() == "http" || url.scheme() == "ftp" || url.scheme() == "retroshare")) 
		{
			QString urlstr =  QString("<a href=\"%1\" ><span style=\" text-decoration: underline; color:#2255AA;\"> %2 </span></a>")
                             .arg(QString(url.toEncoded())).arg(messageName());
			ui->titleLabel->setText(urlstr);
			sitestr = QString("<a href=\"%1\" ><span style=\" text-decoration: underline; color:#0079d3;\"> %1 </span></a>").arg(QString(url.toEncoded()));
		}else
		{
			ui->titleLabel->setText(messageName());
		}
		
		if (urlarray.isEmpty()) ui->siteLabel->hide();
		ui->siteLabel->setText(sitestr);
		
		if(mPost.mImage.mData != NULL)
		{
			QPixmap pixmap;
			GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
			if(pixmap.width() > 800)
				ui->pictureLabel->setPixmap(pixmap.scaledToWidth(800, Qt::SmoothTransformation));
			else
				ui->pictureLabel->setPixmap(pixmap);
		}
		else 
		{
			ui->picture_frame->hide();
		}
	}

	ui->scoreLabel->setText(QString::number(mPost.mTopScore));
    ui->notes->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), RSHTML_FORMATTEXT_EMBED_LINKS));

	if(mFeedHolder)
	{
		ui->commentButton->show();
		ui->commentButton->setText(mPost.mComments ? QString::number(mPost.mComments) + " " + tr("Comments") : tr("Comment"));
		setReadStatus(IS_MSG_NEW(mPost.mMeta.mMsgStatus), IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) || IS_MSG_NEW(mPost.mMeta.mMsgStatus));
	}
	else
	{
		ui->commentButton->hide();
		ui->readButton->hide();
	}

	if (mPost.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
	{
		ui->voteUpButton->setEnabled(false);
		ui->voteDownButton->setEnabled(false);
	}

	mInFill = false;
	emit sizeChanged(this);
}

void PostedCardView::toggleNotes() {}
