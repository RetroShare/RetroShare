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
#include <gui/common/RsCollection.h>
#include <gui/common/RsUrlHandler.h>
#include <gui/common/FilesDefs.h>
#include <gui/common/GroupDefs.h>
#include <gui/gxs/GxsIdDetails.h>
#include "RemoteDirModel.h"
#include <retroshare/rsfiles.h>
#include <retroshare/rstypes.h>
#include <retroshare/rspeers.h>
#include "util/misc.h"

#include <set>
#include <algorithm>
#include <time.h>

/*****
 * #define RDM_DEBUG
 ****/

static const uint32_t FLAT_VIEW_MAX_REFS_PER_SECOND       = 2000 ;
static const uint32_t FLAT_VIEW_MAX_REFS_TABLE_SIZE       = 10000 ; //
static const uint32_t FLAT_VIEW_MIN_DELAY_BETWEEN_UPDATES = 120 ;	// dont rebuild ref list more than every 2 mins.

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

    mDirDetails.ref = (void*)intptr_t(0xffffffff) ;
    mLastRemote = false ;
    mUpdating = false;
}

// QAbstractItemModel::setSupportedDragActions() was replaced by virtual QAbstractItemModel::supportedDragActions()
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
Qt::DropActions RetroshareDirModel::supportedDragActions() const
{
	return Qt::CopyAction;
}
#endif

static bool isNewerThanEpoque(uint32_t ts)
{
    return ts > 0 ;	// this should be conservative enough
}


void RetroshareDirModel::treeStyle()
{
	categoryIcon.addPixmap(QPixmap(":/images/folder16.png"),
	                     QIcon::Normal, QIcon::Off);
	categoryIcon.addPixmap(QPixmap(":/images/folder_video.png"),
	                     QIcon::Normal, QIcon::On);
	peerIcon = QIcon(":/images/user/identity16.png");
}
void TreeStyle_RDM::update()
{
	preMods() ;
	postMods() ;
}
void TreeStyle_RDM::updateRef(const QModelIndex& indx) const
{
    rsFiles->requestDirUpdate(indx.internalPointer()) ;
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

    DirDetails details ;
    if (!requestDirDetails(ref, RemoteMode,details))
    {
		/* error */
#ifdef RDM_DEBUG
		std::cerr << "lookup failed -> false";
		std::cerr << std::endl;
#endif
		return false;
	}

    if (details.type == DIR_TYPE_FILE)
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
    return (details.count > 0); /* do we have children? */
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

    DirDetails details ;

    if (! requestDirDetails(ref, RemoteMode,details))
	{
#ifdef RDM_DEBUG
		std::cerr << "lookup failed -> 0";
		std::cerr << std::endl;
#endif
		return 0;
	}
    if (details.type == DIR_TYPE_FILE)
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
    return details.count;
}

int FlatStyle_RDM::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::rowCount(): " << parent.internalPointer();
	std::cerr << ": ";
