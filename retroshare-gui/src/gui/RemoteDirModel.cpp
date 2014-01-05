/*************************************:***************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
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

#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QMimeData>
#include <QTimer>

#include <retroshare-gui/RsAutoUpdatePage.h>
#include <gui/common/RsCollectionFile.h>
#include <gui/common/RsUrlHandler.h>
#include <gui/common/FilesDefs.h>
#include <gui/common/GroupDefs.h>
#include "RemoteDirModel.h"
#include <retroshare/rsfiles.h>
#include <retroshare/rstypes.h>
#include <retroshare/rspeers.h>
#include "util/misc.h"

#include <set>
#include <algorithm>

/*****
 * #define RDM_DEBUG
 ****/

RetroshareDirModel::RetroshareDirModel(bool mode, QObject *parent)
        : QAbstractItemModel(parent),
         ageIndicator(IND_ALWAYS),
         RemoteMode(mode), nIndex(1), indexSet(1) /* ass zero index cant be used */
{
	_visible = false ;
#if QT_VERSION < QT_VERSION_CHECK (5, 0, 0)
	setSupportedDragActions(Qt::CopyAction);
#endif
	treeStyle();
}

// QAbstractItemModel::setSupportedDragActions() was replaced by virtual QAbstractItemModel::supportedDragActions()
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
Qt::DropActions RetroshareDirModel::supportedDragActions() const
{
	return Qt::CopyAction;
}
#endif

void FlatStyle_RDM::update()
{
	if(_needs_update)
	{
		preMods() ;
		postMods() ;
	}
}
void RetroshareDirModel::treeStyle()
{
	categoryIcon.addPixmap(QPixmap(":/images/folder16.png"),
	                     QIcon::Normal, QIcon::Off);
	categoryIcon.addPixmap(QPixmap(":/images/folder_video.png"),
	                     QIcon::Normal, QIcon::On);
	peerIcon = QIcon(":/images/user/identity16.png");
}

bool TreeStyle_RDM::hasChildren(const QModelIndex &parent) const
{

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::hasChildren() :" << parent.internalPointer();
	std::cerr << ": ";
#endif

	if (!parent.isValid())
	{
#ifdef RDM_DEBUG
		std::cerr << "root -> true ";
		std::cerr << std::endl;
#endif
		return true;
	}

	void *ref = parent.internalPointer();

	const DirDetails *details = requestDirDetails(ref, RemoteMode);

	if (!details)
	{
		/* error */
#ifdef RDM_DEBUG
		std::cerr << "lookup failed -> false";
		std::cerr << std::endl;
#endif
		return false;
	}

	if (details->type == DIR_TYPE_FILE)
	{
#ifdef RDM_DEBUG
		std::cerr << "lookup FILE -> false";
		std::cerr << std::endl;
#endif
		return false;
	}
	/* PERSON/DIR*/
#ifdef RDM_DEBUG
	std::cerr << "lookup PER/DIR #" << details->count;
	std::cerr << std::endl;
#endif
	return (details->count > 0); /* do we have children? */
}
bool FlatStyle_RDM::hasChildren(const QModelIndex &parent) const
{

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::hasChildren() :" << parent.internalPointer();
	std::cerr << ": ";
#endif

	if (!parent.isValid())
	{
#ifdef RDM_DEBUG
		std::cerr << "root -> true ";
		std::cerr << std::endl;
#endif
		return true;
	}
	else
		return false ;
}

int TreeStyle_RDM::rowCount(const QModelIndex &parent) const
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::rowCount(): " << parent.internalPointer();
	std::cerr << ": ";
#endif

	void *ref = (parent.isValid())? parent.internalPointer() : NULL ;

	const DirDetails *details = requestDirDetails(ref, RemoteMode);

	if (!details)
	{
#ifdef RDM_DEBUG
		std::cerr << "lookup failed -> 0";
		std::cerr << std::endl;
#endif
		return 0;
	}
	if (details->type == DIR_TYPE_FILE)
	{
#ifdef RDM_DEBUG
		std::cerr << "lookup FILE: 0";
		std::cerr << std::endl;
#endif
		return 0;
	}

	/* else PERSON/DIR*/
#ifdef RDM_DEBUG
	std::cerr << "lookup PER/DIR #" << details->count;
	std::cerr << std::endl;
#endif
	return details->count;
}

int FlatStyle_RDM::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::rowCount(): " << parent.internalPointer();
	std::cerr << ": ";
