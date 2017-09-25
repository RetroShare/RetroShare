/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012, RetroShare Team
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

#include <QApplication>
#include <QTreeWidgetItem>

#include "GxsForumsFillThread.h"
#include "GxsForumThreadWidget.h"

#include "retroshare/rsgxsflags.h"
#include <retroshare/rsgxsforums.h>

#include <iostream>
#include <algorithm>

//#define DEBUG_FORUMS

#define PROGRESSBAR_MAX 100

GxsForumsFillThread::GxsForumsFillThread(GxsForumThreadWidget *parent)
	: QThread(parent), mParent(parent)
{
	mStopped = false;
	mCompareRole = NULL;

	mExpandNewMessages = true;
	mFillComplete = false;

	mFilterColumn = 0;

	mViewType = 0;
	mFlatView = false;
	mUseChildTS = false;
}

GxsForumsFillThread::~GxsForumsFillThread()
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::~GxsForumsFillThread" << std::endl;
#endif

	// remove all items (when items are available, the thread was terminated)
	QList<QTreeWidgetItem *>::iterator item;
	for (item = mItems.begin (); item != mItems.end (); ++item) {
		if (*item) {
			delete (*item);
		}
	}
	mItems.clear();

	mItemToExpand.clear();
}

void GxsForumsFillThread::stop()
{
	disconnect();
	mStopped = true;
	QApplication::processEvents();
	wait();
}

void GxsForumsFillThread::calculateExpand(const RsGxsForumMsg &msg, QTreeWidgetItem *item)
{
	if (mFillComplete && mExpandNewMessages && IS_MSG_UNREAD(msg.mMeta.mMsgStatus)) {
		QTreeWidgetItem *parentItem = item;
		while ((parentItem = parentItem->parent()) != NULL) {
			if (std::find(mItemToExpand.begin(), mItemToExpand.end(), parentItem) == mItemToExpand.end()) {
				mItemToExpand.push_back(parentItem);
			}
		}
	}
}

static bool decreasing_time_comp(const QPair<time_t,RsGxsMessageId>& e1,const QPair<time_t,RsGxsMessageId>& e2) { return e2.first < e1.first ; }

void GxsForumsFillThread::run()
{
	RsTokenService *service = rsGxsForums->getTokenService();

	emit status(tr("Waiting"));

	/* get all messages of the forum */
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	std::list<RsGxsGroupId> grpIds;
	grpIds.push_back(mForumId);

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::run() forum id " << mForumId << std::endl;
#endif

	uint32_t token;
	service->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds);

	/* wait for the answer */
	uint32_t requestStatus = RsTokenService::GXS_REQUEST_V2_STATUS_PENDING;
	while (!wasStopped()) {
		requestStatus = service->requestStatus(token);
		if (requestStatus == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED ||
			requestStatus == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE) {
			break;
		}
		msleep(100);
	}

	if (wasStopped()) {
#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumsFillThread::run() thread stopped, cancel request" << std::endl;
#endif

		/* cancel request */
		service->cancelRequest(token);
		return;
	}

	if (requestStatus == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED) {
//#TODO
		return;
	}

