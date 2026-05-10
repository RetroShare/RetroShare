/*******************************************************************************
 * retroshare-gui/src/gui/gxs/YouTubeStyleCommentWidget.h                       *
 *                                                                             *
 * YouTube-style comment viewer for issue #1722                                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                            *
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

#ifndef YOUTUBE_STYLE_COMMENT_WIDGET_H
#define YOUTUBE_STYLE_COMMENT_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>

#include <retroshare/rsgxscommon.h>

namespace Ui {
class YouTubeStyleCommentWidget;
}

class CommentItemWidget;

class YouTubeStyleCommentWidget : public QWidget
{
	Q_OBJECT

public:
	explicit YouTubeStyleCommentWidget(QWidget *parent = nullptr);
	~YouTubeStyleCommentWidget();

	// Add a comment at top level or as a reply to a parent
	void addComment(const RsGxsComment &comment, const RsGxsMessageId &parentId = RsGxsMessageId());
	void clearComments();

	// Set the comment service for loading comments
	void setCommentService(RsGxsCommentService *service);

 signals:
	void commentUpvote(const RsGxsGrpMsgIdPair &msgId, bool up);
	void commentDownvote(const RsGxsGrpMsgIdPair &msgId, bool down);
	void commentReply(const RsGxsMessageId &parentId);

public slots:
	void loadCommentsForPost(const RsGxsGroupId &groupId, const RsGxsMessageId &postId);
	void sortComments(int sortMethod);

private:
	void insertCommentIntoTree(CommentItemWidget *widget, const RsGxsMessageId &parentId);
	void expandReply(const RsGxsMessageId &commentId);

	Ui::YouTubeStyleCommentWidget *ui;

	QVBoxLayout *mCommentsLayout;
	QMap<RsGxsMessageId, CommentItemWidget *> mCommentWidgets;
	QMap<RsGxsMessageId, QList<RsGxsMessageId> > mRepliesMap;

	RsGxsCommentService *mCommentService;
	RsGxsGroupId mCurrentGroupId;
	RsGxsMessageId mCurrentPostId;
};

#endif // YOUTUBE_STYLE_COMMENT_WIDGET_H