#endif

	return _ref_entries.size() ;
}
int TreeStyle_RDM::columnCount(const QModelIndex &/*parent*/) const
{
	return 5;
}
int FlatStyle_RDM::columnCount(const QModelIndex &/*parent*/) const
{
	return 5;
}
QString RetroshareDirModel::getFlagsString(FileStorageFlags flags)
{
	char str[11] = "-  -  -" ;

	if(flags & DIR_FLAGS_BROWSABLE_GROUPS) 	str[0] = 'B' ;
	//if(flags & DIR_FLAGS_NETWORK_WIDE_GROUPS) str[3] = 'N' ;
	if(flags & DIR_FLAGS_BROWSABLE_OTHERS) 	str[3] = 'B' ;
	if(flags & DIR_FLAGS_NETWORK_WIDE_OTHERS) str[6] = 'N' ;

	return QString(str) ;
}
QString RetroshareDirModel::getGroupsString(const std::list<std::string>& group_ids)
{
	QString groups_str ;
	RsGroupInfo group_info ;

	for(std::list<std::string>::const_iterator it(group_ids.begin());it!=group_ids.end();)
		if(rsPeers->getGroupInfo(*it,group_info)) 
		{
			groups_str += GroupDefs::name(group_info) ;

			if(++it != group_ids.end())
				groups_str += ", " ;
		}
		else
			++it ;

	return groups_str ;
}

QString RetroshareDirModel::getAgeIndicatorString(const DirDetails &details) const
{
	QString ret("");
	QString nind = tr("NEW");
//	QString oind = tr("OLD");
	uint32_t age = details.age;

	switch (ageIndicator) {
		case IND_LAST_DAY:
			if (age < 24 * 60 * 60) return nind;
			break;
		case IND_LAST_WEEK:
			if (age < 7 * 24 * 60 * 60) return nind;
			break;
		case IND_LAST_MONTH:
			if (age < 30 * 24 * 60 * 60) return nind;
			break;
//		case IND_OLDER:
//			if (age >= 30 * 24 * 60 * 60) return oind;
//			break;
		case IND_ALWAYS:
			return ret;
		default:
			return ret;
	}

	return ret;
}

QVariant RetroshareDirModel::decorationRole(const DirDetails& details,int coln) const
{
	if(coln > 0)
		return QVariant() ;

	if (details.type == DIR_TYPE_PERSON)
	{
		if(details.min_age > ageIndicator)
			return QIcon(":/images/folder_grey.png");
		else if (ageIndicator == IND_LAST_DAY )
			return QIcon(":/images/folder_green.png");
		else if (ageIndicator == IND_LAST_WEEK )
			return QIcon(":/images/folder_yellow.png");
		else if (ageIndicator == IND_LAST_MONTH )
			return QIcon(":/images/folder_red.png");
		else
			return (QIcon(peerIcon));
	}
	else if (details.type == DIR_TYPE_DIR)
	{
		if(details.min_age > ageIndicator)
			return QIcon(":/images/folder_grey.png");
		else if (ageIndicator == IND_LAST_DAY )
			return QIcon(":/images/folder_green.png");
		else if (ageIndicator == IND_LAST_WEEK )
			return QIcon(":/images/folder_yellow.png");
		else if (ageIndicator == IND_LAST_MONTH )
			return QIcon(":/images/folder_red.png");
		else
			return QIcon(categoryIcon);
	}
	else if (details.type == DIR_TYPE_FILE) /* File */
	{
		// extensions predefined
		return FilesDefs::getIconFromFilename(QString::fromUtf8(details.name.c_str()));
	}
	else
		return QVariant();

} /* end of DecorationRole */

QVariant TreeStyle_RDM::displayRole(const DirDetails& details,int coln) const
{

	/*
	 * Person:  name,  id, 0, 0;
	 * File  :  name,  size, rank, (0) ts
	 * Dir   :  name,  (0) count, (0) path, (0) ts
	 */


	if (details.type == DIR_TYPE_PERSON) /* Person */
	{
		switch(coln)
		{
			case 0:
				return (RemoteMode)?(QString::fromUtf8(rsPeers->getPeerName(details.id).c_str())):tr("My files");
			case 1:
				return QString() ;
			case 2:
				return misc::userFriendlyDuration(details.min_age);
			default:
				return QString() ;
		}
	}
	else if (details.type == DIR_TYPE_FILE) /* File */
	{
		switch(coln)
		{
			case 0:
				return QString::fromUtf8(details.name.c_str());
			case 1:
				return  misc::friendlyUnit(details.count);
			case 2:
				return  misc::userFriendlyDuration(details.age);
			case 3:
				return getFlagsString(details.flags);
//			case 4:
//				{
//					QString ind("");
//					if (ageIndicator != IND_ALWAYS)
//						ind = getAgeIndicatorString(details);
//					return ind;
//				}
			case 4:
				return getGroupsString(details.parent_groups) ;

			default:
				return tr("FILE");
		}
	}
	else if (details.type == DIR_TYPE_DIR) /* Dir */
	{
		switch(coln)
		{
			case 0:
				return QString::fromUtf8(details.name.c_str());
				break;
			case 1:
				if (details.count > 1)
				{
					return QString::number(details.count) + " " + tr("Files");
				}
				return QString::number(details.count) + " " + tr("File");
			case 2:
				return misc::userFriendlyDuration(details.min_age);
			case 3:
				return getFlagsString(details.flags);
			case 4: 
				return getGroupsString(details.parent_groups) ;

			default:
				return tr("DIR");
		}
	}
	return QVariant();
} /* end of DisplayRole */

