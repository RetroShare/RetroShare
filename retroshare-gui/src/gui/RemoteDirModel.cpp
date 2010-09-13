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
#include <QDesktopServices>
#include <QUrl>
#include <QMimeData>

#include "RemoteDirModel.h"
#include <retroshare/rsfiles.h>
#include <retroshare/rstypes.h>
#include "util/misc.h"

#include <set>
#include <sstream>
#include <algorithm>

/*****
 * #define RDM_DEBUG
 ****/

RemoteDirModel::RemoteDirModel(bool mode, QObject *parent)
        : QAbstractItemModel(parent),
         ageIndicator(IND_ALWAYS),
         RemoteMode(mode), nIndex(1), indexSet(1) /* ass zero index cant be used */
{
	setSupportedDragActions(Qt::CopyAction);
	treeStyle();
}

void RemoteDirModel::treeStyle()
{
	categoryIcon.addPixmap(QPixmap(":/images/folder16.png"),
	                     QIcon::Normal, QIcon::Off);
	categoryIcon.addPixmap(QPixmap(":/images/folder_video.png"),
	                     QIcon::Normal, QIcon::On);
	peerIcon = QIcon(":/images/user/identity16.png");
}

 bool RemoteDirModel::hasChildren(const QModelIndex &parent) const
 {

#ifdef RDM_DEBUG
     std::cerr << "RemoteDirModel::hasChildren() :" << parent.internalPointer();
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

     DirDetails details;
     uint32_t flags = DIR_FLAGS_CHILDREN;
     if (RemoteMode)
     	flags |= DIR_FLAGS_REMOTE;
     else
     	flags |= DIR_FLAGS_LOCAL;

     if (!rsFiles->RequestDirDetails(ref, details, flags))
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
     std::cerr << "lookup PER/DIR #" << details.count;
     std::cerr << std::endl;
#endif
     return (details.count > 0); /* do we have children? */
 }


 int RemoteDirModel::rowCount(const QModelIndex &parent) const
 {
#ifdef RDM_DEBUG
     std::cerr << "RemoteDirModel::rowCount(): " << parent.internalPointer();
     std::cerr << ": ";
#endif

     void *ref = (parent.isValid())? parent.internalPointer() : NULL ;

     DirDetails details;
     uint32_t flags = DIR_FLAGS_CHILDREN;
     if (RemoteMode)
     	flags |= DIR_FLAGS_REMOTE;
     else
     	flags |= DIR_FLAGS_LOCAL;

     if (!rsFiles->RequestDirDetails(ref, details, flags))
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
     std::cerr << "lookup PER/DIR #" << details.count;
     std::cerr << std::endl;
#endif
     return details.count;
 }

 int RemoteDirModel::columnCount(const QModelIndex &parent) const
 {
	return 5;
 }
QString RemoteDirModel::getFlagsString(uint32_t flags)
{
	switch(flags & (DIR_FLAGS_NETWORK_WIDE|DIR_FLAGS_BROWSABLE))
	{
		case DIR_FLAGS_NETWORK_WIDE: return tr("Anonymous") ;
		case DIR_FLAGS_NETWORK_WIDE | DIR_FLAGS_BROWSABLE: return tr("Anonymous and browsable by friends") ;
		case DIR_FLAGS_BROWSABLE: return tr("Only browsable by friends") ;
		default:
										  return QString() ;
	}
}

