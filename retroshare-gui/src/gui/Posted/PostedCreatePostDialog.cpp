#include "PostedCreatePostDialog.h"
#include "ui_PostedCreatePostDialog.h"

PostedCreatePostDialog::PostedCreatePostDialog(TokenQueue* tokenQ, RsPosted *posted, const RsGxsGroupId& grpId, QWidget *parent):
    QDialog(parent), mTokenQueue(tokenQ), mPosted(posted), mGrpId(grpId),
    ui(new Ui::PostedCreatePostDialog)
{
    ui->setupUi(this);
    connect(this, SIGNAL(accepted()), this, SLOT(createPost()));
}

void PostedCreatePostDialog::createPost()
{
    RsPostedPost post;
    post.mMeta.mGroupId = mGrpId;
    post.mLink = ui->linkEdit->text().toStdString();
    post.mNotes = ui->notesTextEdit->toPlainText().toStdString();
    post.mMeta.mMsgName = ui->titleEdit->text().toStdString();

    uint32_t token;
    mPosted->createPost(token, post);
    mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, TOKEN_USER_TYPE_POST);
    close();
}

PostedCreatePostDialog::~PostedCreatePostDialog()
{
    delete ui;
}
