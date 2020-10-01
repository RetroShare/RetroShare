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
#include <QToolButton>

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

#include "ui_BoardPostDisplayWidget_compact.h"
#include "ui_BoardPostDisplayWidget_card.h"

#include <retroshare/rsposted.h>
#include <iostream>

#define LINK_IMAGE ":/images/thumb-link.png"

// #ifdef DEBUG_BOARDPOSTDISPLAYWIDGET 1

/** Constructor */

const char *BoardPostDisplayWidget_compact::DEFAULT_BOARD_IMAGE = ":/icons/png/newsfeed2.png";

//===================================================================================================================================
//==                                             Base class BoardPostDisplayWidgetBase                                             ==
//===================================================================================================================================

BoardPostDisplayWidgetBase::BoardPostDisplayWidgetBase(const RsPostedPost& post,uint8_t display_flags,QWidget *parent)
    : QWidget(parent), mPost(post),mDisplayFlags(display_flags)
{
}

void BoardPostDisplayWidgetBase::setCommentsSize(int comNb)
{
    QString sComButText ;

    if (comNb == 1)
        sComButText = tr("1 comment");
    else if(comNb > 1)
        sComButText = tr("%1 comments").arg(comNb);
    else
        sComButText = tr("No comments yet. Click to add one.");

    commentButton()->setToolTip(sComButText);

    if(comNb > 0)
        commentButton()->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/comments_blue.png"));
    else
        commentButton()->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/comments.png"));

//	QString sComButText = tr("Comment");
//	if (comNb == 1)
//		sComButText = sComButText.append("(1)");
//	else if(comNb > 1)
//		sComButText = tr("Comments ").append("(%1)").arg(comNb);
//
    commentButton()->setText(tr("Comments"));
}

void BoardPostDisplayWidgetBase::makeDownVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

    voteUpButton()->setEnabled(false);
    voteDownButton()->setEnabled(false);

	emit vote(msgId, false);
}

void BoardPostDisplayWidgetBase::makeUpVote()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mPost.mMeta.mGroupId;
	msgId.second = mPost.mMeta.mMsgId;

    voteUpButton()->setEnabled(false);
    voteDownButton()->setEnabled(false);

	emit vote(msgId, true);
}

void BoardPostDisplayWidgetBase::setReadStatus(bool isNew, bool isUnread)
{
	if (isUnread)
        readButton()->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
	else
        readButton()->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png"));

    newLabel()->setVisible(isNew);
}

void BoardPostDisplayWidget_compact::doExpand(bool e)
{
#ifdef DEBUG_BOARDPOSTDISPLAYWIDGET
     std::cerr << "Expanding" << std::endl;
#endif
     if(e)
         ui->frame_notes->show();
     else
         ui->frame_notes->hide();

    emit expand(mPost.mMeta.mMsgId,e);
}

void BoardPostDisplayWidgetBase::loadComments(bool e)
{
    emit commentsRequested(mPost.mMeta.mMsgId,e);
}

void BoardPostDisplayWidgetBase::readToggled()
{
    bool s = IS_MSG_UNREAD(mPost.mMeta.mMsgStatus);

    emit changeReadStatusRequested(mPost.mMeta.mMsgId,s);
}
void BoardPostDisplayWidgetBase::showAuthorInPeople()
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