QString RemoteDirModel::getAgeIndicatorString(const DirDetails &details) const
{
	QString ret("");
	QString nind("NEW");
	QString oind("OLD");
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

 QVariant RemoteDirModel::data(const QModelIndex &index, int role) const
 {
#ifdef RDM_DEBUG
     std::cerr << "RemoteDirModel::data(): " << index.internalPointer();
     std::cerr << ": ";
     std::cerr << std::endl;
#endif

     if (!index.isValid())
         return QVariant();

     /* get the data from the index */
     void *ref = index.internalPointer();
     int coln = index.column();

     DirDetails details;
     uint32_t flags = DIR_FLAGS_DETAILS;
     if (RemoteMode)
     	flags |= DIR_FLAGS_REMOTE;
     else
     	flags |= DIR_FLAGS_LOCAL;


     if (!rsFiles->RequestDirDetails(ref, details, flags))
     {
     	return QVariant();
     }

    if (role == RemoteDirModel::FileNameRole)
    {
        FileInfo finfo;
        rsFiles->FileDetails(details.hash, 0, finfo);

        return QString::fromStdString(finfo.path) ;
    } /* end of FileNameRole */

    if (role == Qt::TextColorRole)
    {
        FileInfo finfo;
        rsFiles->FileDetails(details.hash, 0, finfo);

        if(details.min_age > ageIndicator)
            return Qt::gray ;
        else
            return Qt::black ;
    } /* end of TextColorRole */


    if (role == Qt::DecorationRole)
    {
        FileInfo finfo;
        rsFiles->FileDetails(details.hash, 0, finfo);
	 
        if (details.type == DIR_TYPE_PERSON)
        {
            switch(coln)
            {
                case 0:
                if(details.min_age > ageIndicator)
                {
                    return QIcon(":/images/folder_grey.png");
                }
                else if (ageIndicator == IND_LAST_DAY )
                {
                    return QIcon(":/images/folder_green.png");
                }
                else if (ageIndicator == IND_LAST_WEEK )
                {
                    return QIcon(":/images/folder_yellow.png");
                }
                else if (ageIndicator == IND_LAST_MONTH )
                {
                    return QIcon(":/images/folder_red.png");
                }
                else
                {
                    return (QIcon(peerIcon));
                }
                break;
			 }
		 }
		 else if (details.type == DIR_TYPE_DIR)
		 {
            switch(coln)
            {
                case 0:
                if(details.min_age > ageIndicator)
                {
                    return QIcon(":/images/folder_grey.png");
                }
                else if (ageIndicator == IND_LAST_DAY )
                {
                    return QIcon(":/images/folder_green.png");
                }
                else if (ageIndicator == IND_LAST_WEEK )
                {
                    return QIcon(":/images/folder_yellow.png");
                }
                else if (ageIndicator == IND_LAST_MONTH )
                {
                    return QIcon(":/images/folder_red.png");
                }
                else
                {
                    return(QIcon(categoryIcon));
                }
                break;
			 }
		 }
		 else if (details.type == DIR_TYPE_FILE) /* File */
		 {
			 // extensions predefined
			 switch(coln)
			 {
				 case 0:
					 QString ext = QFileInfo(QString::fromStdString(details.name)).suffix();
					 if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif"
							 || ext == "bmp" || ext == "ico" || ext == "svg")
					 {
						 QIcon icon(":/images/FileTypePicture.png");
						 return icon;
					 }
					 else if (ext == "avi" || ext == "AVI" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "ogm"
							 || ext == "mkv" || ext == "mp4" || ext == "flv" || ext == "mov"
							 || ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp")
					 {
						 QIcon icon(":/images/FileTypeVideo.png");
						 return icon;
					 }
					 else if (ext == "ogg" || ext == "mp3" || ext == "wav" || ext == "wma" || ext == "xpm")
					 {
						 QIcon icon(":/images/FileTypeAudio.png");
						 return icon;
					 }
					 else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "gz" || ext == "7z"
							 || ext == "rar" || ext == "rpm" || ext == "deb")
					 {
						 QIcon icon(":/images/FileTypeArchive.png");
						 return icon;
					 }
					 else if (ext == "app" || ext == "bat" || ext == "cgi" || ext == "com"
							 || ext == "bin" || ext == "exe" || ext == "js" || ext == "pif"
							 || ext == "py" || ext == "pl" || ext == "sh" || ext == "vb" || ext == "ws")
					 {
						 return(QIcon(":/images/FileTypeProgram.png"));
					 }
					 else if (ext == "iso" || ext == "nrg" || ext == "mdf" )
					 {
						 QIcon icon(":/images/FileTypeCDImage.png");
						 return icon;
					 }
					 else if (ext == "txt" || ext == "cpp" || ext == "c" || ext == "h")
					 {
						 QIcon icon(":/images/FileTypeDocument.png");
						 return icon;
					 }
					 else if (ext == "doc" || ext == "rtf" || ext == "sxw" || ext == "xls"
							 || ext == "sxc" || ext == "odt" || ext == "ods")
					 {
						 QIcon icon(":/images/FileTypeDocument.png");
						 return icon;
					 }
					 else if (ext == "html" || ext == "htm" || ext == "php")
					 {
						 QIcon icon(":/images/FileTypeDocument.png");
						 return icon;
					 }
					 else
					 {
						 QIcon icon(":/images/FileTypeAny.png");
						 return icon;
					 }
					 break;
			 }
		 }
		 else
		 {
			 return QVariant();
		 }
	 } /* end of DecorationRole */

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
    } /* end of TextAlignmentRole */     

    if (role == Qt::DisplayRole)
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
				return QString::fromStdString(details.name);
				break;
			case 1:
				return QString() ;
				break;
			default:
				return QString() ;
				break;
		}
	}
	else if (details.type == DIR_TYPE_FILE) /* File */
	{
		switch(coln)
		{
			case 0:
				return QString::fromUtf8(details.name.c_str());
				break;
			case 1:
				{
                std::ostringstream out;
                return  misc::friendlyUnit(details.count);
                }
			break;
			case 2:
                {
                std::ostringstream out;
                return  misc::userFriendlyDuration(details.age);
                }
			break;
			case 3:
                {
                return getFlagsString(details.flags);
                }
			break;
			case 4:
                {
                QString ind("");
                if (ageIndicator != IND_ALWAYS)
                ind = getAgeIndicatorString(details);
                return ind;
                }
            break;
			default:
                return QString(tr("FILE"));
			break;
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
				{
					std::ostringstream out;
					out << details.count;
					if (details.count > 1)
					{
                    return QString::fromStdString(out.str()) + " " + tr("Files");
					}
					else
                    return QString::fromStdString(out.str()) + " " + tr("File");
				}
				break;
			case 2:
					return misc::userFriendlyDuration(details.min_age);
				break;
			case 3:
					return getFlagsString(details.flags);
				break;
			default:
				return QString(tr("DIR"));
				break;
		}
	}
   } /* end of DisplayRole */
   return QVariant();


}

