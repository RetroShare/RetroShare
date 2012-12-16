#include "AddCommentDialog.h"
#include "ui_AddCommentDialog.h"

AddCommentDialog::AddCommentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddCommentDialog)
{
    ui->setupUi(this);
}

AddCommentDialog::~AddCommentDialog()
{
    delete ui;
}

QString AddCommentDialog::getComment() const
{
    return ui->textEditAddComment->document()->toPlainText();
}