void BoardPostDisplayWidgetBase::setup()
{
    // show/hide things based on the view type

    if(!(mDisplayFlags & SHOW_COMMENTS))
        commentButton()->setChecked(false);
    else
        commentButton()->setChecked(true);

    /* clear ui */
    titleLabel()->setText(tr("Loading"));
    dateLabel()->clear();
    fromLabel()->clear();
    siteLabel()->clear();

    QObject::connect(commentButton(), SIGNAL(toggled(bool)), this, SLOT(loadComments(bool)));
    QObject::connect(voteUpButton(), SIGNAL(clicked()), this, SLOT(makeUpVote()));
    QObject::connect(voteDownButton(), SIGNAL(clicked()), this, SLOT(makeDownVote()));
    QObject::connect(readButton(), SIGNAL(clicked()), this, SLOT(readToggled()));

    QAction *CopyLinkAction = new QAction(QIcon(""),tr("Copy RetroShare Link"), this);
    connect(CopyLinkAction, SIGNAL(triggered()), this, SLOT(copyMessageLink()));

    QAction *showInPeopleAct = new QAction(QIcon(), tr("Show author in people tab"), this);
    connect(showInPeopleAct, SIGNAL(triggered()), this, SLOT(showAuthorInPeople()));

    int S = QFontMetricsF(font()).height() ;

    readButton()->setChecked(false);

    QMenu *menu = new QMenu();
    menu->addAction(CopyLinkAction);
    menu->addSeparator();
    menu->addAction(showInPeopleAct);
    shareButton()->setMenu(menu);

    connect(shareButton(),SIGNAL(pressed()),this,SLOT(handleShareButtonClicked()));

    RsReputationLevel overall_reputation = rsReputations->overallReputationLevel(mPost.mMeta.mAuthorId);
    bool redacted = (overall_reputation == RsReputationLevel::LOCALLY_NEGATIVE);

    if(redacted)
    {
        commentButton()->setDisabled(true);
        voteUpButton()->setDisabled(true);
        voteDownButton()->setDisabled(true);
        fromLabel()->setId(mPost.mMeta.mAuthorId);
        titleLabel()->setText(tr( "<p><font color=\"#ff0000\"><b>The author of this message (with ID %1) is banned.</b>").arg(QString::fromStdString(mPost.mMeta.mAuthorId.toStdString()))) ;
        QDateTime qtime;
        qtime.setTime_t(mPost.mMeta.mPublishTs);
        QString timestamp = qtime.toString("hh:mm dd-MMM-yyyy");
        dateLabel()->setText(timestamp);
    }
    else
    {
        QPixmap sqpixmap2 = FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default.png");

        QDateTime qtime;
        qtime.setTime_t(mPost.mMeta.mPublishTs);
        QString timestamp = qtime.toString("hh:mm dd-MMM-yyyy");
        QString timestamp2 = misc::timeRelativeToNow(mPost.mMeta.mPublishTs) + " " + tr("ago");
        dateLabel()->setText(timestamp2);
        dateLabel()->setToolTip(timestamp);

        fromLabel()->setId(mPost.mMeta.mAuthorId);

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

            titleLabel()->setText(urlstr);
        }
        else
            titleLabel()->setText( QString::fromUtf8(mPost.mMeta.mMsgName.c_str()) );

        if (urlarray.isEmpty())
            siteLabel()->hide();

        siteLabel()->setText(sitestr);
    }

    //QString score = "Hot" + QString::number(post.mHotScore);
    //score += " Top" + QString::number(post.mTopScore);
    //score += " New" + QString::number(post.mNewScore);

    QString score = QString::number(mPost.mTopScore);

    scoreLabel()->setText(score);

    // FIX THIS UP LATER.
    notes()->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
    pictureLabel()->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

    // feed.
    //frame_comment->show();
    commentButton()->show();

    setCommentsSize(mPost.mComments);

    setReadStatus(IS_MSG_NEW(mPost.mMeta.mMsgStatus), IS_MSG_UNREAD(mPost.mMeta.mMsgStatus) || IS_MSG_NEW(mPost.mMeta.mMsgStatus));

    // disable voting buttons - if they have already voted.
    if (mPost.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
    {
        voteUpButton()->setEnabled(false);
        voteDownButton()->setEnabled(false);
    }

    connect(pictureLabel(), SIGNAL(clicked()), this, SLOT(viewPicture()));

#ifdef TODO
    emit sizeChanged(this);
#endif
}
void BoardPostDisplayWidgetBase::handleShareButtonClicked()
{
    emit shareButtonClicked();
}
//===================================================================================================================================
//==                                                 class BoardPostDisplayWidget                                                  ==
//===================================================================================================================================