void RemoteDirModel::getAgeIndicatorRec(DirDetails &details, QString &ret) const {
	if (details.type == DIR_TYPE_FILE) {
		ret = getAgeIndicatorString(details);
		return;
	} else if (details.type == DIR_TYPE_DIR && ret == tr("")) {
		std::list<DirStub>::iterator it;
		for (it = details.children.begin(); it != details.children.end(); it++) {
			void *ref = it->ref;
			DirDetails childDetails;
			uint32_t flags;

			if (RemoteMode)
				flags |= DIR_FLAGS_REMOTE;
			else
				flags |= DIR_FLAGS_LOCAL;

			if (rsFiles->RequestDirDetails(ref, childDetails, flags) && ret == tr(""))
				getAgeIndicatorRec(childDetails, ret);
		}
	}
}

 QVariant RemoteDirModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const
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
					return QString(tr("Friends Directories"));
				}
				else
				{
					return QString(tr("My Directories"));
				}
				break;
			case 1:
				return QString(tr("Size"));
				break;
			case 2:
				return QString(tr("Age"));
				break;
			case 3:
				return QString(tr("Share Type"));
				break;
			case 4:
				return QString(tr("What's new"));
				break;
		}
		return QString("Column %1").arg(section);
	}
	else
		return QString("Row %1").arg(section);
}

QModelIndex RemoteDirModel::index(int row, int column, const QModelIndex & parent) const
{
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::index(): " << parent.internalPointer();
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

	DirDetails details;
	uint32_t flags = DIR_FLAGS_CHILDREN;
	if (RemoteMode)
		flags |= DIR_FLAGS_REMOTE;
	else
		flags |= DIR_FLAGS_LOCAL;

	if (!rsFiles->RequestDirDetails(ref, details, flags))
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

	std::list<DirStub>::iterator it;
	int i = 0;
	for(it = details.children.begin(); ((i < row) && (it != details.children.end())); ++it,++i) ;
	
	if (it == details.children.end())
	{
#ifdef RDM_DEBUG
		std::cerr << "wrong number of children -> invalid";
		std::cerr << std::endl;
#endif
		return QModelIndex();
	}

#ifdef RDM_DEBUG
	std::cerr << "success index(" << row << "," << column << "," << it->ref << ")";
	std::cerr << std::endl;
#endif

	/* we can just grab the reference now */

	return createIndex(row, column, it->ref);
}


QModelIndex RemoteDirModel::parent( const QModelIndex & index ) const
{
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::parent(): " << index.internalPointer();
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

	DirDetails details;
	uint32_t flags = (RemoteMode)?DIR_FLAGS_REMOTE:DIR_FLAGS_LOCAL;

	if (!rsFiles->RequestDirDetails(ref, details, flags))
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

#endif
	return createIndex(details.prow, 0, details.parent);
}

