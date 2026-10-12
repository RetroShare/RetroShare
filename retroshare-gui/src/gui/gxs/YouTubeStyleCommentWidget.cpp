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
#include "GxsIdDetails.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"

#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>

#include <algorithm>

#include "ui_YouTubeStyleCommentWidget.h"

// Issue 1 (avatar) + Issue 3 (nickname):
// - Only update the author name once we have a definitive result (DONE/FAILED),
//   not during LOADING — avoids the raw-ID-prefix "Loading... XXXXX" flash.
// - Always set an avatar for every terminal state so the label is never blank.
static void fillCommentItemWidgetCallback(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &/*data*/)
{
	CommentItemWidget *item = dynamic_cast<CommentItemWidget*>(object);
	if (!item)
		return;

	switch (type) {
	case GXS_ID_DETAILS_TYPE_DONE:
		// getName() uses details.mNickname — the actual display name.
		item->setAuthorName(GxsIdDetails::getNameForType(type, details));
		{
			QPixmap avatar;
			if (details.mAvatar.mSize > 0 && GxsIdDetails::loadPixmapFromData(details.mAvatar.mData, details.mAvatar.mSize, avatar))
				item->setAuthorAvatar(avatar);
			else
				item->setAuthorAvatar(GxsIdDetails::makeDefaultIcon(details.mId));
		}
		break;

	case GXS_ID_DETAILS_TYPE_LOADING:
		// Set generated default avatar so the slot is never blank while loading.
		// Leave the name empty — no raw ID prefix shown.
		if (!details.mId.isNull())
			item->setAuthorAvatar(GxsIdDetails::makeDefaultIcon(details.mId));
		break;

	case GXS_ID_DETAILS_TYPE_FAILED:
		// Identity not available; show truncated ID as fallback, keep default icon.
		if (!details.mId.isNull()) {
			item->setAuthorName(QString::fromStdString(details.mId.toStdString().substr(0, 10)) + QStringLiteral("…"));
			item->setAuthorAvatar(GxsIdDetails::makeDefaultIcon(details.mId));
		}
		break;

	case GXS_ID_DETAILS_TYPE_BANNED:
		item->setAuthorName(QObject::tr("[Banned]"));
		if (!details.mId.isNull())
			item->setAuthorAvatar(GxsIdDetails::makeDefaultIcon(details.mId));
		break;

	case GXS_ID_DETAILS_TYPE_EMPTY:
	default:
		break;
	}
}

YouTubeStyleCommentWidget::YouTubeStyleCommentWidget(QWidget *parent)
	: QWidget(parent), ui(new Ui::YouTubeStyleCommentWidget), mCommentService(nullptr),
	  mSelectedWidget(nullptr)
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

void YouTubeStyleCommentWidget::setVoterId(const RsGxsId &id)
{
	mVoterId = id;
}

void YouTubeStyleCommentWidget::updateReplyCountButtons()
{
	for (auto it = mRepliesMap.begin(); it != mRepliesMap.end(); ++it) {
		CommentItemWidget *parent = mCommentWidgets.value(it.key(), nullptr);
		if (parent)
			parent->setViewRepliesCount(it.value().size());
	}
}

void YouTubeStyleCommentWidget::clearComments()
{
	// Zero before deleting to avoid a dangling pointer if Qt emits signals during destruction
	mSelectedWidget = nullptr;

	// Remove all comment widgets except the trailing stretch spacer
	while (mCommentsLayout->count() > 1) {
		QLayoutItem *item = mCommentsLayout->takeAt(0);
		if (item && item->widget()) {
			delete item->widget();
		}
		delete item;
	}
	mCommentWidgets.clear();
	mRepliesMap.clear();
	mScoreMap.clear();
	mTimestampMap.clear();
}