//#TODO
//	if (failed) {
//		mService->cancelRequest(token);
//		return;
//	}

	emit status(tr("Retrieving"));

	/* get messages */
	std::map<RsGxsMessageId,RsGxsForumMsg> msgs;

	{	// This forces to delete msgs_array after the conversion to std::map.

		std::vector<RsGxsForumMsg> msgs_array;

		if (!rsGxsForums->getMsgData(token, msgs_array)) {
			return;
		}

		// now put everything into a map in order to make search log(n)

		for(uint32_t i=0;i<msgs_array.size();++i)
        {
#ifdef DEBUG_FORUMS
            std::cerr << "Adding message " << msgs_array[i].mMeta.mMsgId << " with parent " << msgs_array[i].mMeta.mParentId << " to message map" << std::endl;
#endif
			msgs[msgs_array[i].mMeta.mMsgId] = msgs_array[i] ;
        }
	}

	emit status(tr("Loading"));

	int count = msgs.size();
	int pos = 0;
	int steps = count / PROGRESSBAR_MAX;
	int step = 0;

    // ThreadList contains the list of parent threads. The algorithm below iterates through all messages
    // and tries to establish parenthood relationships between them, given that we only know the
    // immediate parent of a message and now its children. Some messages have a missing parent and for them
    // a fake top level parent is generated.
    
    // In order to be efficient, we first create a structure that lists the children of every mesage ID in the list.
    // Then the hierarchy of message is build by attaching the kids to every message until all of them have been processed.
    // The messages with missing parents will be the last ones remaining in the list.
    
	std::list<std::pair< RsGxsMessageId, QTreeWidgetItem* > > threadStack;
    std::map<RsGxsMessageId,std::list<RsGxsMessageId> > kids_array ;
    std::set<RsGxsMessageId> missing_parents;

    // First of all, remove all older versions of posts. This is done by first adding all posts into a hierarchy structure
    // and then removing all posts which have a new versions available. The older versions are kept appart.

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::run() Collecting post versions" << std::endl;
#endif
    mPostVersions.clear();
    std::list<RsGxsMessageId> msg_stack ;

	for ( std::map<RsGxsMessageId,RsGxsForumMsg>::iterator msgIt = msgs.begin(); msgIt != msgs.end();++msgIt)
        if(!msgIt->second.mMeta.mOrigMsgId.isNull() && msgIt->second.mMeta.mOrigMsgId != msgIt->second.mMeta.mMsgId)
		{
#ifdef DEBUG_FORUMS
			std::cerr << "  Post " << msgIt->second.mMeta.mMsgId << " is a new version of " << msgIt->second.mMeta.mOrigMsgId << std::endl;
#endif
			std::map<RsGxsMessageId,RsGxsForumMsg>::iterator msgIt2 = msgs.find(msgIt->second.mMeta.mOrigMsgId);

			// Ensuring that the post exists allows to only collect the existing data.

			if(msgIt2 == msgs.end())
				continue ;

			// Make sure that the author is the same than the original message. This should always happen, but nothing can prevent someone to
			// craft a new version of a message with his own signature.

			if(msgIt2->second.mMeta.mAuthorId != msgIt->second.mMeta.mAuthorId)
				continue ;

			// always add the post a self version

			if(mPostVersions[msgIt->second.mMeta.mOrigMsgId].empty())
				mPostVersions[msgIt->second.mMeta.mOrigMsgId].push_back(QPair<time_t,RsGxsMessageId>(msgIt2->second.mMeta.mPublishTs,msgIt2->second.mMeta.mMsgId)) ;

			mPostVersions[msgIt->second.mMeta.mOrigMsgId].push_back(QPair<time_t,RsGxsMessageId>(msgIt->second.mMeta.mPublishTs,msgIt->second.mMeta.mMsgId)) ;
		}

    // The following code assembles all new versions of a given post into the same array, indexed by the oldest version of the post.

    for(QMap<RsGxsMessageId,QVector<QPair<time_t,RsGxsMessageId> > >::iterator it(mPostVersions.begin());it!=mPostVersions.end();++it)
    {
		QVector<QPair<time_t,RsGxsMessageId> >& v(*it) ;

        for(int32_t i=0;i<v.size();++i)
            if(v[i].second != it.key())
			{
				RsGxsMessageId sub_msg_id = v[i].second ;

				QMap<RsGxsMessageId,QVector<QPair<time_t,RsGxsMessageId> > >::iterator it2 = mPostVersions.find(sub_msg_id);

				if(it2 != mPostVersions.end())
				{
					for(int32_t j=0;j<(*it2).size();++j)
						if((*it2)[j].second != sub_msg_id)	// dont copy it, since it is already present at slot i
							v.append((*it2)[j]) ;

					mPostVersions.erase(it2) ;	// it2 is never equal to it
				}
			}
    }


    // Now remove from msg ids, all posts except the most recent one. And make the mPostVersion be indexed by the most recent version of the post,
    // which corresponds to the item in the tree widget.

#ifdef DEBUG_FORUMS
	std::cerr << "Final post versions: " << std::endl;