#endif
    RS_STACK_MUTEX(_ref_mutex) ;

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

    if(flags & DIR_FLAGS_BROWSABLE) 	     str[0] = 'B' ;
    if(flags & DIR_FLAGS_ANONYMOUS_SEARCH) 	 str[3] = 'S' ;
	if(flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD) str[6] = 'N' ;

	return QString(str) ;
}
QString RetroshareDirModel::getGroupsString(FileStorageFlags flags,const std::list<RsNodeGroupId>& group_ids)
{
    if(!(flags & DIR_FLAGS_BROWSABLE))
        return QString();

    if(group_ids.empty())
        return tr("[All friend nodes]") ;

    QString groups_str = tr("Only ");
	RsGroupInfo group_info ;

    for(std::list<RsNodeGroupId>::const_iterator it(group_ids.begin());it!=group_ids.end();)
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
    int32_t age = time(NULL) - details.max_mtime;

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

const QIcon& RetroshareDirModel::getFlagsIcon(FileStorageFlags flags)
{
    static QIcon *static_icons[8] = {NULL};

    int n=0;
    if(flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD) n += 1 ;
    if(flags & DIR_FLAGS_ANONYMOUS_SEARCH  ) n += 2 ;
    if(flags & DIR_FLAGS_BROWSABLE         ) n += 4 ;
    n-= 1;

    if(static_icons[n] == NULL)
    {
        QList<QIcon> icons ;

        if(flags & DIR_FLAGS_ANONYMOUS_SEARCH)
            icons.push_back(QIcon(":icons/search_red_128.png")) ;
        else
            icons.push_back(QIcon(":icons/void_128.png")) ;

        if(flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD)
            icons.push_back(QIcon(":icons/anonymous_blue_128.png")) ;
        else
            icons.push_back(QIcon(":icons/void_128.png")) ;

        if(flags & DIR_FLAGS_BROWSABLE)
            icons.push_back(QIcon(":icons/browsable_green_128.png")) ;
        else
            icons.push_back(QIcon(":icons/void_128.png")) ;

        QPixmap pix ;
        GxsIdDetails::GenerateCombinedPixmap(pix, icons, 128);

        static_icons[n] = new QIcon(pix);

        std::cerr << "Generated icon for flags " << std::hex << flags << std::endl;
    }
    return *static_icons[n] ;
}

QVariant RetroshareDirModel::decorationRole(const DirDetails& details,int coln) const
{
    if(coln == 3)
    {
        if(details.type == DIR_TYPE_PERSON) return QVariant() ;

        return getFlagsIcon(details.flags) ;
    }


	if(coln > 0)
		return QVariant() ;

	if (details.type == DIR_TYPE_PERSON)
    {
        time_t now = time(NULL) ;

		if(ageIndicator != IND_ALWAYS && now > details.max_mtime + ageIndicator)
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
        time_t now = time(NULL) ;

		if(ageIndicator != IND_ALWAYS && now > details.max_mtime + ageIndicator)
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
        if(details.hash.isNull())
            return QIcon(":/images/reset.png") ; // file is being hashed
        else
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
		case 0: {
				//SharedDirStats stats ;
				QString res ;

				if(RemoteMode)
				{
					res = QString::fromUtf8(rsPeers->getPeerName(details.id).c_str());
				}
				else
				{
								res = tr("My files");
				}
				return res ;
			}
		case 1: {
				SharedDirStats stats ;
				QString res ;

				if(RemoteMode)
				{
					//res = QString::fromUtf8(rsPeers->getPeerName(details.id).c_str());
					rsFiles->getSharedDirStatistics(details.id,stats) ;
				}
				else
				{
								//res = tr("My files");
								rsFiles->getSharedDirStatistics(rsPeers->getOwnId(),stats) ;
				}

				if(stats.total_number_of_files > 0)
					res += QString::number(stats.total_number_of_files) + " files, " + misc::friendlyUnit(stats.total_shared_size) ;

				return res ;
			}
		case 2: 	if(!isNewerThanEpoque(details.max_mtime))
					return QString();

				else
		        		return misc::timeRelativeToNow(details.max_mtime);

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
                return  misc::timeRelativeToNow(details.max_mtime);
			case 3:
                return QVariant();
			case 4:
                return getGroupsString(details.flags,details.parent_groups) ;

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
				return misc::timeRelativeToNow(details.max_mtime);
			case 3:
                return QVariant();
			case 4: 
                return getGroupsString(details.flags,details.parent_groups) ;

			default:
				return tr("DIR");
		}
	}
	return QVariant();
} /* end of DisplayRole */

FlatStyle_RDM::FlatStyle_RDM(bool mode)
    : RetroshareDirModel(mode), _ref_mutex("Flat file list")
{
	_needs_update = true ;

	{
		RS_STACK_MUTEX(_ref_mutex) ;
        _last_update = 0 ;
	}
}