void YouTubeStyleCommentWidget::addComment(const RsGxsComment &comment, const RsGxsMessageId &parentId)
{
	CommentItemWidget *itemWidget = new CommentItemWidget();

	itemWidget->setMsgId(comment.mMeta.mMsgId);
	itemWidget->setAuthorId(comment.mMeta.mAuthorId);
	itemWidget->setCommentText(QString::fromStdString(comment.mComment));
	itemWidget->setDateTime(DateTime::formatDateTime(comment.mMeta.mPublishTs));
	itemWidget->setScore(static_cast<int>(comment.mScore));

	mScoreMap[comment.mMeta.mMsgId] = comment.mScore;
	mTimestampMap[comment.mMeta.mMsgId] = comment.mMeta.mPublishTs;

	int level = parentId.isNull() ? 0 : 1;
	itemWidget->setLevel(level);

	// Issue 4: replies start hidden; the parent's "View X replies" button reveals them
	if (level > 0)
		itemWidget->hide();

	mCommentWidgets[comment.mMeta.mMsgId] = itemWidget;

	// Issue 2: vote buttons wired to doVote() which calls the comment service directly,
	// mirroring GxsCommentTreeWidget::vote()
	connect(itemWidget, &CommentItemWidget::upvoteClicked, this, [this](const RsGxsMessageId &msgId) {
		doVote(msgId, true);
	});
	connect(itemWidget, &CommentItemWidget::downvoteClicked, this, [this](const RsGxsMessageId &msgId) {
		doVote(msgId, false);
	});
	// Issue 2: reply button propagates up to GxsCommentDialog via signal
	connect(itemWidget, &CommentItemWidget::replyClicked, this, &YouTubeStyleCommentWidget::commentReply);

	// Issue 4: collapse/expand toggle for reply threads
	connect(itemWidget, &CommentItemWidget::viewRepliesToggled,
	        this, &YouTubeStyleCommentWidget::onViewRepliesToggled);

	// Issue 5: click-to-select within the list
	connect(itemWidget, &CommentItemWidget::commentSelected,
	        this, &YouTubeStyleCommentWidget::onCommentSelected);

	// Issue 1 + 3: async identity resolution fills name and avatar
	GxsIdDetails::process(comment.mMeta.mAuthorId, fillCommentItemWidgetCallback, itemWidget);

	if (parentId.isNull()) {
		mCommentsLayout->insertWidget(mCommentsLayout->count() - 1, itemWidget);
	} else {
		mRepliesMap[parentId].append(comment.mMeta.mMsgId);
		CommentItemWidget *parentWidget = mCommentWidgets.value(parentId, nullptr);
		if (parentWidget) {
			int insertPos = mCommentsLayout->indexOf(parentWidget) + 1;
			// Skip past any replies already placed after this parent
			while (insertPos < mCommentsLayout->count() - 1) {
				CommentItemWidget *ciw = qobject_cast<CommentItemWidget*>(mCommentsLayout->itemAt(insertPos)->widget());
				if (ciw && ciw->getLevel() > 0)
					++insertPos;
				else
					break;
			}
			mCommentsLayout->insertWidget(insertPos, itemWidget);
		} else {
			mCommentsLayout->insertWidget(mCommentsLayout->count() - 1, itemWidget);
		}
	}
}

void YouTubeStyleCommentWidget::loadCommentsForPost(const RsGxsGroupId &groupId, const std::set<RsGxsMessageId> &msgVersions, const RsGxsMessageId &mostRecentMsgId)
{
	mCurrentGroupId = groupId;
	mCurrentPostId  = mostRecentMsgId;
	mLatestMsgId    = mostRecentMsgId;
	mMsgVersions    = msgVersions;

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

			// Issue 4: stamp each top-level comment with its reply count
			updateReplyCountButtons();
		}, this);
	});
}

