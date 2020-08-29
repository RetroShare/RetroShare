/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostsModel.cpp                 *
 *                                                                             *
 * Copyright 2020 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include <QApplication>
#include <QFontMetrics>
#include <QModelIndex>
#include <QIcon>

#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxschannels.h"
#include "retroshare/rsexpr.h"

#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"

#include "GxsChannelPostsModel.h"
#include "GxsChannelPostFilesModel.h"

//#define DEBUG_CHANNEL_MODEL

Q_DECLARE_METATYPE(RsMsgMetaData)

Q_DECLARE_METATYPE(RsGxsChannelPost)

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

RsGxsChannelPostsModel::RsGxsChannelPostsModel(QObject *parent)
    : QAbstractItemModel(parent), mTreeMode(RsGxsChannelPostsModel::TREE_MODE_GRID), mColumns(6)
{
	initEmptyHierarchy();

	mEventHandlerId = 0;
	// Needs to be asynced because this function is called by another thread!

	rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
    }, mEventHandlerId, RsEventType::GXS_CHANNELS );
}

RsGxsChannelPostsModel::~RsGxsChannelPostsModel()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
}

void RsGxsChannelPostsModel::setMode(TreeMode mode)
{
    mTreeMode = mode;

    if(mode == TREE_MODE_LIST)
        setNumColumns(2);

    triggerViewUpdate();
}

void RsGxsChannelPostsModel::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	const RsGxsChannelEvent *e = dynamic_cast<const RsGxsChannelEvent*>(event.get());

	if(!e)
		return;

	switch(e->mChannelEventCode)
	{
	case RsChannelEventCode::UPDATED_MESSAGE:
	case RsChannelEventCode::READ_STATUS_CHANGED:
	{
		// Normally we should just emit dataChanged() on the index of the data that has changed:
		//
		// We need to update the data!

		if(e->mChannelGroupId == mChannelGroup.mMeta.mGroupId)
			RsThread::async([this, e]()
			{
				// 1 - get message data from p3GxsChannels

				std::vector<RsGxsChannelPost> posts;
				std::vector<RsGxsComment>     comments;
				std::vector<RsGxsVote>        votes;

                if(!rsGxsChannels->getChannelContent(mChannelGroup.mMeta.mGroupId,std::set<RsGxsMessageId>{ e->mChannelMsgId }, posts,comments,votes))
				{
					std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve channel message data for channel/msg " << e->mChannelGroupId << "/" << e->mChannelMsgId << std::endl;
					return;
				}

				// 2 - update the model in the UI thread.

				RsQThreadUtils::postToObject( [posts,comments,votes,this]()
				{
					for(uint32_t i=0;i<posts.size();++i)
					{
						// linear search. Not good at all, but normally this is for a single post.

						for(uint32_t j=0;j<mPosts.size();++j)
							if(mPosts[j].mMeta.mMsgId == posts[i].mMeta.mMsgId)
							{
								mPosts[j] = posts[i];

                                triggerViewUpdate();
							}
					}
				},this);
            });

	default:
			break;
		}
	}
}

void RsGxsChannelPostsModel::initEmptyHierarchy()
{
    preMods();

    mPosts.clear();
    mFilteredPosts.clear();

    postMods();
}

void RsGxsChannelPostsModel::preMods()
{
	beginResetModel();
}
void RsGxsChannelPostsModel::postMods()
{
	endResetModel();

    triggerViewUpdate();
}
void RsGxsChannelPostsModel::triggerViewUpdate()
{
    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(rowCount()-1,mColumns-1,(void*)NULL));
}

void RsGxsChannelPostsModel::getFilesList(std::list<ChannelPostFileInfo>& files)
{
    // We use an intermediate map so as to remove duplicates

    std::map<RsFileHash,ChannelPostFileInfo> files_map;

    for(uint32_t i=0;i<mPosts.size();++i)
        for(auto& file:mPosts[i].mFiles)
            files_map.insert(std::make_pair(file.mHash,ChannelPostFileInfo(file,mPosts[i].mMeta.mPublishTs)));

    files.clear();

    for(auto& it:files_map)
        files.push_back(it.second);
}

