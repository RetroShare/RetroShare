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
#include <chrono>

#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxschannels.h"
#include "retroshare/rsexpr.h"

#include "gui/MainWindow.h"
#include "gui/mainpagestack.h"
#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"

#include "GxsChannelPostsModel.h"
#include "GxsChannelPostFilesModel.h"

//#define DEBUG_CHANNEL_MODEL_DATA
//#define DEBUG_CHANNEL_MODEL
//#define GXSPROFILING

Q_DECLARE_METATYPE(RsMsgMetaData)
Q_DECLARE_METATYPE(RsGxsChannelPost)

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

RsGxsChannelPostsModel::RsGxsChannelPostsModel(QObject *parent)
    : QAbstractItemModel(parent), mTreeMode(RsGxsChannelPostsModel::TREE_MODE_GRID), mColumns(6)
{
	initEmptyHierarchy();
}

RsGxsChannelPostsModel::~RsGxsChannelPostsModel()
{
//    rsEvents->unregisterEventsHandler(mEventHandlerId);
}

void RsGxsChannelPostsModel::setMode(TreeMode mode)
{
    mTreeMode = mode;

    if(mode == TREE_MODE_LIST)
        setNumColumns(2);

    triggerViewUpdate(true,true);
}



void RsGxsChannelPostsModel::initEmptyHierarchy()
{
	beginResetModel();

	mPosts.clear();
	mFilteredPosts.clear();

	endResetModel();
}

void RsGxsChannelPostsModel::preMods()
{
	emit layoutAboutToBeChanged();
}
void RsGxsChannelPostsModel::postMods()
{
	emit layoutChanged();
}
void RsGxsChannelPostsModel::triggerViewUpdate(bool data_changed, bool layout_changed)
{
    if(data_changed)
        emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(rowCount()-1,mColumns-1,(void*)NULL));

    if(layout_changed)
        emit layoutChanged();
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

bool RsGxsChannelPostsModel::postPassesFilter(const RsGxsChannelPost& post,const QStringList& strings,bool only_unread) const
{
    bool passes_strings = true;

    for(auto& s:strings)
        passes_strings = passes_strings && QString::fromStdString(post.mMeta.mMsgName).contains(s,Qt::CaseInsensitive);

    if(strings.empty())
        passes_strings = true;

    if(passes_strings && (!only_unread || (IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus))))
        return true;

    return false;
}

void RsGxsChannelPostsModel::setFilter(const QStringList& strings,bool only_unread, uint32_t& count)
{
    mFilteredStrings = strings;
    mFilterUnread = only_unread;

    updateFilter(count);
}

void RsGxsChannelPostsModel::updateFilter(uint32_t& count)
{
	preMods();

	mFilteredPosts.clear();

    for(size_t i=0;i<mPosts.size();++i)
        if(postPassesFilter(mPosts[i],mFilteredStrings,mFilterUnread))
                mFilteredPosts.push_back(i);

    count = mFilteredPosts.size();

	if (rowCount()>0)
	{
		beginInsertRows(QModelIndex(),0,rowCount()-1);
		endInsertRows();
	}

	postMods();
}

int RsGxsChannelPostsModel::rowCount(const QModelIndex& parent) const
{
    if(parent.column() > 0)
        return 0;

    if(mFilteredPosts.empty())	// security. Should never happen.
        return 0;

    if(!parent.isValid())
    {
        if(mTreeMode == TREE_MODE_GRID)
            return (mFilteredPosts.size() + mColumns-1)/mColumns; // mFilteredPosts always has an item at 0, so size()>=1, and mColumn>=1
        else
            return mFilteredPosts.size();
    }

    RsErr() << __PRETTY_FUNCTION__ << " rowCount cannot figure out the proper number of rows." ;
    return 0;
}

int RsGxsChannelPostsModel::columnCount(int row) const
{
    if(mTreeMode == TREE_MODE_GRID)
    {
        if(row+1 == rowCount())
        {
            int r = ((int)mFilteredPosts.size() % (int)mColumns);

            if(r > 0)
                return r;
            else
                return columnCount();
        }
        else
            return columnCount();
    }
    else
        return 2;
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
    if(row < 0 || column < 0 || row >= rowCount() || column >= columnCount(row))
		return QModelIndex();

    quintptr ref = getChildRef(parent.internalId(),(mTreeMode == TREE_MODE_GRID)?(column + row*mColumns):row);

#ifdef DEBUG_CHANNEL_MODEL_DATA
	std::cerr << "index-3(" << row << "," << column << " parent=" << parent << ") : " << createIndex(row,column,ref) << std::endl;
#endif
	return createIndex(row,column,ref) ;
}