QString FlatStyle_RDM::computeDirectoryPath(const DirDetails& details) const
{
	QString dir ;
	const DirDetails *det = requestDirDetails(details.parent,RemoteMode);

	if(!det)
		return QString(); 

#ifdef SHOW_TOTAL_PATH
	do
	{
#endif
		dir = QString::fromUtf8(det->name.c_str())+"/"+dir ;

#ifdef SHOW_TOTAL_PATH
		if(!requestDirDetails(det.parent,det,flags))
			break ;
	}
	while(det->parent != NULL);
#endif
	
	return dir ;
}

QVariant FlatStyle_RDM::displayRole(const DirDetails& details,int coln) const
{
	if (details.type == DIR_TYPE_FILE) /* File */
		switch(coln)
		{
			case 0: return QString::fromUtf8(details.name.c_str());
			case 1: return misc::friendlyUnit(details.count);
			case 2: return misc::userFriendlyDuration(details.age);
			case 3: return QString::fromUtf8(rsPeers->getPeerName(details.id).c_str());
			case 4: return computeDirectoryPath(details);
			default:
				return QVariant() ;
		}

	return QVariant();
} /* end of DisplayRole */
QVariant TreeStyle_RDM::sortRole(const QModelIndex& /*index*/,const DirDetails& details,int coln) const
{
	/*
	 * Person:  name,  id, 0, 0;
	 * File  :  name,  size, rank, (0) ts
	 * Dir   :  name,  (0) count, (0) path, (0) ts
	 */

	if (details.type == DIR_TYPE_PERSON) /* Person */
	{
		switch(coln)
		{
			case 0:
				return (RemoteMode)?(QString::fromUtf8(rsPeers->getPeerName(details.id).c_str())):tr("My files");
			case 1:
				return QString();
			case 2:
				return details.min_age;
			default:
				return QString();
		}
	}
	else if (details.type == DIR_TYPE_FILE) /* File */
	{
		switch(coln)
		{
			case 0:
				return QString::fromUtf8(details.name.c_str());
			case 1:
				return (qulonglong) details.count;
			case 2:
				return  details.age;
			case 3:
				return getFlagsString(details.flags);
			case 4:
				{
					QString ind("");
					if (ageIndicator != IND_ALWAYS)
						ind = getAgeIndicatorString(details);
					return ind;
				}
			default:
				return tr("FILE");
		}
	}
	else if (details.type == DIR_TYPE_DIR) /* Dir */
	{
		switch(coln)
		{
			case 0:
				return QString::fromUtf8(details.name.c_str());
			case 1:
				return (qulonglong) details.count;
			case 2:
				return details.min_age;
			case 3:
				return getFlagsString(details.flags);
			default:
				return tr("DIR");
		}
	}
	return QVariant();
}
QVariant FlatStyle_RDM::sortRole(const QModelIndex& index,const DirDetails& details,int coln) const
{
	/*
	 * Person:  name,  id, 0, 0;
	 * File  :  name,  size, rank, (0) ts
	 * Dir   :  name,  (0) count, (0) path, (0) ts
	 */

	if (details.type == DIR_TYPE_FILE) /* File */
	{
		switch(coln)
		{
			case 0: return QString::fromUtf8(details.name.c_str());
			case 1: return (qulonglong) details.count;
			case 2: return  details.age;
			case 3: return QString::fromUtf8(rsPeers->getPeerName(details.id).c_str());
			case 4: return _ref_entries[index.row()].second ;
		}
	}
	return QVariant();
} /* end of SortRole */




QVariant RetroshareDirModel::data(const QModelIndex &index, int role) const
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::data(): " << index.internalPointer();
	std::cerr << ": ";
	std::cerr << std::endl;