void RsGxsChannelPostsModel::setFilter(const QStringList& strings,bool only_unread, uint32_t& count)
{
    preMods();

	beginRemoveRows(QModelIndex(),0,rowCount()-1);
    endRemoveRows();

    mFilteredPosts.clear();
    //mFilteredPosts.push_back(0);

    for(size_t i=0;i<mPosts.size();++i)
    {
        bool passes_strings = true;

        for(auto& s:strings)
            passes_strings = passes_strings && QString::fromStdString(mPosts[i].mMeta.mMsgName).contains(s,Qt::CaseInsensitive);

        if(strings.empty())
            passes_strings = true;

        if(passes_strings && (!only_unread || (IS_MSG_UNREAD(mPosts[i].mMeta.mMsgStatus) || IS_MSG_NEW(mPosts[i].mMeta.mMsgStatus))))
            mFilteredPosts.push_back(i);
    }

    count = mFilteredPosts.size();

    std::cerr << "After filtering: " << count << " posts remain." << std::endl;

	beginInsertRows(QModelIndex(),0,rowCount()-1);
    endInsertRows();

	postMods();
}

int RsGxsChannelPostsModel::rowCount(const QModelIndex& parent) const
{
    if(parent.column() > 0)
        return 0;

    if(mFilteredPosts.empty())	// security. Should never happen.
        return 0;

    if(!parent.isValid())
        if(mTreeMode == TREE_MODE_GRID)
            return (mFilteredPosts.size() + mColumns-1)/mColumns; // mFilteredPosts always has an item at 0, so size()>=1, and mColumn>=1
        else
            return mFilteredPosts.size();

    RsErr() << __PRETTY_FUNCTION__ << " rowCount cannot figure out the porper number of rows." << std::endl;
    return 0;
}

int RsGxsChannelPostsModel::columnCount(const QModelIndex &/*parent*/) const
{
    if(mTreeMode == TREE_MODE_GRID)
        return std::min((int)mFilteredPosts.size(),(int)mColumns) ;
    else
        return 2;
}

bool RsGxsChannelPostsModel::getPostData(const QModelIndex& i,RsGxsChannelPost& fmpe) const
{
	if(!i.isValid())
        return true;

    quintptr ref = i.internalId();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFilteredPosts.size())
		return false ;

    fmpe = mPosts[mFilteredPosts[entry]];

	return true;

}

bool RsGxsChannelPostsModel::hasChildren(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return true;

	return false;	// by default, no channel post has children
}

bool RsGxsChannelPostsModel::convertTabEntryToRefPointer(uint32_t entry,quintptr& ref)
{
	// the pointer is formed the following way:
	//
	//		[ 32 bits ]
	//
	// This means that the whole software has the following build-in limitation:
	//	  * 4 B   simultaenous posts. Should be enough !

	ref = (intptr_t)(entry+1);

	return true;
}

bool RsGxsChannelPostsModel::convertRefPointerToTabEntry(quintptr ref, uint32_t& entry)
{
    intptr_t val = (intptr_t)ref;

    if(val > (1<<30))	// make sure the pointer is an int that fits in 32bits and not too big which would look suspicious
    {
        RsErr() << "(EE) trying to make a ChannelPostsModelIndex out of a number that is larger than 2^32-1 !" << std::endl;
        return false ;
    }
    if(val==0)
    {
        RsErr() << "(EE) trying to make a ChannelPostsModelIndex out of index 0." << std::endl;
        return false;
    }

	entry = val - 1;

	return true;
}

QModelIndex RsGxsChannelPostsModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0 || column >= (int)mColumns)
		return QModelIndex();

    quintptr ref = getChildRef(parent.internalId(),(mTreeMode == TREE_MODE_GRID)?(column + row*mColumns):row);

#ifdef DEBUG_CHANNEL_MODEL
	std::cerr << "index-3(" << row << "," << column << " parent=" << parent << ") : " << createIndex(row,column,ref) << std::endl;
#endif
	return createIndex(row,column,ref) ;
}

QModelIndex RsGxsChannelPostsModel::parent(const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

	return QModelIndex();	// there's no hierarchy here. So nothing to do!
}

Qt::ItemFlags RsGxsChannelPostsModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

void RsGxsChannelPostsModel::setNumColumns(int n)
{
    if(n < 1)
    {
        RsErr() << __PRETTY_FUNCTION__ << " Attempt to set a number of column of 0. This is wrong." << std::endl;
        return;
    }
	preMods();

	beginRemoveRows(QModelIndex(),0,rowCount()-1);
    endRemoveRows();

    mColumns = n;

	beginInsertRows(QModelIndex(),0,rowCount()-1);
    endInsertRows();

	postMods();
}

