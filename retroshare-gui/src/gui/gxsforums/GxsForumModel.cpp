/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/GxsForumModel.cpp                          *
 *                                                                             *
 * Copyright 2018 by Cyril Soler <csoler@users.sourceforge.net>                *
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
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "GxsForumModel.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxsforums.h"
#include "retroshare/rsexpr.h"

//#define DEBUG_FORUMMODEL

Q_DECLARE_METATYPE(RsMsgMetaData);

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

const QString RsGxsForumModel::FilterString("filtered");

RsGxsForumModel::RsGxsForumModel(QObject *parent)
    : QAbstractItemModel(parent), mUseChildTS(false),mFilteringEnabled(false),mTreeMode(TREE_MODE_TREE)
{
    initEmptyHierarchy(mPosts);
    mFont = QApplication::font();
}

void RsGxsForumModel::preMods()
{
	emit layoutAboutToBeChanged();
}
void RsGxsForumModel::postMods()
{
	if(mTreeMode == TREE_MODE_FLAT)
		emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mPosts.size(),COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
	else
		emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mPosts[0].mChildren.size(),COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));

	emit layoutChanged();
}

void RsGxsForumModel::setTreeMode(TreeMode mode)
{
	if(mode == mTreeMode)
		return;

	preMods();

	beginResetModel();

	mTreeMode = mode;

	endResetModel();

	postMods();
}

void RsGxsForumModel::setSortMode(SortMode mode)
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

int RsGxsForumModel::rowCount(const QModelIndex& parent) const
{
    if(parent.column() > 0)
        return 0;

    if(mPosts.empty())	// security. Should never happen.
        return 0;

    if(!parent.isValid())
       return getChildrenCount(NULL);
    else
       return getChildrenCount(parent.internalPointer());
}

int RsGxsForumModel::columnCount(const QModelIndex &/*parent*/) const
{
	return COLUMN_THREAD_NB_COLUMNS ;
}

std::vector<std::pair<rstime_t,RsGxsMessageId> > RsGxsForumModel::getPostVersions(const RsGxsMessageId& mid) const
{
    auto it = mPostVersions.find(mid);

    if(it != mPostVersions.end())
        return it->second;
    else
        return std::vector<std::pair<rstime_t,RsGxsMessageId> >();
}

bool RsGxsForumModel::getPostData(const QModelIndex& i,ForumModelPostEntry& fmpe) const
{
	if(!i.isValid())
        return true;

    void *ref = i.internalPointer();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
		return false ;

    fmpe = mPosts[entry];

	return true;

}

bool RsGxsForumModel::hasChildren(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return true;

    if(mTreeMode == TREE_MODE_FLAT)
        return false;

    void *ref = parent.internalPointer();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << "hasChildren-2(" << parent << ") : " << false << std::endl;
#endif
		return false ;
	}

#ifdef DEBUG_FORUMMODEL
    std::cerr << "hasChildren-3(" << parent << ") : " << !mPosts[entry].mChildren.empty() << std::endl;
#endif
	return !mPosts[entry].mChildren.empty();
}

bool RsGxsForumModel::convertTabEntryToRefPointer(uint32_t entry,void *& ref)
{
	// the pointer is formed the following way:
	//
	//		[ 32 bits ]
	//
	// This means that the whole software has the following build-in limitation:
	//	  * 4 B   simultaenous posts. Should be enough !

	ref = reinterpret_cast<void*>( (intptr_t)entry );

	return true;
}

bool RsGxsForumModel::convertRefPointerToTabEntry(void *ref,uint32_t& entry)
{
    intptr_t val = (intptr_t)ref;

    if(val > (1<<30))	// make sure the pointer is an int that fits in 32bits and not too big which would look suspicious
    {
        std::cerr << "(EE) trying to make a ForumModelIndex out of a number that is larger than 2^32-1 !" << std::endl;
        return false ;
    }
	entry = uint32_t(val);

	return true;
}

QModelIndex RsGxsForumModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0 || column >= COLUMN_THREAD_NB_COLUMNS)
		return QModelIndex();

    void *ref = getChildRef(parent.internalPointer(),row);