#endif
	QMap<RsGxsMessageId,QVector<QPair<time_t,RsGxsMessageId> > > mTmp;
    std::map<RsGxsMessageId,RsGxsMessageId> most_recent_versions ;

    for(QMap<RsGxsMessageId,QVector<QPair<time_t,RsGxsMessageId> > >::iterator it(mPostVersions.begin());it!=mPostVersions.end();++it)
    {
#ifdef DEBUG_FORUMS
        std::cerr << "Original post: " << it.key() << std::endl;
#endif
        // Finally, sort the posts from newer to older

        qSort((*it).begin(),(*it).end(),decreasing_time_comp) ;

#ifdef DEBUG_FORUMS
		std::cerr << "   most recent version " << (*it)[0].first << "  " << (*it)[0].second << std::endl;
#endif
        for(int32_t i=1;i<(*it).size();++i)
        {
			msgs.erase((*it)[i].second) ;

#ifdef DEBUG_FORUMS
            std::cerr << "   older version " << (*it)[i].first << "  " << (*it)[i].second << std::endl;
#endif
        }

        mTmp[(*it)[0].second] = *it ;	// index the versions map by the ID of the most recent post.

		// Now make sure that message parents are consistent. Indeed, an old post may have the old version of a post as parent. So we need to change that parent
		// to the newest version. So we create a map of which is the most recent version of each message, so that parent messages can be searched in it.

        for(int i=1;i<(*it).size();++i)
            most_recent_versions[(*it)[i].second] = (*it)[0].second ;
    }
    mPostVersions = mTmp ;

    // The next step is to find the top level thread messages. These are defined as the messages without
    // any parent message ID.
    
    // this trick is needed because while we remove messages, the parents a given msg may already have been removed
    // and wrongly understand as a missing parent.

	std::map<RsGxsMessageId,RsGxsForumMsg> kept_msgs;

	for ( std::map<RsGxsMessageId,RsGxsForumMsg>::iterator msgIt = msgs.begin(); msgIt != msgs.end();++msgIt)
    {

        if(mFlatView || msgIt->second.mMeta.mParentId.isNull())
		{

			/* add all threads */
			if (wasStopped())
				return;

			const RsGxsForumMsg& msg = msgIt->second;

#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumsFillThread::run() Adding TopLevel Thread: mId: " << msg.mMeta.mMsgId << std::endl;
#endif

			QTreeWidgetItem *item = mParent->convertMsgToThreadWidget(msg, mUseChildTS, mFilterColumn,NULL);

			if (!mFlatView)
				threadStack.push_back(std::make_pair(msg.mMeta.mMsgId,item)) ;

			calculateExpand(msg, item);

			mItems.append(item);

			if (++step >= steps) {
				step = 0;
				emit progress(++pos, PROGRESSBAR_MAX);
			}
		}
		else
        {
#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumsFillThread::run() Storing kid " << msgIt->first << " of message " << msgIt->second.mMeta.mParentId << std::endl;
#endif
            // The same missing parent may appear multiple times, so we first store them into a unique container.

            RsGxsMessageId parent_msg = msgIt->second.mMeta.mParentId;

            if(msgs.find(parent_msg) == msgs.end())
            {
                // also check that the message is not versionned

                std::map<RsGxsMessageId,RsGxsMessageId>::const_iterator mrit = most_recent_versions.find(parent_msg) ;

                if(mrit != most_recent_versions.end())
                    parent_msg = mrit->second ;
                else
					missing_parents.insert(parent_msg);
            }

            kids_array[parent_msg].push_back(msgIt->first) ;
            kept_msgs.insert(*msgIt) ;
        }
    }

    msgs = kept_msgs;

    // Also create a list of posts by time, when they are new versions of existing posts. Only the last one will have an item created.

    // Add a fake toplevel item for the parent IDs that we dont actually have.

    for(std::set<RsGxsMessageId>::const_iterator it(missing_parents.begin());it!=missing_parents.end();++it)
	{
		// add dummy parent item
		QTreeWidgetItem *parent = mParent->generateMissingItem(*it);
		mItems.append( parent );

		threadStack.push_back(std::make_pair(*it,parent)) ;
	}
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::run() Processing stack:" << std::endl;
#endif
    // Now use a stack to go down the hierarchy

	while (!threadStack.empty())
    {
		std::pair<RsGxsMessageId, QTreeWidgetItem*> threadPair = threadStack.front();
		threadStack.pop_front();

        std::map<RsGxsMessageId, std::list<RsGxsMessageId> >::iterator it = kids_array.find(threadPair.first) ;

#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumsFillThread::run() Node: " << threadPair.first << std::endl;
#endif
        if(it == kids_array.end())
            continue ;

		if (wasStopped())
			return;

        for(std::list<RsGxsMessageId>::const_iterator it2(it->second.begin());it2!=it->second.end();++it2)
        {
            // We iterate through the top level thread items, and look for which message has the current item as parent.
            // When found, the item is put in the thread list itself, as a potential new parent.
            
            std::map<RsGxsMessageId,RsGxsForumMsg>::iterator mit = msgs.find(*it2) ;

            if(mit == msgs.end())
			{
				std::cerr << "GxsForumsFillThread::run()    Cannot find submessage " << *it2 << " !!!" << std::endl;
				continue ;
			}

            const RsGxsForumMsg& msg(mit->second) ;
#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumsFillThread::run()    adding sub_item " << msg.mMeta.mMsgId << std::endl;
#endif

			QTreeWidgetItem *item = mParent->convertMsgToThreadWidget(msg, mUseChildTS, mFilterColumn, threadPair.second);
			calculateExpand(msg, item);

			/* add item to process list */
			threadStack.push_back(std::make_pair(msg.mMeta.mMsgId, item));

			if (++step >= steps) {
				step = 0;
				emit progress(++pos, PROGRESSBAR_MAX);
			}

			msgs.erase(mit);
		}

#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumsFillThread::run() Erasing entry " << it->first << " from kids tab." << std::endl;
#endif
        kids_array.erase(it) ; // This is not strictly needed, but it improves performance by reducing the search space.
	}

#ifdef DEBUG_FORUMS
    std::cerr << "Kids array now has " << kids_array.size() << " elements" << std::endl;
    for(std::map<RsGxsMessageId,std::list<RsGxsMessageId> >::const_iterator it(kids_array.begin());it!=kids_array.end();++it)
    {
        std::cerr << "Node " << it->first << std::endl;
        for(std::list<RsGxsMessageId>::const_iterator it2(it->second.begin());it2!=it->second.end();++it2)
            std::cerr << "  " << *it2 << std::endl;
    }

	std::cerr << "GxsForumsFillThread::run() stopped: " << (wasStopped() ? "yes" : "no") << std::endl;
#endif
}


