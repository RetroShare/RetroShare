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

Q_DECLARE_METATYPE(ChannelPostFileInfo)

#ifdef DEBUG_CHANNEL_MODEL
static std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere
#endif

RsGxsChannelPostFilesModel::RsGxsChannelPostFilesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    initEmptyHierarchy();

    mTimer = new QTimer;
    connect(mTimer,SIGNAL(timeout()),this,SLOT(update()));
}

void RsGxsChannelPostFilesModel::initEmptyHierarchy()
{
	beginResetModel();

	mFiles.clear();
	mFilteredFiles.clear();

	endResetModel();
}

void RsGxsChannelPostFilesModel::preMods()
{
	emit layoutAboutToBeChanged();
}
void RsGxsChannelPostFilesModel::postMods()
{
	emit QAbstractItemModel::dataChanged(createIndex(0,0,(void*)NULL), createIndex(mFilteredFiles.size(),COLUMN_FILES_NB_COLUMNS-1,(void*)NULL));
	emit layoutChanged();
}

void RsGxsChannelPostFilesModel::update()
{
	preMods();
	postMods();
}

int RsGxsChannelPostFilesModel::rowCount(const QModelIndex& parent) const
{
	if(parent.column() > 0)
		return 0;

	if(mFilteredFiles.empty())	// security. Should never happen.
		return 0;

	if(!parent.isValid())
		return mFilteredFiles.size(); // mFilteredPosts always has an item at 0, so size()>=1, and mColumn>=1

	RS_ERR(" rowCount cannot figure out the proper number of rows.");
	return 0;
}

int RsGxsChannelPostFilesModel::columnCount(const QModelIndex &/*parent*/) const
{
	return COLUMN_FILES_NB_COLUMNS ;
}

bool RsGxsChannelPostFilesModel::getFileData(const QModelIndex& i,ChannelPostFileInfo& fmpe) const
{
	if(!i.isValid())
		return true;

	quintptr ref = i.internalId();
	uint32_t entry = 0;

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFiles.size())
		return false ;

	fmpe = mFiles[mFilteredFiles[entry]];

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

	ref = (intptr_t)(entry+1);

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
    if(val==0)
    {
        RsErr() << "(EE) trying to make a ChannelPostsFileModelIndex out of index 0." << std::endl;
        return false;
    }
	entry = val-1;

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

QModelIndex RsGxsChannelPostFilesModel::parent(const QModelIndex& /*index*/) const
{
	return QModelIndex();	// there's no hierarchy here. So nothing to do!
}

Qt::ItemFlags RsGxsChannelPostFilesModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::ItemFlag();

	if(index.column() == COLUMN_FILES_FILE)
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
	else
		return QAbstractItemModel::flags(index);
}

quintptr RsGxsChannelPostFilesModel::getChildRef(quintptr ref,int index) const
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

quintptr RsGxsChannelPostFilesModel::getParentRow(quintptr ref,int& row) const
{
	ChannelPostFilesModelIndex ref_entry;

	if(!convertRefPointerToTabEntry(ref,ref_entry) || ref_entry >= mFilteredFiles.size())
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
	if(ref == quintptr(0))
		return rowCount()-1;

	return 0;
}

QVariant RsGxsChannelPostFilesModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

    switch(section)
    {
    case COLUMN_FILES_FILE: return QString("Status");
    case COLUMN_FILES_SIZE: return QString("Size");
    case COLUMN_FILES_NAME: return QString("File");
    case COLUMN_FILES_DATE: return QString("Published");
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

	if(!convertRefPointerToTabEntry(ref,entry) || entry >= mFilteredFiles.size())
	{
#ifdef DEBUG_CHANNEL_MODEL
		std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
		return QVariant() ;
	}

	const ChannelPostFileInfo& fmpe(mFiles[mFilteredFiles[entry]]);

	switch(role)
	{
	case Qt::DisplayRole:    return displayRole   (fmpe,index.column()) ;
	case Qt::UserRole:	 	 return userRole      (fmpe,index.column()) ;
	case SortRole:           return sortRole      (fmpe,index.column()) ;
	default:
		return QVariant();
	}
}

void RsGxsChannelPostFilesModel::setFilter(const QStringList& strings, uint32_t& count)
{
	preMods();

    mFilteredFiles.clear();

	if(strings.empty())
	{
		for(uint32_t i=0;i<mFiles.size();++i)
			mFilteredFiles.push_back(i);
	}
	else
	{
		for(uint32_t i=0;i<mFiles.size();++i)
		{
			bool passes_strings = true;

			for(auto& s:strings)
				passes_strings = passes_strings && QString::fromStdString(mFiles[i].mName).contains(s,Qt::CaseInsensitive);

			if(passes_strings)
				mFilteredFiles.push_back(i);
		}
	}
	count = mFilteredFiles.size();

	if (rowCount()>0)
	{
		beginInsertRows(QModelIndex(),0,rowCount()-1);
		endInsertRows();
	}

	postMods();
}