void YouTubeStyleCommentWidget::sortComments(int sortMethod)
{
	// Collect top-level CommentItemWidget pointers
	QList<CommentItemWidget *> topLevel;
	for (auto it = mCommentWidgets.begin(); it != mCommentWidgets.end(); ++it) {
		if (it.value()->getLevel() == 0)
			topLevel.append(it.value());
	}

	if (topLevel.isEmpty())
		return;

	std::sort(topLevel.begin(), topLevel.end(), [&](CommentItemWidget *a, CommentItemWidget *b) {
		const RsGxsMessageId aId = a->getMsgId();
		const RsGxsMessageId bId = b->getMsgId();
		if (sortMethod == 1) // New: timestamp descending
			return mTimestampMap.value(aId, 0) > mTimestampMap.value(bId, 0);
		if (sortMethod == 2) // Top: score ascending
			return mScoreMap.value(aId, 0.0) < mScoreMap.value(bId, 0.0);
		// Hot (0, default): score descending
		return mScoreMap.value(aId, 0.0) > mScoreMap.value(bId, 0.0);
	});

	// Detach all comment widgets from the layout without deleting them;
	// delete the QLayoutItem* wrapper (required by Qt), not the widget itself.
	while (mCommentsLayout->count() > 1)
		delete mCommentsLayout->takeAt(0);

	// Re-insert: each top-level widget followed immediately by its replies
	for (CommentItemWidget *parent : topLevel) {
		mCommentsLayout->insertWidget(mCommentsLayout->count() - 1, parent);
		for (const RsGxsMessageId &replyId : mRepliesMap.value(parent->getMsgId())) {
			CommentItemWidget *reply = mCommentWidgets.value(replyId, nullptr);
			if (reply)
				mCommentsLayout->insertWidget(mCommentsLayout->count() - 1, reply);
		}
	}
}

// Issue 2: perform a vote via the comment service, mirroring GxsCommentTreeWidget::vote()
void YouTubeStyleCommentWidget::doVote(const RsGxsMessageId &commentMsgId, bool up)
{
	if (!mCommentService) {
		qDebug() << "YouTubeStyleCommentWidget::doVote: no comment service";
		return;
	}
	if (mVoterId.isNull()) {
		QMessageBox::warning(this, tr("Cannot vote"), tr("Please select an identity to vote with."));
		return;
	}

	RsGxsGroupId groupId             = mCurrentGroupId;
	RsGxsMessageId threadId          = mLatestMsgId;
	RsGxsId voterId                  = mVoterId;
	std::set<RsGxsMessageId> versions = mMsgVersions;

	RsThread::async([this, groupId, threadId, commentMsgId, voterId, up, versions]()
	{
		std::string error_string;
		RsGxsMessageId vote_id;
		RsGxsVoteType tvote = up ? RsGxsVoteType::UP : RsGxsVoteType::DOWN;

		bool res = mCommentService->voteForComment(groupId, threadId, commentMsgId, voterId, tvote, vote_id, error_string);

		RsQThreadUtils::postToObject([this, res, error_string, groupId, versions, threadId]()
		{
			if (res)
				loadCommentsForPost(groupId, versions, threadId);
			else
				QMessageBox::critical(nullptr, tr("Cannot vote"),
				    tr("Error while voting: ") + QString::fromStdString(error_string));
		}, this);
	});
}

// Issue 4: show or hide the reply widgets when the "View X replies" button is toggled
void YouTubeStyleCommentWidget::onViewRepliesToggled(const RsGxsMessageId &msgId, bool show)
{
	for (const RsGxsMessageId &replyId : mRepliesMap.value(msgId)) {
		CommentItemWidget *reply = mCommentWidgets.value(replyId, nullptr);
		if (reply)
			reply->setVisible(show);
	}
}

// Issue 5: track the selected comment widget, deselecting the previous one
void YouTubeStyleCommentWidget::onCommentSelected(const RsGxsMessageId &msgId)
{
	if (mSelectedWidget) {
		mSelectedWidget->setSelected(false);
		mSelectedWidget = nullptr;
	}
	CommentItemWidget *w = mCommentWidgets.value(msgId, nullptr);
	if (w) {
		w->setSelected(true);
		mSelectedWidget = w;
	}
}
