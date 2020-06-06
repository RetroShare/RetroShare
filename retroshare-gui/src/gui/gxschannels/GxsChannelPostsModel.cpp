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

#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "GxsChannelPostsModel.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxschannels.h"
#include "retroshare/rsexpr.h"

//#define DEBUG_CHANNEL_MODEL

Q_DECLARE_METATYPE(RsMsgMetaData)

Q_DECLARE_METATYPE(RsGxsChannelPost)

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

const QString RsGxsChannelPostsModel::FilterString("filtered");

RsGxsChannelPostsModel::RsGxsChannelPostsModel(QObject *parent)
    : QAbstractItemModel(parent), mTreeMode(TREE_MODE_PLAIN), mColumns(6)
{
    initEmptyHierarchy(mPosts);
}

void RsGxsChannelPostsModel::initEmptyHierarchy(std::vector<RsGxsChannelPost>& posts)
{
    preMods();

    posts.resize(1);	// adds a sentinel item
    posts[0].mMeta.mMsgName = "Root sentinel post" ;

    postMods();
}

void RsGxsChannelPostsModel::preMods()
{
	//emit layoutAboutToBeChanged(); //Generate SIGSEGV when click on button move next/prev.

	beginResetModel();
}
void RsGxsChannelPostsModel::postMods()
{
	endResetModel();

	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mPosts.size(),mColumns-1,(void*)NULL));
}

void RsGxsChannelPostsModel::setTreeMode(TreeMode mode)
{
    if(mode == mTreeMode)
        return;

    preMods();

    // We're not removing/adding rows here. We're simply asking for re-draw.

	mTreeMode = mode;
    postMods();
}

#ifdef TODO
void RsGxsChannelPostsModel::setSortMode(SortMode mode)
{
    preMods();

    mSortMode = mode;

    postMods();
}

void RsGxsForumModel::initEmptyHierarchy(std::vector<ForumModelPostEntry>& posts)
{
    preMods();

    posts.resize(1);	// adds a sentinel item
    posts[0].mTitle = "Root sentinel post" ;
    posts[0].mParent = 0;

    postMods();
}
#endif

int RsGxsChannelPostsModel::rowCount(const QModelIndex& parent) const
{
    if(parent.column() > 0)
        return 0;

    if(mPosts.empty())	// security. Should never happen.
        return 0;

    if(!parent.isValid())
       return (getChildrenCount(0) + mColumns-1)/mColumns;

    RsErr() << __PRETTY_FUNCTION__ << " rowCount cannot figure out the porper number of rows." << std::endl;
    return 0;

    //else
    //   return getChildrenCount(parent.internalId());
}

int RsGxsChannelPostsModel::columnCount(const QModelIndex &/*parent*/) const
{
	return mColumns ;
}

// std::vector<std::pair<time_t,RsGxsMessageId> > RsGxsChannelPostsModel::getPostVersions(const RsGxsMessageId& mid) const
// {
//     auto it = mPostVersions.find(mid);
//
//     if(it != mPostVersions.end())
//         return it->second;
//     else
//         return std::vector<std::pair<time_t,RsGxsMessageId> >();
// }

bool RsGxsChannelPostsModel::getPostData(const QModelIndex& i,RsGxsChannelPost& fmpe) const
{
	if(!i.isValid())
        return true;

    quintptr ref = i.internalId();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
		return false ;

    fmpe = mPosts[entry];

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

	ref = (intptr_t)entry;

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
	entry = quintptr(val);

	return true;
}

QModelIndex RsGxsChannelPostsModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0 || column >= mColumns)
		return QModelIndex();

    quintptr ref = getChildRef(parent.internalId(),row + column*mColumns);

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

	ChannelPostsModelIndex entry ;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
		return 0 ;

	if(entry == 0)
	{
		quintptr new_ref;
		convertTabEntryToRefPointer(index+1,new_ref);
		return new_ref;
	}
	else
		return 0 ;
}

