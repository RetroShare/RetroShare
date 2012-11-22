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

#include "gui/gxs/GxsCommentTreeWidget.h"

#include <iostream>

#define PCITEM_COLUMN_DATE		0
#define PCITEM_COLUMN_COMMENT		1
#define PCITEM_COLUMN_AUTHOR		2
#define PCITEM_COLUMN_SERVSTRING	3
#define PCITEM_COLUMN_MSGID		4
#define PCITEM_COLUMN_PARENTID		5

#define GXSCOMMENTS_LOADTHREAD		1

// Temporarily make this specific.
#include "retroshare/rsposted.h"


GxsCommentTreeWidget::GxsCommentTreeWidget(QWidget *parent)
    :QTreeWidget(parent), mRsService(NULL), mTokenQueue(NULL)
{


    return;
}

void GxsCommentTreeWidget::setup(RsTokenService *service)
{
	mRsService = service;
        mTokenQueue = new TokenQueue(service, this);

   	return;
}


/* Load Comments */
void GxsCommentTreeWidget::requestComments(std::string threadId)
{
	/* request comments */

	service_requestComments(threadId);
}

void GxsCommentTreeWidget::service_requestComments(std::string threadId)
{
	/* request comments */
	//std::cerr << "GxsCommentTreeWidget::service_requestComments() ERROR must be overloaded!";
	//std::cerr << std::endl;

	std::cerr << "GxsCommentTreeWidget::service_requestComments(" << threadId << ")";
	std::cerr << std::endl;
	
        RsTokReqOptions opts;
	
	opts.mOptions = RS_TOKREQOPT_MSG_THREAD | RS_TOKREQOPT_MSG_LATEST;
//	opts.mFlagsFilter = RSPOSTED_MSGTYPE_COMMENT;
//	opts.mFlagsMask = RSPOSTED_MSGTYPE_COMMENT;
	
	std::list<std::string> msgIds;
	msgIds.push_back(threadId);

        mThreadId = threadId;
	
	uint32_t token;
//	mTokenQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, GXSCOMMENTS_LOADTHREAD);
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
		else if (parentId == mThreadId)
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


void GxsCommentTreeWidget::service_loadThread(const uint32_t &token)
{
	std::cerr << "GxsCommentTreeWidget::service_loadThread() ERROR must be overloaded!";
	std::cerr << std::endl;

//	RsPostedComment comment;
//        while(rsPosted->getComment(token, comment))
//	{
//		/* convert to a QTreeWidgetItem */
//		std::cerr << "GxsCommentTreeWidget::service_loadThread() Got Comment: " << comment;
//		std::cerr << std::endl;
		
//		QTreeWidgetItem *item = new QTreeWidgetItem();
//		QString text;

//		{
//			QDateTime qtime;
//			qtime.setTime_t(comment.mMeta.mPublishTs);
		
//			text = qtime.toString("yyyy-MM-dd hh:mm:ss");
//			item->setText(PCITEM_COLUMN_DATE, text);
//		}
		
//		text = QString::fromUtf8(comment.mComment.c_str());
//		item->setText(PCITEM_COLUMN_COMMENT, text);
		
//		text = QString::fromUtf8(comment.mMeta.mAuthorId.c_str());
//		if (text.isEmpty())
//		{
//			item->setText(PCITEM_COLUMN_AUTHOR, tr("Anonymous"));
//		}
//		else
//		{
//			item->setText(PCITEM_COLUMN_AUTHOR, text);
//		}


//		text = QString::fromUtf8(comment.mMeta.mMsgId.c_str());
//		item->setText(PCITEM_COLUMN_MSGID, text);

//		text = QString::fromUtf8(comment.mMeta.mParentId.c_str());
//		item->setText(PCITEM_COLUMN_PARENTID, text);

//		text = QString::fromUtf8(comment.mMeta.mServiceString.c_str());
//		item->setText(PCITEM_COLUMN_SERVSTRING, text);

//		addItem(comment.mMeta.mMsgId, comment.mMeta.mParentId, item);
//	}

	return;
}

QTreeWidgetItem *GxsCommentTreeWidget::service_createMissingItem(std::string parent)
{
	//std::cerr << "GxsCommentTreeWidget::service_createMissingItem() ERROR must be overloaded!";
	//std::cerr << std::endl;
	
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
	switch(req.mUserType)
	{
		
		case GXSCOMMENTS_LOADTHREAD:
			loadThread(req.mToken);
			break;
		default:
			std::cerr << "GxsCommentTreeWidget::loadRequest() UNKNOWN UserType ";
			std::cerr << std::endl;
			break;
	}
}



#if 0


QMimeData * GxsCommentTreeWidget::mimeData ( const QList<QTreeWidgetItem *> items ) const
{
	/* extract from each QTreeWidgetItem... all the member text */
	QList<QTreeWidgetItem *>::const_iterator it;
	QString text;
	for(it = items.begin(); it != items.end(); it++)
	{
		QString line = QString("%1/%2/%3/").arg((*it)->text(SR_NAME_COL), (*it)->text(SR_HASH_COL), (*it)->text(SR_SIZE_COL));

		bool isLocal = (*it)->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool();
		if (isLocal)
		{
			line += "Local";
		}
		else
		{
			line += "Remote";
		}
		line += "/\n";

		text += line;
	}

	std::cerr << "Created MimeData:";
	std::cerr << std::endl;

	std::string str = text.toUtf8().constData();
	std::cerr << str;
	std::cerr << std::endl;

	QMimeData *data = new QMimeData();
	data->setData("application/x-rsfilelist", QByteArray(str.c_str()));

	return data;
}



QStringList GxsCommentTreeWidget::mimeTypes () const 
{
	QStringList list;
	list.push_back("application/x-rsfilelist");

	return list;
}


Qt::DropActions GxsCommentTreeWidget::supportedDropActions () const
{
	return Qt::CopyAction;
}

#endif
