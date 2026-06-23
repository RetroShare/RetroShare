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
#include "gui/gxs/FlatViewCommentWidget.h"
#include "gui/gxs/CommentItemWidget.h"
#include "gui/gxs/GxsCreateCommentDialog.h"
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
	: QWidget(parent), ui(new Ui::GxsCommentDialog), mUseFlatView(false), mFlatViewWidget(nullptr), mCommentService(nullptr)
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

	connect(ui->viewModeButton, &QToolButton::toggled, this, &GxsCommentDialog::onFlatViewToggled);

	// default sort method "HOT".
	ui->treeWidget->sortByColumn(4, Qt::DescendingOrder);
	
	int S = QFontMetricsF(font()).height() ;
	
	ui->sortBox->setIconSize(QSize(S*1.5,S*1.5));
	ui->commentButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->viewModeButton->setIconSize(QSize(S*1.5,S*1.5));
}

void GxsCommentDialog::setGxsService(RsGxsCommentService *comment_service)
{
	mCommentService = comment_service;
    ui->treeWidget->setup(comment_service);
}

GxsCommentDialog::GxsCommentDialog(QWidget *parent,const RsGxsId &default_author)
	: QWidget(parent), ui(new Ui::GxsCommentDialog), mUseFlatView(false), mFlatViewWidget(nullptr), mCommentService(nullptr)
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
    if (mFlatViewWidget) {
        mFlatViewWidget->clearComments();
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

	if (mUseFlatView) {
		setupFlatViewWidget();
		if (mFlatViewWidget) {
			mFlatViewWidget->loadCommentsForPost(mGrpId, mMsgVersions, mMostRecentMsgId);
		}
	}
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
	Q_UNUSED(index)
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
		if (mFlatViewWidget)
			mFlatViewWidget->setVoterId(voterId);

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
		if (mUseFlatView && mFlatViewWidget) {
			mFlatViewWidget->sortComments(0);
		} else {
			ui->treeWidget->sortByColumn(4, Qt::DescendingOrder);
		}
		break;
	case 1:
		if (mUseFlatView && mFlatViewWidget) {
			mFlatViewWidget->sortComments(1);
		} else {
			ui->treeWidget->sortByColumn(2, Qt::DescendingOrder);
		}
		break;
	case 2:
		if (mUseFlatView && mFlatViewWidget) {
			mFlatViewWidget->sortComments(2);
		} else {
			ui->treeWidget->sortByColumn(3, Qt::DescendingOrder);
		}
		break;
	}
}

void GxsCommentDialog::setupFlatViewWidget()
{
	if (mFlatViewWidget)
		return;

	mFlatViewWidget = new FlatViewCommentWidget(ui->flatViewPage);
	mFlatViewWidget->setCommentService(mCommentService);

	connect(mFlatViewWidget, &FlatViewCommentWidget::commentReply,
	        this, &GxsCommentDialog::onFlatViewCommentReply);

	// Propagate the already-chosen voter ID if one was selected before the widget was created
	{
		RsGxsId voterId;
		GxsIdChooser::ChosenId_Ret ret = ui->idChooser->getChosenId(voterId);
		if (ret == GxsIdChooser::KnowId || ret == GxsIdChooser::UnKnowId)
			mFlatViewWidget->setVoterId(voterId);
	}

	QVBoxLayout *pageLayout = new QVBoxLayout(ui->flatViewPage);
	pageLayout->setContentsMargins(0, 0, 0, 0);
	pageLayout->addWidget(mFlatViewWidget);
}

void GxsCommentDialog::loadFlatViewComments(const std::vector<RsGxsComment> &comments)
{
	if (!mFlatViewWidget)
		setupFlatViewWidget();

	mUseFlatView = true;
	ui->viewModeButton->setChecked(true);
	ui->commentStackedWidget->setCurrentWidget(ui->flatViewPage);

	mFlatViewWidget->clearComments();

	// Collect all comment msgIds to distinguish top-level (parent not a comment) from replies
	std::set<RsGxsMessageId> commentIds;
	for (const auto &c : comments)
		commentIds.insert(c.mMeta.mMsgId);

	// First pass: top-level comments
	for (const auto &comment : comments) {
		if (!commentIds.count(comment.mMeta.mParentId))
			mFlatViewWidget->addComment(comment, RsGxsMessageId());
	}
	// Second pass: replies
	for (const auto &comment : comments) {
		if (commentIds.count(comment.mMeta.mParentId))
			mFlatViewWidget->addComment(comment, comment.mMeta.mParentId);
	}

	mFlatViewWidget->updateReplyCountButtons();
}

void GxsCommentDialog::loadFlatView()
{
	mUseFlatView = true;
	ui->viewModeButton->setChecked(true);
	setupFlatViewWidget();
	ui->commentStackedWidget->setCurrentWidget(ui->flatViewPage);

	if (mFlatViewWidget)
		mFlatViewWidget->loadCommentsForPost(mGrpId, mMsgVersions, mMostRecentMsgId);
}

void GxsCommentDialog::onFlatViewToggled(bool checked)
{
	if (checked) {
		mUseFlatView = true;
		setupFlatViewWidget();
		ui->commentStackedWidget->setCurrentWidget(ui->flatViewPage);
		if (mFlatViewWidget)
			mFlatViewWidget->loadCommentsForPost(mGrpId, mMsgVersions, mMostRecentMsgId);
	} else {
		mUseFlatView = false;
		ui->commentStackedWidget->setCurrentWidget(ui->classicPage);
	}
}

void GxsCommentDialog::onFlatViewCommentReply(const RsGxsMessageId &parentId)
{
	RsGxsId voterId;
	ui->idChooser->getChosenId(voterId);

	GxsCreateCommentDialog dlg(mCommentService,
	    RsGxsGrpMsgIdPair(mGrpId, parentId),
	    mMostRecentMsgId, voterId, this);

	if (mFlatViewWidget) {
		CommentItemWidget *parentWidget = mFlatViewWidget->getCommentWidget(parentId);
		if (parentWidget) {
			dlg.loadComment(parentWidget->getCommentText(), parentWidget->getAuthorName(), parentWidget->getAuthorId());
		}
	}

	dlg.exec();

	// Reload so the new reply appears
	if (mFlatViewWidget)
		mFlatViewWidget->loadCommentsForPost(mGrpId, mMsgVersions, mMostRecentMsgId);
}
