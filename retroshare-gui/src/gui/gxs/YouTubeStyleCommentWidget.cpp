/*******************************************************************************
 * retroshare-gui/src/gui/gxs/YouTubeStyleCommentWidget.cpp                     *
 *                                                                             *
 * YouTube-style comment viewer for issue #1722                                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify       *
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

#include "YouTubeStyleCommentWidget.h"
#include "CommentItemWidget.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDebug>

#include "ui_YouTubeStyleCommentWidget.h"

YouTubeStyleCommentWidget::YouTubeStyleCommentWidget(QWidget *parent)
	: QWidget(parent), ui(new Ui::YouTubeStyleCommentWidget), mCommentService(nullptr)
{
	ui->setupUi(this);

	// Reuse the layout already defined in the .ui file — creating a second one
	// would silently replace it and lose the trailing spacer.
	mCommentsLayout = qobject_cast<QVBoxLayout*>(ui->commentsScrollArea->widget()->layout());
	mCommentsLayout->setSpacing(8);
}

YouTubeStyleCommentWidget::~YouTubeStyleCommentWidget()
{
	delete ui;
}

void YouTubeStyleCommentWidget::setCommentService(RsGxsCommentService *service)
{
	mCommentService = service;
}

void YouTubeStyleCommentWidget::clearComments()
{
	// Remove all comment widgets except the stretch
	while (mCommentsLayout->count() > 1) {
		QLayoutItem *item = mCommentsLayout->takeAt(0);
		if (item && item->widget()) {
			delete item->widget();
		}
		delete item;
	}
	mCommentWidgets.clear();
	mRepliesMap.clear();
}

void YouTubeStyleCommentWidget::addComment(const RsGxsComment &comment, const RsGxsMessageId &parentId)
{
	// Create comment item widget
	CommentItemWidget *itemWidget = new CommentItemWidget();

	// Set comment data
	itemWidget->setMsgId(comment.mMeta.mMsgId);
	itemWidget->setAuthorId(comment.mMeta.mAuthorId);
	itemWidget->setAuthorName(QString::fromStdString(comment.mMeta.mAuthorId.toStdString()));
	itemWidget->setCommentText(QString::fromStdString(comment.mComment));
	itemWidget->setDateTime(DateTime::formatDateTime(comment.mMeta.mPublishTs));

	int score = 0;
	itemWidget->setScore(score);

	// Set indentation based on reply level
	int level = 0;
	if (!parentId.isNull()) {
		level = 1; // Replies are indented
		// Could be extended for multi-level nesting
	}
	itemWidget->setLevel(level);

	// Store widget reference
	mCommentWidgets[comment.mMeta.mMsgId] = itemWidget;

	if (parentId.isNull()) {
		// Top-level comment - insert before the stretch
		mCommentsLayout->insertWidget(mCommentsLayout->count() - 1, itemWidget);
	} else {
		// Reply - add to parent's replies list
		mRepliesMap[parentId].append(comment.mMeta.mMsgId);
		// Insert after parent in layout
		CommentItemWidget *parentWidget = mCommentWidgets.value(parentId, nullptr);
		if (parentWidget) {
			int parentIndex = mCommentsLayout->indexOf(parentWidget);
			if (parentIndex >= 0) {
				mCommentsLayout->insertWidget(parentIndex + 1, itemWidget);
			} else {
				mCommentsLayout->insertWidget(mCommentsLayout->count() - 1, itemWidget);
			}
		} else {
			// Parent not yet in layout (orphaned reply) — insert at top level
			mCommentsLayout->insertWidget(mCommentsLayout->count() - 1, itemWidget);
		}
	}
}

void YouTubeStyleCommentWidget::insertCommentIntoTree(CommentItemWidget *widget, const RsGxsMessageId &parentId)
{
	Q_UNUSED(widget);
	Q_UNUSED(parentId);
	// This is where nested reply handling would be implemented
	// For YouTube style, replies could be collapsed/expanded
}

void YouTubeStyleCommentWidget::loadCommentsForPost(const RsGxsGroupId &groupId, const std::set<RsGxsMessageId> &msgVersions, const RsGxsMessageId &mostRecentMsgId)
{
	mCurrentGroupId = groupId;
	mCurrentPostId = mostRecentMsgId;

	clearComments();

	if (!mCommentService) {
		qDebug() << "YouTubeStyleCommentWidget: No comment service set";
		return;
	}

	RsThread::async([this, groupId, msgVersions]()
	{
		std::vector<RsGxsComment> comments;

		if (!mCommentService->getRelatedComments(groupId, msgVersions, comments)) {
			std::cerr << "YouTubeStyleCommentWidget: failed to get comments" << std::endl;
			return;
		}

		RsQThreadUtils::postToObject([this, comments, msgVersions]()
		{
			clearComments();

			// First pass: top-level comments — parent is one of the post's own message IDs
			for (const auto &comment : comments) {
				if (msgVersions.count(comment.mMeta.mParentId)) {
					addComment(comment, RsGxsMessageId());
				}
			}

			// Second pass: replies — parent is another comment, not the post itself
			for (const auto &comment : comments) {
				if (!msgVersions.count(comment.mMeta.mParentId)) {
					addComment(comment, comment.mMeta.mParentId);
				}
			}
		}, this);
	});
}

void YouTubeStyleCommentWidget::sortComments(int sortMethod)
{
	// Sorting placeholder - would need to collect, sort, and re-add all comments
	Q_UNUSED(sortMethod);
}

void YouTubeStyleCommentWidget::expandReply(const RsGxsMessageId &commentId)
{
	Q_UNUSED(commentId);
}
