
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
