/*******************************************************************************
 * gui/RemoteDirModel.cpp                                                      *
 *                                                                             *
 * Copyright (c) 2006 Retroshare Team  <retroshare.project@gmail.com>          *
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

#include "RemoteDirModel.h"

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "gui/common/FilesDefs.h"
#include "gui/common/GroupDefs.h"
#include "gui/common/RsCollectionDialog.h"
#include "gui/common/RsUrlHandler.h"
#include "gui/gxs/GxsIdDetails.h"
#include "retroshare/rsfiles.h"
#include "retroshare/rspeers.h"
#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "retroshare/rsexpr.h"

#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <set>
#include <time.h>
#include <iostream> // Keep iostream for debug logs

/*****
 * #define RDM_DEBUG
 * #define RDM_SEARCH_DEBUG
 ****/

#define REMOTEDIRMODEL_COLUMN_NAME          0
#define REMOTEDIRMODEL_COLUMN_FILENB        1
#define REMOTEDIRMODEL_COLUMN_SIZE          2
#define REMOTEDIRMODEL_COLUMN_AGE           3
#define REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS 4
#define REMOTEDIRMODEL_COLUMN_WN_VISU_DIR   5
#define REMOTEDIRMODEL_COLUMN_UPLOADED      6
#define REMOTEDIRMODEL_COLUMN_COUNT         7

static const uint32_t FLAT_VIEW_MAX_REFS_PER_SECOND       = 10000 ;
static const size_t   FLAT_VIEW_MAX_REFS_TABLE_SIZE       = 30000 ; //
static const uint32_t FLAT_VIEW_MIN_DELAY_BETWEEN_UPDATES = 120 ;	// dont rebuild ref list more than every 2 mins.

RetroshareDirModel::RetroshareDirModel(bool mode, QObject *parent)
  : QAbstractItemModel(parent), _visible(false)
  , ageIndicator(IND_ALWAYS)
  , RemoteMode(mode)//, nIndex(1), indexSet(1) /* ass zero index cant be used */
  , mLastRemote(false), mLastReq(0), mUpdating(false)
  , mEventHandlerId(0)
{
#if QT_VERSION < QT_VERSION_CHECK (5, 0, 0)
	setSupportedDragActions(Qt::CopyAction);
#endif
	treeStyle();

	mDirDetails.ref = (void*)intptr_t(0xffffffff) ;

	rsEvents->registerEventsHandler(
	            [this](std::shared_ptr<const RsEvent> event)
	                  {
	                  	RsQThreadUtils::postToObject( [this,event]()  {  handleEvent_main_thread(event);  });
	                  }
	            , mEventHandlerId
	            , RsEventType::SHARED_DIRECTORIES );
}

RetroshareDirModel::~RetroshareDirModel()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
}
void RetroshareDirModel::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    if(event->mType != RsEventType::SHARED_DIRECTORIES) return;

    const RsSharedDirectoriesEvent *fe = dynamic_cast<const RsSharedDirectoriesEvent*>(event.get());
    if(!fe)
        return;

    switch (fe->mEventCode)
    {
    case RsSharedDirectoriesEventCode::EXTRA_LIST_FILE_ADDED:
    case RsSharedDirectoriesEventCode::EXTRA_LIST_FILE_REMOVED:
        update();
        break;
    default:
        break;
    }
}

TreeStyle_RDM::TreeStyle_RDM(bool mode)
  : RetroshareDirModel(mode), _showEmpty(false)
{
	_showEmptyAct = new QAction(QIcon(), tr("Show Empty"), this);
	_showEmptyAct->setCheckable(true);
	_showEmptyAct->setChecked(_showEmpty);
	connect(_showEmptyAct, SIGNAL(toggled(bool)), this, SLOT(showEmpty(bool)));
}

FlatStyle_RDM::FlatStyle_RDM(bool mode)
  : RetroshareDirModel(mode), _ref_mutex("Flat file list")
{
	_needs_update = true ;

	{
		RS_STACK_MUTEX(_ref_mutex) ;
		_last_update = 0 ;
	}
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
	categoryIcon.addPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/folder.png"), QIcon::Normal, QIcon::Off);
	categoryIcon.addPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/folderopen.png"), QIcon::Normal, QIcon::On);
	peerIcon = FilesDefs::getIconFromQtResourcePath(":/icons/folder-account.svg");
}

void TreeStyle_RDM::recalculateDirectoryTotals()
{
    m_folderUploadTotals.clear();
    m_folderFileTotals.clear();
    m_folderSizeTotals.clear();

    // Stats are primarily calculated for local files. 
    // For remote files, branch stats are limited by what the core provides.
    if(RemoteMode)
        return;

    // Start recursion from Root (NULL)
    collectStatsRecursive(NULL);
}