#endif

	if (!index.isValid())
		return QVariant();

	/* get the data from the index */
	void *ref = index.internalPointer();
	int coln = index.column();

	const DirDetails *details = requestDirDetails(ref, RemoteMode);

	if (!details)
		return QVariant();

	if (role == RetroshareDirModel::FileNameRole) /* end of FileNameRole */
		return QString::fromUtf8(details->name.c_str());

	if (role == Qt::TextColorRole)
	{
		if(details->min_age > ageIndicator)
			return QVariant(QColor(Qt::gray)) ;
		else
			return QVariant() ; // standard
	} /* end of TextColorRole */


	if(role == Qt::DecorationRole)
		return decorationRole(*details,coln) ;

	/*****************
	  Qt::EditRole
	  Qt::ToolTipRole
	  Qt::StatusTipRole
	  Qt::WhatsThisRole
	  Qt::SizeHintRole
	 ****************/

	if (role == Qt::SizeHintRole)
	{       
		return QSize(18, 18);     
	} /* end of SizeHintRole */ 

	if (role == Qt::TextAlignmentRole)
	{
		if(coln == 1)
		{
			return int( Qt::AlignRight | Qt::AlignVCenter);
		}
		return QVariant();
	} /* end of TextAlignmentRole */

	if (role == Qt::DisplayRole)
		return displayRole(*details,coln) ;

	if (role == SortRole)
		return sortRole(index,*details,coln) ;

	return QVariant();
}

//void RetroshareDirModel::getAgeIndicatorRec(const DirDetails &details, QString &ret) const {
//	if (details.type == DIR_TYPE_FILE) {
//		ret = getAgeIndicatorString(details);
//		return;
//	} else if (details.type == DIR_TYPE_DIR && ret.isEmpty()) {
//		std::list<DirStub>::const_iterator it;
//		for (it = details.children.begin(); it != details.children.end(); it++) {
//			void *ref = it->ref;
//			const DirDetails *childDetails = requestDirDetails(ref, RemoteMode);

//			if (childDetails && ret == tr(""))
//				getAgeIndicatorRec(*childDetails, ret);
//		}
//	}
//}

QVariant TreeStyle_RDM::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::SizeHintRole)
	{
		int defw = 50;
		int defh = 21;
		if (section < 2)
		{
			defw = 200;
		}
		return QSize(defw, defh);
	}

	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
	{
		switch(section)
		{
			case 0:
				if (RemoteMode)
					return tr("Friends Directories");
				else
					return tr("My Directories");
			case 1:
				return tr("Size");
			case 2:
				return tr("Age");
			case 3:
				if (RemoteMode)
					return tr("Friend");
				else
					return tr("Share Flags");
			case 4:
				if (RemoteMode)
					return tr("What's new");
				else
					return tr("Groups");
		}
		return QString("Column %1").arg(section);
	}
	else
		return QString("Row %1").arg(section);
}
QVariant FlatStyle_RDM::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::SizeHintRole)
	{
		int defw = 50;
		int defh = 21;
		if (section < 2)
		{
			defw = 200;
		}
		return QSize(defw, defh);
	}

	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
	{
		switch(section)
		{
			case 0:
				if (RemoteMode)
				{
					return tr("Friends Directories");
				}
				return tr("My Directories");
			case 1:
				return tr("Size");
			case 2:
				return tr("Age");
			case 3:
				if(RemoteMode)
					return tr("Friend");
				else
					return tr("Share Flags");
			case 4:
				return tr("Directory");
		}
		return QString("Column %1").arg(section);
	}
	else
		return QString("Row %1").arg(section);
}

QModelIndex TreeStyle_RDM::index(int row, int column, const QModelIndex & parent) const
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::index(): " << parent.internalPointer();
	std::cerr << ": row:" << row << " col:" << column << " ";
#endif

	if(row < 0)
		return QModelIndex() ;

	void *ref = (parent.isValid()) ? parent.internalPointer() : NULL;

	/********
	  if (!RemoteMode)
	  {
	  remote = &(rsiface->getLocalDirectoryList());
	  }
	 ********/

	const DirDetailsVector *details = requestDirDetails(ref, RemoteMode);

	if (!details)
	{
#ifdef RDM_DEBUG
		std::cerr << "lookup failed -> invalid";
		std::cerr << std::endl;
#endif
		return QModelIndex();
	}


	/* now iterate through the details to
	 * get the reference number
	 */

	if (row >= (int) details->childrenVector.size())
	{
#ifdef RDM_DEBUG
		std::cerr << "wrong number of children -> invalid";
		std::cerr << std::endl;
#endif
		return QModelIndex();
	}