quintptr RsGxsChannelPostsModel::getChildRef(quintptr ref,int index) const
{
	if (index < 0)
		return 0;

	if(ref == quintptr(0))
	{
		quintptr new_ref;
		convertTabEntryToRefPointer(index,new_ref);
		return new_ref;
	}
	else
		return 0 ;
}

quintptr RsGxsChannelPostsModel::getParentRow(quintptr ref,int& row) const
{
	ChannelPostsModelIndex ref_entry;

	if(!convertRefPointerToTabEntry(ref,ref_entry) || ref_entry >= mFilteredPosts.size())
		return 0 ;

	if(ref_entry == 0)
	{
		RsErr() << "getParentRow() shouldn't be asked for the parent of NULL" << std::endl;
		row = 0;
	}
	else
		row = ref_entry-1;

	return 0;
}

int RsGxsChannelPostsModel::getChildrenCount(quintptr ref) const
{
	uint32_t entry = 0 ;

    if(ref == quintptr(0))
        return rowCount()-1;

	return 0;
}

QVariant RsGxsChannelPostsModel::data(const QModelIndex &index, int role) const
{
#ifdef DEBUG_CHANNEL_MODEL
    std::cerr << "calling data(" << index << ") role=" << role << std::endl;
#endif

	if(!index.isValid())
		return QVariant();

	switch(role)
	{
	case Qt::SizeHintRole: return sizeHintRole(index.column()) ;
    case Qt::StatusTipRole:return QVariant();
    default: break;
	}

	quintptr ref = (index.isValid())?index.internalId():0 ;
	uint32_t entry = 0;

#ifdef DEBUG_CHANNEL_MODEL
	std::cerr << "data(" << index << ")" ;
#endif

	if(!ref)
	{
#ifdef DEBUG_CHANNEL_MODEL
		std::cerr << " [empty]" << std::endl;
#endif
		return QVariant() ;
	}

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFilteredPosts.size())
	{
#ifdef DEBUG_CHANNEL_MODEL
		std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
		return QVariant() ;
	}

	const RsGxsChannelPost& fmpe(mPosts[mFilteredPosts[entry]]);

	switch(role)
	{
	case Qt::DisplayRole:    return displayRole   (fmpe,index.column()) ;
	case Qt::UserRole:	 	 return userRole      (fmpe,index.column()) ;
	default:
		return QVariant();
	}
}

QVariant RsGxsChannelPostsModel::sizeHintRole(int col) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	return QVariant( QSize(factor * 170, factor*14 ));
}

QVariant RsGxsChannelPostsModel::displayRole(const RsGxsChannelPost& fmpe,int col) const
{
	switch(col)
	{
    default:
    	return QString::fromUtf8(fmpe.mMeta.mMsgName.c_str());
		}


	return QVariant("[ERROR]");
}

QVariant RsGxsChannelPostsModel::userRole(const RsGxsChannelPost& fmpe,int col) const
{
	switch(col)
    {
    default:
        return QVariant::fromValue(fmpe);
    }
}

const RsGxsGroupId& RsGxsChannelPostsModel::currentGroupId() const
{
	return mChannelGroup.mMeta.mGroupId;
}

void RsGxsChannelPostsModel::updateChannel(const RsGxsGroupId& channel_group_id)
{
    if(channel_group_id.isNull())
        return;

    update_posts(channel_group_id);
}

void RsGxsChannelPostsModel::clear()
{
    preMods();

    mPosts.clear();
    initEmptyHierarchy();

	postMods();
	emit channelPostsLoaded();
}

bool operator<(const RsGxsChannelPost& p1,const RsGxsChannelPost& p2)
{
    return p1.mMeta.mPublishTs > p2.mMeta.mPublishTs;
}

void RsGxsChannelPostsModel::setPosts(const RsGxsChannelGroup& group, std::vector<RsGxsChannelPost>& posts)
{
    preMods();

	beginRemoveRows(QModelIndex(),0,rowCount()-1);
    endRemoveRows();

    mPosts.clear();
    mChannelGroup = group;

    createPostsArray(posts);

    std::sort(mPosts.begin(),mPosts.end());

    mFilteredPosts.clear();
    for(int i=0;i<mPosts.size();++i)
        mFilteredPosts.push_back(i);

#ifdef DEBUG_CHANNEL_MODEL
   // debug_dump();
#endif

	beginInsertRows(QModelIndex(),0,rowCount()-1);
    endInsertRows();

	postMods();

	emit channelPostsLoaded();
}

