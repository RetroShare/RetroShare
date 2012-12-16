#include "PostedCreateCommentDialog.h"
#include "ui_PostedCreateCommentDialog.h"

PostedCreateCommentDialog::PostedCreateCommentDialog(TokenQueue *tokQ, const RsGxsGrpMsgIdPair &parentId, const RsGxsMessageId& threadId, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PostedCreateCommentDialog), mTokenQueue(tokQ), mParentId(parentId), mThreadId(threadId)
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(createComment()));
}

void PostedCreateCommentDialog::createComment()
{
    RsPostedComment comment;

    comment.mComment = ui->commentTextEdit->document()->toPlainText().toStdString();
    comment.mMeta.mParentId = mParentId.second;
    comment.mMeta.mGroupId = mParentId.first;
    comment.mMeta.mThreadId = mThreadId;

    uint32_t token;
    rsPosted->submitComment(token, comment);
    mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, 0);
    close();
}

PostedCreateCommentDialog::~PostedCreateCommentDialog()
{
    delete ui;
}