void FlatStyle_RDM::update()
{
	if(_needs_update)
	{
		preMods() ;
		postMods() ;
	}
}
QString FlatStyle_RDM::computeDirectoryPath(const DirDetails& details) const
{
	QString dir ;
    DirDetails det ;

    if(!requestDirDetails(details.parent,RemoteMode,det))
        return QString();

#ifdef SHOW_TOTAL_PATH
	do
	{
#endif
        dir = QString::fromUtf8(det.name.c_str())+"/"+dir ;

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
            case 2: return misc::timeRelativeToNow(details.max_mtime);
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
				return details.max_mtime;
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
                return  details.max_mtime;
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
				return details.max_mtime;
			case 3:
				return getFlagsString(details.flags);
			default:
				return tr("DIR");
		}
	}
	return QVariant();
}
QVariant FlatStyle_RDM::sortRole(const QModelIndex& /*index*/,const DirDetails& details,int coln) const
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
            case 2: return  details.max_mtime;
			case 3: return QString::fromUtf8(rsPeers->getPeerName(details.id).c_str());

        case 4: {
            RS_STACK_MUTEX(_ref_mutex) ;

            return computeDirectoryPath(details);
        }
		}
	}
	return QVariant();
} /* end of SortRole */




QVariant RetroshareDirModel::data(const QModelIndex &index, int role) const
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::data(): " << index.internalPointer() << std::endl;
#endif

	if (!index.isValid())
		return QVariant();

    // First we take care of the cases that do not require requestDirDetails()

	int coln = index.column();

	if (role == Qt::SizeHintRole)
		return QVariant();

	if (role == Qt::TextAlignmentRole)
	{
		if(coln == 1)
			return int( Qt::AlignRight | Qt::AlignVCenter);
        else
			return QVariant();
	}

	// This makes sorting a bit arbitrary, but prevents calling requestDirDetails(). The number of calls to sortRole is
    // indeed sometimes very high and somewhat kills the GUI responsivness.
    //
	//if (role == SortRole)
	//   return QVariant(index.row()) ;

	/* get the data from the index */
	void *ref = index.internalPointer();

	DirDetails details;

	if(!requestDirDetails(ref, RemoteMode,details))
		return QVariant() ;

	if (role == RetroshareDirModel::FileNameRole) /* end of FileNameRole */
		return QString::fromUtf8(details.name.c_str()) ;

	if (role == Qt::TextColorRole)
	{
        if(details.type == DIR_TYPE_FILE && details.hash.isNull())
            return QVariant(QColor(Qt::green)) ;
        else if(ageIndicator != IND_ALWAYS && details.max_mtime + ageIndicator < time(NULL))
			return QVariant(QColor(Qt::gray)) ;
        else if(RemoteMode)
        {
            FileInfo info;
            QVariant local_file_color = QVariant(QColor(Qt::red));
            if(rsFiles->alreadyHaveFile(details.hash, info))
                return local_file_color;

            std::list<RsFileHash> downloads;
            rsFiles->FileDownloads(downloads);
            if(std::find(downloads.begin(), downloads.end(), details.hash) != downloads.end())
                return local_file_color;
            else
                return QVariant();
        }
		else
			return QVariant() ; // standard
	} /* end of TextColorRole */


	if(role == Qt::DecorationRole)
        return decorationRole(details,coln) ;

    if(role == Qt::ToolTipRole)
        if(!isNewerThanEpoque(details.max_mtime))
            return tr("This node hasn't sent any directory information yet.") ;

    /*****************
	  Qt::EditRole
	  Qt::ToolTipRole
	  Qt::StatusTipRole
	  Qt::WhatsThisRole
	  Qt::SizeHintRole
	 ****************/

	if (role == Qt::DisplayRole)
        return displayRole(details,coln) ;

	if (role == SortRole)
        return sortRole(index,details,coln) ;

	return QVariant();
}

//void RetroshareDirModel::getAgeIndicatorRec(const DirDetails &details, QString &ret) const {
//	if (details.type == DIR_TYPE_FILE) {
//		ret = getAgeIndicatorString(details);
//		return;
//	} else if (details.type == DIR_TYPE_DIR && ret.isEmpty()) {
//		std::list<DirStub>::const_iterator it;
//		for (it = details.children.begin(); it != details.children.end(); ++it) {
//			void *ref = it->ref;
//			const DirDetails *childDetails = requestDirDetails(ref, RemoteMode);

//			if (childDetails && ret == tr(""))
//				getAgeIndicatorRec(*childDetails, ret);
//		}
//	}
//}

