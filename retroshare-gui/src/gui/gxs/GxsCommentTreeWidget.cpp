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

#include <QMimeData>
#include <QDateTime>
#include <QMenu>

#include "gui/gxs/GxsCommentTreeWidget.h"
#include "gui/gxs/GxsCreateCommentDialog.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"

#include <iostream>

#define PCITEM_COLUMN_COMMENT		0
#define PCITEM_COLUMN_AUTHOR		1
#define PCITEM_COLUMN_DATE		2
#define PCITEM_COLUMN_SCORE		3
#define PCITEM_COLUMN_UPVOTES		4
#define PCITEM_COLUMN_DOWNVOTES		5
#define PCITEM_COLUMN_OWNVOTE		6
#define PCITEM_COLUMN_MSGID		7
#define PCITEM_COLUMN_PARENTID		8

#define GXSCOMMENTS_LOADTHREAD		1

#define COMMENT_VOTE_ACK	0x001234


/* Images for context menu icons */
#define IMAGE_MESSAGE		":/images/folder-draft.png"

GxsCommentTreeWidget::GxsCommentTreeWidget(QWidget *parent)
	:QTreeWidget(parent), mRsTokenService(NULL), mCommentService(NULL), mTokenQueue(NULL)
{
//	QTreeWidget* widget = this;

	setContextMenuPolicy(Qt::CustomContextMenu);
//	QFont font = QFont("ARIAL", 10);
//	font.setBold(true);

//	QString name("test");
//	QTreeWidgetItem *item = new QTreeWidgetItem();
//	item->setText(0, name);
//	item->setFont(0, font);
//	item->setSizeHint(0, QSize(18, 18));
//	item->setForeground(0, QBrush(QColor(79, 79, 79)));

//	addTopLevelItem(item);
//	item->setExpanded(true);

	return;
}

void GxsCommentTreeWidget::setCurrentMsgId(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{

	Q_UNUSED(previous);

	if(current)
	{
		mCurrentMsgId = current->text(PCITEM_COLUMN_MSGID).toStdString();
	}
}

void GxsCommentTreeWidget::customPopUpMenu(const QPoint& point)
{
	QMenu contextMnu( this );
	QAction* action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Reply to Comment"), this, SLOT(replyToComment()));
	action->setDisabled(mCurrentMsgId.empty());
	action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Submit Comment"), this, SLOT(makeComment()));
	action->setDisabled(mThreadId.first.empty());

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Vote Up"), this, SLOT(voteUp()));
	action->setDisabled(mVoterId.empty());
	action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Vote Down"), this, SLOT(voteDown()));
	action->setDisabled(mVoterId.empty());


	if (!mCurrentMsgId.empty())
	{
		contextMnu.addSeparator();
		QMenu *rep_menu = contextMnu.addMenu(tr("Reputation"));

		action = rep_menu->addAction(QIcon(IMAGE_MESSAGE), tr("Show Reputation"), this, SLOT(showReputation()));
		contextMnu.addSeparator();

		action = rep_menu->addAction(QIcon(IMAGE_MESSAGE), tr("Interesting User"), this, SLOT(markInteresting()));
		contextMnu.addSeparator();

		action = rep_menu->addAction(QIcon(IMAGE_MESSAGE), tr("Mark Spammy"), this, SLOT(markSpammer()));
		action = rep_menu->addAction(QIcon(IMAGE_MESSAGE), tr("Ban User"), this, SLOT(banUser()));
	}

	contextMnu.exec(QCursor::pos());
}


void GxsCommentTreeWidget::voteUp()
{
	std::cerr << "GxsCommentTreeWidget::voteUp()";
	std::cerr << std::endl;
	vote(mThreadId.first, mThreadId.second, mCurrentMsgId, mVoterId, true);
}


void GxsCommentTreeWidget::voteDown()
{
	std::cerr << "GxsCommentTreeWidget::voteDown()";
	std::cerr << std::endl;
	vote(mThreadId.first, mThreadId.second, mCurrentMsgId, mVoterId, false);
}

void GxsCommentTreeWidget::setVoteId(const RsGxsId &voterId)
{
	mVoterId = voterId;
	std::cerr << "GxsCommentTreeWidget::setVoterId(" << mVoterId << ")";
	std::cerr << std::endl;
}


void GxsCommentTreeWidget::vote(const RsGxsGroupId &groupId, const RsGxsMessageId &threadId, 
					const RsGxsMessageId &parentId, const RsGxsId &authorId, bool up)
{
        RsGxsVote vote;

        vote.mMeta.mGroupId = groupId;
        vote.mMeta.mThreadId = threadId;
        vote.mMeta.mParentId = parentId;
        vote.mMeta.mAuthorId = authorId;

	if (up)
	{
		vote.mVoteType = GXS_VOTE_UP;
	}
	else
	{
		vote.mVoteType = GXS_VOTE_DOWN;
	}

        std::cerr << "GxsCommentTreeWidget::vote()";
        std::cerr << std::endl;

        std::cerr << "GroupId : " << vote.mMeta.mGroupId << std::endl;
        std::cerr << "ThreadId : " << vote.mMeta.mThreadId << std::endl;
        std::cerr << "ParentId : " << vote.mMeta.mParentId << std::endl;
        std::cerr << "AuthorId : " << vote.mMeta.mAuthorId << std::endl;

	uint32_t token;
        mCommentService->createVote(token, vote);
        mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, COMMENT_VOTE_ACK);
}


