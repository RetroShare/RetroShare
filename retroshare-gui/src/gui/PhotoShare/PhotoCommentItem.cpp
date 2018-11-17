/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoCommentItem.cpp                      *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
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