QVariant TreeStyle_RDM::headerData(int section, Qt::Orientation orientation, int role) const
{
	/*if (role == Qt::SizeHintRole)
	{
		int defw = QFontMetricsF(QWidget().font()).width(headerData(section,Qt::Horizontal,Qt::DisplayRole).toString()) ;
		int defh = QFontMetricsF(QWidget().font()).height();

		if (section < 2)
		{
			defw = 200/16.0*defh;
		}
		return QSize(defw, defh);
	}*/

	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
	{
		switch(section)
		{
			case 0:
				if (RemoteMode)
                    if(mUpdating)
						return tr("Friends Directories [updating...]");
					else
						return tr("Friends Directories");
				else
                    if(mUpdating)
						return tr("My Directories [updating...]");
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
                    return tr("Access");
			case 4:
				if (RemoteMode)
					return tr("What's new");
				else
                    return tr("Visibility");
		}
		return QString("Column %1").arg(section);
	}
	else
		return QString("Row %1").arg(section);
}
QVariant FlatStyle_RDM::headerData(int section, Qt::Orientation orientation, int role) const
{
	/*if (role == Qt::SizeHintRole)
	{
		int defw = QFontMetricsF(QWidget().font()).width(headerData(section,Qt::Horizontal,Qt::DisplayRole).toString()) ;
		int defh = QFontMetricsF(QWidget().font()).height();

		if (section < 2)
		{
			defw = defh*200/16.0;
		}
		return QSize(defw, defh);
	}*/

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

    // This function is used extensively. There's no way we can use requestDirDetails() in it, which would
    // cause far too much overhead. So we use a dedicated function that only grabs the required information.

	if(row < 0)
		return QModelIndex() ;

	void *ref = (parent.isValid()) ? parent.internalPointer() : NULL;

	/********
	  if (!RemoteMode)
	  {
	  remote = &(rsiface->getLocalDirectoryList());
	  }
	 ********/


    void *result ;

    if(rsFiles->findChildPointer(ref, row, result,  ((RemoteMode) ? RS_FILE_HINTS_REMOTE : RS_FILE_HINTS_LOCAL)))
        return createIndex(row, column, result) ;
    else
		return QModelIndex();
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

    RS_STACK_MUTEX(_ref_mutex) ;

    if(row < (int) _ref_entries.size())
	{
        void *ref = _ref_entries[row];

#ifdef RDM_DEBUG
    std::cerr << "Creating index 2 row=" << row << ", column=" << column << ", ref=" << (void*)ref << std::endl;
#endif
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

    DirDetails details ;

    if (! requestDirDetails(ref, RemoteMode,details))
    {
#ifdef RDM_DEBUG
		std::cerr << "Failed Lookup -> invalid";
		std::cerr << std::endl;
#endif
		return QModelIndex();
	}

    if (!(details.parent))
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

    std::cerr << "Creating index 3 row=" << details.prow << ", column=" << 0 << ", ref=" << (void*)details.parent << std::endl;
#endif
    return createIndex(details.prow, 0, details.parent);
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

    DirDetails details ;

    if (! requestDirDetails(ref, RemoteMode,details))
        return Qt::ItemIsSelectable; // Error.

    switch(details.type)
	{
    // we grey out a person that has never been updated. It's easy to spot these, since the min age of the directory is approx equal to time(NULL), which exceeds 40 years.
    case DIR_TYPE_PERSON:return isNewerThanEpoque(details.max_mtime)? (Qt::ItemIsEnabled):(Qt::NoItemFlags) ;
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
    emit layoutAboutToBeChanged();
    mUpdating = true ;
#if QT_VERSION < 0x050000
	reset();
#else
	beginResetModel();
	endResetModel();
#endif

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::preMods()" << std::endl;
#endif
}

/* Callback from */
void RetroshareDirModel::postMods()
{
//	emit layoutAboutToBeChanged();
    mUpdating = false ;
#if QT_VERSION >= 0x040600
	beginResetModel();
#endif

#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::postMods()" << std::endl;
#endif

#if QT_VERSION >= 0x040600
	endResetModel();
#endif
	emit layoutChanged();
}

bool RetroshareDirModel::requestDirDetails(void *ref, bool remote,DirDetails& d) const
{
#ifdef RDM_DEBUG
	std::cerr << "RequestDirDetails:: ref = " << ref << ", remote=" << remote << std::endl;
#endif

    // We look in cache and re-use the last result if the reference and remote are the same.

    time_t now = time(NULL);

    if(mDirDetails.ref == ref && mLastRemote==remote && now < 2+mLastReq)
    {
        d = mDirDetails ;
        return true ;
    }

    FileSearchFlags flags = (remote) ? RS_FILE_HINTS_REMOTE : RS_FILE_HINTS_LOCAL;

    if(rsFiles->RequestDirDetails(ref, d, flags))
	{
		mLastReq = now ;
		mLastRemote = remote ;
		mDirDetails = d;

		return true;
	}

    return false ;
}

void RetroshareDirModel::createCollectionFile(QWidget *parent, const QModelIndexList &list)
{
/*	if(RemoteMode)
	{
		std::cerr << "Cannot create a collection file from remote" << std::endl;
		return ;
	}*/

	std::vector <DirDetails> dirVec;
	getDirDetailsFromSelect(list, dirVec);

	RsCollection(dirVec).openNewColl(parent);
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
            std::list<RsPeerId> srcIds;
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
		std::list<RsPeerId> srcIds ;
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

        for(uint32_t i=0;i<dirDetails.children.size();++i)
		{
            if (!dirDetails.children[i].ref) continue;

            DirDetails subDirDetails ;

            if(!requestDirDetails(dirDetails.children[i].ref, true,subDirDetails))
                continue;

            downloadDirectory(subDirDetails, prefixLen);
		}
	}
}

void RetroshareDirModel::getDirDetailsFromSelect (const QModelIndexList &list, std::vector <DirDetails>& dirVec)
{
    dirVec.clear();

    /* Fire off requests */
    QModelIndexList::const_iterator it;
    for(it = list.begin(); it != list.end(); ++it)
    {
        if(it->column()==1)
        {
            void *ref = it -> internalPointer();

            DirDetails details ;

            if(!requestDirDetails(ref, RemoteMode,details))
                continue;

            dirVec.push_back(details);
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

            DirDetails details;

            if (!requestDirDetails(ref, RemoteMode,details))
				continue;

            if(details.type == DIR_TYPE_PERSON)
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

            if(already_in.find(details.hash.toStdString()+details.name) == already_in.end())
			{
                file_details.push_back(details) ;
                already_in.insert(details.hash.toStdString()+details.name) ;
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

	for (it = files_info.begin(); it != files_info.end(); ++it)
	{
		if ((*it).type & DIR_TYPE_PERSON) continue;

        //std::string path, name;
        //rsFiles->ConvertSharedFilePath((*it).path, path);

        QDir dir(QString::fromUtf8((*it).path.c_str()));
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

void RetroshareDirModel::getFilePath(const QModelIndex& index, std::string& fullpath)
{
    void *ref = index.sibling(index.row(),1).internalPointer();

    DirDetails details ;

    if (!requestDirDetails(ref, false,details) )
    {
#ifdef RDM_DEBUG
        std::cerr << "getFilePaths() Bad Request" << std::endl;
#endif
        return;
    }

    fullpath = details.path + "/" + details.name;
}

void RetroshareDirModel::getFilePaths(const QModelIndexList &list, std::list<std::string> &fullpaths)
{
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::getFilePaths()" << std::endl;
#endif
#warning mr-alice: make sure we atually output something here
	if (RemoteMode)
	{
#ifdef RDM_DEBUG
		std::cerr << "No File Paths for remote files" << std::endl;
#endif
		return;
	}
	/* translate */
    for(QModelIndexList::const_iterator it = list.begin(); it != list.end(); ++it)
	{
        std::string path ;

        getFilePath(*it,path) ;
#ifdef RDM_DEBUG
        std::cerr << "Constructed FilePath: " << path << std::endl;
#endif
#warning mr-alice: TERRIBLE COST here. Use a std::set!
        if (fullpaths.end() == std::find(fullpaths.begin(), fullpaths.end(), path))
            fullpaths.push_back(path);
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
    std::map<RsFileHash, uint64_t> drags;
    std::map<RsFileHash, uint64_t>::iterator dit;

	for(it = indexes.begin(); it != indexes.end(); ++it)
	{
		void *ref = it -> internalPointer();

            DirDetails details ;
            if (!requestDirDetails(ref, RemoteMode,details))
                continue;

#ifdef RDM_DEBUG
		std::cerr << "::::::::::::FileDrag:::: " << std::endl;
		std::cerr << "Name: " << details->name << std::endl;
		std::cerr << "Hash: " << details->hash << std::endl;
		std::cerr << "Size: " << details->count << std::endl;
		std::cerr << "Path: " << details->path << std::endl;
#endif

        if (details.type != DIR_TYPE_FILE)
		{
#ifdef RDM_DEBUG
			std::cerr << "RetroshareDirModel::mimeData() Not File" << std::endl;
#endif
			continue; /* not file! */
		}

        if (drags.end() != (dit = drags.find(details.hash)))
		{
#ifdef RDM_DEBUG
			std::cerr << "RetroshareDirModel::mimeData() Duplicate" << std::endl;
#endif
			continue; /* duplicate */
		}

        drags[details.hash] = details.count;

        QString line = QString("%1/%2/%3/").arg(QString::fromUtf8(details.name.c_str()), QString::fromStdString(details.hash.toStdString()), QString::number(details.count));

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
    time_t now = time(NULL);

    if(_last_update + FLAT_VIEW_MIN_DELAY_BETWEEN_UPDATES > now)
        return ;

    if(visible())
	{
        emit layoutAboutToBeChanged();

        {
            RS_STACK_MUTEX(_ref_mutex) ;
            _ref_stack.clear() ;
            _ref_stack.push_back(NULL) ; // init the stack with the topmost parent directory
            _ref_entries.clear();
            _last_update = now;
        }
        QTimer::singleShot(100,this,SLOT(updateRefs())) ;
    }
	else
		_needs_update = true ;
}

void FlatStyle_RDM::updateRefs()
{
	if(RsAutoUpdatePage::eventsLocked())
	{
		QTimer::singleShot(5000,this,SLOT(updateRefs())) ;
		return ;
	}

    RetroshareDirModel::preMods() ;


	uint32_t nb_treated_refs = 0 ;

    {
        RS_STACK_MUTEX(_ref_mutex) ;

        while(!_ref_stack.empty())
        {
            void *ref = _ref_stack.back() ;
#ifdef RDM_DEBUG
            std::cerr << "FlatStyle_RDM::postMods(): poped ref " << ref << std::endl;
#endif
            _ref_stack.pop_back() ;

            DirDetails details ;

            if (requestDirDetails(ref, RemoteMode,details))
            {
                if(details.type == DIR_TYPE_FILE)		// only push files, not directories nor persons.
                    _ref_entries.push_back(ref) ;
#ifdef RDM_DEBUG
                std::cerr << "FlatStyle_RDM::postMods(): adding ref " << ref << std::endl;
#endif
                for(uint32_t i=0;i<details.children.size();++i)
                    _ref_stack.push_back(details.children[i].ref) ;
            }

            // Limit the size of the table to display, otherwise it becomes impossible to Qt.

            if(_ref_entries.size() > FLAT_VIEW_MAX_REFS_TABLE_SIZE)
                return ;

            if(++nb_treated_refs > FLAT_VIEW_MAX_REFS_PER_SECOND) 	// we've done enough, let's give back hand to
            {															// the user and setup a timer to finish the job later.
                if(visible())
                    QTimer::singleShot(2000,this,SLOT(updateRefs())) ;
                else
                    std::cerr << "Not visible: suspending update"<< std::endl;
                break ;
            }
        }
        std::cerr << "reference tab contains " << std::dec << _ref_entries.size() << " files" << std::endl;
    }

    RetroshareDirModel::postMods() ;
}

