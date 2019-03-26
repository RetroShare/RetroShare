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
#include "ui_GxsCommentDialog.h"

#include <iostream>
#include <sstream>

#include <QTimer>
#include <QMessageBox>
#include <QDateTime>

/** Constructor */
GxsCommentDialog::GxsCommentDialog(QWidget *parent, RsTokenService *token_service, RsGxsCommentService *comment_service)
	: QWidget(parent), ui(new Ui::GxsCommentDialog)
{
	/* Invoke the Qt Designer generated QObject setup routine */
	ui->setupUi(this);

	//ui->postFrame->setVisible(false);

	ui->treeWidget->setup(token_service, comment_service);
	
	/* Set header resize modes and initial section sizes */
	QHeaderView * ttheader = ui->treeWidget->header () ;
	ttheader->resizeSection (0, 440);

	/* fill in the available OwnIds for signing */
	ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, RsGxsId());

	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
	connect(ui->idChooser, SIGNAL(currentIndexChanged( int )), this, SLOT(voterSelectionChanged( int )));
    connect(ui->idChooser, SIGNAL(idsLoaded()), this, SLOT(idChooserReady()));
	
	connect(ui->sortBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sortComments(int)));
	
	// default sort method "HOT".
	ui->treeWidget->sortByColumn(4, Qt::DescendingOrder);
	
	int S = QFontMetricsF(font()).height() ;
	
	ui->sortBox->setIconSize(QSize(S*1.5,S*1.5));
}

GxsCommentDialog::~GxsCommentDialog()
{
	delete(ui);
}

void GxsCommentDialog::commentLoad(const RsGxsGroupId &grpId, const std::set<RsGxsMessageId>& msg_versions,const RsGxsMessageId& most_recent_msgId)
{
	std::cerr << "GxsCommentDialog::commentLoad(" << grpId << ", most recent msg version: " << most_recent_msgId << ")";
	std::cerr << std::endl;

	mGrpId = grpId;
	mMostRecentMsgId = most_recent_msgId;
    mMsgVersions = msg_versions;

	ui->treeWidget->requestComments(mGrpId,msg_versions,most_recent_msgId);
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
	std::cerr << "GxsCommentDialog::voterSelectionChanged(" << index << ")";
	std::cerr << std::endl;

	RsGxsId voterId; 
	switch (ui->idChooser->getChosenId(voterId)) {
		case GxsIdChooser::KnowId:
		case GxsIdChooser::UnKnowId:
		std::cerr << "GxsCommentDialog::voterSelectionChanged() => " << voterId;
		std::cerr << std::endl;
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

	std::cerr << "GxsCommentDialog::setCommentHeader() Adding header to ui,postFrame";
	std::cerr << std::endl;

	//header->setParent(ui->postFrame);
	//ui->postFrame->setVisible(true);

	QLayout *alayout = ui->postFrame->layout();
	alayout->addWidget(header);

#if 0
	ui->postFrame->setVisible(true);

	QDateTime qtime;
	qtime.setTime_t(mCurrentPost.mMeta.mPublishTs);
	QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm");
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
		ui->treeWidget->sortByColumn(4, Qt::DescendingOrder); 
		break;
	case 1:
		ui->treeWidget->sortByColumn(2, Qt::DescendingOrder); 
		break;
	case 2:
		ui->treeWidget->sortByColumn(3, Qt::DescendingOrder); 
		break;
	}

}
