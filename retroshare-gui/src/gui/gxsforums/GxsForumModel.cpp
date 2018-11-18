#include <QApplication>
#include <QFontMetrics>
#include <QModelIndex>

#include "GxsForumModel.h"

#define DEBUG_FORUMMODEL

#define	 COLUMN_TITLE         0
#define	 COLUMN_READ_STATUS   1
#define	 COLUMN_DATE          2
#define	 COLUMN_AUTHOR        3
#define  COLUMN_COUNT         4

Q_DECLARE_METATYPE(RsMsgMetaData);

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

RsGxsForumModel::RsGxsForumModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    mPosts.resize(1);	// adds a sentinel item

    // adds some fake posts to debug

    int N=5 ;
    mPosts.resize(N+1);

    for(int i=1;i<=N;++i)
	{
		mPosts[0].children.push_back(ForumModelIndex(i));
		mPosts[i].parent = ForumModelIndex(0);
		mPosts[i].prow = i-1;

		RsMsgMetaData meta;
		meta.mMsgName = "message " + (QString::number(i).toStdString()) ;
		mPosts[i].meta_versions.push_back(meta);
	}

    // add one child to last post
    mPosts.resize(N+2);
    mPosts[N].children.push_back(ForumModelIndex(N+1));
    mPosts[N+1].parent = ForumModelIndex(N);
    mPosts[N+1].prow = 0;
}

int RsGxsForumModel::rowCount(const QModelIndex& parent) const
{
	void *ref = (parent.isValid())?parent.internalPointer():NULL ;

    if(mPosts.empty())	// security. Should never happen.
        return 0;

	uint32_t entry = 0 ;
	int source_id ;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << "rowCount-2(" << parent << ") : " <<  0 << std::endl;
#endif
		return 0 ;
	}

#ifdef DEBUG_FORUMMODEL
	std::cerr << "rowCount-3(" << parent << ") : " <<  mPosts[entry].children.size() << std::endl;
#endif
	return mPosts[entry].children.size();
}

int RsGxsForumModel::columnCount(const QModelIndex &parent) const
{
	return COLUMN_COUNT ;
}

bool RsGxsForumModel::hasChildren(const QModelIndex &parent) const
{
	void *ref = (parent.isValid())?parent.internalPointer():NULL ;
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mPosts.size())
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << "hasChildren-2(" << parent << ") : " << false << std::endl;
#endif
		return false ;
	}

#ifdef DEBUG_DOWNLOADLIST
	std::cerr << "hasChildren-3(" << parent << ") : " << !mDownloads[entry].peers.empty() << std::endl;
#endif
	return !mPosts[entry].children.empty();
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

    if(val > (intptr_t)(~(uint32_t(0))))	// make sure the pointer is an int that fits in 32bits
    {
        std::cerr << "(EE) trying to make a ForumModelIndex out of a number that is larger than 2^32 !" << std::endl;
        return false ;
    }
	entry = uint32_t(val);

	return true;
}

QModelIndex RsGxsForumModel::index(int row, int column, const QModelIndex & parent) const
{
	if(row < 0 || column < 0 || column >= COLUMN_COUNT)
		return QModelIndex();

	void *parent_ref = (parent.isValid())?parent.internalPointer():NULL ;
	uint32_t parent_entry = 0;
	int source_id=0 ;

    // We dont need to handle parent==NULL because the conversion will return entry=0 which is what we need.

	if(!convertRefPointerToTabEntry(parent_ref,parent_entry) || parent_entry >= mPosts.size())
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << "index-5(" << row << "," << column << " parent=" << parent << ") : " << "NULL"<< std::endl ;
#endif
		return QModelIndex() ;
	}

	void *ref = NULL ;

    if(row >= mPosts[parent_entry].children.size() || !convertTabEntryToRefPointer(mPosts[parent_entry].children[row],ref))
	{
#ifdef DEBUG_FORUMMODEL
		std::cerr << "index-4(" << row << "," << column << " parent=" << parent << ") : " << "NULL" << std::endl;
#endif
		return QModelIndex() ;
	}

