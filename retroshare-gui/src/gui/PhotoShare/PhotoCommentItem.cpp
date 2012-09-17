#include "PhotoCommentItem.h"
#include "ui_PhotoCommentItem.h"

PhotoCommentItem::PhotoCommentItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PhotoCommentItem)
{
    ui->setupUi(this);
}

PhotoCommentItem::~PhotoCommentItem()
{
    delete ui;
}
