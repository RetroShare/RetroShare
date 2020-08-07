/*******************************************************************************
 * retroshare-gui/src/gui/Posted/BoardPostDisplayWidget.cpp                    *
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
#include "BoardPostDisplayWidget.h"
#include "gui/feeds/FeedHolder.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"

#include "ui_BoardPostDisplayWidget.h"

#include <retroshare/rsposted.h>
#include <iostream>

#define LINK_IMAGE ":/images/thumb-link.png"

/** Constructor */

const char *BoardPostDisplayWidget::DEFAULT_BOARD_IMAGE = ":/icons/png/newsfeed2.png";

//===================================================================================================================================
//==                                                 Base class BoardPostDisplayWidget                                             ==
//===================================================================================================================================

BoardPostDisplayWidget::BoardPostDisplayWidget(const RsPostedPost& post,DisplayMode mode,QWidget *parent)
    : QWidget(parent),ui(new Ui::BoardPostDisplayWidget()),dmode(mode),mPost(post)
{
    ui->setupUi(this);
    mExpanded = false;

    setup();
}

void BoardPostDisplayWidget::setCommentsSize(int comNb)
{
	QString sComButText = tr("Comment");
	if (comNb == 1)
		sComButText = sComButText.append("(1)");
	else if(comNb > 1)
		sComButText = tr("Comments ").append("(%1)").arg(comNb);

	ui->commentButton->setText(sComButText);
}

void BoardPostDisplayWidget::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, false);
}

void BoardPostDisplayWidget::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

	ui->voteUpButton->setEnabled(false);
	ui->voteDownButton->setEnabled(false);

	emit vote(msgId, true);
}

void BoardPostDisplayWidget::setReadStatus(bool isNew, bool isUnread)
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

//	ui->mainFrame->setProperty("new", isNew);
//	ui->mainFrame->style()->unpolish(ui->mainFrame);
//	ui->mainFrame->style()->polish(  ui->mainFrame);
}

void BoardPostDisplayWidget::setComment(const RsGxsComment& cmt) {}

BoardPostDisplayWidget::~BoardPostDisplayWidget()
{
	delete(ui);
}

void BoardPostDisplayWidget::toggleNotes() {}