quintptr RsGxsChannelPostsModel::getParentRow(quintptr ref,int& row) const
{
	ChannelPostsModelIndex ref_entry;

	if(!convertRefPointerToTabEntry(ref,ref_entry) || ref_entry >= mPosts.size())
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

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
		return 0 ;

	if(entry == 0)
	{
#ifdef DEBUG_CHANNEL_MODEL
		std::cerr << "Children count (flat mode): " << mPosts.size()-1 << std::endl;
#endif
		return ((int)mPosts.size())-1;
	}
	else
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

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
	{
#ifdef DEBUG_CHANNEL_MODEL
		std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
		return QVariant() ;
	}

	const RsGxsChannelPost& fmpe(mPosts[entry]);

#ifdef TODO
    if(role == Qt::FontRole)
    {
        QFont font ;
		font.setBold( (fmpe.mPostFlags & (ForumModelPostEntry::FLAG_POST_HAS_UNREAD_CHILDREN | ForumModelPostEntry::FLAG_POST_IS_PINNED)) || IS_MSG_UNREAD(fmpe.mMsgStatus));
        return QVariant(font);
    }

    if(role == UnreadChildrenRole)
        return bool(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_HAS_UNREAD_CHILDREN);

#ifdef DEBUG_CHANNEL_MODEL
	std::cerr << " [ok]" << std::endl;
#endif
#endif

	switch(role)
	{
	case Qt::DisplayRole:    return displayRole   (fmpe,index.column()) ;
	case Qt::UserRole:	 	 return userRole      (fmpe,index.column()) ;
#ifdef TODO
	case Qt::DecorationRole: return decorationRole(fmpe,index.column()) ;
	case Qt::ToolTipRole:	 return toolTipRole   (fmpe,index.column()) ;
	case Qt::TextColorRole:  return textColorRole (fmpe,index.column()) ;
	case Qt::BackgroundRole: return backgroundRole(fmpe,index.column()) ;

	case FilterRole:         return filterRole    (fmpe,index.column()) ;
	case ThreadPinnedRole:   return pinnedRole    (fmpe,index.column()) ;
	case MissingRole:        return missingRole   (fmpe,index.column()) ;
	case StatusRole:         return statusRole    (fmpe,index.column()) ;
	case SortRole:           return sortRole      (fmpe,index.column()) ;
#endif
	default:
		return QVariant();
	}
}

#ifdef TODO
QVariant RsGxsForumModel::textColorRole(const ForumModelPostEntry& fmpe,int /*column*/) const
{
    if( (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_MISSING))
        return QVariant(mTextColorMissing);

    if(IS_MSG_UNREAD(fmpe.mMsgStatus) || (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED))
        return QVariant(mTextColorUnread);
    else
        return QVariant(mTextColorRead);

	return QVariant();
}

QVariant RsGxsForumModel::statusRole(const ForumModelPostEntry& fmpe,int column) const
{
 	if(column != COLUMN_THREAD_DATA)
        return QVariant();

    return QVariant(fmpe.mMsgStatus);
}

QVariant RsGxsForumModel::filterRole(const ForumModelPostEntry& fmpe,int /*column*/) const
{
    if(!mFilteringEnabled || (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_CHILDREN_PASSES_FILTER))
        return QVariant(FilterString);

	return QVariant(QString());
}

uint32_t RsGxsForumModel::recursUpdateFilterStatus(ForumModelIndex i,int column,const QStringList& strings)
{
    QString s ;
	uint32_t count = 0;

	switch(column)
	{
	default:
	case COLUMN_THREAD_DATE:
	case COLUMN_THREAD_TITLE: 	s = displayRole(mPosts[i],column).toString();
		break;
	case COLUMN_THREAD_AUTHOR:
	{
		QString comment ;
		QList<QIcon> icons;

		GxsIdDetails::MakeIdDesc(mPosts[i].mAuthorId, false,s, icons, comment,GxsIdDetails::ICON_TYPE_NONE);
	}
		break;
	}

	if(!strings.empty())
	{
		mPosts[i].mPostFlags &= ~(ForumModelPostEntry::FLAG_POST_PASSES_FILTER | ForumModelPostEntry::FLAG_POST_CHILDREN_PASSES_FILTER);

		for(auto iter(strings.begin()); iter != strings.end(); ++iter)
			if(s.contains(*iter,Qt::CaseInsensitive))
			{
				mPosts[i].mPostFlags |= ForumModelPostEntry::FLAG_POST_PASSES_FILTER | ForumModelPostEntry::FLAG_POST_CHILDREN_PASSES_FILTER;

				count++;
				break;
			}
	}
	else
	{
		mPosts[i].mPostFlags |= ForumModelPostEntry::FLAG_POST_PASSES_FILTER |ForumModelPostEntry::FLAG_POST_CHILDREN_PASSES_FILTER;
		count++;
	}

	for(uint32_t j=0;j<mPosts[i].mChildren.size();++j)
	{
		uint32_t tmp = recursUpdateFilterStatus(mPosts[i].mChildren[j],column,strings);
		count += tmp;

		if(tmp > 0)
			mPosts[i].mPostFlags |= ForumModelPostEntry::FLAG_POST_CHILDREN_PASSES_FILTER;
	}

	return count;
}


