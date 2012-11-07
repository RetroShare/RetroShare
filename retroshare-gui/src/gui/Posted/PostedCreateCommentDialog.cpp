#include "PostedCreateCommentDialog.h"
#include "ui_PostedCreateCommentDialog.h"

PostedCreateCommentDialog::PostedCreateCommentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PostedCreateCommentDialog)
{
    ui->setupUi(this);
}

PostedCreateCommentDialog::~PostedCreateCommentDialog()
{
    delete ui;
}