#ifdef DEBUG_FORUMMODEL
	std::cerr << "index-3(" << row << "," << column << " parent=" << parent << ") : " << createIndex(row,column,ref) << std::endl;
#endif
	return createIndex(row,column,ref) ;
}

QModelIndex RsGxsForumModel::parent(const QModelIndex& child) const
{
	void *child_ref = (child.isValid())?child.internalPointer():NULL ;

	if(!child_ref)
		return QModelIndex() ;

    ForumModelIndex child_entry ;

	if(!convertRefPointerToTabEntry(child_ref,child_entry) || child_entry >= mPosts.size())
		return QModelIndex() ;

	void *parent_ref =NULL;
	ForumModelIndex parent_entry = mPosts[child_entry].parent;
	QModelIndex indx;

    if(parent_entry == 0)		// top level index
        indx = QModelIndex() ;
    else
    {
		if(!convertTabEntryToRefPointer(parent_entry,parent_ref))
			return QModelIndex() ;

		indx = createIndex(mPosts[parent_entry].prow,child.column(),parent_ref) ;
    }
#ifdef DEBUG_FORUMMODEL
	std::cerr << "parent-1(" << child << ") : " << indx << std::endl;
#endif
	return indx;
}

QVariant RsGxsForumModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole)
		return QVariant();

	switch(section)
	{
	default:
	case COLUMN_TITLE:        return tr("Title");
	case COLUMN_READ_STATUS:  return tr("Read Status");
	case COLUMN_DATE:         return tr("Date");
	case COLUMN_AUTHOR:       return tr("Author");
	}
}

QVariant RsGxsForumModel::data(const QModelIndex &index, int role) const
{
	if(!index.isValid())
		return QVariant();

	int coln = index.column() ;

	switch(role)
	{
	case Qt::SizeHintRole:       return sizeHintRole(index.column()) ;
	case Qt::TextAlignmentRole:
	case Qt::TextColorRole:
	case Qt::WhatsThisRole:
	case Qt::EditRole:
	case Qt::ToolTipRole:
	case Qt::StatusTipRole:
		return QVariant();
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

	const RsMsgMetaData& meta(mPosts[entry].meta_versions[0]) ;

#ifdef DEBUG_FORUMMODEL
	std::cerr << " [ok]" << std::endl;
#endif

	switch(role)
	{
	case Qt::DisplayRole:    return displayRole   (meta,index.column()) ;
	case Qt::DecorationRole: return decorationRole(meta,index.column()) ;
	case Qt::UserRole:       return userRole      (meta,index.column()) ;
	default:
		return QVariant();
	}
}

QVariant RsGxsForumModel::sizeHintRole(int col) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	switch(col)
	{
	default:
	case COLUMN_TITLE:        return QVariant( factor * 170 );
	case COLUMN_READ_STATUS:  return QVariant( factor * 10  );
	case COLUMN_DATE:         return QVariant( factor * 75  );
	case COLUMN_AUTHOR:       return QVariant( factor * 75  );
	}
}

QVariant RsGxsForumModel::displayRole(const RsMsgMetaData& meta,int col) const
{
	switch(col)
	{
		case COLUMN_TITLE:        return QVariant(QString::fromUtf8(meta.mMsgName.c_str()));
		case COLUMN_READ_STATUS:  return QVariant(meta.mMsgStatus);
		case COLUMN_DATE:         return QVariant(qulonglong(meta.mPublishTs));
		case COLUMN_AUTHOR:       return QVariant(QString::fromStdString(meta.mAuthorId.toStdString()));

		default:
			return QVariant("[ TODO ]");
		}


	return QVariant("[ERROR]");
}