QModelIndex RsGxsChannelPostsModel::parent(const QModelIndex& /*index*/) const
{
	return QModelIndex();	// there's no hierarchy here. So nothing to do!
}

Qt::ItemFlags RsGxsChannelPostsModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return QAbstractItemModel::flags(index);
}

bool RsGxsChannelPostsModel::setNumColumns(int n)
{
	if(n < 1)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Attempt to set a number of column of 0. This is wrong." << std::endl;
		return false;
	}
	if((int)mColumns == n)
		return false;

	preMods();

	mColumns = n;

	postMods();

	return true;
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
	if(ref == quintptr(0))
		return rowCount()-1;

	return 0;
}

QVariant RsGxsChannelPostsModel::data(const QModelIndex &index, int role) const
{
#ifdef DEBUG_CHANNEL_MODEL_DATA
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

#ifdef DEBUG_CHANNEL_MODEL_DATA
	std::cerr << "data(" << index << ")" ;
#endif

	if(!ref)
	{
#ifdef DEBUG_CHANNEL_MODEL_DATA
		std::cerr << " [empty]" << std::endl;
#endif
		return QVariant() ;
	}

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFilteredPosts.size())
	{
#ifdef DEBUG_CHANNEL_MODEL_DATA
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

QVariant RsGxsChannelPostsModel::sizeHintRole(int /* col */) const
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

	initEmptyHierarchy();

	postMods();
	emit channelPostsLoaded();
}

bool operator<(const RsGxsChannelPost& p1,const RsGxsChannelPost& p2)
{
    return p1.mMeta.mPublishTs > p2.mMeta.mPublishTs;
}

void RsGxsChannelPostsModel::updateSinglePost(const RsGxsChannelPost& post,std::set<RsGxsFile>& added_files,std::set<RsGxsFile>& removed_files)
{
#ifdef DEBUG_CHANNEL_MODEL
    RsDbg() << "updating single post for group id=" << currentGroupId() << " and msg id=" << post.mMeta.mMsgId ;
#endif
    added_files.clear();
    removed_files.clear();

    emit layoutAboutToBeChanged();

    // linear search. Not good at all, but normally this is just for a single post.

    bool found = false;
    const auto& new_post_meta(post.mMeta);

    for(uint32_t j=0;j<mPosts.size();++j)
        if(new_post_meta.mMsgId == mPosts[j].mMeta.mMsgId)	// same post updated
        {
            added_files.insert(post.mFiles.begin(),post.mFiles.end());
            removed_files.insert(mPosts[j].mFiles.begin(),mPosts[j].mFiles.end());

            auto save_ucc = mPosts[j].mUnreadCommentCount;
            auto save_cc  = mPosts[j].mCommentCount;

            mPosts[j] = post;

            mPosts[j].mUnreadCommentCount = save_ucc;
            mPosts[j].mCommentCount = save_cc;

#ifdef DEBUG_CHANNEL_MODEL
            RsDbg() << "  post is an updated existing post." ;
#endif
            found=true;
            break;
        }
        else if( (new_post_meta.mOrigMsgId == mPosts[j].mMeta.mOrigMsgId || new_post_meta.mOrigMsgId == mPosts[j].mMeta.mMsgId)
                 && mPosts[j].mMeta.mPublishTs < new_post_meta.mPublishTs)	// new post version
        {
            added_files.insert(post.mFiles.begin(),post.mFiles.end());
            removed_files.insert(mPosts[j].mFiles.begin(),mPosts[j].mFiles.end());

            auto old_post_id = mPosts[j].mMeta.mMsgId;
            auto save_ucc = mPosts[j].mUnreadCommentCount;
            auto save_cc  = mPosts[j].mCommentCount;

            mPosts[j] = post;

            mPosts[j].mCommentCount += save_cc;
            mPosts[j].mUnreadCommentCount += save_ucc;
            mPosts[j].mOlderVersions.insert(old_post_id);
#ifdef DEBUG_CHANNEL_MODEL
            RsDbg() << "  post is an new version of an existing post." ;
#endif
            found=true;
            break;
        }

    if(!found)
    {
#ifdef DEBUG_CHANNEL_MODEL
        RsDbg() << "  post is an new post.";
#endif
        added_files.insert(post.mFiles.begin(),post.mFiles.end());
        mPosts.push_back(post);
    }
    std::sort(mPosts.begin(),mPosts.end());

    uint32_t count;
    updateFilter(count);

    triggerViewUpdate(true,false);
}

