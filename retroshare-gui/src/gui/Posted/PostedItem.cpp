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

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
PostedItem::PostedItem(PostedHolder *parent, const RsPostedPost &post)
:QWidget(NULL)
{
	setupUi(this);
	setAttribute ( Qt::WA_DeleteOnClose, true );

	titleLabel->setText(QString::fromUtf8(post.mMeta.mMsgName.c_str()));
	//dateLabel->setText(QString("Whenever"));
	fromLabel->setText(QString::fromUtf8(post.mMeta.mAuthorId.c_str()));
	//siteLabel->setText(QString::fromUtf8(post.mMeta.mAuthorId.c_str()));
	//scoreLabel->setText(QString("1140"));

        // exposed for testing...
        float score = 0;
	time_t now = time(NULL);

	QString fromLabelTxt = QString(" Age: ") + QString::number(now - post.mMeta.mPublishTs);
	fromLabelTxt += QString(" Score: ") + QString::number(score);
	fromLabel->setText(fromLabelTxt);

	uint32_t votes = 0;
	uint32_t comments = 0;
        //rsPosted->extractPostedCache(post.mMeta.mServiceString, votes, comments);
	scoreLabel->setText(QString::number(votes));
	QString commentLabel = QString("Comments: ") + QString::number(comments);
	commentLabel += QString(" Votes: ") + QString::number(votes);
	siteLabel->setText(commentLabel);
	
	QDateTime ts;
	ts.setTime_t(post.mMeta.mPublishTs);
	dateLabel->setText(ts.toString(QString("yyyy/MM/dd hh:mm:ss")));

	mThreadId = post.mMeta.mThreadId;
	mParent = parent;

	connect( commentButton, SIGNAL( clicked() ), this, SLOT( loadComments() ) );

	return;
}


void PostedItem::loadComments()
{
	std::cerr << "PostedItem::loadComments() Requesting for " << mThreadId;
	std::cerr << std::endl;
	mParent->requestComments(mThreadId);
}
