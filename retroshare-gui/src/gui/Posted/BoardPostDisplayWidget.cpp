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
#include "PhotoView.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "gui/Identity/IdDialog.h"
#include "gui/MainWindow.h"

#include "ui_BoardPostDisplayWidget.h"

#include <retroshare/rsposted.h>
#include <iostream>

#define LINK_IMAGE ":/images/thumb-link.png"

/** Constructor */

const char *BoardPostDisplayWidget::DEFAULT_BOARD_IMAGE = ":/icons/png/newsfeed2.png";

//===================================================================================================================================
//==                                                 Base class BoardPostDisplayWidget                                             ==
//===================================================================================================================================

BoardPostDisplayWidget::BoardPostDisplayWidget(const RsPostedPost& post, DisplayMode mode, uint8_t display_flags, QWidget *parent)
    : QWidget(parent),mPost(post),dmode(mode),mDisplayFlags(display_flags),ui(new Ui::BoardPostDisplayWidget())
{
    ui->setupUi(this);
    setup();

    ui->verticalLayout->addStretch();
    ui->verticalLayout->setAlignment(Qt::AlignTop);
    ui->topLayout->setAlignment(Qt::AlignTop);
    ui->arrowsLayout->addStretch();
    ui->arrowsLayout->setAlignment(Qt::AlignTop);
    ui->verticalLayout_2->addStretch();
//    ui->verticalLayout_3->addStretch();
//    ui->verticalLayout_3->setAlignment(Qt::AlignTop);

    adjustSize();

//    if(display_flags & SHOW_COMMENTS)
//    {
//        ui->commentsWidget->setTokenService(rsPosted->getTokenService(),rsPosted);
//
//        std::set<RsGxsMessageId> post_versions ;
//        post_versions.insert(post.mMeta.mMsgId) ;
//
//        ui->commentsWidget->commentLoad(post.mMeta.mGroupId, post_versions,mPost.mMeta.mMsgId,true);
//    }
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


void BoardPostDisplayWidget::viewPicture()
{
    if(mPost.mImage.mData == NULL)
        return;

    QString timestamp = misc::timeRelativeToNow(mPost.mMeta.mPublishTs);
    QPixmap pixmap;
    GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
    RsGxsId authorID = mPost.mMeta.mAuthorId;

    PhotoView *PView = new PhotoView(this);

    PView->setPixmap(pixmap);
    PView->setTitle(QString::fromUtf8(mPost.mMeta.mMsgName.c_str()));
    PView->setName(authorID);
    PView->setTime(timestamp);
    PView->setGroupId(mPost.mMeta.mGroupId);
    PView->setMessageId(mPost.mMeta.mMsgId);

    PView->show();

    /* window will destroy itself! */
}

void BoardPostDisplayWidget::toggleNotes() {}

void BoardPostDisplayWidget::setup()
{
    // show/hide things based on the view type

    switch(dmode)
    {
    default:
    case  DISPLAY_MODE_COMPACT:
    {
        ui->pictureLabel_compact->show();
        //ui->expandButton->hide();
        ui->expandButton->show();	// always hide, since we have the photoview already

        ui->pictureLabel->hide();
        ui->notes->hide();
        ui->siteLabel->hide();
    }
        break;
    case DISPLAY_MODE_CARD_VIEW:
    {
        ui->frame_picture->hide();
        ui->pictureLabel_compact->hide();
        ui->expandButton->hide();
    }
        break;
    }


    if(mDisplayFlags & SHOW_NOTES)
    {
        ui->frame_picture->show();
        ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/images/decrease.png")));
        ui->expandButton->setToolTip(tr("Hide"));
        ui->expandButton->setChecked(true);
    }
    else
    {
        ui->frame_picture->hide();
        ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/images/expand.png")));
        ui->expandButton->setToolTip(tr("Expand"));
        ui->expandButton->setChecked(false);
    }

    if(!(mDisplayFlags & SHOW_COMMENTS))
    {
        //ui->commentsWidget->hide();
        ui->commentButton->setChecked(false);
    }
    else
        ui->commentButton->setChecked(true);

    setAttribute(Qt::WA_DeleteOnClose, true);

    /* clear ui */
    ui->titleLabel->setText(tr("Loading"));
    ui->dateLabel->clear();
    ui->fromLabel->clear();
    ui->siteLabel->clear();

    connect(ui->expandButton, SIGNAL(toggled(bool)), this, SLOT(doExpand(bool)));
    connect(ui->commentButton, SIGNAL(toggled(bool)), this, SLOT(loadComments(bool)));
    connect(ui->voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
    connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT(makeDownVote()));
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
        QString timestamp2 = misc::timeRelativeToNow(mPost.mMeta.mPublishTs) + " " + tr("ago");
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

                int desired_height = QFontMetricsF(font()).height() * 5;

                QPixmap scaledpixmap = pixmap.scaledToHeight(desired_height, Qt::SmoothTransformation);
                ui->pictureLabel_compact->setPixmap(scaledpixmap);
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
    ui->pictureLabel_2->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

    QTextDocument doc;
    doc.setHtml(ui->notes->text());

    if(doc.toPlainText().trimmed().isEmpty())
    {
        ui->notes->hide();
        ui->expandButton->hide();
    }

    // feed.
    //frame_comment->show();
    ui->commentButton->show();

    if (mPost.mComments)
    {
        QString commentText = tr("Comments (%1)").arg(QString::number(mPost.mComments));
        ui->commentButton->setText(commentText);
    }
    else
        ui->commentButton->setText(tr("Comment"));

    setReadStatus(IS_MSG_NEW(mPost.mMeta.mMsgStatus), IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) || IS_MSG_NEW(mPost.mMeta.mMsgStatus));

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

    ui->pictureLabel_compact->setUseStyleSheet(false);	// If not this, causes dilation of the image.
    connect(ui->pictureLabel_compact, SIGNAL(clicked()), this, SLOT(viewPicture()));

    updateGeometry();

#ifdef TODO
    emit sizeChanged(this);
#endif
}

void BoardPostDisplayWidget::doExpand(bool e)
{
     std::cerr << "Expanding" << std::endl;
     if(e)
     {
         //ui->notes->show();
         ui->pictureLabel_2->show();
     }
     else
         {
         //ui->notes->hide();
         ui->pictureLabel_2->hide();
     }
    emit expand(mPost.mMeta.mMsgId,e);
}

void BoardPostDisplayWidget::loadComments(bool e)
{
    emit commentsRequested(mPost.mMeta.mMsgId,e);
}

void BoardPostDisplayWidget::showAuthorInPeople()
{
    if(mPost.mMeta.mAuthorId.isNull())
    {
        std::cerr << "(EE) GxsForumThreadWidget::loadMsgData_showAuthorInPeople() ERROR Missing Message Data...";
        std::cerr << std::endl;
    }

    /* window will destroy itself! */
    IdDialog *idDialog = dynamic_cast<IdDialog*>(MainWindow::getPage(MainWindow::People));

    if (!idDialog)
        return ;

    MainWindow::showWindow(MainWindow::People);
    idDialog->navigate(RsGxsId(mPost.mMeta.mAuthorId));
}