void RsGxsForumModel::setFilter(int column,const QStringList& strings,uint32_t& count)
{
    preMods();

    if(!strings.empty())
    {
		count = recursUpdateFilterStatus(ForumModelIndex(0),column,strings);
        mFilteringEnabled = true;
    }
    else
    {
		count=0;
        mFilteringEnabled = false;
    }

	postMods();
}

QVariant RsGxsForumModel::missingRole(const ForumModelPostEntry& fmpe,int /*column*/) const
{
    if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_MISSING)
        return QVariant(true);
    else
        return QVariant(false);
}

QVariant RsGxsForumModel::toolTipRole(const ForumModelPostEntry& fmpe,int column) const
{
    if(column == COLUMN_THREAD_DISTRIBUTION)
		switch(fmpe.mReputationWarningLevel)
		{
		case 3: return QVariant(tr("Information for this identity is currently missing.")) ;
		case 2: return QVariant(tr("You have banned this ID. The message will not be\ndisplayed nor forwarded to your friends.")) ;
		case 1: return QVariant(tr("You have not set an opinion for this person,\n and your friends do not vote positively: Spam regulation \nprevents the message to be forwarded to your friends.")) ;
		case 0: return QVariant(tr("Message will be forwarded to your friends.")) ;
		default:
			return QVariant("[ERROR: missing reputation level information - contact the developers]");
		}

    if(column == COLUMN_THREAD_AUTHOR)
	{
		QString str,comment ;
		QList<QIcon> icons;

		if(!GxsIdDetails::MakeIdDesc(fmpe.mAuthorId, true, str, icons, comment,GxsIdDetails::ICON_TYPE_AVATAR))
			return QVariant();

		int S = QFontMetricsF(QApplication::font()).height();
		QImage pix( (*icons.begin()).pixmap(QSize(4*S,4*S)).toImage());

		QString embeddedImage;
		if(RsHtml::makeEmbeddedImage(pix.scaled(QSize(4*S,4*S), Qt::KeepAspectRatio, Qt::SmoothTransformation), embeddedImage, 8*S * 8*S))
			comment = "<table><tr><td>" + embeddedImage + "</td><td>" + comment + "</td></table>";

		return comment;
	}

    return QVariant();
}

QVariant RsGxsForumModel::pinnedRole(const ForumModelPostEntry& fmpe,int /*column*/) const
{
    if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED)
        return QVariant(true);
    else
        return QVariant(false);
}

QVariant RsGxsForumModel::backgroundRole(const ForumModelPostEntry& fmpe,int /*column*/) const
{
    if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED)
        return QVariant(QBrush(QColor(255,200,180)));

    if(mFilteringEnabled && (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_PASSES_FILTER))
        return QVariant(QBrush(QColor(255,240,210)));

    return QVariant();
}
#endif

QVariant RsGxsChannelPostsModel::sizeHintRole(int col) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	return QVariant( QSize(factor * 170, factor*14 ));
#ifdef TODO
	switch(col)
	{
	default:
	case COLUMN_THREAD_TITLE:        return QVariant( QSize(factor * 170, factor*14 ));
	case COLUMN_THREAD_DATE:         return QVariant( QSize(factor * 75 , factor*14 ));
	case COLUMN_THREAD_AUTHOR:       return QVariant( QSize(factor * 75 , factor*14 ));
	case COLUMN_THREAD_DISTRIBUTION: return QVariant( QSize(factor * 15 , factor*14 ));
	}
#endif
}

#ifdef TODO
QVariant RsGxsForumModel::authorRole(const ForumModelPostEntry& fmpe,int column) const
{
    if(column == COLUMN_THREAD_DATA)
        return QVariant(QString::fromStdString(fmpe.mAuthorId.toStdString()));

    return QVariant();
}

