/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
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

#ifndef _GXS_COMMENT_TREE_WIDGET_H
#define _GXS_COMMENT_TREE_WIDGET_H

#include <QTreeWidget>

#include "util/TokenQueue.h"
#include <retroshare/rsgxscommon.h>
#include <retroshare/rsidentity.h>

class GxsCommentTreeWidget : public QTreeWidget, public TokenResponse
{
    Q_OBJECT
        
public:
    GxsCommentTreeWidget(QWidget *parent = 0);
    void setup(RsTokenService *token_service, RsGxsCommentService *comment_service);

    void requestComments(const RsGxsGrpMsgIdPair& threadId);
    void getCurrentMsgId(RsGxsMessageId& parentId);
    void applyRankings(std::map<RsGxsMessageId, uint32_t>& positions);

    void loadRequest(const TokenQueue *queue, const TokenRequest &req);
    void setVoteId(const RsGxsId &voterId);

protected:

    /* to be overloaded */
    virtual void service_requestComments(const RsGxsGrpMsgIdPair& threadId);
    virtual void service_loadThread(const uint32_t &token);
    virtual QTreeWidgetItem *service_createMissingItem(const RsGxsMessageId& parent);

    void clearItems();
    void completeItems();

    void acknowledgeComment(const uint32_t& token);
    void acknowledgeVote(const uint32_t &token);

    void loadThread(const uint32_t &token);

    void addItem(std::string itemId, std::string parentId, QTreeWidgetItem *item);

public slots:
    void customPopUpMenu(const QPoint& point);
    void setCurrentMsgId(QTreeWidgetItem* current, QTreeWidgetItem* previous);


    void makeComment();
    void replyToComment();

    void voteUp();
    void voteDown();

    void showReputation();
    void markInteresting();
    void markSpammer();
    void banUser();

protected:

	void vote(const RsGxsGroupId &groupId, const RsGxsMessageId &threadId,
			const RsGxsMessageId &parentId, const RsGxsId &authorId, bool up);

    /* Data */
    RsGxsGrpMsgIdPair mThreadId;
    RsGxsMessageId mCurrentMsgId;
    RsGxsId mVoterId;

    std::map<std::string, QTreeWidgetItem *> mLoadingMap;
    std::multimap<std::string, QTreeWidgetItem *> mPendingInsertMap;

    TokenQueue *mTokenQueue;
    RsTokenService *mRsTokenService;
    RsGxsCommentService *mCommentService;

};


#endif

