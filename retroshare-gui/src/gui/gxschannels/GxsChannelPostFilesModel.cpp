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
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxschannels.h"
#include "retroshare/rsexpr.h"

#include "GxsChannelPostFilesModel.h"

//#define DEBUG_CHANNEL_MODEL

Q_DECLARE_METATYPE(RsGxsFile)

static std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

RsGxsChannelPostFilesModel::RsGxsChannelPostFilesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    initEmptyHierarchy(mFiles);

    mTimer = new QTimer;
    connect(mTimer,SIGNAL(timeout()),this,SLOT(update()));
}

void RsGxsChannelPostFilesModel::initEmptyHierarchy(std::vector<RsGxsFile>& files)
{
    preMods();

    files.resize(1);	// adds a sentinel item
    files[0].mName = "Root sentinel post" ;

    postMods();
}

void RsGxsChannelPostFilesModel::preMods()
{
	//emit layoutAboutToBeChanged(); //Generate SIGSEGV when click on button move next/prev.

	beginResetModel();
}
void RsGxsChannelPostFilesModel::postMods()
{
	endResetModel();

	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mFiles.size(),COLUMN_FILES_NB_COLUMNS-1,(void*)NULL));
}

void RsGxsChannelPostFilesModel::update()
{
	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mFiles.size(),COLUMN_FILES_NB_COLUMNS-1,(void*)NULL));
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

int RsGxsChannelPostFilesModel::rowCount(const QModelIndex& parent) const
{
    if(parent.column() > 0)
        return 0;

    if(mFiles.empty())	// security. Should never happen.
        return 0;

    if(!parent.isValid())
       return getChildrenCount(0);

    RsErr() << __PRETTY_FUNCTION__ << " rowCount cannot figure out the porper number of rows." << std::endl;
    return 0;
}

int RsGxsChannelPostFilesModel::columnCount(const QModelIndex &/*parent*/) const
{
	return COLUMN_FILES_NB_COLUMNS ;
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

bool RsGxsChannelPostFilesModel::getFileData(const QModelIndex& i,RsGxsFile& fmpe) const
{
	if(!i.isValid())
        return true;

    quintptr ref = i.internalId();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFiles.size())
		return false ;

    fmpe = mFiles[entry];

	return true;

}

bool RsGxsChannelPostFilesModel::hasChildren(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return true;

	return false;	// by default, no channel post has children
}

bool RsGxsChannelPostFilesModel::convertTabEntryToRefPointer(uint32_t entry,quintptr& ref)
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

bool RsGxsChannelPostFilesModel::convertRefPointerToTabEntry(quintptr ref, uint32_t& entry)
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

QModelIndex RsGxsChannelPostFilesModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0 || column >= COLUMN_FILES_NB_COLUMNS)
		return QModelIndex();

    quintptr ref = getChildRef(parent.internalId(),row);

#ifdef DEBUG_CHANNEL_MODEL
	std::cerr << "index-3(" << row << "," << column << " parent=" << parent << ") : " << createIndex(row,column,ref) << std::endl;
#endif
	return createIndex(row,column,ref) ;
}

QModelIndex RsGxsChannelPostFilesModel::parent(const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

	return QModelIndex();	// there's no hierarchy here. So nothing to do!
}

Qt::ItemFlags RsGxsChannelPostFilesModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    if(index.column() == COLUMN_FILES_FILE)
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    else
		return QAbstractItemModel::flags(index);
}

quintptr RsGxsChannelPostFilesModel::getChildRef(quintptr ref,int index) const
{
	if (index < 0)
		return 0;

	ChannelPostFilesModelIndex entry ;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFiles.size())
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

quintptr RsGxsChannelPostFilesModel::getParentRow(quintptr ref,int& row) const
{
	ChannelPostFilesModelIndex ref_entry;

	if(!convertRefPointerToTabEntry(ref,ref_entry) || ref_entry >= mFiles.size())
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

int RsGxsChannelPostFilesModel::getChildrenCount(quintptr ref) const
{
	uint32_t entry = 0 ;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFiles.size())
		return 0 ;

	if(entry == 0)
	{
#ifdef DEBUG_CHANNEL_MODEL
		std::cerr << "Children count (flat mode): " << mFiles.size()-1 << std::endl;
#endif
		return ((int)mFiles.size())-1;
	}
	else
		return 0;
}

