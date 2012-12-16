#include <QDateTime>

#include "PhotoCommentItem.h"
#include "ui_PhotoCommentItem.h"


PhotoCommentItem::PhotoCommentItem(const RsPhotoComment& comment, QWidget *parent):
    QWidget(parent),
    ui(new Ui::PhotoCommentItem), mComment(comment)
{
    ui->setupUi(this);
    setUp();
}

PhotoCommentItem::~PhotoCommentItem()
{
    delete ui;
}

const RsPhotoComment& PhotoCommentItem::getComment()
{
    return mComment;
}

void PhotoCommentItem::setUp()
{
    ui->labelComment->setText(QString::fromUtf8(mComment.mComment.c_str()));
    QDateTime qtime;
    qtime.setTime_t(mComment.mMeta.mPublishTs);
    QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm");
    ui->datetimelabel->setText(timestamp);
}
