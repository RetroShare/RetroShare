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
#include "retroshare/rsexpr.h"

#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"

#include "PostedPostsModel.h"

//#define DEBUG_CHANNEL_MODEL

Q_DECLARE_METATYPE(RsMsgMetaData)
Q_DECLARE_METATYPE(RsPostedPost)

const uint32_t RsPostedPostsModel::DEFAULT_DISPLAYED_NB_POSTS = 10;

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

RsPostedPostsModel::RsPostedPostsModel(QObject *parent)
    : QAbstractItemModel(parent), mTreeMode(TREE_MODE_PLAIN)
{
	initEmptyHierarchy();

	mEventHandlerId = 0;
    mSortingStrategy = SORT_NEW_SCORE;

	// Needs to be asynced because this function is called by another thread!

	rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this );
    }, mEventHandlerId, RsEventType::GXS_POSTED);
}

RsPostedPostsModel::~RsPostedPostsModel()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
}

void RsPostedPostsModel::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	const RsGxsPostedEvent *e = dynamic_cast<const RsGxsPostedEvent*>(event.get());

	if(!e)
		return;

	switch(e->mPostedEventCode)
	{
		case RsPostedEventCode::UPDATED_MESSAGE:
		case RsPostedEventCode::READ_STATUS_CHANGED:
		case RsPostedEventCode::MESSAGE_VOTES_UPDATED:
		case RsPostedEventCode::NEW_MESSAGE:
		{
			// Normally we should just emit dataChanged() on the index of the data that has changed:
			//
			// We need to update the data!

			RsGxsPostedEvent E(*e);

			if(E.mPostedGroupId == mPostedGroup.mMeta.mGroupId)
				RsThread::async([this, E]()
				{
					// 1 - get message data from p3GxsChannels

					std::vector<RsPostedPost> posts;
					std::vector<RsGxsComment> comments;
					std::vector<RsGxsVote>    votes;

					if(!rsPosted->getBoardContent(mPostedGroup.mMeta.mGroupId,std::set<RsGxsMessageId>{ E.mPostedMsgId }, posts,comments,votes))
					{
						RS_ERR(" failed to retrieve channel message data for channel/msg ", E.mPostedGroupId, "/", E.mPostedMsgId);
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

									//emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mFilteredPosts.size(),0,(void*)NULL));

									preMods();
									postMods();
								}
						}
					},this);
				});

		}
		default:
		break;
	}
}

void RsPostedPostsModel::initEmptyHierarchy()
{
    preMods();

    mPosts.clear();
    mFilteredPosts.clear();
    mDisplayedNbPosts = DEFAULT_DISPLAYED_NB_POSTS;
    mDisplayedStartIndex = 0;

    postMods();
}

void RsPostedPostsModel::preMods()
{
	emit layoutAboutToBeChanged();
}
void RsPostedPostsModel::postMods()
{
	update();
	emit layoutChanged();
}
void RsPostedPostsModel::update()
{
	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mDisplayedNbPosts,0,(void*)NULL));
}
void RsPostedPostsModel::triggerRedraw()
{
	preMods();
	postMods();
}

void RsPostedPostsModel::setFilter(const QStringList& strings, uint32_t& count)
{
	preMods();

	beginResetModel();
	mFilteredPosts.clear();
	endResetModel();

	if(strings.empty())
	{
		for(int i=0;i<(int)(mPosts.size());++i)
			mFilteredPosts.push_back(i);
	}
	else
	{
		for(int i=0;i<static_cast<int>(mPosts.size());++i)
		{
			bool passes_strings = true;

			for(auto& s:strings)
			{
				if (s.startsWith("ID:",Qt::CaseInsensitive))
					passes_strings = passes_strings && mPosts[i].mMeta.mMsgId == RsGxsMessageId(s.right(s.length() - 3).toStdString());
				else
					passes_strings = passes_strings && QString::fromStdString(mPosts[i].mMeta.mMsgName).contains(s,Qt::CaseInsensitive);
			}

			if(passes_strings)
				mFilteredPosts.push_back(i);
		}
	}
	count = mFilteredPosts.size();

	mDisplayedStartIndex = 0;
	mDisplayedNbPosts = std::min(count,DEFAULT_DISPLAYED_NB_POSTS) ;

	std::cerr << "After filtering: " << count << " posts remain." << std::endl;

	if (rowCount()>0)
	{
		beginInsertRows(QModelIndex(),0,rowCount()-1);
		endInsertRows();
	}

	postMods();
}

int RsPostedPostsModel::rowCount(const QModelIndex& parent) const
{
    if(parent.column() > 0)
        return 0;

    if(mFilteredPosts.empty())  // rowCount is called by internal Qt so maybe before posts are populated.
        return 0;

    if(!parent.isValid())
        return mDisplayedNbPosts;

    RsErr() << __PRETTY_FUNCTION__ << " rowCount cannot figure out the porper number of rows." << std::endl;
    return 0;
}

int RsPostedPostsModel::columnCount(const QModelIndex &/*parent*/) const
{
	return 1;
}

