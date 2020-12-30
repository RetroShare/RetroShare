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
}

void RsGxsForumModel::preMods()
{
	//emit layoutAboutToBeChanged(); //Generate SIGSEGV when click on button move next/prev.

	beginResetModel();
}
void RsGxsForumModel::postMods()
{
	endResetModel();

    if(mTreeMode == TREE_MODE_FLAT)
		emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mPosts.size(),COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
    else
		emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mPosts[0].mChildren.size(),COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
}

void RsGxsForumModel::setTreeMode(TreeMode mode)
{
    if(mode == mTreeMode)
        return;

    preMods();

    if(mode == TREE_MODE_TREE)	// means we were in FLAT mode, so the last rows are removed.
    {
		beginRemoveRows(QModelIndex(),mPosts[0].mChildren.size(),mPosts.size()-1);
		endRemoveRows();
    }

	mTreeMode = mode;

    if(mode == TREE_MODE_FLAT)	// means we were in tree mode, so the last rows are added.
	{
		beginInsertRows(QModelIndex(),mPosts[0].mChildren.size(),mPosts.size()-1);
		endInsertRows();
	}

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

std::vector<std::pair<time_t,RsGxsMessageId> > RsGxsForumModel::getPostVersions(const RsGxsMessageId& mid) const
{
    auto it = mPostVersions.find(mid);

    if(it != mPostVersions.end())
        return it->second;
    else
        return std::vector<std::pair<time_t,RsGxsMessageId> >();
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
        return 0;

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
		case COLUMN_THREAD_DATE:         return tr("Date");
		case COLUMN_THREAD_AUTHOR:       return tr("Author");
		default:
			return QVariant();
		}

	if(role == Qt::DecorationRole)
		switch(section)
		{
        case COLUMN_THREAD_DISTRIBUTION: return FilesDefs::getIconFromQtResourcePath(":/icons/flag-green.png");
		case COLUMN_THREAD_READ:         return FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png");
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
        QFont font ;
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
//    if(fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_IS_PINNED)
//        return QVariant(QBrush(mBackgroundColorPinned));

    if(mFilteringEnabled && (fmpe.mPostFlags & ForumModelPostEntry::FLAG_POST_PASSES_FILTER))
        return QVariant(QBrush(mBackgroundColorFiltered));

    return QVariant();
}

QVariant RsGxsForumModel::sizeHintRole(int col) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

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
//                                    return QVariant( QString("<img src=\":/icons/pinned_64.png\" height=%1/>").arg(QFontMetricsF(QFont()).height())
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

void RsGxsForumModel::setPosts(const RsGxsForumGroup& group, const std::vector<ForumModelPostEntry>& posts,const std::map<RsGxsMessageId,std::vector<std::pair<time_t,RsGxsMessageId> > >& post_versions)
{
    preMods();

    if(mTreeMode == TREE_MODE_FLAT)
		beginRemoveRows(QModelIndex(),0,mPosts.size()-1);
	else
		beginRemoveRows(QModelIndex(),0,mPosts[0].mChildren.size()-1);

    endRemoveRows();

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

    if(mTreeMode == TREE_MODE_FLAT)
		beginInsertRows(QModelIndex(),0,mPosts.size()-1);
    else
		beginInsertRows(QModelIndex(),0,mPosts[0].mChildren.size()-1);
    endInsertRows();
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
		std::vector<RsMsgMetaData> msg_metas;
		std::vector<RsGxsForumGroup> groups;

        forumIds.push_back(group_id);

		if(!rsGxsForums->getForumsInfo(forumIds,groups) || groups.size() != 1)
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve forum group info for forum " << group_id << std::endl;
			return;
        }

		if(!rsGxsForums->getForumMsgMetaData(group_id,msg_metas))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve forum message info for forum " << group_id << std::endl;
			return;
		}

        // 2 - sort the messages into a proper hierarchy

        auto post_versions = new std::map<RsGxsMessageId,std::vector<std::pair<time_t, RsGxsMessageId> > >() ;
        std::vector<ForumModelPostEntry> *vect = new std::vector<ForumModelPostEntry>();
        RsGxsForumGroup group = groups[0];

        computeMessagesHierarchy(group,msg_metas,*vect,*post_versions);

        // 3 - update the model in the UI thread.

        RsQThreadUtils::postToObject( [group,vect,post_versions,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

            setPosts(group,*vect,*post_versions) ;

            delete vect;
            delete post_versions;


		}, this );

    });
}

