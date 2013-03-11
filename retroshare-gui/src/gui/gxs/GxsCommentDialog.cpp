/*
 * Retroshare Comment Dialog
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

#include "gui/gxs/GxsCommentDialog.h"

//#include "gxs/GxsCreateComment.h"
//#include <retroshare/rsposted.h>


#include <iostream>
#include <sstream>

#include <QTimer>
#include <QMessageBox>
#include <QDateTime>



/** Constructor */
GxsCommentDialog::GxsCommentDialog(QWidget *parent, RsTokenService *token_service, RsGxsCommentService *comment_service)
:QWidget(parent)
{
    ui.setupUi(this);
    //ui.postFrame->setVisible(false);

    ui.treeWidget->setup(token_service, comment_service);
}

void GxsCommentDialog::commentLoad(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId)
{
        std::cerr << "GxsCommentDialog::commentLoad(" << grpId << ", " << msgId << ")";
	std::cerr << std::endl;

	mGrpId = grpId;
	mMsgId = msgId;

        RsGxsGrpMsgIdPair threadId;

        threadId.first = grpId;
        threadId.second = msgId;

	ui.treeWidget->requestComments(threadId);
}


void GxsCommentDialog::setCommentHeader(GxsCommentHeader *header)
{

#if 0
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
#endif

}

