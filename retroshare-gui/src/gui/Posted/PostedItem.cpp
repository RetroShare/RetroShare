/*
 * Retroshare Posted Plugin.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include <QDateTime>
#include <QMessageBox>
#include <QMouseEvent>
#include <QBuffer>
#include <time.h>

#include "PostedItem.h"

#include <retroshare/rsposted.h>

#include <algorithm>
#include <iostream>


/** Constructor */
PostedItem::PostedItem(PostedHolder *postHolder, const RsPostedPost &post)
:QWidget(NULL), mPostHolder(postHolder), mPost(post)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );

        QDateTime qtime;
        qtime.setTime_t(mPost.mMeta.mPublishTs);
        QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm");
        dateLabel->setText(timestamp);
	fromLabel->setText(QString::fromUtf8(post.mMeta.mAuthorId.c_str()));
        titleLabel->setText("<a href=" + QString::fromStdString(post.mLink) +
                           "><span style=\" text-decoration: underline; color:#0000ff;\">" +
                           QString::fromStdString(post.mMeta.mMsgName) + "</span></a>");
        siteLabel->setText("<a href=" + QString::fromStdString(post.mLink) +
                           "><span style=\" text-decoration: underline; color:#0000ff;\">" +
                           QString::fromStdString(post.mLink) + "</span></a>");

        scoreLabel->setText(QString("1"));

	connect( commentButton, SIGNAL( clicked() ), this, SLOT( loadComments() ) );

	return;
}

RsPostedPost PostedItem::getPost() const
{
    return mPost;
}

void PostedItem::loadComments()
{
	std::cerr << "PostedItem::loadComments() Requesting for " << mThreadId;
	std::cerr << std::endl;
        mPostHolder->showComments(mPost);
}