ForumModelIndex RsGxsForumModel::addEntry(std::vector<ForumModelPostEntry>& posts,const ForumModelPostEntry& entry,ForumModelIndex parent)
{
    uint32_t N = posts.size();
    posts.push_back(entry);

    posts[N].mParent = parent;
    posts[parent].mChildren.push_back(N);
#ifdef DEBUG_FORUMMODEL
    std::cerr << "Added new entry " << N << " children of " << parent << std::endl;
#endif
    if(N == parent)
        std::cerr << "(EE) trying to add a post as its own parent!" << std::endl;
    return ForumModelIndex(N);
}

void RsGxsForumModel::generateMissingItem(const RsGxsMessageId &msgId,ForumModelPostEntry& entry)
{
    entry.mPostFlags = ForumModelPostEntry::FLAG_POST_IS_MISSING ;
    entry.mTitle = std::string(tr("[ ... Missing Message ... ]").toUtf8());
    entry.mMsgId = msgId;
    entry.mAuthorId.clear();
    entry.mPublishTs=0;
    entry.mReputationWarningLevel = 3;
}

void RsGxsForumModel::convertMsgToPostEntry(const RsGxsForumGroup& mForumGroup,const RsMsgMetaData& msg, bool /*useChildTS*/, ForumModelPostEntry& fentry)
{
    fentry.mTitle     = msg.mMsgName;
    fentry.mAuthorId  = msg.mAuthorId;
    fentry.mMsgId     = msg.mMsgId;
    fentry.mPublishTs = msg.mPublishTs;
    fentry.mPostFlags = 0;
    fentry.mMsgStatus = msg.mMsgStatus;

    if(mForumGroup.mPinnedPosts.ids.find(msg.mMsgId) != mForumGroup.mPinnedPosts.ids.end())
		fentry.mPostFlags |= ForumModelPostEntry::FLAG_POST_IS_PINNED;

	// Early check for a message that should be hidden because its author
	// is flagged with a bad reputation

    computeReputationLevel(mForumGroup.mMeta.mSignFlags,fentry);
}

void RsGxsForumModel::computeReputationLevel(uint32_t forum_sign_flags,ForumModelPostEntry& fentry)
{
    uint32_t idflags =0;
	RsReputationLevel reputation_level =
	        rsReputations->overallReputationLevel(fentry.mAuthorId, &idflags);
	bool redacted = false;

	if(reputation_level == RsReputationLevel::LOCALLY_NEGATIVE)
        fentry.mPostFlags |=  ForumModelPostEntry::FLAG_POST_IS_REDACTED;
    else
        fentry.mPostFlags &= ~ForumModelPostEntry::FLAG_POST_IS_REDACTED;

    // We use a specific item model for forums in order to handle the post pinning.

	if(reputation_level == RsReputationLevel::UNKNOWN)
        fentry.mReputationWarningLevel = 3 ;
	else if(reputation_level == RsReputationLevel::LOCALLY_NEGATIVE)
        fentry.mReputationWarningLevel = 2 ;
    else if(reputation_level < rsGxsForums->minReputationForForwardingMessages(forum_sign_flags,idflags))
        fentry.mReputationWarningLevel = 1 ;
    else
        fentry.mReputationWarningLevel = 0 ;
}

static bool decreasing_time_comp(const std::pair<time_t,RsGxsMessageId>& e1,const std::pair<time_t,RsGxsMessageId>& e2) { return e2.first < e1.first ; }

