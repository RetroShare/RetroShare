/*******************************************************************************
 * retroshare-gui/src/gui/gxs/CommentItemWidget.h                              *
 *                                                                             *
 * YouTube-style comment item widget for issue #1722                           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify      *
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

#ifndef COMMENT_ITEM_WIDGET_H
#define COMMENT_ITEM_WIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QMouseEvent>

#include <retroshare/rsgxscommon.h>

namespace Ui {
class CommentItemWidget;
}

class CommentItemWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CommentItemWidget(QWidget *parent = nullptr);
	~CommentItemWidget();

	void setMsgId(const RsGxsMessageId &id);
	RsGxsMessageId getMsgId() const { return mMsgId; }
	void setAuthorId(const RsGxsId &id);
	void setAuthorName(const QString &name);
	void setAuthorAvatar(const QPixmap &avatar);
	void setCommentText(const QString &text);
	void setDateTime(const QString &datetime);
	void setScore(int score);
	void setUpvote(bool upvoted);
	void setDownvote(bool downvoted);
	void setReplyCount(int count);
	void setLevel(int level);
	void setViewRepliesCount(int count);
	void setSelected(bool selected);

	int getLevel() const { return mLevel; }
	QString getCommentText() const { return mCommentText; }
	QString getAuthorName() const { return mAuthorName; }

 signals:
	void upvoteClicked(const RsGxsMessageId &msgId);
	void downvoteClicked(const RsGxsMessageId &msgId);
	void replyClicked(const RsGxsMessageId &msgId);
	void authorClicked(const RsGxsId &authorId);
	void viewRepliesToggled(const RsGxsMessageId &msgId, bool show);
	void commentSelected(const RsGxsMessageId &msgId);

protected:
	void mousePressEvent(QMouseEvent *event) override;

private slots:
	void on_upvoteButton_clicked();
	void on_downvoteButton_clicked();
	void on_replyButton_clicked();
	void on_authorLabel_linkActivated(const QString &link);
	void on_viewRepliesButton_clicked();

private:
	void setupStyle();

	Ui::CommentItemWidget *ui;

	QPushButton *mViewRepliesButton;
	int mLevel;
	bool mUpvoteActive;
	bool mDownvoteActive;
	bool mSelected;
	bool mRepliesExpanded;
	RsGxsMessageId mMsgId;
	RsGxsId mAuthorId;
	QString mCommentText;
	QString mAuthorName;
	int mReplyCount;
};

#endif // COMMENT_ITEM_WIDGET_H