#ifdef RDM_DEBUG
	std::cerr << "success index(" << row << "," << column << "," << details->childrenVector[row].ref << ")";
	std::cerr << std::endl;
#endif

	/* we can just grab the reference now */

	return createIndex(row, column, details->childrenVector[row].ref);
}
QModelIndex FlatStyle_RDM::index(int row, int column, const QModelIndex & parent) const
{
	Q_UNUSED(parent);
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::index(): " << parent.internalPointer();
	std::cerr << ": row:" << row << " col:" << column << " ";
#endif

	if(row < 0)
		return QModelIndex() ;

	if(row < (int) _ref_entries.size())
	{
		void *ref = _ref_entries[row].first ;

		return createIndex(row, column, ref);
	}
	else
		return QModelIndex();
}

QModelIndex TreeStyle_RDM::parent( const QModelIndex & index ) const
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::parent(): " << index.internalPointer();
	std::cerr << ": ";
#endif

	/* create the index */
	if (!index.isValid())
	{
#ifdef RDM_DEBUG
		std::cerr << "Invalid Index -> invalid";
		std::cerr << std::endl;
#endif
		/* Parent is invalid too */
		return QModelIndex();
	}
	void *ref = index.internalPointer();

	const DirDetails *details = requestDirDetails(ref, RemoteMode);

	if (!details)
	{
#ifdef RDM_DEBUG
		std::cerr << "Failed Lookup -> invalid";
		std::cerr << std::endl;
#endif
		return QModelIndex();
	}

	if (!(details->parent))
	{
#ifdef RDM_DEBUG
		std::cerr << "success. parent is Root/NULL --> invalid";
		std::cerr << std::endl;
#endif
		return QModelIndex();
	}

#ifdef RDM_DEBUG
	std::cerr << "success index(" << details->prow << ",0," << details->parent << ")";
	std::cerr << std::endl;

#endif
	return createIndex(details->prow, 0, details->parent);
}
QModelIndex FlatStyle_RDM::parent( const QModelIndex & index ) const
{
	Q_UNUSED(index);

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::parent(): " << index.internalPointer();
	std::cerr << ": ";
#endif

	return QModelIndex();
}
Qt::ItemFlags RetroshareDirModel::flags( const QModelIndex & index ) const
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::flags()";
	std::cerr << std::endl;
#endif

	if (!index.isValid())
		return (Qt::ItemIsSelectable); // Error.

	void *ref = index.internalPointer();

	const DirDetails *details = requestDirDetails(ref, RemoteMode);

	if (!details)
		return Qt::ItemIsSelectable; // Error.

	switch(details->type)
	{
	case DIR_TYPE_PERSON: return Qt::ItemIsEnabled;
	case DIR_TYPE_DIR:	 return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	case DIR_TYPE_FILE:	 return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
	}

	return Qt::ItemIsSelectable;
}

// The other flags...
//Qt::ItemIsUserCheckable
//Qt::ItemIsEditable
//Qt::ItemIsDropEnabled
//Qt::ItemIsTristate



/* Callback from */
void RetroshareDirModel::preMods()
{
	reset();

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::preMods()" << std::endl;
#endif
}

/* Callback from */
void RetroshareDirModel::postMods()
{
	emit layoutAboutToBeChanged();
#if QT_VERSION >= 0x040600
	beginResetModel();
#endif

//	QModelIndexList piList = persistentIndexList();
//	QModelIndexList empty;
//	for (int i = 0; i < piList.size(); i++) {
//		empty.append(QModelIndex());
//	}
//	changePersistentIndexList(piList, empty);

	/* Clear caches */
	mCache.clear();

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::postMods()" << std::endl;
#endif

#if QT_VERSION >= 0x040600
	endResetModel();
#endif
	emit layoutChanged();
}

const DirDetailsVector *RetroshareDirModel::requestDirDetails(void *ref, bool remote) const
{
	const QMap<void*, DirDetailsVector>::const_iterator it = mCache.constFind(ref);
	if (it != mCache.constEnd()) {
		/* Details found in cache */
		return &it.value();
	}

	/* Get details from the lib */
	DirDetailsVector details;
	FileSearchFlags flags = (remote) ? RS_FILE_HINTS_REMOTE : RS_FILE_HINTS_LOCAL;
	if (rsFiles->RequestDirDetails(ref, details, flags)) {
		/* Convert std::list to std::vector for fast access with index */
		std::list<DirStub>::const_iterator childIt;
		for (childIt = details.children.begin(); childIt != details.children.end(); ++childIt) {
			details.childrenVector.push_back(*childIt);
		}

		/* Add to cache, must cast to none const */
		const QMap<void*, DirDetailsVector>::iterator it1 = ((QMap<void*, DirDetailsVector>*) &mCache)->insert(ref, details);
		return &it1.value();
	}

	/* No details found */
	return NULL;
}