BoardPostDisplayWidget_compact::BoardPostDisplayWidget_compact(const RsPostedPost& post, uint8_t display_flags,QWidget *parent=nullptr)
    : BoardPostDisplayWidgetBase(post,display_flags,parent), ui(new Ui::BoardPostDisplayWidget_compact())
{
    ui->setupUi(this);
    setup();

    ui->verticalLayout->addStretch();
    ui->verticalLayout->setAlignment(Qt::AlignTop);
    ui->topLayout->setAlignment(Qt::AlignTop);
    ui->arrowsLayout->addStretch();
    ui->arrowsLayout->setAlignment(Qt::AlignTop);
    ui->verticalLayout_2->addStretch();

    adjustSize();
}

BoardPostDisplayWidget_compact::~BoardPostDisplayWidget_compact()
{
    delete ui;
}

void BoardPostDisplayWidget_compact::setup()
{
    BoardPostDisplayWidgetBase::setup();

    // show/hide things based on the view type

    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->pictureLabel->setEnableZoom(false);

    RsReputationLevel overall_reputation = rsReputations->overallReputationLevel(mPost.mMeta.mAuthorId);
    bool redacted = (overall_reputation == RsReputationLevel::LOCALLY_NEGATIVE);

    if(redacted)
    {
    }
    else
    {
            if(mPost.mImage.mData != NULL)
            {
                QPixmap pixmap;
                GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
                // Wiping data - as its been passed to thumbnail.

#ifdef DEBUG_BOARDPOSTDISPLAYWIDGET
                std::cerr << "Got pixmap of size " << pixmap.width() << " x " << pixmap.height() << std::endl;
                std::cerr << "Saving to pix.png" << std::endl;
                pixmap.save("pix.png","PNG");
#endif

                int desired_height = QFontMetricsF(font()).height() * 5;
                ui->pictureLabel->setFixedSize(16/9.0*desired_height,desired_height);
                ui->pictureLabel->setPicture(pixmap);
            }
            else if (mPost.mImage.mData == NULL)
                ui->pictureLabel->hide();
            else
                ui->pictureLabel->show();

        }

    ui->notes->setText(RsHtml().formatText(NULL, QString::fromUtf8(mPost.mNotes.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

    QObject::connect(ui->expandButton, SIGNAL(toggled(bool)), this, SLOT(doExpand(bool)));

    QTextDocument doc;
    doc.setHtml(notes()->text());

    if(mDisplayFlags & SHOW_NOTES)
    {
        ui->frame_notes->show();
        ui->expandButton->setChecked(true);
    }
    else
    {
        ui->frame_notes->hide();
        ui->expandButton->setChecked(false);
    }

    if(doc.toPlainText().trimmed().isEmpty())
    {
        ui->frame_notes->hide();
        ui->expandButton->hide();
    }
    updateGeometry();

#ifdef TODO
    emit sizeChanged(this);
#endif
}

void BoardPostDisplayWidget_compact::viewPicture()
{
    if(mPost.mImage.mData == NULL)
        return;

    QString timestamp = misc::timeRelativeToNow(mPost.mMeta.mPublishTs);
    QPixmap pixmap;
    GxsIdDetails::loadPixmapFromData(mPost.mImage.mData, mPost.mImage.mSize, pixmap,GxsIdDetails::ORIGINAL);
    RsGxsId authorID = mPost.mMeta.mAuthorId;

    PhotoView *PView = new PhotoView();

    PView->setPixmap(pixmap);
    PView->setTitle(QString::fromUtf8(mPost.mMeta.mMsgName.c_str()));
    PView->setName(authorID);
    PView->setTime(timestamp);
    PView->setGroupId(mPost.mMeta.mGroupId);
    PView->setMessageId(mPost.mMeta.mMsgId);

    PView->show();

    emit thumbnailOpenned();
}

QToolButton    *BoardPostDisplayWidget_compact::voteUpButton()   { return ui->voteUpButton; }
QToolButton    *BoardPostDisplayWidget_compact::commentButton()  { return ui->commentButton; }
QToolButton    *BoardPostDisplayWidget_compact::voteDownButton() { return ui->voteDownButton; }
QLabel         *BoardPostDisplayWidget_compact::newLabel()       { return ui->newLabel; }
QToolButton    *BoardPostDisplayWidget_compact::readButton()     { return ui->readButton; }
QLabel         *BoardPostDisplayWidget_compact::siteLabel()      { return ui->siteLabel; }
GxsIdLabel     *BoardPostDisplayWidget_compact::fromLabel()      { return ui->fromLabel; }
QLabel         *BoardPostDisplayWidget_compact::dateLabel()      { return ui->dateLabel; }
QLabel         *BoardPostDisplayWidget_compact::titleLabel()     { return ui->titleLabel; }
QLabel         *BoardPostDisplayWidget_compact::scoreLabel()     { return ui->scoreLabel; }
QLabel         *BoardPostDisplayWidget_compact::notes()          { return ui->notes; }
QPushButton    *BoardPostDisplayWidget_compact::shareButton()    { return ui->shareButton; }
QLabel         *BoardPostDisplayWidget_compact::pictureLabel()   { return ui->pictureLabel; }

//===================================================================================================================================
//==                                                 class BoardPostDisplayWidget_card                                             ==
//===================================================================================================================================

BoardPostDisplayWidget_card::BoardPostDisplayWidget_card(const RsPostedPost& post, uint8_t display_flags, QWidget *parent)
    : BoardPostDisplayWidgetBase(post,display_flags,parent), ui(new Ui::BoardPostDisplayWidget_card())
{
    ui->setupUi(this);
    setup();

    ui->verticalLayout->addStretch();
    ui->verticalLayout->setAlignment(Qt::AlignTop);
    ui->topLayout->setAlignment(Qt::AlignTop);
    ui->arrowsLayout->addStretch();
    ui->arrowsLayout->setAlignment(Qt::AlignTop);
    ui->verticalLayout_2->addStretch();

    adjustSize();
}

BoardPostDisplayWidget_card::~BoardPostDisplayWidget_card()
{
    delete ui;
}

void BoardPostDisplayWidget_card::setup()
{
    BoardPostDisplayWidgetBase::setup();

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
        pictureLabel()->hide();
    else
        pictureLabel()->show();

    QTextDocument doc;
    doc.setHtml(notes()->text());

    if(doc.toPlainText().trimmed().isEmpty())
        notes()->hide();
}

QToolButton    *BoardPostDisplayWidget_card::voteUpButton()   { return ui->voteUpButton; }
QToolButton    *BoardPostDisplayWidget_card::commentButton()  { return ui->commentButton; }
QToolButton    *BoardPostDisplayWidget_card::voteDownButton() { return ui->voteDownButton; }
QLabel         *BoardPostDisplayWidget_card::newLabel()       { return ui->newLabel; }
QToolButton    *BoardPostDisplayWidget_card::readButton()     { return ui->readButton; }
QLabel         *BoardPostDisplayWidget_card::siteLabel()      { return ui->siteLabel; }
GxsIdLabel     *BoardPostDisplayWidget_card::fromLabel()      { return ui->fromLabel; }
QLabel         *BoardPostDisplayWidget_card::dateLabel()      { return ui->dateLabel; }
QLabel         *BoardPostDisplayWidget_card::titleLabel()     { return ui->titleLabel; }
QLabel         *BoardPostDisplayWidget_card::scoreLabel()     { return ui->scoreLabel; }
QLabel         *BoardPostDisplayWidget_card::notes()          { return ui->notes; }
QPushButton    *BoardPostDisplayWidget_card::shareButton()    { return ui->shareButton; }
QLabel         *BoardPostDisplayWidget_card::pictureLabel()   { return ui->pictureLabel; }