void GxsCommentTreeWidget::showReputation()
{
	std::cerr << "GxsCommentTreeWidget::showReputation() TODO";
	std::cerr << std::endl;
}

void GxsCommentTreeWidget::markInteresting()
{
	std::cerr << "GxsCommentTreeWidget::markInteresting() TODO";
	std::cerr << std::endl;
}

void GxsCommentTreeWidget::markSpammer()
{
	std::cerr << "GxsCommentTreeWidget::markSpammer() TODO";
	std::cerr << std::endl;
}

void GxsCommentTreeWidget::banUser()
{
	std::cerr << "GxsCommentTreeWidget::banUser() TODO";
	std::cerr << std::endl;
}

void GxsCommentTreeWidget::makeComment()
{
	GxsCreateCommentDialog pcc(mTokenQueue, mCommentService, mThreadId, mThreadId.second, this);
	pcc.exec();
}

void GxsCommentTreeWidget::replyToComment()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mThreadId.first;
	msgId.second = mCurrentMsgId;
	GxsCreateCommentDialog pcc(mTokenQueue, mCommentService, msgId, mThreadId.second, this);
	pcc.exec();
}

void GxsCommentTreeWidget::setup(RsTokenService *token_service, RsGxsCommentService *comment_service)
{
	mRsTokenService = token_service;
	mCommentService = comment_service;
	mTokenQueue = new TokenQueue(token_service, this);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPopUpMenu(QPoint)));
	connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(setCurrentMsgId(QTreeWidgetItem*, QTreeWidgetItem*)));

	return;
}


/* Load Comments */
void GxsCommentTreeWidget::requestComments(const RsGxsGrpMsgIdPair& threadId)
{
	/* request comments */

	mThreadId = threadId;
	service_requestComments(threadId);
}

void GxsCommentTreeWidget::service_requestComments(const RsGxsGrpMsgIdPair& threadId)
{
	/* request comments */
	std::cerr << "GxsCommentTreeWidget::service_requestComments(" << threadId.second << ")";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_THREAD | RS_TOKREQOPT_MSG_LATEST;

	std::vector<RsGxsGrpMsgIdPair> msgIds;
	msgIds.push_back(threadId);

	uint32_t token;
	mTokenQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, GXSCOMMENTS_LOADTHREAD);
}


/* Generic Handling */
void GxsCommentTreeWidget::clearItems()
{
	mPendingInsertMap.clear();
	mLoadingMap.clear();
}


void GxsCommentTreeWidget::completeItems()
{
	/* handle pending items */
	std::string parentId;
	QTreeWidgetItem *parent = NULL;
	QList<QTreeWidgetItem *> topLevelItems;

	std::map<std::string, QTreeWidgetItem *>::iterator lit;
	std::multimap<std::string, QTreeWidgetItem *>::iterator pit;

	std::cerr << "GxsCommentTreeWidget::completeItems() " << mPendingInsertMap.size();
	std::cerr << " PendingItems";
	std::cerr << std::endl;

	for(pit = mPendingInsertMap.begin(); pit != mPendingInsertMap.end(); pit++)
	{
		std::cerr << "GxsCommentTreeWidget::completeItems() item->parent: " << pit->first;
		std::cerr << std::endl;

		if (pit->first != parentId)
		{
			/* find parent */
			parentId = pit->first;
			lit = mLoadingMap.find(pit->first);
			if (lit != mLoadingMap.end())
			{
					parent = lit->second;
			}
			else
			{
					parent = NULL;
			}
		}

		if (parent)
		{
			std::cerr << "GxsCommentTreeWidget::completeItems() Added to Parent";
			std::cerr << std::endl;

			parent->addChild(pit->second);
		}
		else if (parentId == mThreadId.second)
		{
			std::cerr << "GxsCommentTreeWidget::completeItems() Added to topLevelItems";
			std::cerr << std::endl;

			topLevelItems.append(pit->second);
		}
		else
		{

			/* missing parent -> insert At Top Level */
			QTreeWidgetItem *missingItem = service_createMissingItem(pit->first);

			std::cerr << "GxsCommentTreeWidget::completeItems() Added MissingItem";
			std::cerr << std::endl;

			parent = missingItem;
			parent->addChild(pit->second);
			topLevelItems.append(parent);
		}
	}

	/* now push final tree into Tree */
	clear();
	insertTopLevelItems(0, topLevelItems);

	/* cleanup temp stuff */
	mLoadingMap.clear();
	mPendingInsertMap.clear();
}