Qt::ItemFlags RemoteDirModel::flags( const QModelIndex & index ) const
{
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::flags()";
	std::cerr << std::endl;
#endif

	if (!index.isValid())
		return (Qt::ItemIsSelectable); // Error.

	void *ref = index.internalPointer();

	DirDetails details;
	uint32_t flags = DIR_FLAGS_DETAILS;
	if (RemoteMode)
		flags |= DIR_FLAGS_REMOTE;
	else
		flags |= DIR_FLAGS_LOCAL;

	if (!rsFiles->RequestDirDetails(ref, details, flags))
		return Qt::ItemIsSelectable; // Error.

	switch(details.type)
	{
		case DIR_TYPE_PERSON: return Qt::ItemIsEnabled;
		case DIR_TYPE_DIR:	 return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
		default: ;
		case DIR_TYPE_FILE:	 return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
	}
}

// The other flags...
//Qt::ItemIsUserCheckable
//Qt::ItemIsEditable
//Qt::ItemIsDropEnabled
//Qt::ItemIsTristate



/* Callback from */
 void RemoteDirModel::preMods()
 {
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::preMods()" << std::endl;
#endif
	//modelAboutToBeReset();
//	reset();
	layoutAboutToBeChanged();
 }

/* Callback from */
 void RemoteDirModel::postMods()
 {
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::postMods()" << std::endl;
#endif
	//modelReset();
	layoutChanged();
	//reset();
 }


//void RemoteDirModel::update (const QModelIndex &index )
//{
//#ifdef RDM_DEBUG
//	//std::cerr << "Directory Request(" << id << ") : ";
//	//std::cerr << path << std::endl;
//#endif
//	//rsFiles -> RequestDirectories(id, path, 1);
//}

void RemoteDirModel::downloadSelected(QModelIndexList list)
{
	if (!RemoteMode)
	{
#ifdef RDM_DEBUG
		std::cerr << "Cannot download from local" << std::endl;
#endif
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
            std::cerr << "RemoteDirModel::downloadSelected() Calling File Request";
            std::cerr << std::endl;
            std::list<std::string> srcIds;
            srcIds.push_back(details.id);
            rsFiles -> FileRequest(details.name, details.hash,
                    details.count, "", RS_FILE_HINTS_NETWORK_WIDE, srcIds);
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
void RemoteDirModel::downloadDirectory(const DirDetails & dirDetails, int prefixLen)
{
	if (dirDetails.type & DIR_TYPE_FILE)
	{
		std::list<std::string> srcIds;
		QString cleanPath = QDir::cleanPath((rsFiles->getDownloadDirectory() + "/" + dirDetails.path.substr(prefixLen)).c_str());

		srcIds.push_back(dirDetails.id);
		rsFiles->FileRequest(dirDetails.name, dirDetails.hash, dirDetails.count, cleanPath.toStdString(), RS_FILE_HINTS_NETWORK_WIDE, srcIds);
	}
	else if (dirDetails.type & DIR_TYPE_DIR)
	{
		std::list<DirStub>::const_iterator it;
		QDir dwlDir(rsFiles->getDownloadDirectory().c_str());
		QString cleanPath = QDir::cleanPath(QString(dirDetails.path.c_str()).right(dirDetails.path.length() - prefixLen));

		if (!dwlDir.mkpath(cleanPath)) return;

		for (it = dirDetails.children.begin(); it != dirDetails.children.end(); it++)
		{
			if (!it->ref) continue;

			DirDetails subDirDetails;
			uint32_t flags = DIR_FLAGS_CHILDREN | DIR_FLAGS_REMOTE;

			if (!rsFiles->RequestDirDetails(it->ref, subDirDetails, flags)) continue;

			downloadDirectory(subDirDetails, prefixLen);
		}
	}
}

void RemoteDirModel::getDirDetailsFromSelect (QModelIndexList list, std::vector <DirDetails>& dirVec)
{
    dirVec.clear();

    /* Fire off requests */
    QModelIndexList::iterator it;
    for(it = list.begin(); it != list.end(); it++)
    {
        if(it->column()==1)
        {
            void *ref = it -> internalPointer();

            DirDetails details;
            uint32_t flags = DIR_FLAGS_DETAILS;
            if (RemoteMode)
            {
                flags |= DIR_FLAGS_REMOTE;
            }
            else
            {
                flags |= DIR_FLAGS_LOCAL;
            }

            if (!rsFiles->RequestDirDetails(ref, details, flags))
            {
                continue;
            }

            dirVec.push_back(details);
        }
    }
}

/****************************************************************************
 * OLD RECOMMEND SYSTEM - DISABLED
 *
 */

void RemoteDirModel::getFileInfoFromIndexList(const QModelIndexList& list, std::list<DirDetails>& file_details)
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
			uint32_t flags = DIR_FLAGS_DETAILS;

			if (RemoteMode)
				flags |= DIR_FLAGS_REMOTE;
			else
				flags |= DIR_FLAGS_LOCAL;

			if (!rsFiles->RequestDirDetails(ref, details, flags))
				continue;

			if(details.type == DIR_TYPE_PERSON)
				continue ;

#ifdef RDM_DEBUG
			std::cerr << "::::::::::::FileRecommend:::: " << std::endl;
			std::cerr << "Name: " << details.name << std::endl;
			std::cerr << "Hash: " << details.hash << std::endl;
			std::cerr << "Size: " << details.count << std::endl;
			std::cerr << "Path: " << details.path << std::endl;
#endif
			// Note: for directories, the returned hash, is the peer id, so if we collect
			// dirs, we need to be a bit more conservative for the 

			if(already_in.find(details.hash+details.name) == already_in.end())
			{
				file_details.push_back(details) ;
				already_in.insert(details.hash+details.name) ;
			}
		}
#ifdef RDM_DEBUG
	std::cerr << "::::::::::::Done FileRecommend" << std::endl;
#endif
}