TreeStyle_RDM::FolderStats TreeStyle_RDM::collectStatsRecursive(void* ref)
{
    FolderStats stats;
    
    // CRITICAL FIX: Safety check for NULL pointers (except for Root which can be NULL start)
    // Actually requestDirDetails handles NULL ref for Root correctly.
    
    DirDetails details;
    if(!requestDirDetails(ref, RemoteMode, details))
    {
        return stats;
    }

    // If it's a file, we return its stats
    if(details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE)
    {
        stats.size = details.size;
        stats.count = 1;
        stats.uploads = rsFiles->getCumulativeUpload(details.hash);

        // Special handling for Extra Files (virtual root)
        if (details.type == DIR_TYPE_EXTRA_FILE) 
        {
            m_folderUploadTotals["!!RS_EXTRA_FILES_ROOT!!"] += stats.uploads;
            m_folderFileTotals["!!RS_EXTRA_FILES_ROOT!!"] += stats.count;
            m_folderSizeTotals["!!RS_EXTRA_FILES_ROOT!!"] += stats.size;
        }
        
        return stats;
    }
    
    // If it's a directory or root node, we dive into children
    if(details.type == DIR_TYPE_DIR || details.type == DIR_TYPE_ROOT || details.type == DIR_TYPE_PERSON)
    {
        for(const auto& child : details.children)
        {
            FolderStats childStats = collectStatsRecursive(child.ref);
            
            stats.size += childStats.size;
            stats.count += childStats.count;
            stats.uploads += childStats.uploads;
        }

        // If this is a real directory, store the aggregated totals
        if(details.type == DIR_TYPE_DIR)
        {
            QString path = QDir::cleanPath(QString::fromUtf8(details.path.c_str()));
            
            m_folderUploadTotals[path] = stats.uploads;
            m_folderFileTotals[path]   = stats.count;
            m_folderSizeTotals[path]   = stats.size;
        }
    }
    
    return stats;
}

// MODIFICATION D: Check if a specific node or any of its descendants have uploads
// hasUploads for TreeStyle - Defensive Check
bool TreeStyle_RDM::hasUploads(void *ref) const
{
    // CRITICAL FIX: Safety check for NULL pointers
    if (ref == NULL) {
        return false;
    }

    DirDetails details;
    if (!requestDirDetails(ref, RemoteMode, details)) return false;

    if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE) {
        return rsFiles->getCumulativeUpload(details.hash) > 0;
    }

    if (details.type == DIR_TYPE_PERSON && details.id != rsPeers->getOwnId()) {
        return m_folderUploadTotals.value("!!RS_EXTRA_FILES_ROOT!!", 0) > 0;
    }

    if (details.type == DIR_TYPE_DIR) {
        QString path = QDir::cleanPath(QString::fromUtf8(details.path.c_str()));
        return m_folderUploadTotals.value(path, 0) > 0;
    }

    return true; 
}

void TreeStyle_RDM::update()
{

    // Recalculate totals before notifying view update
    recalculateDirectoryTotals();

	preMods() ;
	postMods() ;
}
void TreeStyle_RDM::updateRef(const QModelIndex& indx) const
{
    rsFiles->requestDirUpdate(indx.internalPointer()) ;
}

QMenu* TreeStyle_RDM::getContextMenu(QMenu* contextMenu)
{
	if (!contextMenu){
		contextMenu = new QMenu();
	} else {
		if (RemoteMode)
			contextMenu->addSeparator();
	}

	if (RemoteMode)
		contextMenu->addAction( _showEmptyAct) ;

	return contextMenu;
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

    if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE)
	{
#ifdef RDM_DEBUG
		std::cerr << "lookup FILE -> false";
		std::cerr << std::endl;
#endif
		return false;
	}
	/* PERSON/DIR*/
#ifdef RDM_DEBUG
    std::cerr << "lookup PER/DIR #" << details.size;
	std::cerr << std::endl;
#endif
    return (details.children.size() > 0); /* do we have children? */
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

	if ((!ref) && RemoteMode)
		_parentRow.clear(); //Only clear it when asking root child number and in remote mode.

	DirDetails details ;

	if (! requestDirDetails(ref, RemoteMode,details))
	{
#ifdef RDM_DEBUG
		std::cerr << "lookup failed -> 0";
		std::cerr << std::endl;
#endif
		return 0;
	}
	if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE)
	{
#ifdef RDM_DEBUG
		std::cerr << "lookup FILE: 0";
		std::cerr << std::endl;
#endif
		return 0;
	}

	/* else PERSON/DIR*/
#ifdef RDM_DEBUG
	std::cerr << "lookup PER/DIR #" << details.size;
	std::cerr << std::endl;
#endif
	if ((details.type == DIR_TYPE_ROOT) && !_showEmpty && RemoteMode)
	{
		DirDetails childDetails;
		//Scan all children to know if they are empty.
        for(uint64_t i = 0; i < details.children.size(); ++i)
		{
            if (requestDirDetails(details.children[i].ref, RemoteMode,childDetails) && (childDetails.children.size() > 0))
				_parentRow.push_back(i);
		}
		return _parentRow.size();
	}

    // MODIFICATION: Removed the manual mFilteredPointers count logic. 
    // QSortFilterProxyModel relies on the source model reporting all rows. 
    // Filtering here creates an index mismatch that results in an empty list.
	return details.children.size();
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
	return REMOTEDIRMODEL_COLUMN_COUNT;
}
int FlatStyle_RDM::columnCount(const QModelIndex &/*parent*/) const
{
	return REMOTEDIRMODEL_COLUMN_COUNT;
}

