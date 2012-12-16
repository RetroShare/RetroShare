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

    setContent(mPost);

    connect( commentButton, SIGNAL( clicked() ), this, SLOT( loadComments() ) );
    connect( voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
    connect( voteDownButton, SIGNAL(clicked()), this, SLOT( makeDownVote()));

    return;
}

void PostedItem::setContent(const RsPostedPost &post)
{
    mPost = post;
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

    uint32_t up, down, nComments;

    bool ok = rsPosted->retrieveScores(mPost.mMeta.mServiceString, up, down, nComments);

    if(ok)
    {
        int32_t vote = up - down;
        scoreLabel->setText(QString::number(vote));

        numCommentsLabel->setText("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px;"
                                  "margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span"
                                  "style=\" font-size:10pt; font-weight:600;\">#</span><span "
                                  "style=\" font-size:8pt; font-weight:600;\"> Comments:  "
                                  + QString::number(nComments) + "</span></p>");
    }

}

RsPostedPost PostedItem::getPost() const
{
    return mPost;
}

void PostedItem::makeDownVote()
{
    RsGxsGrpMsgIdPair msgId;
    msgId.first = mPost.mMeta.mGroupId;
    msgId.second = mPost.mMeta.mMsgId;
    emit vote(msgId, false);
}

void PostedItem::makeUpVote()
{
    RsGxsGrpMsgIdPair msgId;
    msgId.first = mPost.mMeta.mGroupId;
    msgId.second = mPost.mMeta.mMsgId;
    emit vote(msgId, true);
}

void PostedItem::loadComments()
{
    std::cerr << "PostedItem::loadComments() Requesting for " << mThreadId;
    std::cerr << std::endl;
    mPostHolder->showComments(mPost);
}
