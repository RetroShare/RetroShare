#include "PostedCreatePostDialog.h"
#include "ui_PostedCreatePostDialog.h"

PostedCreatePostDialog::PostedCreatePostDialog(TokenQueue* tokenQ, RsPosted *posted, QWidget *parent):
    QDialog(parent), mTokenQueue(tokenQ), mPosted(posted),
    ui(new Ui::PostedCreatePostDialog)
{
    ui->setupUi(this);
}

void PostedCreatePostDialog::createPost()
{


    close();
}

PostedCreatePostDialog::~PostedCreatePostDialog()
{
    delete ui;
}
