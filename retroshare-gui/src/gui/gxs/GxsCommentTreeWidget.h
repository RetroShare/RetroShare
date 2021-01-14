/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCommentTreeWidget.h                           *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#ifndef _GXS_COMMENT_TREE_WIDGET_H
#define _GXS_COMMENT_TREE_WIDGET_H

#include <QTreeWidget>
#include <QMutex>

#include "util/TokenQueue.h"
#include <retroshare/rsgxscommon.h>
#include <retroshare/rsidentity.h>

class RSTreeWidgetItemCompareRole;

class GxsCommentTreeWidget : public QTreeWidget, public TokenResponse
{
    Q_OBJECT
        
public:
    GxsCommentTreeWidget(QWidget *parent = 0);
    ~GxsCommentTreeWidget();
    void setup(RsTokenService *token_service, RsGxsCommentService *comment_service);

    void requestComments(const RsGxsGroupId& group, const std::set<RsGxsMessageId> &message_versions, const RsGxsMessageId &most_recent_message);
    void getCurrentMsgId(RsGxsMessageId& parentId);
    void applyRankings(std::map<RsGxsMessageId, uint32_t>& positions);

    void loadRequest(const TokenQueue *queue, const TokenRequest &req);
    void setVoteId(const RsGxsId &voterId);

    void setUseCache(bool b) { mUseCache = b ;}

protected slots:
    void updateContent();

protected:
    void mouseMoveEvent(QMouseEvent *e) override;

    /* to be overloaded */
    virtual void service_requestComments(const RsGxsGroupId &group_id, const std::set<RsGxsMessageId> &msgIds);
    virtual void service_loadThread(const uint32_t &token);

    virtual QTreeWidgetItem *service_createMissingItem(const RsGxsMessageId& parent);

    void clearItems();
    void completeItems();

    void acknowledgeComment(const uint32_t& token);
    void acknowledgeVote(const uint32_t &token);

    void loadThread(const uint32_t &token);

    void insertComments(const std::vector<RsGxsComment>& comments);
    void addItem(RsGxsMessageId itemId, RsGxsMessageId parentId, QTreeWidgetItem *item);
public slots:
    void customPopUpMenu(const QPoint& point);
    void setCurrentCommentMsgId(QTreeWidgetItem* current, QTreeWidgetItem* previous);


    void makeComment();
    void replyToComment();

    void copyComment();

    void voteUp();
    void voteDown();

    void showReputation();
    void markInteresting();
    void markSpammer();
    void banUser();

signals:
    void commentsLoaded(int);

protected:

	void vote(const RsGxsGroupId &groupId, const RsGxsMessageId &threadId,
			const RsGxsMessageId &parentId, const RsGxsId &authorId, bool up);

    /* Data */
    RsGxsGroupId mGroupId;
    std::set<RsGxsMessageId> mMsgVersions;
    RsGxsMessageId mLatestMsgId;
    RsGxsMessageId mCurrentCommentMsgId;
    QString mCurrentCommentText;
	QString mCurrentCommentAuthor;
	RsGxsId mCurrentCommentAuthorId;

    RsGxsId mVoterId;

    std::map<RsGxsMessageId, QTreeWidgetItem *> mLoadingMap;
    std::multimap<RsGxsMessageId, QTreeWidgetItem *> mPendingInsertMap;
	
	RSTreeWidgetItemCompareRole *commentsRole;

    TokenQueue *mTokenQueue;
    RsTokenService *mRsTokenService;
    RsGxsCommentService *mCommentService;

    bool mUseCache;
    static std::map<RsGxsMessageId, std::vector<RsGxsComment> > mCommentsCache;
    static QMutex mCacheMutex;
};


#endif