void RetroshareDirModel::createCollectionFile(QWidget *parent, const QModelIndexList &list)
{
	if(RemoteMode)
	{
		std::cerr << "Cannot create a collection file from remote" << std::endl;
		return ;
	}

	std::vector <DirDetails> dirVec;
	getDirDetailsFromSelect(list, dirVec);

	RsCollectionFile(dirVec).save(parent);
}

void RetroshareDirModel::downloadSelected(const QModelIndexList &list)
{
	if (!RemoteMode)
	{
#ifdef RDM_DEBUG
		std::cerr << "Cannot download from local" << std::endl;
#endif
		return ;
	}

	/* so for all the selected .... get the name out,
	 * make it into something the RsControl can understand
	 */

	 std::vector <DirDetails> dirVec;

   getDirDetailsFromSelect(list, dirVec);

	/* Fire off requests */
    for (int i = 0, n = dirVec.size(); i < n; ++i)
    {
        if (!RemoteMode)
        {
            continue; /* don't try to download local stuff */
        }

        const DirDetails& details = dirVec[i];

        /* if it is a file */
        if (details.type == DIR_TYPE_FILE)
        {
            std::cerr << "RetroshareDirModel::downloadSelected() Calling File Request";
            std::cerr << std::endl;
            std::list<std::string> srcIds;
            srcIds.push_back(details.id);
            rsFiles -> FileRequest(details.name, details.hash,
                    details.count, "", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds);
        }
        /* if it is a dir, copy all files included*/
        else if (details.type == DIR_TYPE_DIR)
        {
        	int prefixLen = details.path.rfind(details.name);
        	if (prefixLen < 0) continue;
        	downloadDirectory(details, prefixLen);
        }
    }
}

/* recursively download a directory */
void RetroshareDirModel::downloadDirectory(const DirDetails & dirDetails, int prefixLen)
{
	if (dirDetails.type & DIR_TYPE_FILE)
	{
		std::list<std::string> srcIds;
		QString cleanPath = QDir::cleanPath(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()) + "/" + QString::fromUtf8(dirDetails.path.substr(prefixLen).c_str()));

		srcIds.push_back(dirDetails.id);
		rsFiles->FileRequest(dirDetails.name, dirDetails.hash, dirDetails.count, cleanPath.toUtf8().constData(), RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds);
	}
	else if (dirDetails.type & DIR_TYPE_DIR)
	{
		std::list<DirStub>::const_iterator it;
		QDir dwlDir(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()));
		QString cleanPath = QDir::cleanPath(QString::fromUtf8(dirDetails.path.substr(prefixLen).c_str()));

		if (!dwlDir.mkpath(cleanPath)) return;

		for (it = dirDetails.children.begin(); it != dirDetails.children.end(); it++)
		{
			if (!it->ref) continue;

			const DirDetails *subDirDetails = requestDirDetails(it->ref, true);

			if (!subDirDetails) continue;

			downloadDirectory(*subDirDetails, prefixLen);
		}
	}
}

void RetroshareDirModel::getDirDetailsFromSelect (const QModelIndexList &list, std::vector <DirDetails>& dirVec)
{
    dirVec.clear();

    /* Fire off requests */
    QModelIndexList::const_iterator it;
    for(it = list.begin(); it != list.end(); it++)
    {
        if(it->column()==1)
        {
            void *ref = it -> internalPointer();

            const DirDetails *details = requestDirDetails(ref, RemoteMode);

            if (!details)
                continue;

            dirVec.push_back(*details);
        }
    }
}

/****************************************************************************
 * OLD RECOMMEND SYSTEM - DISABLED
 *
 */