bool RsPostedPostsModel::getPostData(const QModelIndex& i,RsPostedPost& fmpe) const
{
	if(!i.isValid())
        return true;

    quintptr ref = i.internalId();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry))
		return false ;

    fmpe = mPosts[mFilteredPosts[entry]];

	return true;

}

bool RsPostedPostsModel::hasChildren(const QModelIndex &parent) const
{
	if(!parent.isValid())
		return true;

	return false;	// by default, no post has children
}

bool RsPostedPostsModel::convertTabEntryToRefPointer(uint32_t entry,quintptr& ref)
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

bool RsPostedPostsModel::convertRefPointerToTabEntry(quintptr ref, uint32_t& entry)
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

QModelIndex RsPostedPostsModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0)
		return QModelIndex();

    quintptr ref = getChildRef(parent.internalId(),row);

#ifdef DEBUG_CHANNEL_MODEL
	std::cerr << "index-3(" << row << "," << column << " parent=" << parent << ") : " << createIndex(row,column,ref) << std::endl;
#endif
	return createIndex(row,column,ref) ;
}

QModelIndex RsPostedPostsModel::parent(const QModelIndex& index) const
{
	if(!index.isValid())
		return QModelIndex();

	return QModelIndex();	// there's no hierarchy here. So nothing to do!
}

quintptr RsPostedPostsModel::getChildRef(quintptr ref,int index) const
{
	if (index < 0)
		return 0;

	if(ref == quintptr(0))
	{
		quintptr new_ref;
		convertTabEntryToRefPointer(index+mDisplayedStartIndex,new_ref);
		return new_ref;
	}
	else
		return 0 ;
}

quintptr RsPostedPostsModel::getParentRow(quintptr ref,int& row) const
{
	PostedPostsModelIndex ref_entry;

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

int RsPostedPostsModel::getChildrenCount(quintptr ref) const
{
	//uint32_t entry = 0 ;

	if(ref == quintptr(0))
		return rowCount()-1;

	return 0;
}

QVariant RsPostedPostsModel::data(const QModelIndex& index, int role) const
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

	const RsPostedPost& fmpe(mPosts[mFilteredPosts[entry]]);

	switch(role)
	{
	case Qt::DisplayRole:    return displayRole   (fmpe,index.column()) ;
	case Qt::UserRole:	 	 return userRole      (fmpe,index.column()) ;
	default:
		return QVariant();
	}
}

QVariant RsPostedPostsModel::sizeHintRole(int /*col*/) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	return QVariant( QSize(factor * 170, factor*14 ));
}

QVariant RsPostedPostsModel::displayRole(const RsPostedPost& fmpe,int col) const
{
	switch(col)
	{
    default:
    	return QString::fromUtf8(fmpe.mMeta.mMsgName.c_str());
		}


	return QVariant("[ERROR]");
}

QVariant RsPostedPostsModel::userRole(const RsPostedPost& fmpe,int col) const
{
	switch(col)
    {
    default:
        return QVariant::fromValue(fmpe);
    }
}

const RsGxsGroupId& RsPostedPostsModel::currentGroupId() const
{
	return mPostedGroup.mMeta.mGroupId;
}

void RsPostedPostsModel::updateBoard(const RsGxsGroupId& board_group_id)
{
    if(board_group_id.isNull())
        return;

    update_posts(board_group_id);
}

void RsPostedPostsModel::clear()
{
    preMods();

    initEmptyHierarchy();

	postMods();
	emit boardPostsLoaded();
}

class PostSorter
{
public:

    PostSorter(RsPostedPostsModel::SortingStrategy s) : mSortingStrategy(s) {}

	bool operator()(const RsPostedPost& p1,const RsPostedPost& p2) const
	{
        switch(mSortingStrategy)
        {
        default:
        case RsPostedPostsModel::SORT_NEW_SCORE   :  return p1.mNewScore > p2.mNewScore;
        case RsPostedPostsModel::SORT_TOP_SCORE   :  return p1.mTopScore > p2.mTopScore;
        case RsPostedPostsModel::SORT_HOT_SCORE   :  return p1.mHotScore > p2.mHotScore;
        }
	}

private:
    RsPostedPostsModel::SortingStrategy mSortingStrategy;
};

Qt::ItemFlags RsPostedPostsModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::ItemFlags();

	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void RsPostedPostsModel::setSortingStrategy(RsPostedPostsModel::SortingStrategy s)
{
    preMods();

    mSortingStrategy = s;
    std::sort(mPosts.begin(),mPosts.end(), PostSorter(s));

	postMods();
}

void RsPostedPostsModel::setPostsInterval(int start,int nb_posts)
{
	if(start >= (int)mFilteredPosts.size())
		return;

	preMods();

	uint32_t old_nb_rows = rowCount() ;

	mDisplayedNbPosts = (uint32_t)std::min(nb_posts,(int)mFilteredPosts.size() - (start+1));
	mDisplayedStartIndex = start;

	beginRemoveRows(QModelIndex(),mDisplayedNbPosts,old_nb_rows);
	endRemoveRows();

	beginInsertRows(QModelIndex(),old_nb_rows,mDisplayedNbPosts);
	endInsertRows();

	postMods();
}