QVariant RsGxsForumModel::sortRole(const ForumModelPostEntry& fmpe,int column) const
{
    switch(column)
    {
	case COLUMN_THREAD_DATE:         if(mSortMode == SORT_MODE_PUBLISH_TS)
            							return QVariant(QString::number(fmpe.mPublishTs)); // we should probably have leading zeroes here
        							 else
            							return QVariant(QString::number(fmpe.mMostRecentTsInThread)); // we should probably have leading zeroes here

	case COLUMN_THREAD_READ:         return QVariant((bool)IS_MSG_UNREAD(fmpe.mMsgStatus));
    case COLUMN_THREAD_DISTRIBUTION: return decorationRole(fmpe,column);
    case COLUMN_THREAD_AUTHOR:
    {
        QString str,comment ;
        QList<QIcon> icons;
		GxsIdDetails::MakeIdDesc(fmpe.mAuthorId, false, str, icons, comment,GxsIdDetails::ICON_TYPE_NONE);

        return QVariant(str);
    }
    default:
        return displayRole(fmpe,column);
    }
}
#endif

QVariant RsGxsChannelPostsModel::displayRole(const RsGxsChannelPost& fmpe,int col) const
{
	switch(col)
	{
    default:
    	return QString::fromUtf8(fmpe.mMeta.mMsgName.c_str());
#ifdef TODO
		case COLUMN_THREAD_TITLE:  if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_REDACTED)
									return QVariant(tr("[ ... Redacted message ... ]"));
								else if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED)
									return QVariant(tr("[PINNED] ") + QString::fromUtf8(fmpe.mTitle.c_str()));
								else
									return QVariant(QString::fromUtf8(fmpe.mTitle.c_str()));

		case COLUMN_THREAD_READ:return QVariant();
    	case COLUMN_THREAD_DATE:{
        							if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_MISSING)
                                        return QVariant(QString());

    							    QDateTime qtime;
									qtime.setTime_t(fmpe.mPublishTs);

									return QVariant(DateTime::formatDateTime(qtime));
    							}

		case COLUMN_THREAD_DISTRIBUTION:
		case COLUMN_THREAD_AUTHOR:{
			QString name;
			RsGxsId id = RsGxsId(fmpe.mAuthorId.toStdString());

			if(id.isNull())
				return QVariant(tr("[Notification]"));
			if(GxsIdTreeItemDelegate::computeName(id,name))
				return name;
			return QVariant(tr("[Unknown]"));
		}
		case COLUMN_THREAD_MSGID: return QVariant();
#endif
#ifdef TODO
	if (filterColumn == COLUMN_THREAD_CONTENT) {
		// need content for filter
		QTextDocument doc;
		doc.setHtml(QString::fromUtf8(msg.mMsg.c_str()));
		item->setText(COLUMN_THREAD_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
	}
#endif
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

#ifdef TODO
QVariant RsGxsForumModel::decorationRole(const ForumModelPostEntry& fmpe,int col) const
{
	bool exist=false;
	switch(col)
	{
		case COLUMN_THREAD_DISTRIBUTION:
		return QVariant(fmpe.mReputationWarningLevel);
		case COLUMN_THREAD_READ:
		return QVariant(fmpe.mMsgStatus);
		case COLUMN_THREAD_AUTHOR://Return icon as place holder.
		return FilesDefs::getIconFromGxsIdCache(RsGxsId(fmpe.mAuthorId.toStdString()),QIcon(), exist);
	}
	return QVariant();
}
#endif

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
    initEmptyHierarchy(mPosts);

	postMods();
	emit channelLoaded();
}

