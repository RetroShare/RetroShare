/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCreateCommentDialog.cpp                       *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#include "GxsCreateCommentDialog.h"
#include "ui_GxsCreateCommentDialog.h"
#include "util/HandleRichText.h"

#include <QPushButton>
#include <QMessageBox>
#include <iostream>

GxsCreateCommentDialog::GxsCreateCommentDialog(RsGxsCommentService *service,  const RsGxsGrpMsgIdPair &parentId, const RsGxsMessageId& threadId, const RsGxsId& default_author,QWidget *parent) :
	QDialog(parent),
	ui(new Ui::GxsCreateCommentDialog), mCommentService(service), mParentId(parentId), mThreadId(threadId)
{
	ui->setupUi(this);
	connect(ui->postButton, SIGNAL(clicked()), this, SLOT(createComment()));
	connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->commentTextEdit, SIGNAL(textChanged()), this, SLOT(checkLength()));

	/* fill in the available OwnIds for signing */
	ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, default_author);
}

void GxsCreateCommentDialog::loadComment(const QString &msgText, const QString &msgAuthor, const RsGxsId &msgAuthorId)
{

	setWindowTitle(tr("Reply to Comment") );
	ui->titleLabel->setId(msgAuthorId);
	ui->commentLabel->setText(msgText);

	ui->avatarLabel->setGxsId(msgAuthorId);
	ui->avatarLabel->setFrameType(AvatarWidget::NO_FRAME);

	ui->replyToLabel->setId(msgAuthorId);
	ui->replyToLabel->setText( tr("Replying to") + " @" + msgAuthor);
	
	ui->commentTextEdit->setPlaceholderText( tr("Type your reply"));
	ui->postButton->setText("Reply");
	ui->signedLabel->setText("Reply as");
}

void GxsCreateCommentDialog::createComment()
{
	RsGxsComment comment;

    QString text = ui->commentTextEdit->toPlainText();
    // RsHtml::optimizeHtml(text);
	std::string msg = text.toUtf8().constData();

	comment.mComment = msg;
	comment.mMeta.mParentId = mParentId.second;
	comment.mMeta.mGroupId = mParentId.first;
	comment.mMeta.mThreadId = mThreadId;

	std::cerr << "GxsCreateCommentDialog::createComment()";
	std::cerr << std::endl;

	std::cerr << "GroupId : " << comment.mMeta.mGroupId << std::endl;
	std::cerr << "ThreadId : " << comment.mMeta.mThreadId << std::endl;
	std::cerr << "ParentId : " << comment.mMeta.mParentId << std::endl;

	RsGxsId authorId;
	switch (ui->idChooser->getChosenId(authorId))
    {
		case GxsIdChooser::KnowId:
		case GxsIdChooser::UnKnowId:
		comment.mMeta.mAuthorId = authorId;
		std::cerr << "AuthorId : " << comment.mMeta.mAuthorId << std::endl;
		std::cerr << std::endl;

		break;
		case GxsIdChooser::NoId:
		case GxsIdChooser::None:
		default:
		std::cerr << "GxsCreateCommentDialog::createComment() ERROR GETTING AuthorId!";
		std::cerr << std::endl;

		QMessageBox::information(this, tr("Comment Signing Error"), tr("You need to create an Identity\n" "before you can comment"), QMessageBox::Ok);
		return;
	}

	mCommentService->createComment(comment);
	close();
}

GxsCreateCommentDialog::~GxsCreateCommentDialog()
{
	delete ui;
}

void GxsCreateCommentDialog::checkLength(){
	QString text;
	RsHtml::optimizeHtml(ui->commentTextEdit, text);
	std::wstring msg = text.toStdWString();
    int charRemains = MAX_ALLOWED_GXS_MESSAGE_SIZE * 0.9 - msg.length();	// factor 0.9 safely allows headers, crypto, etc.
	if(charRemains >= 0) {
		text = tr("It remains %1 characters after HTML conversion.").arg(charRemains);
		ui->info_Label->setStyleSheet("QLabel#info_Label { }");
	}else{
		text = tr("Warning: This message is too big of %1 characters after HTML conversion.").arg((0-charRemains));
		ui->info_Label->setStyleSheet("QLabel#info_Label {color: red; font: bold; }");
	}
	ui->postButton->setEnabled(charRemains>=0);
	ui->info_Label->setText(text);
}