// MODIFIED: Return empty string to hide B, S, N letters
QString RetroshareDirModel::getFlagsString(FileStorageFlags /*flags*/)
{
	return QString("") ;
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
    if(static_icons[0] == NULL)
        static_icons[0] = new QIcon();

    int n=0;
    if(flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD) n += 1 ;
    if(flags & DIR_FLAGS_ANONYMOUS_SEARCH  ) n += 2 ;
    if(flags & DIR_FLAGS_BROWSABLE         ) n += 4 ;

    if(static_icons[n] == NULL)
    {
        QList<QIcon> icons ;

        if(flags & DIR_FLAGS_ANONYMOUS_SEARCH)
            icons.push_back(FilesDefs::getIconFromQtResourcePath(":icons/search_red_128.png")) ;
        else
            icons.push_back(FilesDefs::getIconFromQtResourcePath(":icons/void_128.png")) ;

        if(flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD)
            icons.push_back(FilesDefs::getIconFromQtResourcePath(":icons/anonymous_blue_128.png")) ;
        else
            icons.push_back(FilesDefs::getIconFromQtResourcePath(":icons/void_128.png")) ;

        if(flags & DIR_FLAGS_BROWSABLE)
            icons.push_back(FilesDefs::getIconFromQtResourcePath(":icons/browsable_green_128.png")) ;
        else
            icons.push_back(FilesDefs::getIconFromQtResourcePath(":icons/void_128.png")) ;

        QPixmap pix ;
        GxsIdDetails::GenerateCombinedPixmap(pix, icons, 128);

        static_icons[n] = new QIcon(pix);

        std::cerr << "Generated icon for flags " << std::hex << flags << std::dec << std::endl;
    }
    return *static_icons[n] ;
}

QVariant RetroshareDirModel::filterRole(const DirDetails& details, int /*coln*/) const
{
	if(mFilteredPointers.empty() || mFilteredPointers.find(details.ref) != mFilteredPointers.end())
		return QString(RETROSHARE_DIR_MODEL_FILTER_STRING);
	else
		return QVariant();
}

QVariant RetroshareDirModel::decorationRole(const DirDetails& details,int coln) const
{
    if(coln == REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS)
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
			return FilesDefs::getIconFromQtResourcePath(":/images/folder_grey.png");
		else if (ageIndicator == IND_LAST_DAY )
			return FilesDefs::getIconFromQtResourcePath(":/images/folder_green.png");
		else if (ageIndicator == IND_LAST_WEEK )
			return FilesDefs::getIconFromQtResourcePath(":/images/folder_yellow.png");
		else if (ageIndicator == IND_LAST_MONTH )
			return FilesDefs::getIconFromQtResourcePath(":/images/folder_red.png");
		else
			return (QIcon(peerIcon));
	}
	else if (details.type == DIR_TYPE_DIR)
	{
        time_t now = time(NULL) ;

		if(ageIndicator != IND_ALWAYS && now > details.max_mtime + ageIndicator)
			return FilesDefs::getIconFromQtResourcePath(":/images/folder_grey.png");
		else if (ageIndicator == IND_LAST_DAY )
			return FilesDefs::getIconFromQtResourcePath(":/images/folder_green.png");
		else if (ageIndicator == IND_LAST_WEEK )
			return FilesDefs::getIconFromQtResourcePath(":/images/folder_yellow.png");
		else if (ageIndicator == IND_LAST_MONTH )
			return FilesDefs::getIconFromQtResourcePath(":/images/folder_red.png");
		else
			return QIcon(categoryIcon);
	}
	else if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE) /* File */
	{
		// extensions predefined
        if(details.hash.isNull())
            return FilesDefs::getIconFromQtResourcePath(":/images/reset.png") ; // file is being hashed
        else
            return FilesDefs::getIconFromFileType(QString::fromUtf8(details.name.c_str()));
	}
	else
		return QVariant();

} /* end of DecorationRole */

void FlatStyle_RDM::update()
{
	// MODIFICATION: Always update to support filtering changes.
	// Previously, this was guarded by if(_needs_update), preventing filter updates.
// if(_needs_update)
	{
        if(mUpdating) return;


        
        {
            RS_STACK_MUTEX(_ref_mutex);
            m_cache.clear();
        }

		preMods() ;
		postMods() ;
	}
}

bool FlatStyle_RDM::isMaxRefsTableSize(size_t *maxSize/*=NULL*/)
{
	if (maxSize)
		*maxSize = FLAT_VIEW_MAX_REFS_TABLE_SIZE;

	return (_ref_entries.size() >= FLAT_VIEW_MAX_REFS_TABLE_SIZE);
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

QVariant FlatStyle_RDM::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        void *ref = index.internalPointer();
        
        // Try Lock-less read first? No, use lock for safety matching updateRefs
        RS_STACK_MUTEX(_ref_mutex);
        if (m_cache.contains(ref))
        {
            const CachedFileDetails &cfd = m_cache[ref];
            switch(index.column())
            {
                case REMOTEDIRMODEL_COLUMN_NAME: return cfd.name;
                case REMOTEDIRMODEL_COLUMN_SIZE: return cfd.sizeStr;
                case REMOTEDIRMODEL_COLUMN_AGE:  return cfd.ageStr;
                case REMOTEDIRMODEL_COLUMN_UPLOADED: return cfd.uploadStr;
            }
        }
    }

    return RetroshareDirModel::data(index, role); 
}

