/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include "GxsCreateCommentDialog.h"
#include "ui_GxsCreateCommentDialog.h"

#include <QMessageBox>
#include <iostream>

GxsCreateCommentDialog::GxsCreateCommentDialog(TokenQueue *tokQ, RsGxsCommentService *service, 
			const RsGxsGrpMsgIdPair &parentId, const RsGxsMessageId& threadId, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::GxsCreateCommentDialog), mTokenQueue(tokQ), mCommentService(service), mParentId(parentId), mThreadId(threadId)
{
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(createComment()));

	/* fill in the available OwnIds for signing */
	ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, "");
}

void GxsCreateCommentDialog::createComment()
{
	RsGxsComment comment;

	comment.mComment = std::string(ui->commentTextEdit->document()->toPlainText().toUtf8());
	comment.mMeta.mParentId = mParentId.second;
	comment.mMeta.mGroupId = mParentId.first;
	comment.mMeta.mThreadId = mThreadId;

	std::cerr << "GxsCreateCommentDialog::createComment()";
	std::cerr << std::endl;

	std::cerr << "GroupId : " << comment.mMeta.mGroupId << std::endl;
	std::cerr << "ThreadId : " << comment.mMeta.mThreadId << std::endl;
	std::cerr << "ParentId : " << comment.mMeta.mParentId << std::endl;


	RsGxsId authorId;
	if (ui->idChooser->getChosenId(authorId))
	{
		comment.mMeta.mAuthorId = authorId;
		std::cerr << "AuthorId : " << comment.mMeta.mAuthorId << std::endl;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "GxsCreateCommentDialog::createComment() ERROR GETTING AuthorId!";
		std::cerr << std::endl;

		int ret = QMessageBox::information(this, tr("Comment Signing Error"),
					   tr("You need to create an Identity\n"
						"before you can comment"),
					   QMessageBox::Ok);
		return;
	}

	uint32_t token;
	mCommentService->createComment(token, comment);
	mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
	close();
}

GxsCreateCommentDialog::~GxsCreateCommentDialog()
{
	delete ui;
}