void RsGxsChannelPostsModel::update_posts(const RsGxsGroupId& group_id)
{
    if(group_id.isNull())
        return;

	RsThread::async([this, group_id]()
	{
        // 1 - get message data from p3GxsChannels

        std::list<RsGxsGroupId> channelIds;
		std::vector<RsMsgMetaData> msg_metas;
		std::vector<RsGxsChannelGroup> groups;

        channelIds.push_back(group_id);

		if(!rsGxsChannels->getChannelsInfo(channelIds,groups) || groups.size() != 1)
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve channel group info for channel " << group_id << std::endl;
			return;
        }

        RsGxsChannelGroup group = groups[0];

        // We use the heap because the arrays need to be stored accross async

		std::vector<RsGxsChannelPost> *posts    = new std::vector<RsGxsChannelPost>();
		std::vector<RsGxsComment>     *comments = new std::vector<RsGxsComment>();
		std::vector<RsGxsVote>        *votes    = new std::vector<RsGxsVote>();

		if(!rsGxsChannels->getChannelAllContent(group_id, *posts,*comments,*votes))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve channel messages for channel " << group_id << std::endl;
			return;
		}

        // 2 - update the model in the UI thread.

        RsQThreadUtils::postToObject( [group,posts,comments,votes,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

            setPosts(group,*posts) ;

            delete posts;
            delete comments;
            delete votes;

		}, this );

    });
}

static bool decreasing_time_comp(const std::pair<time_t,RsGxsMessageId>& e1,const std::pair<time_t,RsGxsMessageId>& e2) { return e2.first < e1.first ; }

void RsGxsChannelPostsModel::createPostsArray(std::vector<RsGxsChannelPost>& posts)
{
    // collect new versions of posts if any

#ifdef DEBUG_CHANNEL_MODEL
    std::cerr << "Inserting channel posts" << std::endl;
#endif

    std::vector<uint32_t> new_versions ;
    for (uint32_t i=0;i<posts.size();++i)
    {
		if(posts[i].mMeta.mOrigMsgId == posts[i].mMeta.mMsgId)
			posts[i].mMeta.mOrigMsgId.clear();

#ifdef DEBUG_CHANNEL_MODEL
        std::cerr << "  " << i << ": name=\"" << posts[i].mMeta.mMsgName << "\" msg_id=" << posts[i].mMeta.mMsgId << ": orig msg id = " << posts[i].mMeta.mOrigMsgId << std::endl;
#endif

        if(!posts[i].mMeta.mOrigMsgId.isNull())
            new_versions.push_back(i) ;
    }

#ifdef DEBUG_CHANNEL_MODEL
    std::cerr << "New versions: " << new_versions.size() << std::endl;
#endif

    if(!new_versions.empty())
    {
#ifdef DEBUG_CHANNEL_MODEL
        std::cerr << "  New versions present. Replacing them..." << std::endl;
        std::cerr << "  Creating search map."  << std::endl;
#endif

        // make a quick search map
        std::map<RsGxsMessageId,uint32_t> search_map ;
		for (uint32_t i=0;i<posts.size();++i)
            search_map[posts[i].mMeta.mMsgId] = i ;

        for(uint32_t i=0;i<new_versions.size();++i)
        {
#ifdef DEBUG_CHANNEL_MODEL
            std::cerr << "  Taking care of new version  at index " << new_versions[i] << std::endl;
#endif

            uint32_t current_index = new_versions[i] ;
            uint32_t source_index  = new_versions[i] ;
#ifdef DEBUG_CHANNEL_MODEL
            RsGxsMessageId source_msg_id = posts[source_index].mMeta.mMsgId ;
#endif

            // What we do is everytime we find a replacement post, we climb up the replacement graph until we find the original post
            // (or the most recent version of it). When we reach this post, we replace it with the data of the source post.
            // In the mean time, all other posts have their MsgId cleared, so that the posts are removed from the list.

            //std::vector<uint32_t> versions ;
            std::map<RsGxsMessageId,uint32_t>::const_iterator vit ;

            while(search_map.end() != (vit=search_map.find(posts[current_index].mMeta.mOrigMsgId)))
            {
#ifdef DEBUG_CHANNEL_MODEL
                std::cerr << "    post at index " << current_index << " replaces a post at position " << vit->second ;
#endif

				// Now replace the post only if the new versionis more recent. It may happen indeed that the same post has been corrected multiple
				// times. In this case, we only need to replace the post with the newest version

				//uint32_t prev_index = current_index ;
				current_index = vit->second ;

				if(posts[current_index].mMeta.mMsgId.isNull())	// This handles the branching situation where this post has been already erased. No need to go down further.
                {
#ifdef DEBUG_CHANNEL_MODEL
                    std::cerr << "  already erased. Stopping." << std::endl;
#endif
                    break ;
                }

				if(posts[current_index].mMeta.mPublishTs < posts[source_index].mMeta.mPublishTs)
				{
#ifdef DEBUG_CHANNEL_MODEL
                    std::cerr << " and is more recent => following" << std::endl;
#endif
                    for(std::set<RsGxsMessageId>::const_iterator itt(posts[current_index].mOlderVersions.begin());itt!=posts[current_index].mOlderVersions.end();++itt)
						posts[source_index].mOlderVersions.insert(*itt);

					posts[source_index].mOlderVersions.insert(posts[current_index].mMeta.mMsgId);
					posts[current_index].mMeta.mMsgId.clear();	    // clear the msg Id so the post will be ignored
				}
#ifdef DEBUG_CHANNEL_MODEL
                else
                    std::cerr << " but is older -> Stopping" << std::endl;
#endif
            }
        }
    }

#ifdef DEBUG_CHANNEL_MODEL
    std::cerr << "Now adding " << posts.size() << " posts into array structure..." << std::endl;
#endif

    mPosts.clear();

    for (std::vector<RsGxsChannelPost>::const_reverse_iterator it = posts.rbegin(); it != posts.rend(); ++it)
    {
        if(!(*it).mMeta.mMsgId.isNull())
		{
#ifdef DEBUG_CHANNEL_MODEL
            std::cerr << " adding post \"" << (*it).mMeta.mMsgName << "\"" << std::endl;
#endif
			mPosts.push_back(*it);
		}
#ifdef DEBUG_CHANNEL_MODEL
        else
            std::cerr << " skipped older version post \"" << (*it).mMeta.mMsgName << "\"" << std::endl;
#endif
    }
}