QVariant FlatStyle_RDM::displayRole(const DirDetails& details,int coln) const
{

	if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE) /* File */
		switch(coln)
		{
			case REMOTEDIRMODEL_COLUMN_NAME: return QString::fromUtf8(details.name.c_str());
			case REMOTEDIRMODEL_COLUMN_FILENB: return QString();
            case REMOTEDIRMODEL_COLUMN_SIZE: return misc::friendlyUnit(details.size);
			case REMOTEDIRMODEL_COLUMN_AGE: return misc::timeRelativeToNow(details.max_mtime);
			case REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS: return QString::fromUtf8(rsPeers->getPeerName(details.id).c_str());
			case REMOTEDIRMODEL_COLUMN_WN_VISU_DIR: return computeDirectoryPath(details);
			case REMOTEDIRMODEL_COLUMN_UPLOADED:
			{
				uint64_t x = rsFiles->getCumulativeUpload(details.hash);
				if(x)
					return misc::friendlyUnit(x);
				else
					return QString();
			}
			default:
				return QVariant() ;
		}

	return QVariant();
} /* end of DisplayRole */

QVariant TreeStyle_RDM::sortRole(const QModelIndex& /*index*/,const DirDetails& details,int coln) const
{
	if (details.type == DIR_TYPE_PERSON) /* Person */
	{
		switch(coln)
		{
			case REMOTEDIRMODEL_COLUMN_NAME:
				return (RemoteMode)?(QString::fromUtf8(rsPeers->getPeerName(details.id).c_str())):tr("My files");
			case REMOTEDIRMODEL_COLUMN_FILENB: {
				SharedDirStats stats ;
				if(RemoteMode)
					rsFiles->getSharedDirStatistics(details.id,stats) ;
				else if(details.id == rsPeers->getOwnId())
					rsFiles->getSharedDirStatistics(rsPeers->getOwnId(),stats) ;
                else
                    return (qulonglong) m_folderFileTotals.value("!!RS_EXTRA_FILES_ROOT!!", 0);

				return (qulonglong) stats.total_number_of_files;
			}
			case REMOTEDIRMODEL_COLUMN_SIZE: {
				SharedDirStats stats ;
				if(RemoteMode)
					rsFiles->getSharedDirStatistics(details.id,stats) ;
				else if(details.id == rsPeers->getOwnId())
					rsFiles->getSharedDirStatistics(rsPeers->getOwnId(),stats) ;
                else
                    return (qulonglong) m_folderSizeTotals.value("!!RS_EXTRA_FILES_ROOT!!", 0);

				return (qulonglong) stats.total_shared_size;
			}
			case REMOTEDIRMODEL_COLUMN_AGE:
				return details.max_mtime;
			default:
				return QString();
		}
	}
	else if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE) /* File */
	{
		switch(coln)
		{
			case REMOTEDIRMODEL_COLUMN_NAME:
				return QString::fromUtf8(details.name.c_str());
			case REMOTEDIRMODEL_COLUMN_FILENB:
				return (qulonglong) 0;
			case REMOTEDIRMODEL_COLUMN_SIZE:
                return (qulonglong) details.size;
			case REMOTEDIRMODEL_COLUMN_AGE:
				return details.max_mtime;
			case REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS:
				return getFlagsString(details.flags);
			case REMOTEDIRMODEL_COLUMN_WN_VISU_DIR:
				{
					QString ind("");
					if (ageIndicator != IND_ALWAYS)
						ind = getAgeIndicatorString(details);
					return ind;
				}
			case REMOTEDIRMODEL_COLUMN_UPLOADED:
				return (qulonglong) rsFiles->getCumulativeUpload(details.hash);
			default:
				return tr("FILE");
		}
	}
	else if (details.type == DIR_TYPE_DIR) /* Dir */
	{
        QString path = QDir::cleanPath(QString::fromUtf8(details.path.c_str()));
		switch(coln)
		{
			case REMOTEDIRMODEL_COLUMN_NAME:
				return QString::fromUtf8(details.name.c_str());
			case REMOTEDIRMODEL_COLUMN_FILENB:
                return (qulonglong) m_folderFileTotals.value(path, 0);
			case REMOTEDIRMODEL_COLUMN_SIZE:
				return (qulonglong) m_folderSizeTotals.value(path, 0);
			case REMOTEDIRMODEL_COLUMN_AGE:
				return details.max_mtime;
			case REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS:
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

	if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE) /* File */
	{
		switch(coln)
		{
			case REMOTEDIRMODEL_COLUMN_NAME: return QString::fromUtf8(details.name.c_str());
			case REMOTEDIRMODEL_COLUMN_FILENB: return (qulonglong) 0;
            case REMOTEDIRMODEL_COLUMN_SIZE: return (qulonglong) details.size;
			case REMOTEDIRMODEL_COLUMN_AGE: return  details.max_mtime;
			case REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS: return QString::fromUtf8(rsPeers->getPeerName(details.id).c_str());
			case REMOTEDIRMODEL_COLUMN_WN_VISU_DIR: {
				RS_STACK_MUTEX(_ref_mutex) ;

				return computeDirectoryPath(details);
			}
			case REMOTEDIRMODEL_COLUMN_UPLOADED: return (qulonglong) rsFiles->getCumulativeUpload(details.hash);
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
		if((coln == REMOTEDIRMODEL_COLUMN_FILENB) || (coln == REMOTEDIRMODEL_COLUMN_SIZE))
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

	if (role == Qt::ForegroundRole)
	{
        // MODIFIED: Colorize Uploaded column (Directory=Blue, File=Green)
        if (coln == REMOTEDIRMODEL_COLUMN_UPLOADED)
        {
            if (details.type == DIR_TYPE_DIR)
                return QColor(Qt::blue);
            else if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE)
                return QColor(Qt::darkGreen);
        }

        if((details.type == DIR_TYPE_FILE  || details.type == DIR_TYPE_EXTRA_FILE) && details.hash.isNull())
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
        if(!isNewerThanEpoque(details.max_mtime) && details.type == DIR_TYPE_PERSON)
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

	if (role == FilterRole)
		return filterRole(details,coln) ;

	return QVariant();
}



/****
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
****/

QVariant TreeStyle_RDM::headerData(int section, Qt::Orientation orientation, int role) const
{
	/*if (role == Qt::SizeHintRole)
	{
		int defw = QFontMetricsF(QWidget().font()).width(headerData(section,Qt::Horizontal,Qt::DisplayRole).toString()) ;
		int defh = QFontMetricsF(QWidget().font()).height();

		if (section < REMOTEDIRMODEL_COLUMN_AGE)
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
			case REMOTEDIRMODEL_COLUMN_NAME:
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
			case REMOTEDIRMODEL_COLUMN_FILENB:
				return tr("# Files");
			case REMOTEDIRMODEL_COLUMN_SIZE:
				return tr("Size");
			case REMOTEDIRMODEL_COLUMN_AGE:
				return tr("Age");
			case REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS:
				if (RemoteMode)
					return tr("Friend");
				else
					return tr("Access");
			case REMOTEDIRMODEL_COLUMN_WN_VISU_DIR:
				if (RemoteMode)
					return tr("What's new");
				else
					return tr("Visibility");
			case REMOTEDIRMODEL_COLUMN_UPLOADED:
				return tr("Uploaded");
		}
		return tr("Column %1").arg(section);
	}
	else
		return tr("Row %1").arg(section);
}
QVariant FlatStyle_RDM::headerData(int section, Qt::Orientation orientation, int role) const
{
	/*if (role == Qt::SizeHintRole)
	{
		int defw = QFontMetricsF(QWidget().font()).width(headerData(section,Qt::Horizontal,Qt::DisplayRole).toString()) ;
		int defh = QFontMetricsF(QWidget().font()).height();

		if (section < REMOTEDIRMODEL_COLUMN_AGE)
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
			case REMOTEDIRMODEL_COLUMN_NAME:
				if (RemoteMode)
				{
					return tr("Friends Directories");
				}
				return tr("My Directories");
			case REMOTEDIRMODEL_COLUMN_FILENB:
				return tr("# Files");
			case REMOTEDIRMODEL_COLUMN_SIZE:
				return tr("Size");
			case REMOTEDIRMODEL_COLUMN_AGE:
				return tr("Age");
			case REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS:
				if(RemoteMode)
					return tr("Friend");
				else
					return tr("Share Flags");
			case REMOTEDIRMODEL_COLUMN_WN_VISU_DIR:
				return tr("Directory");
			case REMOTEDIRMODEL_COLUMN_UPLOADED:
				return tr("Uploaded");
		}
		return tr("Column %1").arg(section);
	}
	else
		return tr("Row %1").arg(section);
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

	//If on root and don't show empty child, get real parent row
	if ((!ref) && (!_showEmpty) && RemoteMode && ((size_t)row >= _parentRow.size()))
		return QModelIndex();

	int parentRow = ((!ref) && (!_showEmpty) && RemoteMode) ? _parentRow[row] : row ;

	void *result ;

	if(rsFiles->findChildPointer(ref, parentRow, result,  ((RemoteMode) ? RS_FILE_HINTS_REMOTE : RS_FILE_HINTS_LOCAL)))
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
	std::cerr << "success index(" << details.prow << ",0," << details.parent << ")";
	std::cerr << std::endl;

	std::cerr << "Creating index 3 row=" << details.prow << ", column=" << 0 << ", ref=" << (void*)details.parent << std::endl;
#endif

	return createIndex(details.prow, REMOTEDIRMODEL_COLUMN_NAME, details.parent);
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
		case DIR_TYPE_PERSON: return isNewerThanEpoque(details.max_mtime)? (Qt::ItemIsEnabled):(Qt::NoItemFlags) ;
		case DIR_TYPE_DIR:    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
		case DIR_TYPE_FILE:   return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
		case DIR_TYPE_EXTRA_FILE:  return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
	}

	return Qt::ItemIsSelectable;
}

// The other flags...
//Qt::ItemIsUserCheckable
//Qt::ItemIsEditable
//Qt::ItemIsDropEnabled
//Qt::ItemIsTristate



/* Callback from Core*/
void RetroshareDirModel::preMods()
{
    mUpdating = true ;

	beginResetModel();
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::preMods()" << std::endl;
#endif
}

/* Callback from Core*/
void RetroshareDirModel::postMods()
{
    mUpdating = false ;
#ifdef RDM_DEBUG
	std::cerr << "RetroshareDirModel::postMods()" << std::endl;
#endif

	endResetModel();

    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(rowCount()-1,REMOTEDIRMODEL_COLUMN_COUNT-1,(void*)NULL));
}

void FlatStyle_RDM::postMods()
{
    time_t now = time(NULL);

    // MODIFICATION: Removed throttling because it caused stuck model state (beginReset without endReset)
    // if(_last_update + FLAT_VIEW_MIN_DELAY_BETWEEN_UPDATES > now)
    //    return ;

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

void RetroshareDirModel::downloadSelected(const QModelIndexList &list,bool interactive)
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
   FileSearchFlags f = RemoteMode?RS_FILE_HINTS_REMOTE:RS_FILE_HINTS_LOCAL ;

   if(interactive)
       RsCollectionDialog::downloadFiles(RsCollection(dirVec,f)) ;
   else /* Fire off requests */
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
               rsFiles -> FileRequest(details.name, details.hash, details.size, "", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds);
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
        rsFiles->FileRequest(dirDetails.name, dirDetails.hash, dirDetails.size, cleanPath.toUtf8().constData(), RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds);
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
		if(it->column()==REMOTEDIRMODEL_COLUMN_FILENB)
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
		if(it->column()==REMOTEDIRMODEL_COLUMN_NAME)
		{
			void *ref = it -> internalPointer();

            DirDetails details;

            if (!requestDirDetails(ref, RemoteMode,details))
				continue;

            if(details.type == DIR_TYPE_PERSON)
				continue ;

#ifdef RDM_DEBUG
			std::cerr << "::::::::::::FileRecommend:::: " << std::endl;
			std::cerr << "Name: " << details.name << std::endl;
			std::cerr << "Hash: " << details.hash << std::endl;
            std::cerr << "Size: " << details.size << std::endl;
			std::cerr << "Path: " << details.path << std::endl;
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

void RetroshareDirModel::openSelected(const QModelIndexList &qmil, bool openDir)
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

	std::list<DirDetails> files_info;
	std::list<DirDetails>::iterator it;
	getFileInfoFromIndexList(qmil, files_info);

	for (it = files_info.begin(); it != files_info.end(); ++it)
	{
		if ((*it).type & DIR_TYPE_PERSON) continue;

		QDir dir(QString::fromUtf8((*it).path.c_str()));
		QString dest;
		if ((*it).type & DIR_TYPE_FILE || (!openDir && (*it).type & DIR_TYPE_EXTRA_FILE)) {
			dest = dir.absoluteFilePath(QString::fromUtf8(it->name.c_str()));
		} else if ((*it).type & DIR_TYPE_DIR) {
			dest = dir.absolutePath();
		} else if (openDir) // extra
		{
			QDir d = QFileInfo(it->name.c_str()).absoluteDir();
			dest = d.absolutePath();
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
    void *ref = index.sibling(index.row(),REMOTEDIRMODEL_COLUMN_FILENB).internalPointer();

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

void RetroshareDirModel::filterItems(const std::list<std::string>& keywords, uint32_t& found)
{
    FileSearchFlags flags = RemoteMode ? RS_FILE_HINTS_REMOTE : RS_FILE_HINTS_LOCAL;

    std::list<DirDetails> result_list ;
    found = 0 ;

    if(keywords.empty())
    {
        mFilteredPointers.clear();
        // MODIFICATION: Call update to refresh the view when the filter is cleared
        update();
        return ;
    }
    else if(keywords.size() > 1)
    {
        RsRegularExpression::NameExpression exp(RsRegularExpression::ContainsAllStrings, keywords, true);
        rsFiles->SearchBoolExp(&exp, result_list, flags) ;
    }
    else
        rsFiles->SearchKeywords(keywords, result_list, flags) ;

    if(result_list.empty())
    {
        mFilteredPointers.clear();
        // MODIFICATION: Insert NULL to signal that we have an active filter which returned no results.
        // updateRefs will see mFilteredPointers is not empty, enter the filtered path, encounter NULL, skip it,
        // and resulting view will be empty (correct behavior) instead of showing all files.
        mFilteredPointers.insert(NULL);
        update();
        return ;
    }

    mFilteredPointers.clear();

    // Mark matching pointers and their hierarchy as visible
    for(auto it(result_list.begin()); it != result_list.end(); ++it)
    {
        DirDetails& det(*it) ;
        void *p = det.ref ;
        mFilteredPointers.insert(p) ;
        ++found ;

        // MODIFICATION: Included DIR_TYPE_ROOT in the climbing logic.
        // In Tree View, every parent must be in mFilteredPointers to show the child.
        // Shared folder roots are often DIR_TYPE_ROOT.
        while(det.type == DIR_TYPE_FILE || det.type == DIR_TYPE_EXTRA_FILE || 
              det.type == DIR_TYPE_DIR || det.type == DIR_TYPE_PERSON || det.type == DIR_TYPE_ROOT)
        {
            p = det.parent;
            if (p == NULL) break; 

            if (!rsFiles->RequestDirDetails(p, det, flags))
                break;

            mFilteredPointers.insert(p); // Mark parent node as visible

            // If we reach the Person node (root of files), stop climbing
            if (det.type == DIR_TYPE_PERSON) break;
        }
    }
    
    // MODIFICATION: Restore the update call. 
    // This triggers beginResetModel/endResetModel which notifies the view 
    // that row counts and visibility have changed.
    update(); 
}

QVariant TreeStyle_RDM::displayRole(const DirDetails& details, int coln) const
{
    if (details.type == DIR_TYPE_PERSON) /* Person */
    {
        switch(coln)
        {
            case REMOTEDIRMODEL_COLUMN_NAME:
            {
                QString res ;
                if(RemoteMode)
                    res = QString::fromUtf8(rsPeers->getPeerName(details.id).c_str());
                else if(details.id == rsPeers->getOwnId())
                    res = tr("My files");
                else
                    res = tr("Temporary shared files");
                return res ;
            }
            break;

	    case REMOTEDIRMODEL_COLUMN_FILENB:
            {
                SharedDirStats stats;
                if(RemoteMode)
                    rsFiles->getSharedDirStatistics(details.id, stats) ;
                else if(details.id == rsPeers->getOwnId())
                    rsFiles->getSharedDirStatistics(rsPeers->getOwnId(), stats) ;
                else {
                    // Specific handling for "Temporary shared files" node (Extra files root)
                    uint32_t nb = m_folderFileTotals.value("!!RS_EXTRA_FILES_ROOT!!", 0);
                    if(nb > 1) return QString::number(nb) + " " + tr("Files");
                    if(nb == 1) return QString::number(nb) + " " + tr("File");
                    return tr("Empty");
                }
                
                if(stats.total_number_of_files > 0)
                    return QString::number(stats.total_number_of_files) + " " + (stats.total_number_of_files > 1 ? tr("Files") : tr("File"));
                return tr("Empty");
            }
            break;

	    case REMOTEDIRMODEL_COLUMN_SIZE:
            {
                SharedDirStats stats ;
                if(RemoteMode)
                    rsFiles->getSharedDirStatistics(details.id,stats) ;
                else if(details.id == rsPeers->getOwnId())
                    rsFiles->getSharedDirStatistics(rsPeers->getOwnId(),stats) ;
                else
                    return misc::friendlyUnit(m_folderSizeTotals.value("!!RS_EXTRA_FILES_ROOT!!", 0));

                if(stats.total_shared_size > 0)
                    return misc::friendlyUnit(stats.total_shared_size) ;
                return QString();
            }
            break;

            case REMOTEDIRMODEL_COLUMN_AGE:
            {
                if(!isNewerThanEpoque(details.max_mtime))
                    return QString();
                else if(details.id != rsPeers->getOwnId())
                    return QString();
                else
                    return misc::timeRelativeToNow(details.max_mtime);
            }
            break;

            case REMOTEDIRMODEL_COLUMN_UPLOADED:
            {
                if(!RemoteMode && details.id == rsPeers->getOwnId())
                {
                    uint64_t n = rsFiles->getCumulativeUploadNum();
                    if(n)
                        return QString(misc::friendlyUnit(rsFiles->getCumulativeUploadAll()) + QString(" - %1 files").arg(n));
                    else
                        return QString("-");
                }
                return QString();
            }
            break;

            default: return QString();
        }
    }
    else if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE) /* File */
    {
        switch(coln)
        {
            case REMOTEDIRMODEL_COLUMN_NAME: return QString::fromUtf8(details.name.c_str());
            case REMOTEDIRMODEL_COLUMN_SIZE: return misc::friendlyUnit(details.size);
            case REMOTEDIRMODEL_COLUMN_AGE:
            {
                if(details.type == DIR_TYPE_FILE)
                    return misc::timeRelativeToNow(details.max_mtime);
                else if(details.type == DIR_TYPE_EXTRA_FILE)
                {
                    FileInfo fi;
                    if (rsFiles->FileDetails(details.hash, RS_FILE_HINTS_EXTRA , fi))
                        return misc::timeRelativeToNow((rstime_t)fi.age-(30 * 3600 * 24));
                    return QString();
                }
                return QString();
            }
            case REMOTEDIRMODEL_COLUMN_UPLOADED:
            {
                uint64_t x = rsFiles->getCumulativeUpload(details.hash);
                return x ? misc::friendlyUnit(x) : QString();
            }
            break;
            default: return QVariant();
        }
    }
    else if (details.type == DIR_TYPE_DIR) /* Directory */
    {
        QString path = QDir::cleanPath(QString::fromUtf8(details.path.c_str()));
        switch(coln)
        {
            case REMOTEDIRMODEL_COLUMN_NAME: return QString::fromUtf8(details.name.c_str());
            case REMOTEDIRMODEL_COLUMN_FILENB:
            {
                uint32_t totalFiles = m_folderFileTotals.value(path, 0);
                return QString::number(totalFiles) + " " + (totalFiles > 1 ? tr("Files") : tr("File"));
            }
            case REMOTEDIRMODEL_COLUMN_SIZE: 
                return misc::friendlyUnit(m_folderSizeTotals.value(path, 0));
            case REMOTEDIRMODEL_COLUMN_AGE: return misc::timeRelativeToNow(details.max_mtime);
            case REMOTEDIRMODEL_COLUMN_FRIEND_ACCESS: return getFlagsString(details.flags);
            case REMOTEDIRMODEL_COLUMN_WN_VISU_DIR: return getGroupsString(details.flags,details.parent_groups) ;
            case REMOTEDIRMODEL_COLUMN_UPLOADED:
            {
                uint64_t totalUpload = m_folderUploadTotals.value(path, 0);
                return totalUpload > 0 ? misc::friendlyUnit(totalUpload) : "";
            }
            break;
            default: return QVariant();
        }
    }
    return QVariant();
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
		std::cerr << "Name: " << details.name << std::endl;
		std::cerr << "Hash: " << details.hash << std::endl;
        std::cerr << "Size: " << details.size << std::endl;
		std::cerr << "Path: " << details.path << std::endl;
#endif

        if (details.type != DIR_TYPE_FILE && details.type != DIR_TYPE_EXTRA_FILE)
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

        drags[details.hash] = details.children.size();
        QString line = QString("%1/%2/%3/").arg(QString::fromUtf8(details.name.c_str()), QString::fromStdString(details.hash.toStdString()), QString::number(details.size));

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

TreeStyle_RDM::~TreeStyle_RDM()
{
}
FlatStyle_RDM::~FlatStyle_RDM()
{
}

void FlatStyle_RDM::updateRefs()
{
	if(RsAutoUpdatePage::eventsLocked())
	{
		QTimer::singleShot(5000,this,SLOT(updateRefs())) ;
		return ;
	}
 
     // MODIFICATION: Avoid double-reset. update() already called preMods().
     if (!mUpdating)
        RetroshareDirModel::preMods() ;


	uint32_t nb_treated_refs = 0 ;


    {
        RS_STACK_MUTEX(_ref_mutex) ;

        // MODIFICATION: instant search result display
        // If we have a filter active, use the filtered set directly instead of crawling.
        if (!mFilteredPointers.empty()) 
        {
            _ref_stack.clear(); // Stop any pending crawl
            _ref_entries.clear();

            for(auto it = mFilteredPointers.begin(); it != mFilteredPointers.end(); ++it)
            {
                void* ref = *it;
                DirDetails details;
                // Only add files to the flat list
                if(requestDirDetails(ref, RemoteMode, details))
                {
                    if(details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE)
                    {
                        _ref_entries.push_back(ref);
                        
                        CachedFileDetails cfd;
                        cfd.name = QString::fromUtf8(details.name.c_str());
                        cfd.sizeStr = misc::friendlyUnit(details.size);
                        
                        if(details.type == DIR_TYPE_FILE)
                            cfd.ageStr = misc::timeRelativeToNow(details.max_mtime);
                        else {
                            FileInfo fi;
                            if (rsFiles->FileDetails(details.hash, RS_FILE_HINTS_EXTRA , fi))
                                cfd.ageStr = misc::timeRelativeToNow((rstime_t)fi.age-(30 * 3600 * 24));
                        }

                        uint64_t x = rsFiles->getCumulativeUpload(details.hash);
                        cfd.uploadStr = x ? misc::friendlyUnit(x) : QString();

                        m_cache[ref] = cfd;
                    }
                }
            }

             _needs_update = false; // We are done
        }
        else 
        {
            // Standard crawling behavior (when no filter or cleared)
            while( !_ref_stack.empty() && (_ref_entries.size() <= FLAT_VIEW_MAX_REFS_TABLE_SIZE) )
            {
                void *ref = _ref_stack.back() ;
#ifdef RDM_DEBUG
            std::cerr << "FlatStyle_RDM::postMods(): poped ref " << ref << std::endl;
#endif
            _ref_stack.pop_back() ;

            DirDetails details ;

            if (requestDirDetails(ref, RemoteMode,details))
            {
                if(details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE)		// only push files, not directories nor persons.
                {
                    _ref_entries.push_back(ref) ;
                    
                    CachedFileDetails cfd;
                    cfd.name = QString::fromUtf8(details.name.c_str());
                    cfd.sizeStr = misc::friendlyUnit(details.size);
                    
                    if(details.type == DIR_TYPE_FILE)
                        cfd.ageStr = misc::timeRelativeToNow(details.max_mtime);
                    else {
                        FileInfo fi;
                        if (rsFiles->FileDetails(details.hash, RS_FILE_HINTS_EXTRA , fi))
                            cfd.ageStr = misc::timeRelativeToNow((rstime_t)fi.age-(30 * 3600 * 24));
                    }

                    uint64_t x = rsFiles->getCumulativeUpload(details.hash);
                    cfd.uploadStr = x ? misc::friendlyUnit(x) : QString();

                    m_cache[ref] = cfd;
                }
#ifdef RDM_DEBUG
                std::cerr << "FlatStyle_RDM::postMods(): adding ref " << ref << std::endl;
#endif
                for(uint32_t i=0;i<details.children.size();++i)
                    _ref_stack.push_back(details.children[i].ref) ;
            }

            // Limit the size of the table to display, otherwise it becomes impossible to Qt.

            if(_ref_entries.size() > FLAT_VIEW_MAX_REFS_TABLE_SIZE)
                continue;

            if(++nb_treated_refs > FLAT_VIEW_MAX_REFS_PER_SECOND) 	// we've done enough, let's give back hand to
            {															// the user and setup a timer to finish the job later.

                if(visible())
                    QTimer::singleShot(10,this,SLOT(updateRefs())) ; // Reduced from 2000ms to 10ms for faster loading
                else
                    std::cerr << "Not visible: suspending update"<< std::endl;
                break ;
            }
        } // End of standard crawling loop
        } // End of else block
        std::cerr << "reference tab contains " << std::dec << _ref_entries.size() << " files" << std::endl;
    }

	// The loop is finished.
	if(_ref_stack.empty()) {

		_needs_update = false ;
    }


    RetroshareDirModel::postMods() ;
}

void TreeStyle_RDM::showEmpty(const bool value)
{
	_showEmpty = value;
	update();
}

// hasUploads for FlatStyle - Defensive Check
bool FlatStyle_RDM::hasUploads(void *ref) const
{
    // CRITICAL FIX: Safety check for NULL pointers
    if (ref == NULL) {
        return false;
    }

    DirDetails details;
    if (!requestDirDetails(ref, RemoteMode, details)) return false;

    if (details.type == DIR_TYPE_FILE || details.type == DIR_TYPE_EXTRA_FILE) {
        return rsFiles->getCumulativeUpload(details.hash) > 0;
    }

    return false;
}