void GxsCommentTreeWidget::addItem(std::string itemId, std::string parentId, QTreeWidgetItem *item)
{
	std::cerr << "GxsCommentTreeWidget::addItem() Id: " << itemId;
	std::cerr << " ParentId: " << parentId;
	std::cerr << std::endl;

	/* store in map -> for children */
	mLoadingMap[itemId] = item;

	std::map<std::string, QTreeWidgetItem *>::iterator it;
	it = mLoadingMap.find(parentId);
	if (it != mLoadingMap.end())
	{
		std::cerr << "GxsCommentTreeWidget::addItem() Added to Parent";
		std::cerr << std::endl;

		it->second->addChild(item);
	}
	else
	{
		std::cerr << "GxsCommentTreeWidget::addItem() Added to Pending List";
		std::cerr << std::endl;

		mPendingInsertMap.insert(std::make_pair(parentId, item));
	}
}

void GxsCommentTreeWidget::loadThread(const uint32_t &token)
{
	clearItems();

	service_loadThread(token);

	completeItems();
}

void GxsCommentTreeWidget::acknowledgeComment(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;
	mCommentService->acknowledgeComment(token, msgId);

	// simply reload data
	service_requestComments(mThreadId);
}


void GxsCommentTreeWidget::acknowledgeVote(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;
	if (mCommentService->acknowledgeVote(token, msgId))
	{
		// reload data if vote was added.
		service_requestComments(mThreadId);
	}
}


void GxsCommentTreeWidget::service_loadThread(const uint32_t &token)
{
	std::cerr << "GxsCommentTreeWidget::service_loadThread() ERROR must be overloaded!";
	std::cerr << std::endl;

	std::vector<RsGxsComment> comments;
	mCommentService->getRelatedComments(token, comments);

	std::vector<RsGxsComment>::iterator vit;

	for(vit = comments.begin(); vit != comments.end(); vit++)
	{
		RsGxsComment &comment = *vit;
		/* convert to a QTreeWidgetItem */
		std::cerr << "GxsCommentTreeWidget::service_loadThread() Got Comment: " << comment.mMeta.mMsgId;
		std::cerr << std::endl;

		GxsIdTreeWidgetItem *item = new GxsIdTreeWidgetItem();
		QString text;

		{
				QDateTime qtime;
				qtime.setTime_t(comment.mMeta.mPublishTs);

				text = qtime.toString("yyyy-MM-dd hh:mm:ss");
				item->setText(PCITEM_COLUMN_DATE, text);
		}

		text = QString::fromUtf8(comment.mComment.c_str());
		item->setText(PCITEM_COLUMN_COMMENT, text);

		RsGxsId authorId = comment.mMeta.mAuthorId;
		item->setId(authorId, PCITEM_COLUMN_AUTHOR);

		text = QString::number(comment.mScore);
		item->setText(PCITEM_COLUMN_SCORE, text);

		text = QString::number(comment.mUpVotes);
		item->setText(PCITEM_COLUMN_UPVOTES, text);

		text = QString::number(comment.mDownVotes);
		item->setText(PCITEM_COLUMN_DOWNVOTES, text);

		text = QString::number(comment.mOwnVote);
		item->setText(PCITEM_COLUMN_OWNVOTE, text);

		text = QString::fromUtf8(comment.mMeta.mMsgId.c_str());
		item->setText(PCITEM_COLUMN_MSGID, text);

		text = QString::fromUtf8(comment.mMeta.mParentId.c_str());
		item->setText(PCITEM_COLUMN_PARENTID, text);


		addItem(comment.mMeta.mMsgId, comment.mMeta.mParentId, item);
	}

	return;
}

QTreeWidgetItem *GxsCommentTreeWidget::service_createMissingItem(const RsGxsMessageId& parent)
{
	std::cerr << "GxsCommentTreeWidget::service_createMissingItem()";
	std::cerr << std::endl;
		
	QTreeWidgetItem *item = new QTreeWidgetItem();
	QString text("Unknown");

	item->setText(PCITEM_COLUMN_DATE, text);
		
	item->setText(PCITEM_COLUMN_COMMENT, text);
		
	item->setText(PCITEM_COLUMN_AUTHOR, text);

	item->setText(PCITEM_COLUMN_MSGID, text);


		text = QString::fromUtf8(parent.c_str());
	item->setText(PCITEM_COLUMN_PARENTID, text);

	return item;
}	



void GxsCommentTreeWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "GxsCommentTreeWidget::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
		
	if (queue != mTokenQueue)
	{
		std::cerr << "GxsCommentTreeWidget::loadRequest() Queue ERROR";
		std::cerr << std::endl;
		return;
	}
		
	/* now switch on req */
		switch(req.mType)
	{
		
			case TOKENREQ_MSGINFO:
			{
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_ACK:
						if (req.mUserType == COMMENT_VOTE_ACK)
						{
							acknowledgeVote(req.mToken);
						}
						else
						{
							acknowledgeComment(req.mToken);
						}
						break;
					case RS_TOKREQ_ANSTYPE_DATA:
						loadThread(req.mToken);
						break;
				}
			}
			break;
			default:
					std::cerr << "GxsCommentTreeWidget::loadRequest() UNKNOWN UserType ";
					std::cerr << std::endl;
					break;

	}
}