void RsGxsChannelPostsModel::setPosts(const RsGxsChannelGroup& group, std::vector<RsGxsChannelPost>& posts)
{
#ifdef GXSPROFILING
    // Start timer to measure UI thread work
    auto startTime = std::chrono::steady_clock::now();
#endif

    preMods();

    initEmptyHierarchy();
    mChannelGroup = group;

    mPosts = posts;

    // Sorting can be expensive on the UI thread for large amounts of data
    std::sort(mPosts.begin(), mPosts.end());

    mFilteredPosts.clear();
    for(uint32_t i=0; i<mPosts.size(); ++i)
        mFilteredPosts.push_back(i);

    if (rowCount() > 0)
    {
        beginInsertRows(QModelIndex(), 0, rowCount() - 1);
        endInsertRows();
    }

    postMods();

#ifdef GXSPROFILING
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    RsDbg() << "GXSPROFILING [UI-MODEL]: setPosts (UI Thread sorting/reset) took: " << duration << " ms for " << mPosts.size() << " posts";
#endif

    emit channelPostsLoaded();
}

void RsGxsChannelPostsModel::update_posts(const RsGxsGroupId& group_id)
{
    if(group_id.isNull())
        return;

#ifdef GXSPROFILING
    // Start global timer for the loading process
    auto totalRequestStart = std::chrono::steady_clock::now();

    RsDbg() << "GXSPROFILING [UI-LOAD]: Starting update_posts for group: " << group_id;
#endif

    MainWindow::getPage(MainWindow::Channels)->setCursor(Qt::WaitCursor);

    RsThread::async([this, group_id
#ifdef GXSPROFILING
                     , totalRequestStart
#endif
                    ]()
    {
#ifdef GXSPROFILING
        auto fetchStart = std::chrono::steady_clock::now();
#endif

        std::list<RsGxsGroupId> channelIds;
        std::vector<RsGxsChannelGroup> groups;

        channelIds.push_back(group_id);

        if(!rsGxsChannels->getChannelsInfo(channelIds,groups) || groups.size() != 1)
        {
            RsErr() << "DEBUG [UI-LOAD]: Failed to retrieve channel group info for " << group_id << std::endl;
            return;
        }

        RsGxsChannelGroup group = groups[0];

        std::vector<RsGxsChannelPost> *posts = new std::vector<RsGxsChannelPost>();
        std::vector<RsGxsComment> comments;
        std::vector<RsGxsVote> votes;

        // The following call is the heavy database operation
        if(!rsGxsChannels->getChannelAllContent(group_id, *posts, comments, votes))
        {
            RsErr() << "DEBUG [UI-LOAD]: Failed to retrieve channel content for " << group_id << std::endl;
            delete posts;
            return;
        }

#ifdef GXSPROFILING
        auto fetchEnd = std::chrono::steady_clock::now();
        auto fetchMs = std::chrono::duration_cast<std::chrono::milliseconds>(fetchEnd - fetchStart).count();

        RsDbg() << "GXSPROFILING [UI-LOAD]: Backend fetch finished. Items: " << posts->size() 
                << " posts. Time: " << fetchMs << " ms";
#endif

        // Return to UI thread for model update
        RsQThreadUtils::postToObject( [group, posts, this
#ifdef GXSPROFILING
                                       , totalRequestStart
#endif
                                      ]()
        {
#ifdef GXSPROFILING
            auto uiUpdateStart = std::chrono::steady_clock::now();
#endif

            setPosts(group, *posts);

#ifdef GXSPROFILING
            auto now = std::chrono::steady_clock::now();
            auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - totalRequestStart).count();
            auto uiDispatchMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - uiUpdateStart).count();

            RsDbg() << "GXSPROFILING [UI-LOAD]: UI model update finished. Dispatch/Update time: " << uiDispatchMs << " ms";
            RsDbg() << "GXSPROFILING [UI-LOAD]: TOTAL LOADING TIME (User perceived): " << totalMs << " ms";
#endif

            delete posts;
            MainWindow::getPage(MainWindow::Channels)->setCursor(Qt::ArrowCursor);

        }, this );
    });
}

