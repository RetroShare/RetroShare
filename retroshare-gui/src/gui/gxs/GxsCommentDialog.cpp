/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCommentDialog.cpp                             *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#include "gui/gxs/GxsCommentDialog.h"
#include "gui/gxs/GxsCommentTreeWidget.h"
#include "gui/gxs/YouTubeStyleCommentWidget.h"
#include "ui_GxsCommentDialog.h"

#include <iostream>
#include <sstream>

#include <QTimer>
#include <QMessageBox>
#include <QDateTime>
#include <QToolButton>
#include <QVBoxLayout>

//#define DEBUG_COMMENT_DIALOG 1

/** Constructor */
GxsCommentDialog::GxsCommentDialog(QWidget *parent, const RsGxsId &default_author, RsGxsCommentService *comment_service)
	: QWidget(parent), ui(new Ui::GxsCommentDialog), mUseYouTubeStyle(false), mYouTubeStyleWidget(nullptr), mCommentService(nullptr)
{
	/* Invoke the Qt Designer generated QObject setup routine */
	ui->setupUi(this);

    setGxsService(comment_service);
    init(default_author);
}
	
void GxsCommentDialog::init(const RsGxsId& default_author)
{
    ui->refreshButton->hide();	// this is not needed anymore. Let's keep this piece of code for some time just in case.

    /* Set header resize modes and initial section sizes */
	QHeaderView * ttheader = ui->treeWidget->header () ;
	ttheader->resizeSection (0, 440);

	/* fill in the available OwnIds for signing */
    ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, default_author);

	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
	connect(ui->idChooser, SIGNAL(currentIndexChanged( int )), this, SLOT(voterSelectionChanged( int )));
    connect(ui->idChooser, SIGNAL(idsLoaded()), this, SLOT(idChooserReady()));
    connect(ui->treeWidget,SIGNAL(commentsLoaded(int)),this,SLOT(notifyCommentsLoaded(int)));
	
	connect(ui->commentButton, SIGNAL(clicked()), ui->treeWidget, SLOT(makeComment()));
	connect(ui->sortBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sortComments(int)));
	connect(ui->youTubeStyleButton, &QToolButton::toggled, this, &GxsCommentDialog::onYouTubeStyleToggled);

	// default sort method "HOT".
	ui->treeWidget->sortByColumn(4, Qt::DescendingOrder);
	
	int S = QFontMetricsF(font()).height() ;
	
	ui->sortBox->setIconSize(QSize(S*1.5,S*1.5));
}

void GxsCommentDialog::setGxsService(RsGxsCommentService *comment_service)
{
	mCommentService = comment_service;
    ui->treeWidget->setup(comment_service);
}

GxsCommentDialog::GxsCommentDialog(QWidget *parent,const RsGxsId &default_author)
	: QWidget(parent), ui(new Ui::GxsCommentDialog), mUseYouTubeStyle(false), mYouTubeStyleWidget(nullptr), mCommentService(nullptr)
{
	/* Invoke the Qt Designer generated QObject setup routine */
	ui->setupUi(this);

    init(default_author);
}

GxsCommentDialog::~GxsCommentDialog()
{
	delete(ui);
}

void GxsCommentDialog::commentClear()
{
    ui->treeWidget->clear();
    if (mYouTubeStyleWidget) {
        mYouTubeStyleWidget->clearComments();
    }
    mGrpId.clear();
    mMostRecentMsgId.clear();
    mMsgVersions.clear();
}
void GxsCommentDialog::commentLoad(const RsGxsGroupId &grpId, const std::set<RsGxsMessageId>& msg_versions,const RsGxsMessageId& most_recent_msgId,bool use_cache)
{
#ifdef DEBUG_COMMENT_DIALOG
    std::cerr << "GxsCommentDialog::commentLoad(" << grpId << ", most recent msg version: " << most_recent_msgId << ")" << std::endl;
    for(const auto& mid:msg_versions)
        std::cerr << "  msg version: " << mid << std::endl;
#endif

	mGrpId = grpId;
	mMostRecentMsgId = most_recent_msgId;
    mMsgVersions = msg_versions;

    ui->treeWidget->setUseCache(use_cache);
    ui->treeWidget->requestComments(mGrpId,msg_versions,most_recent_msgId);
}

void GxsCommentDialog::notifyCommentsLoaded(int n)
{
    emit commentsLoaded(n);
}

void GxsCommentDialog::refresh()
{
	std::cerr << "GxsCommentDialog::refresh()";
	std::cerr << std::endl;

	commentLoad(mGrpId, mMsgVersions,mMostRecentMsgId);
}

void GxsCommentDialog::idChooserReady()
{
    voterSelectionChanged(0);
}

void GxsCommentDialog::voterSelectionChanged( int index )
{
#ifdef DEBUG_COMMENT_DIALOG
	std::cerr << "GxsCommentDialog::voterSelectionChanged(" << index << ")";
	std::cerr << std::endl;
#endif

	RsGxsId voterId; 
	switch (ui->idChooser->getChosenId(voterId)) {
		case GxsIdChooser::KnowId:
		case GxsIdChooser::UnKnowId:
#ifdef DEBUG_COMMENT_DIALOG
        std::cerr << "GxsCommentDialog::voterSelectionChanged() => " << voterId;
		std::cerr << std::endl;
#endif
		ui->treeWidget->setVoteId(voterId);

		break;
		case GxsIdChooser::NoId:
		case GxsIdChooser::None:
		default:
		std::cerr << "GxsCommentDialog::voterSelectionChanged() ERROR nothing selected";
		std::cerr << std::endl;
	}//switch (ui->idChooser->getChosenId(voterId))
}

