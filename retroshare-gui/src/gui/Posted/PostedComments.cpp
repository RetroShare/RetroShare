/*
 * Retroshare Posted Comments
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

#include "PostedComments.h"
#include "PostedCreateCommentDialog.h"

#include <retroshare/rsposted.h>

#include <iostream>
#include <sstream>

#include <QTimer>
#include <QMessageBox>
#include <QDateTime>



/****************************************************************
 * New Photo Display Widget.
 *
 * This has two 'lists'.
 * Top list shows Albums.
 * Lower list is photos from the selected Album.
 * 
 * Notes:
 *   Each Item will be an AlbumItem, which contains a thumbnail & random details.
 *   We will limit Items to < 100. With a 'Filter to see more message.
 * 
 *   Thumbnails will come from Service.
 *   Option to Share albums / pictures onward (if permissions allow).
 *   Option to Download the albums to a specified directory. (is this required if sharing an album?)
 *
 *   Will introduce a FullScreen SlideShow later... first get basics happening.
 */




/** Constructor */
PostedComments::PostedComments(QWidget *parent)
:QWidget(parent)
{
    ui.setupUi(this);
    ui.postFrame->setVisible(false);
    ui.treeWidget->setup(rsPosted->getTokenService());
}

void PostedComments::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{

}

void PostedComments::loadComments(const RsPostedPost& post)
{
        std::cerr << "PostedComments::loadComments(" << post.mMeta.mOrigMsgId << ")";
	std::cerr << std::endl;

        mCurrentPost = post;
        setUpPostFrame();

        RsGxsGrpMsgIdPair threadId;

        threadId.first = post.mMeta.mGroupId;
        threadId.second = post.mMeta.mOrigMsgId;

	ui.treeWidget->requestComments(threadId);
}


void PostedComments::setUpPostFrame()
{
    ui.postFrame->setVisible(true);

    QDateTime qtime;
    qtime.setTime_t(mCurrentPost.mMeta.mPublishTs);
    QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm");
    ui.dateLabel->setText(timestamp);
    ui.fromLabel->setText(QString::fromUtf8(mCurrentPost.mMeta.mAuthorId.c_str()));
    ui.titleLabel->setText("<a href=" + QString::fromStdString(mCurrentPost.mLink) +
                       "><span style=\" text-decoration: underline; color:#0000ff;\">" +
                       QString::fromStdString(mCurrentPost.mMeta.mMsgName) + "</span></a>");
    ui.siteLabel->setText("<a href=" + QString::fromStdString(mCurrentPost.mLink) +
                       "><span style=\" text-decoration: underline; color:#0000ff;\">" +
                       QString::fromStdString(mCurrentPost.mLink) + "</span></a>");

    ui.scoreLabel->setText(QString("0"));

    ui.notesBrowser->setPlainText(QString::fromStdString(mCurrentPost.mNotes));

}

