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

namespace Ui {
class YouTubeStyleCommentWidget;
}

YouTubeStyleCommentWidget::YouTubeStyleCommentWidget(QWidget *parent)
	: QWidget(parent), ui(new Ui::YouTubeStyleCommentWidget), mCommentService(nullptr)
{
	ui->setupUi(this);

	// Get the container layout from the scroll area
	QWidget *scrollAreaWidget = ui->commentsScrollArea->widget();
	mCommentsLayout = new QVBoxLayout(scrollAreaWidget);
	mCommentsLayout->setSpacing(8);
	mCommentsLayout->setContentsMargins(0, 0, 0, 0);
	mCommentsLayout->addStretch(); // Add stretch at the end to push items to top
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
	itemWidget->setAuthorName(QString::fromUtf8(comment.mMeta.mAuthorId.c_str()));
	itemWidget->setCommentText(QString::fromUtf8(comment.mContent.c_str()));
	itemWidget->setDateTime(util::DateTime::formatDateTime(comment.mMeta.mPublishTs));

	int score = comment.mMeta.mUpvotes - comment.mMeta.mDownvotes;
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
			// Find position after parent and insert
			int parentIndex = mCommentsLayout->indexOf(parentWidget);
			if (parentIndex >= 0) {
				mCommentsLayout->insertWidget(parentIndex + 1, itemWidget);
			}
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

void YouTubeStyleCommentWidget::loadCommentsForPost(const RsGxsGroupId &groupId, const RsGxsMessageId &postId)
{
	mCurrentGroupId = groupId;
	mCurrentPostId = postId;

	clearComments();

	if (!mCommentService) {
		qDebug() << "YouTubeStyleCommentWidget: No comment service set";
		return;
	}

	RsThread::async([this, groupId, postId]()
	{
		std::vector<RsGxsComment> comments;
		std::set<RsGxsMessageId> msgIds;
		msgIds.insert(postId);

		if (!mCommentService->getRelatedComments(groupId, msgIds, comments)) {
			std::cerr << "YouTubeStyleCommentWidget: failed to get comments" << std::endl;
			return;
		}

		RsQThreadUtils::postToObject([this, comments]()
		{
			clearComments();
			mRepliesMap.clear();

			// First pass: collect all replies
			for (const auto &comment : comments) {
				if (!comment.mMeta.mParentId.isNull()) {
					mRepliesMap[comment.mMeta.mParentId].append(comment.mMeta.mMsgId);
				}
			}

			// Second pass: add top-level comments
			for (const auto &comment : comments) {
				if (comment.mMeta.mParentId.isNull()) {
					addComment(comment, RsGxsMessageId());
				}
			}

			// Third pass: add replies after their parents
			for (const auto &comment : comments) {
				if (!comment.mMeta.mParentId.isNull()) {
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