void RsGxsChannelPostsModel::setPosts(const RsGxsChannelGroup& group, std::vector<RsGxsChannelPost>& posts)
{
    preMods();

	beginRemoveRows(QModelIndex(),0,rowCount()-1);
    endRemoveRows();

    mPosts.clear();
    mChannelGroup = group;

    createPostsArray(posts);

#ifdef TODO
    recursUpdateReadStatusAndTimes(0,has_unread_below,has_read_below) ;
    recursUpdateFilterStatus(0,0,QStringList());
#endif

#ifdef DEBUG_CHANNEL_MODEL
   // debug_dump();
#endif

	beginInsertRows(QModelIndex(),0,rowCount()-1);
    endInsertRows();

	postMods();

	emit channelLoaded();
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

//ChannelPostsModelIndex RsGxsChannelPostsModel::addEntry(std::vector<ChannelPostsModelPostEntry>& posts,const ChannelPostsModelPostEntry& entry)
//{
//    uint32_t N = posts.size();
//    posts.push_back(entry);
//
//#ifdef DEBUG_FORUMMODEL
//    std::cerr << "Added new entry " << N << " children of " << parent << std::endl;
//#endif
//    if(N == parent)
//        std::cerr << "(EE) trying to add a post as its own parent!" << std::endl;
//
//    return ChannelPostsModelIndex(N);
//}

//void RsGxsChannelPostsModel::convertMsgToPostEntry(const RsGxsChannelGroup& mChannelGroup,const RsMsgMetaData& msg, bool /*useChildTS*/, ChannelPostsModelPostEntry& fentry)
//{
//    fentry.mTitle     = msg.mMsgName;
//    fentry.mMsgId     = msg.mMsgId;
//    fentry.mPublishTs = msg.mPublishTs;
//    fentry.mPostFlags = 0;
//    fentry.mMsgStatus = msg.mMsgStatus;
//
//	// Early check for a message that should be hidden because its author
//	// is flagged with a bad reputation
//}

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
        std::cerr << "  " << i << ": msg_id=" << posts[i].mMeta.mMsgId << ": orig msg id = " << posts[i].mMeta.mOrigMsgId << std::endl;
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
#ifdef DEBUG_CHANNEL_MODEL
		std::cerr << "  adding post: " << (*it).mMeta.mMsgId ;
#endif

        if(!(*it).mMeta.mMsgId.isNull())
		{
#ifdef DEBUG_CHANNEL_MODEL
            std::cerr << " added" << std::endl;
#endif
			mPosts.push_back(*it);
		}
#ifdef DEBUG_CHANNEL_MODEL
        else
            std::cerr << " skipped" << std::endl;
#endif
    }
}

void RsGxsChannelPostsModel::setMsgReadStatus(const QModelIndex& i,bool read_status,bool with_children)
{
	if(!i.isValid())
		return ;

	// no need to call preMods()/postMods() here because we'renot changing the model

	quintptr ref = i.internalId();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
		return ;

#warning TODO
//	bool has_unread_below, has_read_below;
//	recursSetMsgReadStatus(entry,read_status,with_children) ;
//	recursUpdateReadStatusAndTimes(0,has_unread_below,has_read_below);

}

#ifdef TODO
void RsGxsForumModel::recursSetMsgReadStatus(ForumModelIndex i,bool read_status,bool with_children)
{
	int newStatus = (read_status ? mPosts[i].mMsgStatus & ~static_cast<int>(GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD)
	                             : mPosts[i].mMsgStatus |  static_cast<int>(GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD));
	bool bChanged = (mPosts[i].mMsgStatus != newStatus);
	mPosts[i].mMsgStatus = newStatus;
	//Remove Unprocessed and New flags
	mPosts[i].mMsgStatus &= ~(GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_GUI_NEW);

	if (bChanged)
	{
		//Don't recurs post versions as this should be done before, if no change.
		uint32_t token;
		auto s = getPostVersions(mPosts[i].mMsgId) ;

		if(!s.empty())
			for(auto it(s.begin());it!=s.end();++it)
			{
				rsGxsForums->setMessageReadStatus(token,std::make_pair( mForumGroup.mMeta.mGroupId, it->second ), read_status);
				std::cerr << "Setting version " << it->second << " of post " << mPosts[i].mMsgId << " as read." << std::endl;
			}
		else
			rsGxsForums->setMessageReadStatus(token,std::make_pair( mForumGroup.mMeta.mGroupId, mPosts[i].mMsgId ), read_status);

		QModelIndex itemIndex = createIndex(i - 1, 0, &mPosts[i]);
		emit dataChanged(itemIndex, itemIndex);
	}

	if(!with_children)
		return;

	for(uint32_t j=0;j<mPosts[i].mChildren.size();++j)
		recursSetMsgReadStatus(mPosts[i].mChildren[j],read_status,with_children);
}

void RsGxsForumModel::recursUpdateReadStatusAndTimes(ForumModelIndex i,bool& has_unread_below,bool& has_read_below)
{
    has_unread_below =  IS_MSG_UNREAD(mPosts[i].mMsgStatus);
    has_read_below   = !IS_MSG_UNREAD(mPosts[i].mMsgStatus);

    mPosts[i].mMostRecentTsInThread = mPosts[i].mPublishTs;

    for(uint32_t j=0;j<mPosts[i].mChildren.size();++j)
    {
        bool ub,rb;

        recursUpdateReadStatusAndTimes(mPosts[i].mChildren[j],ub,rb);

        has_unread_below = has_unread_below || ub ;
        has_read_below   = has_read_below   || rb ;

		if(mPosts[i].mMostRecentTsInThread < mPosts[mPosts[i].mChildren[j]].mMostRecentTsInThread)
			mPosts[i].mMostRecentTsInThread = mPosts[mPosts[i].mChildren[j]].mMostRecentTsInThread;
    }

    if(has_unread_below)
		mPosts[i].mPostFlags |=  ForumModelPostEntry::FLAG_POST_HAS_UNREAD_CHILDREN;
    else
		mPosts[i].mPostFlags &= ~ForumModelPostEntry::FLAG_POST_HAS_UNREAD_CHILDREN;

    if(has_read_below)
		mPosts[i].mPostFlags |=  ForumModelPostEntry::FLAG_POST_HAS_READ_CHILDREN;
    else
		mPosts[i].mPostFlags &= ~ForumModelPostEntry::FLAG_POST_HAS_READ_CHILDREN;
}
#endif