QVariant RsGxsForumModel::userRole(const RsMsgMetaData &meta, int col) const
{
#ifdef TODO
		switch(col)
		{
		case COLUMN_PROGRESS:
		{
			FileChunksInfo fcinfo;
			if (!rsFiles->FileDownloadChunksDetails(fileInfo.hash, fcinfo))
				return QVariant();

			FileProgressInfo pinfo;
			pinfo.cmap = fcinfo.chunks;
			pinfo.type = FileProgressInfo::DOWNLOAD_LINE;
			pinfo.progress = (fileInfo.size == 0) ? 0 : (fileInfo.transfered * 100.0 / fileInfo.size);
			pinfo.nb_chunks = pinfo.cmap._map.empty() ? 0 : fcinfo.chunks.size();

			for (uint32_t i = 0; i < fcinfo.chunks.size(); ++i)
				switch(fcinfo.chunks[i])
				{
				case FileChunksInfo::CHUNK_CHECKING: pinfo.chunks_in_checking.push_back(i);
					break ;
				case FileChunksInfo::CHUNK_ACTIVE: 	 pinfo.chunks_in_progress.push_back(i);
					break ;
				case FileChunksInfo::CHUNK_DONE:
				case FileChunksInfo::CHUNK_OUTSTANDING:
					break ;
				}

			return QVariant::fromValue(pinfo);
		}

		case COLUMN_ID:   return QVariant(QString::fromStdString(fileInfo.hash.toStdString()));


		default:
			return QVariant();
        }
        }
#endif
	return QVariant();

}

QVariant RsGxsForumModel::decorationRole(const RsMsgMetaData& meta,int col) const
{
	return QVariant();
}

void RsGxsForumModel::update_posts()
{
#ifdef TODO
	std::list<RsFileHash> downHashes;
	rsFiles->FileDownloads(downHashes);

	size_t old_size = mDownloads.size();

	mDownloads.resize(downHashes.size()) ;

	if(old_size < mDownloads.size())
	{
		beginInsertRows(QModelIndex(), old_size, mDownloads.size()-1);
		insertRows(old_size, mDownloads.size() - old_size);
		endInsertRows();
	}
	else if(mDownloads.size() < old_size)
	{
		beginRemoveRows(QModelIndex(), mDownloads.size(), old_size-1);
		removeRows(mDownloads.size(), old_size - mDownloads.size());
		endRemoveRows();
	}

	uint32_t i=0;

	for(auto it(downHashes.begin());it!=downHashes.end();++it,++i)
	{
		FileInfo fileInfo(mDownloads[i]);	// we dont update the data itself but only a copy of it....
		int old_size = fileInfo.peers.size() ;

		rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, fileInfo);

		int new_size = fileInfo.peers.size() ;

		if(old_size < new_size)
		{
			beginInsertRows(index(i,0), old_size, new_size-1);
			insertRows(old_size, new_size - old_size,index(i,0));
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "called insert rows ( " << old_size << ", " << new_size - old_size << ",index(" << index(i,0)<< "))" << std::endl;
#endif
			endInsertRows();
		}
		else if(new_size < old_size)
		{
			beginRemoveRows(index(i,0), new_size, old_size-1);
			removeRows(new_size, old_size - new_size,index(i,0));
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "called remove rows ( " << old_size << ", " << old_size - new_size << ",index(" << index(i,0)<< "))" << std::endl;
#endif
			endRemoveRows();
		}

		uint32_t old_status = mDownloads[i].downloadStatus ;

		mDownloads[i] = fileInfo ; // ... because insertRows() calls rowCount() which needs to be consistent with the *old* number of rows.

		if(fileInfo.downloadStatus == FT_STATE_DOWNLOADING || old_status != fileInfo.downloadStatus)
		{
			QModelIndex topLeft = createIndex(i,0), bottomRight = createIndex(i, COLUMN_COUNT-1);
			emit dataChanged(topLeft, bottomRight);
		}

		// This is apparently not needed.
		//
		// if(!mDownloads.empty())
		// {
		// 	QModelIndex topLeft = createIndex(0,0), bottomRight = createIndex(mDownloads.size()-1, COLUMN_COUNT-1);
		// 	emit dataChanged(topLeft, bottomRight);
		// 	mDownloads[i] = fileInfo ;
		// }
	}
#endif
}