void GxsCommentDialog::setCommentHeader(QWidget *header)
{
	if (!header)
	{
		std::cerr << "GxsCommentDialog::setCommentHeader() NULL header!";
		std::cerr << std::endl;
		return;
	}

#ifdef DEBUG_COMMENT_DIALOG
    std::cerr << "GxsCommentDialog::setCommentHeader() Adding header to ui,postFrame";
	std::cerr << std::endl;
#endif

	//header->setParent(ui->postFrame);
	//ui->postFrame->setVisible(true);

#if 0
    QLayout *alayout = ui->postFrame->layout();
	alayout->addWidget(header);

	ui->postFrame->setVisible(true);

	QDateTime qtime;
	qtime.setTime_t(mCurrentPost.mMeta.mPublishTs);
	QString timestamp = DateTime::formatDateTime(qtime);
	ui->dateLabel->setText(timestamp);
	ui->fromLabel->setText(QString::fromUtf8(mCurrentPost.mMeta.mAuthorId.c_str()));
	ui->titleLabel->setText("<a href=" + QString::fromStdString(mCurrentPost.mLink) +
					   "><span style=\" text-decoration: underline; color:#0000ff;\">" +
					   QString::fromStdString(mCurrentPost.mMeta.mMsgName) + "</span></a>");
	ui->siteLabel->setText("<a href=" + QString::fromStdString(mCurrentPost.mLink) +
					   "><span style=\" text-decoration: underline; color:#0000ff;\">" +
					   QString::fromStdString(mCurrentPost.mLink) + "</span></a>");

	ui->scoreLabel->setText(QString("0"));

	ui->notesBrowser->setPlainText(QString::fromStdString(mCurrentPost.mNotes));
#endif
}

void GxsCommentDialog::sortComments(int i)
{
	switch(i)
	{
	default:
	case 0:
		if (mUseYouTubeStyle && mYouTubeStyleWidget) {
			mYouTubeStyleWidget->sortComments(0);
		} else {
			ui->treeWidget->sortByColumn(4, Qt::DescendingOrder);
		}
		break;
	case 1:
		if (mUseYouTubeStyle && mYouTubeStyleWidget) {
			mYouTubeStyleWidget->sortComments(1);
		} else {
			ui->treeWidget->sortByColumn(2, Qt::DescendingOrder);
		}
		break;
	case 2:
		if (mUseYouTubeStyle && mYouTubeStyleWidget) {
			mYouTubeStyleWidget->sortComments(2);
		} else {
			ui->treeWidget->sortByColumn(3, Qt::DescendingOrder);
		}
		break;
	}
}

void GxsCommentDialog::setupYouTubeStyleWidget()
{
	if (mYouTubeStyleWidget)
		return;

	mYouTubeStyleWidget = new YouTubeStyleCommentWidget(ui->youTubePage);
	mYouTubeStyleWidget->setCommentService(mCommentService);

	QVBoxLayout *pageLayout = new QVBoxLayout(ui->youTubePage);
	pageLayout->setContentsMargins(0, 0, 0, 0);
	pageLayout->addWidget(mYouTubeStyleWidget);
}

void GxsCommentDialog::loadYouTubeStyleComments(const std::vector<RsGxsComment> &comments)
{
	if (!mYouTubeStyleWidget)
		setupYouTubeStyleWidget();

	mUseYouTubeStyle = true;
	ui->youTubeStyleButton->setChecked(true);
	ui->commentStackedWidget->setCurrentWidget(ui->youTubePage);

	mYouTubeStyleWidget->clearComments();
	for (const auto &comment : comments)
		mYouTubeStyleWidget->addComment(comment, comment.mMeta.mParentId);
}

void GxsCommentDialog::loadYouTubeStyle()
{
	mUseYouTubeStyle = true;
	ui->youTubeStyleButton->setChecked(true);
	setupYouTubeStyleWidget();
	ui->commentStackedWidget->setCurrentWidget(ui->youTubePage);

	if (mYouTubeStyleWidget)
		mYouTubeStyleWidget->loadCommentsForPost(mGrpId, mMsgVersions, mMostRecentMsgId);
}

void GxsCommentDialog::onYouTubeStyleToggled(bool checked)
{
	if (checked) {
		mUseYouTubeStyle = true;
		setupYouTubeStyleWidget();
		ui->commentStackedWidget->setCurrentWidget(ui->youTubePage);
		if (mYouTubeStyleWidget)
			mYouTubeStyleWidget->loadCommentsForPost(mGrpId, mMsgVersions, mMostRecentMsgId);
	} else {
		mUseYouTubeStyle = false;
		ui->commentStackedWidget->setCurrentWidget(ui->classicPage);
	}
}