#ifdef DEBUG_FORUMMODEL
	std::cerr << "index-3(" << row << "," << column << " parent=" << parent << ") : " << createIndex(row,column,ref) << std::endl;
#endif
	return createIndex(row,column,ref) ;
}

QModelIndex RsGxsForumModel::parent(const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

    if(mTreeMode == TREE_MODE_FLAT)
        return QModelIndex();

    void *child_ref = index.internalPointer();
    int row=0;

    void *parent_ref = getParentRef(child_ref,row) ;

    if(parent_ref == NULL)		// root
        return QModelIndex() ;

    return createIndex(row,0,parent_ref);
}

Qt::ItemFlags RsGxsForumModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return QAbstractItemModel::flags(index);
}

void *RsGxsForumModel::getChildRef(void *ref,int row) const
{
	if (row < 0)
		return nullptr;

    ForumModelIndex entry ;

    if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
        return NULL ;

    void *new_ref;

	if(mTreeMode == TREE_MODE_FLAT)
	{
		if(entry == 0)
		{
			convertTabEntryToRefPointer(row+1,new_ref);
			return new_ref;
		}
		else
		{
			return NULL ;
		}
	}

    if(static_cast<size_t>(row) >= mPosts[entry].mChildren.size())
        return NULL;

    convertTabEntryToRefPointer(mPosts[entry].mChildren[row],new_ref);

    return new_ref;
}

void *RsGxsForumModel::getParentRef(void *ref,int& row) const
{
    ForumModelIndex ref_entry;

    if(!convertRefPointerToTabEntry(ref,ref_entry) || ref_entry >= mPosts.size())
        return NULL ;

    if(mTreeMode == TREE_MODE_FLAT)
    {
        if(ref_entry == 0)
        {
            RsErr() << "getParentRef() shouldn't be asked for the parent of NULL" << std::endl;
            row = 0;
        }
        else
			row = ref_entry-1;

        return NULL;
    }

    ForumModelIndex parent_entry = mPosts[ref_entry].mParent;

    if(parent_entry == 0)		// top level index
    {
        row = 0;
        return NULL ;
    }
    else
    {
        void *parent_ref;
        convertTabEntryToRefPointer(parent_entry,parent_ref);
        row = mPosts[parent_entry].prow;

        return parent_ref;
    }
}

int RsGxsForumModel::getChildrenCount(void *ref) const
{
    uint32_t entry = 0 ;

    if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
        return 0 ;

    if(mTreeMode == TREE_MODE_FLAT)
        if(entry == 0)
        {
#ifdef DEBUG_FORUMMODEL
            std::cerr << "Children count (flat mode): " << mPosts.size()-1 << std::endl;
#endif
			return ((int)mPosts.size())-1;
        }
		else
            return 0;
    else
    {
#ifdef DEBUG_FORUMMODEL
		std::cerr << "Children count (tree mode): " << mPosts[entry].mChildren.size() << std::endl;
#endif
		return mPosts[entry].mChildren.size();
    }
}

QVariant RsGxsForumModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if(role == Qt::DisplayRole)
		switch(section)
		{
		case COLUMN_THREAD_TITLE:        return tr("Title");
		case COLUMN_THREAD_READ:         return tr("UnRead");
		case COLUMN_THREAD_DATE:         return tr("Date");
		case COLUMN_THREAD_AUTHOR:       return tr("Author");
		default:
			return QVariant();
		}

	if(role == Qt::DecorationRole)
		switch(section)
		{
			case COLUMN_THREAD_DISTRIBUTION: return FilesDefs::getIconFromQtResourcePath(":/icons/flag-green.png");
			default:
			return QVariant();
		}

	return QVariant();
}

QVariant RsGxsForumModel::data(const QModelIndex &index, int role) const
{
#ifdef DEBUG_FORUMMODEL
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

	void *ref = (index.isValid())?index.internalPointer():NULL ;
	uint32_t entry = 0;

#ifdef DEBUG_FORUMMODEL
	std::cerr << "data(" << index << ")" ;
#endif

	if(!ref)
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << " [empty]" << std::endl;
#endif
		return QVariant() ;
	}

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
		return QVariant() ;
	}

	const ForumModelPostEntry& fmpe(mPosts[entry]);

    if(role == Qt::FontRole)
    {
        QFont font = mFont;
        font.setBold( (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_HAS_UNREAD_CHILDREN) || IS_MSG_UNREAD(fmpe.mMsgStatus));
        return QVariant(font);
    }

    if(role == UnreadChildrenRole)
        return bool(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_HAS_UNREAD_CHILDREN);