void RsPostedPostsModel::deepUpdate()
{
    auto posts(mPosts);
    setPosts(mPostedGroup,posts);
}

void RsPostedPostsModel::setPosts(const RsPostedGroup& group, std::vector<RsPostedPost>& posts)
{
	preMods();

	beginResetModel();

	mPosts.clear();
	mPostedGroup = group;

	endResetModel();

	createPostsArray(posts);

	std::sort(mPosts.begin(),mPosts.end(), PostSorter(mSortingStrategy));

	uint32_t tmpval;
	setFilter(QStringList(),tmpval);

	mDisplayedNbPosts = std::min((uint32_t)mFilteredPosts.size(),DEFAULT_DISPLAYED_NB_POSTS);
	mDisplayedStartIndex = 0;

	if (rowCount()>0)
	{
		beginInsertRows(QModelIndex(),0,rowCount()-1);
		endInsertRows();
	}

	postMods();

	emit boardPostsLoaded();
}

void RsPostedPostsModel::update_posts(const RsGxsGroupId& group_id)
{
	if(group_id.isNull())
		return;

	RsThread::async([this, group_id]()
	{
        // 1 - get message data from p3GxsChannels

        std::list<RsGxsGroupId> groupIds;
		std::vector<RsMsgMetaData> msg_metas;
		std::vector<RsPostedGroup> groups;

        groupIds.push_back(group_id);

		if(!rsPosted->getBoardsInfo(groupIds,groups) || groups.size() != 1)
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve channel group info for channel " << group_id << std::endl;
			return;
        }

        RsPostedGroup group = groups[0];

        // We use the heap because the arrays need to be stored accross async

		std::vector<RsPostedPost> *posts    = new std::vector<RsPostedPost>();
		std::vector<RsGxsComment> *comments = new std::vector<RsGxsComment>();
		std::vector<RsGxsVote>    *votes    = new std::vector<RsGxsVote>();

		if(!rsPosted->getBoardAllContent(group_id, *posts,*comments,*votes))
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

void RsPostedPostsModel::createPostsArray(std::vector<RsPostedPost>& posts)
{
#ifdef TODO
    // Collect new versions of posts if any. For now Boards do not have versions, but if that ever happens, we'll be handlign it already. This code
    // is a blind copy of the channel code.

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
#endif

#ifdef DEBUG_CHANNEL_MODEL
    std::cerr << "Now adding " << posts.size() << " posts into array structure..." << std::endl;
#endif

    mPosts.clear();

    for (std::vector<RsPostedPost>::const_reverse_iterator it = posts.rbegin(); it != posts.rend(); ++it)
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

void RsPostedPostsModel::setAllMsgReadStatus(bool read)
{
    // make a temporary listof pairs

    std::list<RsGxsGrpMsgIdPair> pairs;

    for(uint32_t i=0;i<mPosts.size();++i)
        pairs.push_back(RsGxsGrpMsgIdPair(mPosts[i].mMeta.mGroupId,mPosts[i].mMeta.mMsgId));

    RsThread::async([read,pairs]()
    {
        // Call blocking API

        for(auto& p:pairs)
            rsPosted->setPostReadStatus(p,read);
    } );
}
void RsPostedPostsModel::setMsgReadStatus(const QModelIndex& i,bool read_status)
{
	if(!i.isValid())
		return ;

	// no need to call preMods()/postMods() here because we'renot changing the model

	quintptr ref = i.internalId();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFilteredPosts.size())
		return ;

    RsGxsGroupId grpId = mPosts[entry].mMeta.mGroupId;
    RsGxsMessageId msgId = mPosts[entry].mMeta.mMsgId;

    bool current_read_status = !(IS_MSG_UNREAD(mPosts[entry].mMeta.mMsgStatus) || IS_MSG_NEW(mPosts[entry].mMeta.mMsgStatus));

    if(current_read_status != read_status)
        RsThread::async([msgId,grpId,read_status]()
        {
            // Call blocking API

            rsPosted->setPostReadStatus(RsGxsGrpMsgIdPair(grpId,msgId),read_status);
        } );
}

QModelIndex RsPostedPostsModel::getIndexOfMessage(const RsGxsMessageId& mid) const
{
	// Brutal search. This is not so nice, so dont call that in a loop! If too costly, we'll use a map.

	RsGxsMessageId postId = mid;

	for(uint32_t i=mDisplayedStartIndex;i<mDisplayedStartIndex+mDisplayedNbPosts;++i)
	{
		// First look into msg versions, in case the msg is a version of an existing message

#ifdef TODO
		for(auto& msg_id:mPosts[mFilteredPosts[i]].mOlderVersions)
			if(msg_id == postId)
			{
				quintptr ref ;
				convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

				return createIndex(i,0, ref);
			}
#endif

		if(   mFilteredPosts.size() > (size_t)(i)
		   && mPosts[mFilteredPosts[i]].mMeta.mMsgId == postId )
		{
			quintptr ref ;
			convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

			return createIndex(i,0, ref);
		}
	}

	return QModelIndex();
}