void RetroshareDirModel::getFileInfoFromIndexList(const QModelIndexList& list, std::list<DirDetails>& file_details)
{
	file_details.clear() ;

#ifdef RDM_DEBUG
	std::cerr << "recommendSelected()" << std::endl;
#endif
	if (RemoteMode)
	{
#ifdef RDM_DEBUG
		std::cerr << "Cannot recommend remote! (should download)" << std::endl;
#endif
	}
	/* Fire off requests */

	std::set<std::string> already_in ;

	for(QModelIndexList::const_iterator it(list.begin()); it != list.end(); ++it)
		if(it->column()==0)
		{
			void *ref = it -> internalPointer();

			const DirDetails *details = requestDirDetails(ref, RemoteMode);

			if (!details)
				continue;

			if(details->type == DIR_TYPE_PERSON)
				continue ;

#ifdef RDM_DEBUG
			std::cerr << "::::::::::::FileRecommend:::: " << std::endl;
			std::cerr << "Name: " << details->name << std::endl;
			std::cerr << "Hash: " << details->hash << std::endl;
			std::cerr << "Size: " << details->count << std::endl;
			std::cerr << "Path: " << details->path << std::endl;
#endif
			// Note: for directories, the returned hash, is the peer id, so if we collect
			// dirs, we need to be a bit more conservative for the 

			if(already_in.find(details->hash+details->name) == already_in.end())
			{
				file_details.push_back(*details) ;
				already_in.insert(details->hash+details->name) ;
			}
		}
#ifdef RDM_DEBUG
	std::cerr << "::::::::::::Done FileRecommend" << std::endl;
#endif
}

/****************************************************************************
 * OLD RECOMMEND SYSTEM - DISABLED
 ******/

void RetroshareDirModel::openSelected(const QModelIndexList &qmil)
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::openSelected()" << std::endl;
#endif

	if (RemoteMode) {
#ifdef RDM_DEBUG
	std::cerr << "Cannot open remote. Download first." << std::endl;
#endif
	return;
	}

	std::list<std::string> dirs_to_open;

	std::list<DirDetails> files_info;
	std::list<DirDetails>::iterator it;
	getFileInfoFromIndexList(qmil, files_info);

	for (it = files_info.begin(); it != files_info.end(); it++) 
	{
		if ((*it).type & DIR_TYPE_PERSON) continue;

		std::string path, name;
		rsFiles->ConvertSharedFilePath((*it).path, path);

		QDir dir(QString::fromUtf8(path.c_str()));
		QString dest;
		if ((*it).type & DIR_TYPE_FILE) {
			dest = dir.absoluteFilePath(QString::fromUtf8(it->name.c_str()));
		} else if ((*it).type & DIR_TYPE_DIR) {
			dest = dir.absolutePath();
		}

		std::cerr << "Opening this file: " << dest.toStdString() << std::endl ;

		RsUrlHandler::openUrl(QUrl::fromLocalFile(dest));
	}

#ifdef RDM_DEBUG
	std::cerr << "::::::::::::Done RetroshareDirModel::openSelected()" << std::endl;
#endif
}

void RetroshareDirModel::getFilePaths(const QModelIndexList &list, std::list<std::string> &fullpaths)
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::getFilePaths()" << std::endl;
#endif
	if (RemoteMode)
	{
#ifdef RDM_DEBUG
		std::cerr << "No File Paths for remote files" << std::endl;
#endif
		return;
	}
	/* translate */
	QModelIndexList::const_iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		void *ref = it -> internalPointer();

    		const DirDetails *details = requestDirDetails(ref, false);

    		if (!details)
     		{
#ifdef RDM_DEBUG
			std::cerr << "getFilePaths() Bad Request" << std::endl;
#endif
			continue;
     		}

		if (details->type != DIR_TYPE_FILE)
		{
#ifdef RDM_DEBUG
			std::cerr << "getFilePaths() Not File" << std::endl;
#endif
			continue; /* not file! */
		}

#ifdef RDM_DEBUG
		std::cerr << "::::::::::::File Details:::: " << std::endl;
		std::cerr << "Name: " << details->name << std::endl;
		std::cerr << "Hash: " << details->hash << std::endl;
		std::cerr << "Size: " << details->count << std::endl;
		std::cerr << "Path: " << details->path << std::endl;
#endif

		std::string filepath = details->path + "/";
		filepath += details->name;

#ifdef RDM_DEBUG
		std::cerr << "Constructed FilePath: " << filepath << std::endl;
#endif
		if (fullpaths.end() == std::find(fullpaths.begin(), fullpaths.end(), filepath))
		{
			fullpaths.push_back(filepath);
		}
	}
#ifdef RDM_DEBUG
	std::cerr << "::::::::::::Done getFilePaths" << std::endl;
#endif
}

  /* Drag and Drop Functionality */
