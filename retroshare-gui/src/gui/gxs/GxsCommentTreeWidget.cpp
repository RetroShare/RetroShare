/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCommentTreeWidget.cpp                         *
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

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QTextDocument>

#include "gui/common/RSElidedItemDelegate.h"
#include "gui/gxs/GxsCommentTreeWidget.h"
#include "gui/gxs/GxsCreateCommentDialog.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/common/RSTreeWidgetItem.h"

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
#define PCITEM_COLUMN_AUTHORID		9

#define ROLE_SORT           Qt::UserRole + 1

#define GXSCOMMENTS_LOADTHREAD		1

#define COMMENT_VOTE_ACK	0x001234

#define POST_CELL_SIZE_ROLE (Qt::UserRole+1)
#define POST_COLOR_ROLE     (Qt::UserRole+2)

/* Images for context menu icons */
#define IMAGE_MESSAGE       ":/images/folder-draft.png"
#define IMAGE_COPY          ":/images/copy.png"
#define IMAGE_VOTEUP        ":/images/vote_up.png"
#define IMAGE_VOTEDOWN      ":/images/vote_down.png"

// This class allows to draw the item using an appropriate size

class MultiLinesCommentDelegate: public QStyledItemDelegate
{
public:
    MultiLinesCommentDelegate(QFontMetricsF f) : qf(f){}

    QSize sizeHint(const QStyleOptionViewItem &/*option*/, const QModelIndex &index) const
    {
        return index.data(POST_CELL_SIZE_ROLE).toSize() ;
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        Q_ASSERT(index.isValid());

        QStyleOptionViewItemV4 opt = option;
        initStyleOption(&opt, index);
        // disable default icon
        opt.icon = QIcon();
        opt.text = QString();

        // draw default item background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            const QWidget *widget = opt.widget;
            QStyle *style = widget ? widget->style() : QApplication::style();
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);
        }

        const QRect r = option.rect.adjusted(0,0,-option.decorationSize.width(),0);

        QTextDocument td ;
        td.setHtml("<html>"+index.data(Qt::DisplayRole).toString()+"</html>");
        td.setTextWidth(r.width());
        QSizeF s = td.documentLayout()->documentSize();

        int m = QFontMetricsF(QFont()).height();

        QSize full_area(std::min(r.width(),(int)s.width())+m,std::min(r.height(),(int)s.height())+m);

        QPixmap px(full_area.width(),full_area.height());
        px.fill(QColor(0,0,0,0));//Transparent background as item background is already paint.
        QPainter p(&px) ;
        p.setRenderHint(QPainter::Antialiasing);

        QPainterPath path ;
        path.addRoundedRect(QRectF(m/4.0,m/4.0,s.width()+m/2.0,s.height()+m/2.0),m,m) ;
        QPen pen(Qt::gray,m/7.0f);
        p.setPen(pen);
        p.fillPath(path,QColor::fromHsv( index.data(POST_COLOR_ROLE).toInt()/255.0*360,40,220));	// varies the color according to the post author
        p.drawPath(path);

        QAbstractTextDocumentLayout::PaintContext ctx;
        ctx.clip = QRectF(0,0,s.width(),s.height());
        p.translate(QPointF(m/2.0,m/2.0));
        td.documentLayout()->draw( &p, ctx );

        painter->drawPixmap(r.topLeft(),px);

        const_cast<QAbstractItemModel*>(index.model())->setData(index,px.size(),POST_CELL_SIZE_ROLE);
    }

private:
	QFontMetricsF qf;
};