class compareOperator
{
public:
    compareOperator(int column,Qt::SortOrder order): col(column),ord(order) {}

	bool operator()(const ChannelPostFileInfo& f1,const ChannelPostFileInfo& f2) const
	{
     	switch(col)
		{
		default:
		case RsGxsChannelPostFilesModel::COLUMN_FILES_NAME: return (ord==Qt::AscendingOrder)?(f1.mName<f2.mName):(f1.mName>f2.mName);
		case RsGxsChannelPostFilesModel::COLUMN_FILES_SIZE: return (ord==Qt::AscendingOrder)?(f1.mSize<f2.mSize):(f1.mSize>f2.mSize);
		case RsGxsChannelPostFilesModel::COLUMN_FILES_DATE: return (ord==Qt::AscendingOrder)?(f1.mPublishTime<f2.mPublishTime):(f1.mPublishTime>f2.mPublishTime);
		case RsGxsChannelPostFilesModel::COLUMN_FILES_FILE:
		{
			FileInfo fi1,fi2;
			rsFiles->FileDetails(f1.mHash,RS_FILE_HINTS_DOWNLOAD,fi1);
			rsFiles->FileDetails(f2.mHash,RS_FILE_HINTS_DOWNLOAD,fi2);

			return (ord==Qt::AscendingOrder)?(fi1.transfered<fi2.transfered):(fi1.transfered>fi2.transfered);
		}
		}

	}

private:
	int col;
	Qt::SortOrder ord;
};

void RsGxsChannelPostFilesModel::sort(int column, Qt::SortOrder order)
{
    std::sort(mFiles.begin(),mFiles.end(),compareOperator(column,order));

    update();
}

QVariant RsGxsChannelPostFilesModel::sizeHintRole(int /*col*/) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	return QVariant( QSize(factor * 170, factor*14 ));
}

QVariant RsGxsChannelPostFilesModel::sortRole(const ChannelPostFileInfo& fmpe,int column) const
{
    switch(column)
    {
	case COLUMN_FILES_NAME: return QVariant(QString::fromUtf8(fmpe.mName.c_str()));
	case COLUMN_FILES_SIZE: return QVariant(qulonglong(fmpe.mSize));
	case COLUMN_FILES_DATE: return QVariant(qulonglong(fmpe.mPublishTime));
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

QVariant RsGxsChannelPostFilesModel::displayRole(const ChannelPostFileInfo& fmpe,int col) const
{
	switch(col)
	{
	case COLUMN_FILES_NAME: return QString::fromUtf8(fmpe.mName.c_str());
	case COLUMN_FILES_SIZE: return QString::number(fmpe.mSize);
	case COLUMN_FILES_DATE: return QString::number(fmpe.mPublishTime);
	case COLUMN_FILES_FILE: {
		FileInfo finfo;
		if(rsFiles->FileDetails(fmpe.mHash,RS_FILE_HINTS_DOWNLOAD,finfo))
			return qulonglong(finfo.transfered);
		else
			return 0;
	}
	default:
		return QString();

	}


	return QVariant("[ERROR]");
}

QVariant RsGxsChannelPostFilesModel::userRole(const ChannelPostFileInfo& fmpe,int col) const
{
	switch(col)
    {
    default:
        return QVariant::fromValue(fmpe);
    }
}

void RsGxsChannelPostFilesModel::clear()
{
	initEmptyHierarchy();

	emit channelLoaded();
}

void RsGxsChannelPostFilesModel::setFiles(const std::list<ChannelPostFileInfo> &files)
{
	preMods();

	initEmptyHierarchy();

	for(auto& file:files)
		mFiles.push_back(file);

	for(uint32_t i=0;i<mFiles.size();++i)
		mFilteredFiles.push_back(i);

#ifdef DEBUG_CHANNEL_MODEL
	// debug_dump();
#endif

	if (mFilteredFiles.size()>0)
	{
		beginInsertRows(QModelIndex(),0,mFilteredFiles.size()-1);
		endInsertRows();
	}

	emit channelLoaded();

	if(!files.empty())
		mTimer->start(5000);
	else
		mTimer->stop();

	postMods();
}

void RsGxsChannelPostFilesModel::update_files(const std::set<RsGxsFile>& added_files,const std::set<RsGxsFile>& removed_files)
{
#error TODO
}