#ifdef DEBUG_FORUMMODEL
	std::cerr << " [ok]" << std::endl;
#endif

	switch(role)
	{
	case Qt::DisplayRole:    return displayRole   (fmpe,index.column()) ;
	case Qt::DecorationRole: return decorationRole(fmpe,index.column()) ;
	case Qt::ToolTipRole:	 return toolTipRole   (fmpe,index.column()) ;
	case Qt::UserRole:	 	 return userRole      (fmpe,index.column()) ;
	case Qt::TextColorRole:  return textColorRole (fmpe,index.column()) ;
	case Qt::BackgroundRole: return backgroundRole(fmpe,index.column()) ;

	case FilterRole:         return filterRole    (fmpe,index.column()) ;
	case ThreadPinnedRole:   return pinnedRole    (fmpe,index.column()) ;
	case MissingRole:        return missingRole   (fmpe,index.column()) ;
	case StatusRole:         return statusRole    (fmpe,index.column()) ;
	case SortRole:           return sortRole      (fmpe,index.column()) ;
	default:
		return QVariant();
	}
}

QVariant RsGxsForumModel::textColorRole(const ForumModelPostEntry& fmpe,int /*column*/) const
{
    if( (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_MISSING))
        return QVariant(mTextColorMissing);

    if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED)
        return QVariant(mTextColorPinned);

    if(IS_MSG_UNREAD(fmpe.mMsgStatus))
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

void RsGxsForumModel::setFont(const QFont &font)
{
    preMods();

    mFont = font;

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

		int S = QFontMetricsF(mFont).height();
		QImage pix( (*icons.begin()).pixmap(QSize(5*S,5*S)).toImage());

		QString embeddedImage;
        if(RsHtml::makeEmbeddedImage(pix.scaled(QSize(5*S,5*S), Qt::KeepAspectRatio, Qt::SmoothTransformation), embeddedImage, -1))
		{
			embeddedImage.insert(embeddedImage.indexOf("src="), "style=\"float:left\" ");
			comment = "<table><tr><td>" + embeddedImage + "</td><td>" + comment + "</td></table>";
		}

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
//    if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED)
//        return QVariant(QBrush(mBackgroundColorPinned));

    if(mFilteringEnabled && (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_PASSES_FILTER))
        return QVariant(QBrush(mBackgroundColorFiltered));

    return QVariant();
}

QVariant RsGxsForumModel::sizeHintRole(int col) const
{
	float factor = QFontMetricsF(mFont).height()/14.0f ;

	switch(col)
	{
	default:
	case COLUMN_THREAD_TITLE:        return QVariant( QSize(factor * 170, factor*14 ));
	case COLUMN_THREAD_DATE:         return QVariant( QSize(factor * 75 , factor*14 ));
	case COLUMN_THREAD_AUTHOR:       return QVariant( QSize(factor * 75 , factor*14 ));
	case COLUMN_THREAD_DISTRIBUTION: return QVariant( QSize(factor * 15 , factor*14 ));
	}
}

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

QVariant RsGxsForumModel::displayRole(const ForumModelPostEntry& fmpe,int col) const
{
	switch(col)
	{
		case COLUMN_THREAD_TITLE:  if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_REDACTED)
									return QVariant(tr("[ ... Redacted message ... ]"));
//                                else if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED)
//                                    return QVariant( QString("<img src=\":/icons/pinned_64.png\" height=%1/>").arg(QFontMetricsF(mFont).height())
//                                                     + QString::fromUtf8(fmpe.mTitle.c_str()));
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

		case COLUMN_THREAD_DISTRIBUTION:	// passthrough // handled by delegate.
		case COLUMN_THREAD_MSGID:
        								return QVariant();
		case COLUMN_THREAD_AUTHOR:
    	{
			QString name;
			RsGxsId id = RsGxsId(fmpe.mAuthorId.toStdString());

			if(id.isNull())
				return QVariant(tr("[Notification]"));
			if(GxsIdTreeItemDelegate::computeName(id,name))
				return name;
			return QVariant(tr("[Unknown]"));
		}
#ifdef TODO
	if (filterColumn == COLUMN_THREAD_CONTENT) {
		// need content for filter
		QTextDocument doc;
		doc.setHtml(QString::fromUtf8(msg.mMsg.c_str()));
		item->setText(COLUMN_THREAD_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
	}
#endif
		default:
			return QVariant("[ TODO ]");
		}


	return QVariant("[ERROR]");
}