GxsCommentTreeWidget::GxsCommentTreeWidget(QWidget *parent)
	:QTreeWidget(parent), mTokenQueue(NULL), mRsTokenService(NULL), mCommentService(NULL)
{
//	QTreeWidget* widget = this;

	setContextMenuPolicy(Qt::CustomContextMenu);
	RSElidedItemDelegate *itemDelegate = new RSElidedItemDelegate(this);
	itemDelegate->setSpacing(QSize(0, 2));
	setItemDelegate(itemDelegate);
	setWordWrap(true);

    setItemDelegateForColumn(PCITEM_COLUMN_COMMENT,new MultiLinesCommentDelegate(QFontMetricsF(font()))) ;
	
	commentsRole = new RSTreeWidgetItemCompareRole;
    commentsRole->setRole(PCITEM_COLUMN_DATE, ROLE_SORT);

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

GxsCommentTreeWidget::~GxsCommentTreeWidget()
{
	if (mTokenQueue) {
		delete(mTokenQueue);
	}
}

void GxsCommentTreeWidget::setCurrentCommentMsgId(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{

	Q_UNUSED(previous);

	if(current)
	{
		mCurrentCommentMsgId = RsGxsMessageId(current->text(PCITEM_COLUMN_MSGID).toStdString());
		mCurrentCommentText = current->text(PCITEM_COLUMN_COMMENT);
		mCurrentCommentAuthor = current->text(PCITEM_COLUMN_AUTHOR);
		mCurrentCommentAuthorId = RsGxsId(current->text(PCITEM_COLUMN_AUTHORID).toStdString());

	}
}

void GxsCommentTreeWidget::customPopUpMenu(const QPoint& /*point*/)
{
	QMenu contextMnu( this );
	QAction* action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Reply to Comment"), this, SLOT(replyToComment()));
	action->setDisabled(mCurrentCommentMsgId.isNull());
	action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Submit Comment"), this, SLOT(makeComment()));
	action->setDisabled(mMsgVersions.empty());
	action = contextMnu.addAction(QIcon(IMAGE_COPY), tr("Copy Comment"), this, SLOT(copyComment()));
	action->setDisabled(mCurrentCommentMsgId.isNull());

	contextMnu.addSeparator();

	action = contextMnu.addAction(QIcon(IMAGE_VOTEUP), tr("Vote Up"), this, SLOT(voteUp()));
	action->setDisabled(mVoterId.isNull());
	action = contextMnu.addAction(QIcon(IMAGE_VOTEDOWN), tr("Vote Down"), this, SLOT(voteDown()));
	action->setDisabled(mVoterId.isNull());


	if (!mCurrentCommentMsgId.isNull())
	{
        // not implemented yet
        /*
		contextMnu.addSeparator();
		QMenu *rep_menu = contextMnu.addMenu(tr("Reputation"));

		action = rep_menu->addAction(QIcon(IMAGE_MESSAGE), tr("Show Reputation"), this, SLOT(showReputation()));
		contextMnu.addSeparator();

		action = rep_menu->addAction(QIcon(IMAGE_MESSAGE), tr("Interesting User"), this, SLOT(markInteresting()));
		contextMnu.addSeparator();

		action = rep_menu->addAction(QIcon(IMAGE_MESSAGE), tr("Mark Spammy"), this, SLOT(markSpammer()));
		action = rep_menu->addAction(QIcon(IMAGE_MESSAGE), tr("Ban User"), this, SLOT(banUser()));
        */
	}

	contextMnu.exec(QCursor::pos());
}


void GxsCommentTreeWidget::voteUp()
{
	std::cerr << "GxsCommentTreeWidget::voteUp()";
	std::cerr << std::endl;

	vote(mGroupId, mLatestMsgId, mCurrentCommentMsgId, mVoterId, true);
}


void GxsCommentTreeWidget::voteDown()
{
	std::cerr << "GxsCommentTreeWidget::voteDown()";
	std::cerr << std::endl;

	vote(mGroupId, mLatestMsgId, mCurrentCommentMsgId, mVoterId, false);
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
        mCommentService->createNewVote(token, vote);
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
	GxsCreateCommentDialog pcc(mCommentService, std::make_pair(mGroupId,mLatestMsgId), mLatestMsgId, this);
	pcc.exec();
}

void GxsCommentTreeWidget::replyToComment()
{
	RsGxsGrpMsgIdPair msgId;
	msgId.first = mGroupId;
	msgId.second = mCurrentCommentMsgId;
	GxsCreateCommentDialog pcc(mCommentService, msgId, mLatestMsgId, this);

	pcc.loadComment(mCurrentCommentText, mCurrentCommentAuthor, mCurrentCommentAuthorId);
	pcc.exec();
}

void GxsCommentTreeWidget::copyComment()
{
	QMimeData *mimeData = new QMimeData();
	mimeData->setHtml("<html>"+mCurrentCommentText+"</html>");
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setMimeData(mimeData, QClipboard::Clipboard);
}

void GxsCommentTreeWidget::setup(RsTokenService *token_service, RsGxsCommentService *comment_service)
{
	mRsTokenService = token_service;
	mCommentService = comment_service;
	mTokenQueue = new TokenQueue(token_service, this);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customPopUpMenu(QPoint)));
	connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(setCurrentCommentMsgId(QTreeWidgetItem*, QTreeWidgetItem*)));

	return;
}


/* Load Comments */
void GxsCommentTreeWidget::requestComments(const RsGxsGroupId& group, const std::set<RsGxsMessageId>& message_versions,const RsGxsMessageId& most_recent_message)
{
	/* request comments */

    mGroupId = group ;
	mMsgVersions = message_versions ;
    mLatestMsgId = most_recent_message;

	service_requestComments(group,message_versions);
}

