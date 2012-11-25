/****************************************************************
*  RetroShare is distributed under the following license:
*
*  Copyright (C) 2008 Robert Fernie
*
*  This program is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public License
*  as published by the Free Software Foundation; either version 2
*  of the License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
*  Boston, MA  02110-1301, USA.
****************************************************************/

#include <QMimeData>
#include <QDateTime>
#include <QMenu>

#include "gui/gxs/GxsCommentTreeWidget.h"
#include "gui/Posted/PostedCreateCommentDialog.h"

#include <iostream>

#define PCITEM_COLUMN_COMMENT		0
#define PCITEM_COLUMN_AUTHOR		1
#define PCITEM_COLUMN_DATE		2
#define PCITEM_COLUMN_SERVSTRING	3
#define PCITEM_COLUMN_MSGID		4
#define PCITEM_COLUMN_PARENTID		5

#define GXSCOMMENTS_LOADTHREAD		1

// Temporarily make this specific.
#include "retroshare/rsposted.h"

/* Images for context menu icons */
#define IMAGE_MESSAGE        ":/images/folder-draft.png"

GxsCommentTreeWidget::GxsCommentTreeWidget(QWidget *parent)
    :QTreeWidget(parent), mRsService(NULL), mTokenQueue(NULL)
{
//    QTreeWidget* widget = this;


//    QFont font = QFont("ARIAL", 10);
//    font.setBold(true);

//    QString name("test");
//    QTreeWidgetItem *item = new QTreeWidgetItem();
//    item->setText(0, name);
//    item->setFont(0, font);
//    item->setSizeHint(0, QSize(18, 18));
//    item->setForeground(0, QBrush(QColor(79, 79, 79)));

//    addTopLevelItem(item);
//    item->setExpanded(true);

    return;
}

void GxsCommentTreeWidget::setCurrentMsgId(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{

    Q_UNUSED(previous);

    if(current)
    {
        mCurrentMsgId = current->text(PCITEM_COLUMN_MSGID).toStdString();
    }else{
        mCurrentMsgId = "";
    }
}

void GxsCommentTreeWidget::customPopUpMenu(const QPoint& point)
{
    QMenu contextMnu( this );
    contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Submit Comment"), this, SLOT(makeComment()));
    contextMnu.exec(QCursor::pos());
}

void GxsCommentTreeWidget::makeComment()
{

    if(mCurrentMsgId.empty())
    {
        PostedCreateCommentDialog pcc(mTokenQueue, mThreadId, mThreadId.second, this);
        pcc.exec();
    }
    else
    {
        RsGxsGrpMsgIdPair msgId;
        msgId.first = mThreadId.first;
        msgId.second = mCurrentMsgId;
        PostedCreateCommentDialog pcc(mTokenQueue, msgId, mThreadId.second, this);
        pcc.exec();
    }
}

void GxsCommentTreeWidget::setup(RsTokenService *service)
{
    mRsService = service;
    mTokenQueue = new TokenQueue(service, this);
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
        opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
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
    rsPosted->acknowledgeMsg(token, msgId);

    // simply reload data
    service_requestComments(mThreadId);
}


void GxsCommentTreeWidget::service_loadThread(const uint32_t &token)
{
    std::cerr << "GxsCommentTreeWidget::service_loadThread() ERROR must be overloaded!";
    std::cerr << std::endl;

    PostedRelatedCommentResult commentResult;
    rsPosted->getRelatedComment(token, commentResult);

    std::vector<RsPostedComment>& commentV = commentResult[mThreadId];
    std::vector<RsPostedComment>::iterator vit = commentV.begin();

    for(; vit != commentV.end(); vit++)
    {
        RsPostedComment& comment = *vit;
        /* convert to a QTreeWidgetItem */
        std::cerr << "GxsCommentTreeWidget::service_loadThread() Got Comment: " << comment.mMeta.mMsgId;
        std::cerr << std::endl;

        QTreeWidgetItem *item = new QTreeWidgetItem();
        QString text;

        {
                QDateTime qtime;
                qtime.setTime_t(comment.mMeta.mPublishTs);

                text = qtime.toString("yyyy-MM-dd hh:mm:ss");
                item->setText(PCITEM_COLUMN_DATE, text);
        }

        text = QString::fromUtf8(comment.mComment.c_str());
        item->setText(PCITEM_COLUMN_COMMENT, text);

        text = QString::fromUtf8(comment.mMeta.mAuthorId.c_str());
        if (text.isEmpty())
        {
                item->setText(PCITEM_COLUMN_AUTHOR, tr("Anonymous"));
        }
        else
        {
                item->setText(PCITEM_COLUMN_AUTHOR, text);
        }

        text = QString::fromUtf8(comment.mMeta.mMsgId.c_str());
        item->setText(PCITEM_COLUMN_MSGID, text);

        text = QString::fromUtf8(comment.mMeta.mParentId.c_str());
        item->setText(PCITEM_COLUMN_PARENTID, text);

        text = QString::fromUtf8(comment.mMeta.mServiceString.c_str());
        item->setText(PCITEM_COLUMN_SERVSTRING, text);

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

	item->setText(PCITEM_COLUMN_SERVSTRING, text);

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
                        acknowledgeComment(req.mToken);
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