QVariant RsGxsForumModel::userRole(const ForumModelPostEntry& fmpe,int col) const
{
	switch(col)
    {
     	case COLUMN_THREAD_AUTHOR:   return QVariant(QString::fromStdString(fmpe.mAuthorId.toStdString()));
     	case COLUMN_THREAD_MSGID:    return QVariant(QString::fromStdString(fmpe.mMsgId.toStdString()));
    default:
        return QVariant();
    }
}

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

const RsGxsGroupId& RsGxsForumModel::currentGroupId() const
{
	return mForumGroup.mMeta.mGroupId;
}

void RsGxsForumModel::updateForum(const RsGxsGroupId& forum_group_id)
{
    if(forum_group_id.isNull())
        return;

    update_posts(forum_group_id);
}

void RsGxsForumModel::clear()
{
    preMods();

    mPosts.clear();
    mPostVersions.clear();

	postMods();
	emit forumLoaded();
}

void RsGxsForumModel::setPosts(const RsGxsForumGroup& group, const std::vector<ForumModelPostEntry>& posts,const std::map<RsGxsMessageId,std::vector<std::pair<rstime_t,RsGxsMessageId> > >& post_versions)
{
	preMods();

	beginResetModel();
	endResetModel();

	mForumGroup = group;
	mPosts = posts;
	mPostVersions = post_versions;

	// now update prow for all posts

	for(uint32_t i=0;i<mPosts.size();++i)
		for(uint32_t j=0;j<mPosts[i].mChildren.size();++j)
			mPosts[mPosts[i].mChildren[j]].prow = j;

	mPosts[0].prow = 0;

	bool has_unread_below,has_read_below ;

	recursUpdateReadStatusAndTimes(0,has_unread_below,has_read_below) ;
	recursUpdateFilterStatus(0,0,QStringList());

#ifdef DEBUG_FORUMMODEL
	debug_dump();
#endif

	int count = 0;
	if(mTreeMode == TREE_MODE_FLAT)
		count = mPosts.size();
	else
		count = mPosts[0].mChildren.size();

	if(count>0)
	{
		beginInsertRows(QModelIndex(),0,count-1);
		endInsertRows();
	}

	postMods();
	emit forumLoaded();
}

void RsGxsForumModel::update_posts(const RsGxsGroupId& group_id)
{
	if(group_id.isNull())
		return;

	RsThread::async([this, group_id]()
	{
		// 1 - get message data from p3GxsForums

		std::list<RsGxsGroupId> forumIds;
		std::vector<RsGxsForumGroup> groups;

		forumIds.push_back(group_id);

		if(!rsGxsForums->getForumsInfo(forumIds,groups) || groups.size() != 1)
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve forum group info for forum " << group_id << std::endl;
			return;
		}

        // 2 - sort messages into a proper hierarchy

		auto post_versions = new std::map<RsGxsMessageId,std::vector<std::pair<rstime_t, RsGxsMessageId> > >() ;
        std::vector<ForumPostEntry> *vect = new std::vector<ForumPostEntry>();
		RsGxsForumGroup group = groups[0];

        if(!rsGxsForums->getForumPostsHierarchy(group,*vect,*post_versions))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve forum hierarchy of message info for forum " << group_id << std::endl;
            return;
        }

        // 3 - update the model in the UI thread.

		RsQThreadUtils::postToObject( [group,vect,post_versions,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

            std::vector<ForumModelPostEntry> psts;
            for(const auto& p:*vect)
                psts.push_back( ForumModelPostEntry(p));

            setPosts(group,psts,*post_versions) ;

            delete vect;
            delete post_versions;


		}, this );

	});
}