QMimeData * RetroshareDirModel::mimeData ( const QModelIndexList & indexes ) const
{
	/* extract from each the member text */
	QString text;
	QModelIndexList::const_iterator it;
	std::map<std::string, uint64_t> drags;
	std::map<std::string, uint64_t>::iterator dit;

	for(it = indexes.begin(); it != indexes.end(); it++)
	{
		void *ref = it -> internalPointer();

    		const DirDetails *details = requestDirDetails(ref, RemoteMode);

     		if (!details)
     		{
			continue;
     		}

#ifdef RDM_DEBUG
		std::cerr << "::::::::::::FileDrag:::: " << std::endl;
		std::cerr << "Name: " << details->name << std::endl;
		std::cerr << "Hash: " << details->hash << std::endl;
		std::cerr << "Size: " << details->count << std::endl;
		std::cerr << "Path: " << details->path << std::endl;
#endif

		if (details->type != DIR_TYPE_FILE)
		{
#ifdef RDM_DEBUG
			std::cerr << "RetroshareDirModel::mimeData() Not File" << std::endl;
#endif
			continue; /* not file! */
		}

		if (drags.end() != (dit = drags.find(details->hash)))
		{
#ifdef RDM_DEBUG
			std::cerr << "RetroshareDirModel::mimeData() Duplicate" << std::endl;
#endif
			continue; /* duplicate */
		}

		drags[details->hash] = details->count;

		QString line = QString("%1/%2/%3/").arg(QString::fromUtf8(details->name.c_str()), QString::fromStdString(details->hash), QString::number(details->count));

		if (RemoteMode)
		{
			line += "Remote";
		}
		else
		{
			line += "Local";
		}
		line += "/\n";

		text += line;
	}

#ifdef RDM_DEBUG
	std::cerr << "Created MimeData:";
	std::cerr << std::endl;

	std::cerr << text.toStdString();
	std::cerr << std::endl;
#endif

	QMimeData *data = new QMimeData();
	data->setData("application/x-rsfilelist", text.toUtf8());

	return data;
}

QStringList RetroshareDirModel::mimeTypes () const
{
	QStringList list;
	list.push_back("application/x-rsfilelist");

	return list;
}

//============================================================================

int RetroshareDirModel::getType ( const QModelIndex & index ) const
{
	//if (RemoteMode) // only local files can be opened
	//    return ;

	FileSearchFlags flags = (RemoteMode)?RS_FILE_HINTS_REMOTE:RS_FILE_HINTS_LOCAL;

	return rsFiles->getType(index.internalPointer(),flags);
}

FlatStyle_RDM::~FlatStyle_RDM()
{
}

TreeStyle_RDM::~TreeStyle_RDM()
{
}
void FlatStyle_RDM::postMods()
{
	if(visible())
	{
		_ref_entries.clear() ;
		_ref_stack.clear() ;

		_ref_stack.push_back(NULL) ; // init the stack with the topmost parent directory

		std::cerr << "FlatStyle_RDM::postMods(): cleared ref entries" << std::endl;
		_needs_update = false ;
		updateRefs() ;
	}
	else
		_needs_update = true ;
}

void FlatStyle_RDM::updateRefs()
{
	if(RsAutoUpdatePage::eventsLocked())
	{
		_needs_update = true ;
		return ;
	}

	RetroshareDirModel::preMods() ;

	static const uint32_t MAX_REFS_PER_SECOND = 2000 ;
	uint32_t nb_treated_refs = 0 ;

	while(!_ref_stack.empty())
	{
		void *ref = _ref_stack.back() ;
#ifdef RDM_DEBUG
		std::cerr << "FlatStyle_RDM::postMods(): poped ref " << ref << std::endl;
#endif
		_ref_stack.pop_back() ;
		const DirDetails *details = requestDirDetails(ref, RemoteMode) ;

		if (details)
		{
			if(details->type == DIR_TYPE_FILE)		// only push files, not directories nor persons.
				_ref_entries.push_back(std::pair<void*,QString>(ref,computeDirectoryPath(*details)));
#ifdef RDM_DEBUG
			std::cerr << "FlatStyle_RDM::postMods(): addign ref " << ref << std::endl;
#endif
			for(std::list<DirStub>::const_iterator it = details->children.begin(); it != details->children.end(); it++)
				_ref_stack.push_back(it->ref) ;
		}
		if(++nb_treated_refs > MAX_REFS_PER_SECOND) 	// we've done enough, let's give back hand to 
		{															// the user and setup a timer to finish the job later.
			_needs_update = true ;

			if(visible())
				QTimer::singleShot(2000,this,SLOT(updateRefs())) ;
			else
				std::cerr << "Not visible: suspending update"<< std::endl;
			break ;
		}
	}
	std::cerr << "reference tab contains " << _ref_entries.size() << " files" << std::endl;

	if(_ref_stack.empty())
		_needs_update = false ;

	RetroshareDirModel::postMods() ;
}