void RsGxsChannelPostsModel::setAllMsgReadStatus(bool read_status)
{
#ifdef GXSPROFILING
    // Start timer to measure how long the UI thread is occupied
    auto startTime = std::chrono::steady_clock::now();

    RsDbg() << "GXSPROFILING [UI-CHANNELS]: Starting setAllMsgReadStatus for " << mPosts.size() << " messages";
#endif

    // 1 - copy all msg/grp id groups
    // This part is done on the UI thread and can be slow if mPosts is large
    std::vector<RsGxsGrpMsgIdPair> pairs;

    for(uint32_t i=0; i<mPosts.size(); ++i)
    {
        bool post_status = !((IS_MSG_UNREAD(mPosts[i].mMeta.mMsgStatus) || IS_MSG_NEW(mPosts[i].mMeta.mMsgStatus)));

        if(post_status != read_status)
            pairs.push_back(RsGxsGrpMsgIdPair(mPosts[i].mMeta.mGroupId, mPosts[i].mMeta.mMsgId));
    }

#ifdef GXSPROFILING
    RsDbg() << "GXSPROFILING [UI-CHANNELS]: Found " << pairs.size() << " messages requiring status change";
#endif

    // 2 - then call the async methods
    // We are launching N threads. The overhead of creating these threads 
    // stays on the UI thread and contributes to the freeze.
    for(uint32_t i=0; i<pairs.size(); ++i)
    {
        RsThread::async([p=pairs[i], read_status]()
        {
            if(!rsGxsChannels->setMessageReadStatus(p, read_status))
                RsErr() << "setAllMsgReadStatus: failed to change status of msg " << p.first << " in group " << p.second << " to status " << read_status << std::endl;
        });

#ifdef GXSPROFILING
        if (i > 0 && i % 1000 == 0) {
            RsDbg() << "GXSPROFILING [UI-CHANNELS]: Launched " << i << " async threads...";
        }
#endif
    }

    // 3 - update the local model data
    // This is the visual update part
    for(uint32_t i=0; i<mPosts.size(); ++i)
    {
        if(read_status)
            mPosts[i].mMeta.mMsgStatus &= ~(GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD | GXS_SERV::GXS_MSG_STATUS_GUI_NEW);
        else
            mPosts[i].mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD ;
    }

    // Calculate total time spent in this function before returning control to the UI
#ifdef GXSPROFILING
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    RsDbg() << "GXSPROFILING [UI-CHANNELS]: Finished setAllMsgReadStatus";
    RsDbg() << "GXSPROFILING [UI-CHANNELS]: TOTAL UI THREAD FREEZE: " << duration << " ms";
#endif

    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(rowCount()-1, mColumns-1, (void*)NULL));
}

void RsGxsChannelPostsModel::updatePostWithNewComment(const RsGxsMessageId& msg_id)
{
    for(uint32_t i=0;i<mPosts.size();++i)
        if(mPosts[i].mMeta.mMsgId == msg_id)
        {
            ++mPosts[i].mUnreadCommentCount;
            emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(rowCount()-1,mColumns-1,(void*)NULL));	// update everything because we don't know the index.
            break;
        }
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

    rsGxsChannels->setMessageReadStatus(RsGxsGrpMsgIdPair(mPosts[mFilteredPosts[entry]].mMeta.mGroupId,mPosts[mFilteredPosts[entry]].mMeta.mMsgId),read_status);

    // Quick update to the msg itself. Normally setMsgReadStatus will launch an event,
    // that we can catch to update the msg, but all the information is already here.

    if(read_status)
        mPosts[mFilteredPosts[entry]].mMeta.mMsgStatus &= ~(GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD | GXS_SERV::GXS_MSG_STATUS_GUI_NEW);
    else
        mPosts[mFilteredPosts[entry]].mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;

    mPosts[mFilteredPosts[entry]].mUnreadCommentCount = 0;

    emit dataChanged(i,i);
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