void RsGxsForumModel::setMsgReadStatus(const QModelIndex& i,bool read_status,bool with_children)
{
	if(!i.isValid())
		return ;

	// no need to call preMods()/postMods() here because we'renot changing the model

	void *ref = i.internalPointer();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
		return ;

	bool has_unread_below, has_read_below;
	recursSetMsgReadStatus(entry,read_status,with_children) ;
	recursUpdateReadStatusAndTimes(0,has_unread_below,has_read_below);

    // also emit dataChanged() for parents since they need to re-draw

    for(QModelIndex j = i.parent(); j.isValid(); j=j.parent())
    {
        emit dataChanged(j,j);
        j = j.parent();
    }
}

void RsGxsForumModel::recursSetMsgReadStatus(ForumModelIndex i,bool read_status,bool with_children)
{
    uint32_t newStatus = (read_status ? mPosts[i].mMsgStatus & ~static_cast<int>(GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD)
                                      : mPosts[i].mMsgStatus |  static_cast<int>(GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD));
	bool bChanged = (mPosts[i].mMsgStatus != newStatus);
	mPosts[i].mMsgStatus = newStatus;
	//Remove Unprocessed and New flags
	mPosts[i].mMsgStatus &= ~(GXS_SERV::GXS_MSG_STATUS_UNPROCESSED | GXS_SERV::GXS_MSG_STATUS_GUI_NEW);

	if (bChanged)
	{
		//Don't recurs post versions as this should be done before, if no change.
		auto s = getPostVersions(mPosts[i].mMsgId) ;

		if(!s.empty())
			for(auto it(s.begin());it!=s.end();++it)
			{
                RsThread::async( [grpId=mForumGroup.mMeta.mGroupId,msgId=it->second,original_msg_id=mPosts[i].mMsgId,read_status]()
                {
                    rsGxsForums->markRead(std::make_pair( grpId, msgId ), read_status);
                    std::cerr << "Setting version " << msgId << " of post " << original_msg_id << " as read." << std::endl;
                });
			}
		else
            RsThread::async( [grpId=mForumGroup.mMeta.mGroupId,original_msg_id=mPosts[i].mMsgId,read_status]()
            {
                rsGxsForums->markRead(std::make_pair( grpId, original_msg_id), read_status);
            });

        void *ref ;
        convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

        QModelIndex itemIndex = (mTreeMode == TREE_MODE_FLAT)?createIndex(i - 1, 0, ref):createIndex(mPosts[i].prow,0,ref);
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

QModelIndex RsGxsForumModel::getIndexOfMessage(const RsGxsMessageId& mid) const
{
    // Brutal search. This is not so nice, so dont call that in a loop! If too costly, we'll use a map.

    RsGxsMessageId postId = mid;

    // First look into msg versions, in case the msg is a version of an existing message

    for(auto it(mPostVersions.begin());it!=mPostVersions.end() && postId==mid;++it)
    	for(uint32_t i=0;i<it->second.size();++i)
            if(it->second[i].second == mid)
            {
                postId = it->first;
                break;
            }

    for(uint32_t i=1;i<mPosts.size();++i)
        if(mPosts[i].mMsgId == postId)
        {
            void *ref ;
            convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

            if(mTreeMode == TREE_MODE_FLAT)
				return createIndex(i-1,0,ref);
            else
				return createIndex(mPosts[i].prow,0,ref);
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

		for(uint32_t j=0;j<e.mChildren.size();++j)
			std::cerr << " " << e.mChildren[j] ;

		QDateTime qtime;
		qtime.setTime_t(e.mPublishTs);

        std::cerr << " (" << e.mParent << ")";
		std::cerr << " " << qtime.toString().toStdString() << " \"" << e.mTitle << "\"" << std::endl;
    }

    // recursive print
    recursPrintModel(mPosts,ForumModelIndex(0),0);
}
#endif

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
            rsGxsForums->updateReputationLevel(mForumGroup.mMeta.mSignFlags,mPosts[i]);

			// notify the widgets that the data has changed.
			emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(0,COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
        }
}
