
#include "GxsCreateCommentDialog.h"
#include "ui_GxsCreateCommentDialog.h"

GxsCreateCommentDialog::GxsCreateCommentDialog(TokenQueue *tokQ, RsGxsCommentService *service, 
			const RsGxsGrpMsgIdPair &parentId, const RsGxsMessageId& threadId, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::GxsCreateCommentDialog), mTokenQueue(tokQ), mCommentService(service), mParentId(parentId), mThreadId(threadId)
{
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(createComment()));
}

void GxsCreateCommentDialog::createComment()
{
	RsGxsComment comment;

	comment.mComment = ui->commentTextEdit->document()->toPlainText().toStdString();
	comment.mMeta.mParentId = mParentId.second;
	comment.mMeta.mGroupId = mParentId.first;
	comment.mMeta.mThreadId = mThreadId;

	uint32_t token;
	mCommentService->createComment(token, comment);
	mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
	close();
}

GxsCreateCommentDialog::~GxsCreateCommentDialog()
{
	delete ui;
}
