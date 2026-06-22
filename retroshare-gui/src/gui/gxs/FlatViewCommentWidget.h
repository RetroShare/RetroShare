/*******************************************************************************
 * retroshare-gui/src/gui/gxs/FlatViewCommentWidget.h                          *
 *                                                                             *
 * Copyright 206 by RetroShare Team   <retroshare.project@gmail.com>           *
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

#ifndef FLAT_VIEW_COMMENT_WIDGET_H
#define FLAT_VIEW_COMMENT_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>

#include <set>

#include <retroshare/rsgxscommon.h>

namespace Ui {
class FlatViewCommentWidget;
}

class CommentItemWidget;

class FlatViewCommentWidget : public QWidget
{
	Q_OBJECT

public:
	explicit FlatViewCommentWidget(QWidget *parent = nullptr);
	~FlatViewCommentWidget();

	// Add a comment at top level or as a reply to a parent
	void addComment(const RsGxsComment &comment, const RsGxsMessageId &parentId = RsGxsMessageId());
	void clearComments();

	// Set the comment service for loading comments
	void setCommentService(RsGxsCommentService *service);
	void setVoterId(const RsGxsId &id);
	void updateReplyCountButtons();
	CommentItemWidget *getCommentWidget(const RsGxsMessageId &msgId) const;

 signals:
	void commentUpvote(const RsGxsGrpMsgIdPair &msgId, bool up);
	void commentDownvote(const RsGxsGrpMsgIdPair &msgId, bool down);
	void commentReply(const RsGxsMessageId &parentId);

public slots:
	void loadCommentsForPost(const RsGxsGroupId &groupId, const std::set<RsGxsMessageId> &msgVersions, const RsGxsMessageId &mostRecentMsgId);
	void sortComments(int sortMethod);

private slots:
	void onViewRepliesToggled(const RsGxsMessageId &msgId, bool show);
	void onCommentSelected(const RsGxsMessageId &msgId);

private:
	void doVote(const RsGxsMessageId &commentMsgId, bool up);

	Ui::FlatViewCommentWidget *ui;

	QVBoxLayout *mCommentsLayout;
	QMap<RsGxsMessageId, CommentItemWidget *> mCommentWidgets;
	QMap<RsGxsMessageId, QList<RsGxsMessageId> > mRepliesMap;
	QMap<RsGxsMessageId, double> mScoreMap;
	QMap<RsGxsMessageId, rstime_t> mTimestampMap;

	RsGxsCommentService *mCommentService;
	RsGxsGroupId mCurrentGroupId;
	RsGxsMessageId mCurrentPostId;
	RsGxsMessageId mLatestMsgId;
	std::set<RsGxsMessageId> mMsgVersions;
	RsGxsId mVoterId;
	CommentItemWidget *mSelectedWidget;
};

#endif // FLAT_VIEW_COMMENT_WIDGET_H