#if 0
void RemoteDirModel::recommendSelectedOnly(QModelIndexList list)
{
#ifdef RDM_DEBUG
	std::cerr << "recommendSelectedOnly()" << std::endl;
#endif
	if (RemoteMode)
	{
#ifdef RDM_DEBUG
		std::cerr << "Cannot recommend remote! (should download)" << std::endl;
#endif
	}
     	rsFiles->ClearInRecommend();

	/* Fire off requests */
	QModelIndexList::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		void *ref = it -> internalPointer();

     		DirDetails details;
     		uint32_t flags = DIR_FLAGS_DETAILS;
     		if (RemoteMode)
		{
     			flags |= DIR_FLAGS_REMOTE;
			continue; /* don't recommend remote stuff */
		}
     		else
		{
     			flags |= DIR_FLAGS_LOCAL;
		}

     		if (!rsFiles->RequestDirDetails(ref, details, flags))
     		{
			continue;
     		}

#ifdef RDM_DEBUG
		std::cerr << "::::::::::::FileRecommend:::: " << std::endl;
		std::cerr << "Name: " << details.name << std::endl;
		std::cerr << "Hash: " << details.hash << std::endl;
		std::cerr << "Size: " << details.count << std::endl;
		std::cerr << "Path: " << details.path << std::endl;
#endif

		rsFiles -> FileRecommend(details.name, details.hash, details.count);
     		rsFiles -> SetInRecommend(details.name, true);
	}
#ifdef RDM_DEBUG
	std::cerr << "::::::::::::Done FileRecommend" << std::endl;
#endif
}
#endif

/****************************************************************************
 * OLD RECOMMEND SYSTEM - DISABLED
 ******/

void RemoteDirModel::openSelected(QModelIndexList qmil, bool openFolder)
{
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::openSelected()" << std::endl;
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

		std::string fullpath, name;
		rsFiles->ConvertSharedFilePath((*it).path, fullpath);
		int len = fullpath.length();
		if (len && (fullpath[len - 1] != '/')) fullpath += '/';

		if ((*it).type & DIR_TYPE_FILE) {
			name = fullpath + (*it).name;
		} else if ((*it).type & DIR_TYPE_DIR) {
			name = fullpath;
		}

		std::cerr << "Opennign this file: " << name << std::endl ;

		QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromUtf8(name.c_str(),name.length())));
	}

//	if (openFolder) {
//		std::list<std::string>::iterator dit;
//		for (dit = dirs_to_open.begin(); dit != dirs_to_open.end(); dit++) 
//		{
//			std::cerr << "Opennign this folder: " << (*dit).c_str() << std::endl ;
//			QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromUtf8((*dit).c_str())));
//		}
//	}

#ifdef RDM_DEBUG
	std::cerr << "::::::::::::Done RemoteDirModel::openSelected()" << std::endl;
#endif
}