void RsGxsForumModel::computeMessagesHierarchy(const RsGxsForumGroup& forum_group,
                                               const std::vector<RsMsgMetaData>& msgs_metas_array,
                                               std::vector<ForumModelPostEntry>& posts,
                                               std::map<RsGxsMessageId,std::vector<std::pair<time_t,RsGxsMessageId> > >& mPostVersions
                                               )
{
    std::cerr << "updating messages data with " << msgs_metas_array.size() << " messages" << std::endl;

#ifdef DEBUG_FORUMS
    std::cerr << "Retrieved group data: " << std::endl;
    std::cerr << "  Group ID: " << forum_group.mMeta.mGroupId << std::endl;
    std::cerr << "  Admin lst: " << forum_group.mAdminList.ids.size() << " elements." << std::endl;
    for(auto it(forum_group.mAdminList.ids.begin());it!=forum_group.mAdminList.ids.end();++it)
        std::cerr << "    " << *it << std::endl;
    std::cerr << "  Pinned Post: " << forum_group.mPinnedPosts.ids.size() << " messages." << std::endl;
    for(auto it(forum_group.mPinnedPosts.ids.begin());it!=forum_group.mPinnedPosts.ids.end();++it)
        std::cerr << "    " << *it << std::endl;
#endif

	/* get messages */
	std::map<RsGxsMessageId,RsMsgMetaData> msgs;

	for(uint32_t i=0;i<msgs_metas_array.size();++i)
	{
#ifdef DEBUG_FORUMS
		std::cerr << "Adding message " << msgs_metas_array[i].mMeta.mMsgId << " with parent " << msgs_metas_array[i].mMeta.mParentId << " to message map" << std::endl;
#endif
		msgs[msgs_metas_array[i].mMsgId] = msgs_metas_array[i] ;
	}

#ifdef DEBUG_FORUMS
	size_t count = msgs.size();
#endif
//	int pos = 0;
//	int steps = count / PROGRESSBAR_MAX;
//	int step = 0;

    initEmptyHierarchy(posts);

    // ThreadList contains the list of parent threads. The algorithm below iterates through all messages
    // and tries to establish parenthood relationships between them, given that we only know the
    // immediate parent of a message and now its children. Some messages have a missing parent and for them
    // a fake top level parent is generated.

    // In order to be efficient, we first create a structure that lists the children of every mesage ID in the list.
    // Then the hierarchy of message is build by attaching the kids to every message until all of them have been processed.
    // The messages with missing parents will be the last ones remaining in the list.

	std::list<std::pair< RsGxsMessageId, ForumModelIndex > > threadStack;
    std::map<RsGxsMessageId,std::list<RsGxsMessageId> > kids_array ;
    std::set<RsGxsMessageId> missing_parents;

    // First of all, remove all older versions of posts. This is done by first adding all posts into a hierarchy structure
    // and then removing all posts which have a new versions available. The older versions are kept appart.

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::run() Collecting post versions" << std::endl;
#endif
    mPostVersions.clear();
    std::list<RsGxsMessageId> msg_stack ;

	for ( auto msgIt = msgs.begin(); msgIt != msgs.end();++msgIt)
    {
        if(!msgIt->second.mOrigMsgId.isNull() && msgIt->second.mOrigMsgId != msgIt->second.mMsgId)
		{
#ifdef DEBUG_FORUMS
			std::cerr << "  Post " << msgIt->second.mMeta.mMsgId << " is a new version of " << msgIt->second.mMeta.mOrigMsgId << std::endl;
#endif
			auto msgIt2 = msgs.find(msgIt->second.mOrigMsgId);

			// Ensuring that the post exists allows to only collect the existing data.

			if(msgIt2 == msgs.end())
				continue ;

			// Make sure that the author is the same than the original message, or is a moderator. This should always happen when messages are constructed using
            // the UI but nothing can prevent a nasty user to craft a new version of a message with his own signature.

			if(msgIt2->second.mAuthorId != msgIt->second.mAuthorId)
			{
				if( !IS_FORUM_MSG_MODERATION(msgIt->second.mMsgFlags) )			// if authors are different the moderation flag needs to be set on the editing msg
					continue ;

				if( !forum_group.canEditPosts(msgIt->second.mAuthorId))			// if author is not a moderator, continue
					continue ;
			}

			// always add the post a self version

			if(mPostVersions[msgIt->second.mOrigMsgId].empty())
				mPostVersions[msgIt->second.mOrigMsgId].push_back(std::make_pair(msgIt2->second.mPublishTs,msgIt2->second.mMsgId)) ;

			mPostVersions[msgIt->second.mOrigMsgId].push_back(std::make_pair(msgIt->second.mPublishTs,msgIt->second.mMsgId)) ;
		}
    }

    // The following code assembles all new versions of a given post into the same array, indexed by the oldest version of the post.

    for(auto it(mPostVersions.begin());it!=mPostVersions.end();++it)
    {
		auto& v(it->second) ;

		for(size_t i=0;i<v.size();++i)
		{
            if(v[i].second != it->first)
			{
				RsGxsMessageId sub_msg_id = v[i].second ;

				auto it2 = mPostVersions.find(sub_msg_id);

				if(it2 != mPostVersions.end())
				{
					for(size_t j=0;j<it2->second.size();++j)
						if(it2->second[j].second != sub_msg_id)	// dont copy it, since it is already present at slot i
							v.push_back(it2->second[j]) ;

					mPostVersions.erase(it2) ;	// it2 is never equal to it
				}
			}
		}
    }


    // Now remove from msg ids, all posts except the most recent one. And make the mPostVersion be indexed by the most recent version of the post,
    // which corresponds to the item in the tree widget.

#ifdef DEBUG_FORUMS
	std::cerr << "Final post versions: " << std::endl;
#endif
	std::map<RsGxsMessageId,std::vector<std::pair<time_t,RsGxsMessageId> > > mTmp;
    std::map<RsGxsMessageId,RsGxsMessageId> most_recent_versions ;

    for(auto it(mPostVersions.begin());it!=mPostVersions.end();++it)
    {
#ifdef DEBUG_FORUMS
        std::cerr << "Original post: " << it.key() << std::endl;
#endif
        // Finally, sort the posts from newer to older

        std::sort(it->second.begin(),it->second.end(),decreasing_time_comp) ;

#ifdef DEBUG_FORUMS
		std::cerr << "   most recent version " << (*it)[0].first << "  " << (*it)[0].second << std::endl;
#endif
		for(size_t i=1;i<it->second.size();++i)
		{
			msgs.erase(it->second[i].second) ;

#ifdef DEBUG_FORUMS
			std::cerr << "   older version " << (*it)[i].first << "  " << (*it)[i].second << std::endl;
#endif
		}

        mTmp[it->second[0].second] = it->second ;	// index the versions map by the ID of the most recent post.

		// Now make sure that message parents are consistent. Indeed, an old post may have the old version of a post as parent. So we need to change that parent
		// to the newest version. So we create a map of which is the most recent version of each message, so that parent messages can be searched in it.

	for(size_t i=1;i<it->second.size();++i)
		most_recent_versions[it->second[i].second] = it->second[0].second ;
	}
    mPostVersions = mTmp ;

    // The next step is to find the top level thread messages. These are defined as the messages without
    // any parent message ID.

    // this trick is needed because while we remove messages, the parents a given msg may already have been removed
    // and wrongly understand as a missing parent.

	std::map<RsGxsMessageId,RsMsgMetaData> kept_msgs;

	for ( auto msgIt = msgs.begin(); msgIt != msgs.end();++msgIt)
    {

        if(msgIt->second.mParentId.isNull())
		{

			/* add all threads */
			const RsMsgMetaData& msg = msgIt->second;

#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumsFillThread::run() Adding TopLevel Thread: mId: " << msg.mMsgId << std::endl;
#endif

            ForumModelPostEntry entry;
			convertMsgToPostEntry(forum_group,msg, mUseChildTS, entry);

            ForumModelIndex entry_index = addEntry(posts,entry,0);

			//if (!mFlatView)
				threadStack.push_back(std::make_pair(msg.mMsgId,entry_index)) ;

			//calculateExpand(msg, item);
			//mItems.append(entry_index);
		}
		else
        {
#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumsFillThread::run() Storing kid " << msgIt->first << " of message " << msgIt->second.mParentId << std::endl;
#endif
            // The same missing parent may appear multiple times, so we first store them into a unique container.

            RsGxsMessageId parent_msg = msgIt->second.mParentId;

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
        ForumModelPostEntry e ;
        generateMissingItem(*it,e);

        ForumModelIndex e_index = addEntry(posts,e,0);	// no parent -> parent is level 0
		//mItems.append( e_index );

		threadStack.push_back(std::make_pair(*it,e_index)) ;
	}
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::run() Processing stack:" << std::endl;
#endif
    // Now use a stack to go down the hierarchy

	while (!threadStack.empty())
    {
        std::pair<RsGxsMessageId, uint32_t> threadPair = threadStack.front();
		threadStack.pop_front();

        std::map<RsGxsMessageId, std::list<RsGxsMessageId> >::iterator it = kids_array.find(threadPair.first) ;

#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumsFillThread::run() Node: " << threadPair.first << std::endl;
#endif
        if(it == kids_array.end())
            continue ;


        for(std::list<RsGxsMessageId>::const_iterator it2(it->second.begin());it2!=it->second.end();++it2)
        {
            // We iterate through the top level thread items, and look for which message has the current item as parent.
            // When found, the item is put in the thread list itself, as a potential new parent.

            auto mit = msgs.find(*it2) ;

            if(mit == msgs.end())
			{
				std::cerr << "GxsForumsFillThread::run()    Cannot find submessage " << *it2 << " !!!" << std::endl;
				continue ;
			}

            const RsMsgMetaData& msg(mit->second) ;
#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumsFillThread::run()    adding sub_item " << msg.mMsgId << std::endl;
#endif


            ForumModelPostEntry e ;
			convertMsgToPostEntry(forum_group,msg,mUseChildTS,e) ;
            ForumModelIndex e_index = addEntry(posts,e, threadPair.second);

			//calculateExpand(msg, item);

			/* add item to process list */
			threadStack.push_back(std::make_pair(msg.mMsgId, e_index));

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