void RsGxsChannelPostsModel::setAllMsgReadStatus(bool read_status)
{
    // No need to call preMods()/postMods() here because we're not changing the model
    // All operations below are done async

    RsThread::async([this, read_status]()
    {
        for(uint32_t i=0;i<mPosts.size();++i)
            rsGxsChannels->markRead(RsGxsGrpMsgIdPair(mPosts[i].mMeta.mGroupId,mPosts[i].mMeta.mMsgId),read_status);
    });
}

void RsGxsChannelPostsModel::setMsgReadStatus(const QModelIndex& i,bool read_status)
{
	if(!i.isValid())
		return ;

	// no need to call preMods()/postMods() here because we'renot changing the model

	quintptr ref = i.internalId();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFilteredPosts.size())
		return ;

    rsGxsChannels->markRead(RsGxsGrpMsgIdPair(mPosts[mFilteredPosts[entry]].mMeta.mGroupId,mPosts[mFilteredPosts[entry]].mMeta.mMsgId),read_status);
}

QModelIndex RsGxsChannelPostsModel::getIndexOfMessage(const RsGxsMessageId& mid) const
{
    // Brutal search. This is not so nice, so dont call that in a loop! If too costly, we'll use a map.

    RsGxsMessageId postId = mid;

    for(uint32_t i=0;i<mFilteredPosts.size();++i)
    {
        // First look into msg versions, in case the msg is a version of an existing message

        for(auto& msg_id:mPosts[mFilteredPosts[i]].mOlderVersions)
            if(msg_id == postId)
            {
                quintptr ref ;
                convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

                if(mTreeMode == TREE_MODE_GRID)
                    return createIndex(i/mColumns,i%mColumns, ref);
                else
                    return createIndex(i,0, ref);
            }

        if(mPosts[mFilteredPosts[i]].mMeta.mMsgId == postId)
        {
            quintptr ref ;
            convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

            if(mTreeMode == TREE_MODE_GRID)
                return createIndex(i/mColumns,i%mColumns, ref);
            else
                return createIndex(i,0, ref);
        }
    }

    return QModelIndex();
}