void RemoteDirModel::getFilePaths(QModelIndexList list, std::list<std::string> &fullpaths)
{
#ifdef RDM_DEBUG
	std::cerr << "RemoteDirModel::getFilePaths()" << std::endl;
#endif
	if (RemoteMode)
	{
#ifdef RDM_DEBUG
		std::cerr << "No File Paths for remote files" << std::endl;
#endif
		return;
	}
	/* translate */
	QModelIndexList::iterator it;
	for(it = list.begin(); it != list.end(); it++)
	{
		void *ref = it -> internalPointer();

     		DirDetails details;
     		uint32_t flags = DIR_FLAGS_DETAILS;
     		flags |= DIR_FLAGS_LOCAL;

     		if (!rsFiles->RequestDirDetails(ref, details, flags))
     		{
#ifdef RDM_DEBUG
			std::cerr << "getFilePaths() Bad Request" << std::endl;
#endif
			continue;
     		}

		if (details.type != DIR_TYPE_FILE)
		{
#ifdef RDM_DEBUG
			std::cerr << "getFilePaths() Not File" << std::endl;
#endif
			continue; /* not file! */
		}

#ifdef RDM_DEBUG
		std::cerr << "::::::::::::File Details:::: " << std::endl;
		std::cerr << "Name: " << details.name << std::endl;
		std::cerr << "Hash: " << details.hash << std::endl;
		std::cerr << "Size: " << details.count << std::endl;
		std::cerr << "Path: " << details.path << std::endl;
#endif

		std::string filepath = details.path + "/";
		filepath += details.name;

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
QMimeData * RemoteDirModel::mimeData ( const QModelIndexList & indexes ) const
{
	/* extract from each the member text */
	std::string  text;
	QModelIndexList::const_iterator it;
	std::map<std::string, uint64_t> drags;
	std::map<std::string, uint64_t>::iterator dit;

	for(it = indexes.begin(); it != indexes.end(); it++)
	{
		void *ref = it -> internalPointer();

     		DirDetails details;
     		uint32_t flags = DIR_FLAGS_DETAILS;
     		if (RemoteMode)
		{
     			flags |= DIR_FLAGS_REMOTE;
		}
     		else
		{
     			flags |= DIR_FLAGS_LOCAL;
		}

     		if (!rsFiles->RequestDirDetails(ref, details, flags))
     		{
			continue;
     		}

#ifdef RDM_DEBUG
		std::cerr << "::::::::::::FileDrag:::: " << std::endl;
		std::cerr << "Name: " << details.name << std::endl;
		std::cerr << "Hash: " << details.hash << std::endl;
		std::cerr << "Size: " << details.count << std::endl;
		std::cerr << "Path: " << details.path << std::endl;
#endif

		if (details.type != DIR_TYPE_FILE)
		{
#ifdef RDM_DEBUG
			std::cerr << "RemoteDirModel::mimeData() Not File" << std::endl;
#endif
			continue; /* not file! */
		}

		if (drags.end() != (dit = drags.find(details.hash)))
		{
#ifdef RDM_DEBUG
			std::cerr << "RemoteDirModel::mimeData() Duplicate" << std::endl;
#endif
			continue; /* duplicate */
		}

		drags[details.hash] = details.count;

		std::string line = details.name;
		line += "/";
		line += details.hash;
		line += "/";

		{
			std::ostringstream out;
			out << details.count;
			line += out.str();
			line += "/";
		}

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

	std::cerr << text;
	std::cerr << std::endl;
#endif

	QMimeData *data = new QMimeData();
	data->setData("application/x-rsfilelist", QByteArray(text.c_str()));

	return data;


}

QStringList RemoteDirModel::mimeTypes () const
{
	QStringList list;
	list.push_back("application/x-rsfilelist");

	return list;
}

//============================================================================

bool
RemoteDirModel::isDir ( const QModelIndex & index ) const
{
    //if (RemoteMode) // only local files can be opened
    //    return ;
    void *ref = index.internalPointer();
    if (!ref)
        return false;

    DirDetails details;
    uint32_t flags = DIR_FLAGS_DETAILS;
    if (RemoteMode)
        flags |= DIR_FLAGS_REMOTE;
    else
        flags |= DIR_FLAGS_LOCAL;

    if (!rsFiles->RequestDirDetails(ref, details, flags))
    {
        return false;//not good, but....
    }

    return (details.type == DIR_TYPE_DIR) ;
}