QModelIndex RsGxsChannelPostsModel::getIndexOfMessage(const RsGxsMessageId& mid) const
{
    // Brutal search. This is not so nice, so dont call that in a loop! If too costly, we'll use a map.

    RsGxsMessageId postId = mid;

    for(uint32_t i=1;i<mPosts.size();++i)
    {
		// First look into msg versions, in case the msg is a version of an existing message

        for(auto& msg_id:mPosts[i].mOlderVersions)
            if(msg_id == postId)
            {
				quintptr ref ;
				convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

				return createIndex(i-1,0,ref);
            }

        if(mPosts[i].mMeta.mMsgId == postId)
        {
            quintptr ref ;
            convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

			return createIndex(i-1,0,ref);
        }
    }

    return QModelIndex();
}

#ifdef DEBUG_FORUMMODEL
static void recursPrintModel(const std::vector<ForumModelPostEntry>& entries,ForumModelIndex index,int depth)
{
    const ForumModelPostEntry& e(entries[index]);

	QDateTime qtime;
	qtime.setTime_t(e.mPublishTs);

    std::cerr << std::string(depth*2,' ') << index << " : " << e.mAuthorId.toStdString() << " "
              << QString("%1").arg((uint32_t)e.mPostFlags,8,16,QChar('0')).toStdString() << " "
              << QString("%1").arg((uint32_t)e.mMsgStatus,8,16,QChar('0')).toStdString() << " "
              << qtime.toString().toStdString() << " \"" << e.mTitle << "\"" << std::endl;

    for(uint32_t i=0;i<e.mChildren.size();++i)
        recursPrintModel(entries,e.mChildren[i],depth+1);
}

void RsGxsForumModel::debug_dump()
{
    std::cerr << "Model data dump:" << std::endl;
    std::cerr << "  Entries: " << mPosts.size() << std::endl;

    // non recursive print

    for(uint32_t i=0;i<mPosts.size();++i)
    {
		const ForumModelPostEntry& e(mPosts[i]);

		std::cerr << "    " << i << " : " << e.mMsgId << " (from " << e.mAuthorId.toStdString() << ") "
                  << QString("%1").arg((uint32_t)e.mPostFlags,8,16,QChar('0')).toStdString() << " "
                  << QString("%1").arg((uint32_t)e.mMsgStatus,8,16,QChar('0')).toStdString() << " ";

    	for(uint32_t i=0;i<e.mChildren.size();++i)
            std::cerr << " " << e.mChildren[i] ;

		QDateTime qtime;
		qtime.setTime_t(e.mPublishTs);

        std::cerr << " (" << e.mParent << ")";
		std::cerr << " " << qtime.toString().toStdString() << " \"" << e.mTitle << "\"" << std::endl;
    }

    // recursive print
    recursPrintModel(mPosts,ForumModelIndex(0),0);
}
#endif

#ifdef TODO
void RsGxsForumModel::setAuthorOpinion(const QModelIndex& indx, RsOpinion op)
{
	if(!indx.isValid())
		return ;

	void *ref = indx.internalPointer();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
		return ;

	std::cerr << "Setting own opinion for author " << mPosts[entry].mAuthorId
	          << " to " << static_cast<uint32_t>(op) << std::endl;
    RsGxsId author_id = mPosts[entry].mAuthorId;

	rsReputations->setOwnOpinion(author_id,op) ;

    // update opinions and distribution flags. No need to re-load all posts.

    for(uint32_t i=0;i<mPosts.size();++i)
    	if(mPosts[i].mAuthorId == author_id)
        {
			computeReputationLevel(mForumGroup.mMeta.mSignFlags,mPosts[i]);

			// notify the widgets that the data has changed.
			emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(0,COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
        }
}
#endif