void BoardPostDisplayWidget::setup()
{
    // show/hide things based on the view type

    if(dmode == DISPLAY_MODE_COMPACT)
    {
        ui->pictureLabel_compact->show();
        ui->expandButton->show();

        if(mExpanded)
        {
            ui->frame_picture->show();
            ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/images/decrease.png")));
            ui->expandButton->setToolTip(tr("Hide"));
        }
        else
        {
            ui->frame_picture->hide();
            ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/images/expand.png")));
            ui->expandButton->setToolTip(tr("Expand"));
        }
    }
    else
    {
        ui->frame_picture->hide();
        ui->pictureLabel_compact->hide();
        ui->expandButton->hide();
    }

    setAttribute(Qt::WA_DeleteOnClose, true);

    /* clear ui */
    ui->titleLabel->setText(tr("Loading"));
    ui->dateLabel->clear();
    ui->fromLabel->clear();
    ui->siteLabel->clear();

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

    RsReputationLevel overall_reputation = rsReputations->overallReputationLevel(mPost.mMeta.mAuthorId);
    bool redacted = (overall_reputation == RsReputationLevel::LOCALLY_NEGATIVE);

    if(redacted)
    {
        ui->commentButton->setDisabled(true);
        ui->voteUpButton->setDisabled(true);
        ui->voteDownButton->setDisabled(true);
        ui->fromLabel->setId(mPost.mMeta.mAuthorId);
        ui->titleLabel->setText(tr( "<p><font color=\"#ff0000\"><b>The author of this message (with ID %1) is banned.</b>").arg(QString::fromStdString(mPost.mMeta.mAuthorId.toStdString()))) ;
        QDateTime qtime;
        qtime.setTime_t(mPost.mMeta.mPublishTs);
        QString timestamp = qtime.toString("hh:mm dd-MMM-yyyy");
        ui->dateLabel->setText(timestamp);
    }
    else
    {
        QPixmap sqpixmap2 = FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default.png");

        QDateTime qtime;
        qtime.setTime_t(mPost.mMeta.mPublishTs);
        QString timestamp = qtime.toString("hh:mm dd-MMM-yyyy");
        QString timestamp2 = misc::timeRelativeToNow(mPost.mMeta.mPublishTs);
        ui->dateLabel->setText(timestamp2);
        ui->dateLabel->setToolTip(timestamp);

        ui->fromLabel->setId(mPost.mMeta.mAuthorId);

        // Use QUrl to check/parse our URL
        // The only combination that seems to work: load as EncodedUrl, extract toEncoded().
        QByteArray urlarray(mPost.mLink.c_str());
        QUrl url = QUrl::fromEncoded(urlarray.trimmed());
        QString urlstr = "Invalid Link";
        QString sitestr = "Invalid Link";

        bool urlOkay = url.isValid();
        if (urlOkay)
        {
            QString scheme = url.scheme();
            if ((scheme != "https")
                            && (scheme != "http")
                            && (scheme != "ftp")
                            && (scheme != "retroshare"))
            {
                urlOkay = false;
                sitestr = "Invalid Link Scheme";
            }
        }

        if (urlOkay)
        {
            urlstr =  QString("<a href=\"");
            urlstr += QString(url.toEncoded());
            urlstr += QString("\" ><span style=\" text-decoration: underline; color:#2255AA;\"> ");
            urlstr += QString::fromUtf8(mPost.mMeta.mMsgName.c_str());
            urlstr += QString(" </span></a>");

            QString siteurl = url.toEncoded();
            sitestr = QString("<a href=\"%1\" ><span style=\" text-decoration: underline; color:#0079d3;\"> %2 </span></a>").arg(siteurl).arg(siteurl);

            ui->titleLabel->setText(urlstr);
        }
        else
            ui->titleLabel->setText( QString::fromUtf8(mPost.mMeta.mMsgName.c_str()) );

        if (urlarray.isEmpty())
        {
            ui->siteLabel->hide();
        }

        ui->siteLabel->setText(sitestr);

        if(dmode == DISPLAY_MODE_COMPACT)
        {
            if(mPost.mImage.mData != NULL)
            {
                QPixmap pixmap;
                GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
                // Wiping data - as its been passed to thumbnail.


                QPixmap scaledpixmap;
                if(pixmap.width() > 800){
                    QPixmap scaledpixmap = pixmap.scaledToWidth(800, Qt::SmoothTransformation);
                    ui->pictureLabel_compact->setPixmap(scaledpixmap);
                }else{
                    ui->pictureLabel_compact->setPixmap(pixmap);
                }
            }
            else if (mPost.mImage.mData == NULL)
                ui->pictureLabel_compact->hide();
            else
                ui->pictureLabel_compact->show();
        }
        else
        {
            if(mPost.mImage.mData != NULL)
            {
                QPixmap pixmap;
                GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
                // Wiping data - as its been passed to thumbnail.


                QPixmap scaledpixmap;
                if(pixmap.width() > 800){
                    QPixmap scaledpixmap = pixmap.scaledToWidth(800, Qt::SmoothTransformation);
                    ui->pictureLabel->setPixmap(scaledpixmap);
                }else{
                    ui->pictureLabel->setPixmap(pixmap);
                }
            }
            else if (mPost.mImage.mData == NULL)
                ui->pictureLabel->hide();
            else
                ui->pictureLabel->show();
        }
    }

    //QString score = "Hot" + QString::number(post.mHotScore);
    //score += " Top" + QString::number(post.mTopScore);
    //score += " New" + QString::number(post.mNewScore);

    QString score = QString::number(mPost.mTopScore);

    ui->scoreLabel->setText(score);

    // FIX THIS UP LATER.
    ui->notes->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

    QTextDocument doc;
    doc.setHtml(ui->notes->text());

    if(doc.toPlainText().trimmed().isEmpty())
        ui->notes->hide();

#ifdef TO_REMOVE
    // differences between Feed or Top of Comment.
    if (mFeedHolder)
    {
        // feed.
        //frame_comment->show();
        ui->commentButton->show();

        if (mPost.mComments)
        {
            QString commentText = QString::number(mPost.mComments);
            commentText += " ";
            commentText += tr("Comments");
            ui->commentButton->setText(commentText);
        }
        else
        {
            ui->commentButton->setText(tr("Comment"));
        }

        setReadStatus(IS_MSG_NEW(mPost.mMeta.mMsgStatus), IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) || IS_MSG_NEW(mPost.mMeta.mMsgStatus));
    }
    else
#endif
    {
        // no feed.
        //frame_comment->hide();
        ui->commentButton->hide();

        ui->readButton->hide();
        ui->newLabel->hide();
    }

#ifdef TO_REMOVE
    if (mIsHome)
    {
        ui->clearButton->hide();
        ui->readAndClearButton->hide();
    }
    else
#endif
    {
        ui->clearButton->show();
        ui->readAndClearButton->show();
    }

    // disable voting buttons - if they have already voted.
    if (mPost.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
    {
        ui->voteUpButton->setEnabled(false);
        ui->voteDownButton->setEnabled(false);
    }

#if 0
    uint32_t up, down, nComments;

    bool ok = rsPosted->retrieveScores(mPost.mMeta.mServiceString, up, down, nComments);

    if(ok)
    {
        int32_t vote = up - down;
        scoreLabel->setText(QString::number(vote));

        numCommentsLabel->setText("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px;"
                                  "margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span"
                                  "style=\" font-size:10pt; font-weight:600;\">#</span><span "
                                  "style=\" font-size:8pt; font-weight:600;\"> Comments:  "
                                  + QString::number(nComments) + "</span></p>");
    }

    mInFill = false;
#endif

#ifdef TODO
    emit sizeChanged(this);
#endif
}