void GxsCommentTreeWidget::service_requestComments(const RsGxsGroupId& group_id,const std::set<RsGxsMessageId> & msgIds)
{
	/* request comments */
	std::cerr << "GxsCommentTreeWidget::service_requestComments for group " << group_id << std::endl;

    std::vector<RsGxsGrpMsgIdPair> ids_to_ask;

    for(std::set<RsGxsMessageId>::const_iterator it(msgIds.begin());it!=msgIds.end();++it)
    {
		std::cerr << "   asking for msg " << *it << std::endl;

        ids_to_ask.push_back(std::make_pair(group_id,*it));
    }

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_THREAD | RS_TOKREQOPT_MSG_LATEST;

	uint32_t token;
	mTokenQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, ids_to_ask, GXSCOMMENTS_LOADTHREAD);
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
	RsGxsMessageId parentId;
	QTreeWidgetItem *parent = NULL;
	QList<QTreeWidgetItem *> topLevelItems;

	std::map<RsGxsMessageId, QTreeWidgetItem *>::iterator lit;
	std::multimap<RsGxsMessageId, QTreeWidgetItem *>::iterator pit;

	std::cerr << "GxsCommentTreeWidget::completeItems() " << mPendingInsertMap.size();
	std::cerr << " PendingItems";
	std::cerr << std::endl;

	for(pit = mPendingInsertMap.begin(); pit != mPendingInsertMap.end(); ++pit)
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
		else if (mMsgVersions.find(parentId) != mMsgVersions.end())
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


void GxsCommentTreeWidget::addItem(RsGxsMessageId itemId, RsGxsMessageId parentId, QTreeWidgetItem *item)
{
	std::cerr << "GxsCommentTreeWidget::addItem() Id: " << itemId;
	std::cerr << " ParentId: " << parentId;
	std::cerr << std::endl;

	/* store in map -> for children */
	mLoadingMap[itemId] = item;

	std::map<RsGxsMessageId, QTreeWidgetItem *>::iterator it;
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
	service_requestComments(mGroupId,mMsgVersions);
}


void GxsCommentTreeWidget::acknowledgeVote(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;
	if (mCommentService->acknowledgeVote(token, msgId))
	{
		// reload data if vote was added.
		service_requestComments(mGroupId,mMsgVersions);
	}
}


void GxsCommentTreeWidget::service_loadThread(const uint32_t &token)
{
	std::cerr << "GxsCommentTreeWidget::service_loadThread() ERROR must be overloaded!";
	std::cerr << std::endl;

	std::vector<RsGxsComment> comments;
	mCommentService->getRelatedComments(token, comments);

	std::vector<RsGxsComment>::iterator vit;

	for(vit = comments.begin(); vit != comments.end(); ++vit)
	{
		RsGxsComment &comment = *vit;
		/* convert to a QTreeWidgetItem */
		std::cerr << "GxsCommentTreeWidget::service_loadThread() Got Comment: " << comment.mMeta.mMsgId;
		std::cerr << std::endl;

		GxsIdRSTreeWidgetItem *item = new GxsIdRSTreeWidgetItem(NULL,GxsIdDetails::ICON_TYPE_AVATAR) ;
		QString text;

		{
			QDateTime qtime ;
			qtime.setTime_t(comment.mMeta.mPublishTs) ;

			text = qtime.toString("yyyy-MM-dd hh:mm:ss") ;
			item->setText(PCITEM_COLUMN_DATE, text) ;
			item->setToolTip(PCITEM_COLUMN_DATE, text) ;
			item->setData(PCITEM_COLUMN_DATE, ROLE_SORT, QVariant(qlonglong(comment.mMeta.mPublishTs)));

		}

		text = QString::fromUtf8(comment.mComment.c_str());
		item->setText(PCITEM_COLUMN_COMMENT, text);
		item->setToolTip(PCITEM_COLUMN_COMMENT, text);

		RsGxsId authorId = comment.mMeta.mAuthorId;
		item->setId(authorId, PCITEM_COLUMN_AUTHOR, false);
        item->setData(PCITEM_COLUMN_COMMENT,POST_COLOR_ROLE,QVariant(authorId.toByteArray()[1]));

		text = QString::number(comment.mScore);
		item->setText(PCITEM_COLUMN_SCORE, text);

		text = QString::number(comment.mUpVotes);
		item->setText(PCITEM_COLUMN_UPVOTES, text);

		text = QString::number(comment.mDownVotes);
		item->setText(PCITEM_COLUMN_DOWNVOTES, text);

		text = QString::number(comment.mOwnVote);
		item->setText(PCITEM_COLUMN_OWNVOTE, text);

		text = QString::fromUtf8(comment.mMeta.mMsgId.toStdString().c_str());
		item->setText(PCITEM_COLUMN_MSGID, text);

		text = QString::fromUtf8(comment.mMeta.mParentId.toStdString().c_str());
		item->setText(PCITEM_COLUMN_PARENTID, text);
		
		text = QString::fromUtf8(comment.mMeta.mAuthorId.toStdString().c_str());
		item->setText(PCITEM_COLUMN_AUTHORID, text);


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
	
	item->setText(PCITEM_COLUMN_AUTHORID, text);


		text = QString::fromUtf8(parent.toStdString().c_str());
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