QVariant RsGxsChannelPostFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

    switch(section)
    {
    case COLUMN_FILES_FILE: return QString("Status");
    case COLUMN_FILES_SIZE: return QString("Size");
    case COLUMN_FILES_NAME: return QString("File");
    default:
        return QString("[No data]");
    }
}

QVariant RsGxsChannelPostFilesModel::data(const QModelIndex &index, int role) const
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

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFiles.size())
	{
#ifdef DEBUG_CHANNEL_MODEL
		std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
		return QVariant() ;
	}

	const RsGxsFile& fmpe(mFiles[entry]);

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
	case SortRole:           return sortRole      (fmpe,index.column()) ;
#ifdef TODO
	case Qt::DecorationRole: return decorationRole(fmpe,index.column()) ;
	case Qt::ToolTipRole:	 return toolTipRole   (fmpe,index.column()) ;
	case Qt::TextColorRole:  return textColorRole (fmpe,index.column()) ;
	case Qt::BackgroundRole: return backgroundRole(fmpe,index.column()) ;

	case FilterRole:         return filterRole    (fmpe,index.column()) ;
	case ThreadPinnedRole:   return pinnedRole    (fmpe,index.column()) ;
	case MissingRole:        return missingRole   (fmpe,index.column()) ;
	case StatusRole:         return statusRole    (fmpe,index.column()) ;
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

QVariant RsGxsChannelPostFilesModel::sizeHintRole(int col) const
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

QVariant RsGxsChannelPostFilesModel::sortRole(const RsGxsFile& fmpe,int column) const
{
    switch(column)
    {
	case COLUMN_FILES_NAME: return QVariant(QString::fromUtf8(fmpe.mName.c_str()));
	case COLUMN_FILES_SIZE: return QVariant(qulonglong(fmpe.mSize));
	case COLUMN_FILES_FILE:
    {
        FileInfo finfo;
        if(rsFiles->FileDetails(fmpe.mHash,RS_FILE_HINTS_DOWNLOAD,finfo))
            return qulonglong(finfo.transfered);

        return QVariant(qulonglong(fmpe.mSize));
    }
        break;

    default:
        return displayRole(fmpe,column);
    }
}

QVariant RsGxsChannelPostFilesModel::displayRole(const RsGxsFile& fmpe,int col) const
{
	switch(col)
	{
    case 0: return QString::fromUtf8(fmpe.mName.c_str());
    case 1: return QString::number(fmpe.mSize);
    case 2: {
        FileInfo finfo;
        if(rsFiles->FileDetails(fmpe.mHash,RS_FILE_HINTS_DOWNLOAD,finfo))
            return qulonglong(finfo.transfered);
        else
            return 0;
    }
    default:
        return QString();
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

QVariant RsGxsChannelPostFilesModel::userRole(const RsGxsFile& fmpe,int col) const
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

void RsGxsChannelPostFilesModel::clear()
{
    preMods();

    mFiles.clear();
    initEmptyHierarchy(mFiles);

	postMods();
	emit channelLoaded();
}

void RsGxsChannelPostFilesModel::setFiles(const std::list<RsGxsFile>& files)
{
    preMods();

	beginRemoveRows(QModelIndex(),0,mFiles.size()-1);
    endRemoveRows();

    mFiles.clear();
    initEmptyHierarchy(mFiles);

    for(auto& file:files)
        mFiles.push_back(file);

#ifdef TODO
    recursUpdateReadStatusAndTimes(0,has_unread_below,has_read_below) ;
    recursUpdateFilterStatus(0,0,QStringList());
#endif

#ifdef DEBUG_CHANNEL_MODEL
   // debug_dump();
#endif

	beginInsertRows(QModelIndex(),0,mFiles.size()-1);
    endInsertRows();

	postMods();

	emit channelLoaded();

    if(!files.empty())
        mTimer->start(5000);
    else
        mTimer->stop();
}

QModelIndex RsGxsChannelPostFilesModel::getIndexOfFile(const RsFileHash& hash) const
{
    // Brutal search. This is not so nice, so dont call that in a loop! If too costly, we'll use a map.

    for(uint32_t i=1;i<mFiles.size();++i)
        if(mFiles[i].mHash == hash)
        {
            quintptr ref ;
            convertTabEntryToRefPointer(i,ref);	// we dont use i+1 here because i is not a row, but an index in the mPosts tab

			return createIndex(i-1,0,ref);
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
