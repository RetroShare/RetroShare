/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/TransfersDialog.cpp                     *
 *                                                                             *
 * Copyright (c) 2007 Crypton         <retroshare.project@gmail.com>           *
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

#include "TransfersDialog.h"

#include "gui/notifyqt.h"
#include "gui/SoundManager.h"
#include "gui/RetroShareLink.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/RsCollection.h"
#include "gui/common/RSTreeView.h"
#include "gui/common/RsUrlHandler.h"
#include "gui/FileTransfer/DetailsDialog.h"
#include "gui/FileTransfer/DLListDelegate.h"
#include "gui/FileTransfer/FileTransferInfoWidget.h"
#include "gui/FileTransfer/SearchDialog.h"
#include "gui/FileTransfer/SharedFilesDialog.h"
#include "gui/FileTransfer/TransferUserNotify.h"
#include "gui/FileTransfer/ULListDelegate.h"
#include "gui/FileTransfer/xprogressbar.h"
#include "gui/settings/rsharesettings.h"
#include "util/misc.h"
#include "util/QtVersion.h"
#include "util/RsFile.h"
#include "util/qtthreadsutils.h"

#include "retroshare/rsdisc.h"
#include "retroshare/rsfiles.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsplugin.h"
#include "retroshare/rsturtle.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QShortcut>
#include <QStandardItemModel>

#include <algorithm>
#include <limits>
#include <math.h>

/* Images for context menu icons */
#define IMAGE_INFO                 ":/images/fileinfo.png"
#define IMAGE_CANCEL               ":/images/delete.png"
#define IMAGE_CLEARCOMPLETED       ":/images/deleteall.png"
#define IMAGE_PLAY                 ":/images/player_play.png"
#define IMAGE_COPYLINK             ":/images/copyrslink.png"
#define IMAGE_PASTELINK            ":/images/pasterslink.png"
#define IMAGE_PAUSE					       ":/images/pause.png"
#define IMAGE_RESUME				       ":/images/resume.png"
#define IMAGE_OPENFOLDER			     ":/images/folderopen.png"
#define IMAGE_OPENFILE			       ":/images/fileopen.png"
#define IMAGE_STOP			           ":/images/stop.png"
#define IMAGE_PREVIEW			         ":/images/preview.png"
#define IMAGE_PRIORITY			       ":/images/filepriority.png"
#define IMAGE_PRIORITYLOW	     ":/images/prioritylow.png"
#define IMAGE_PRIORITYNORMAL		 ":/images/prioritynormal.png"
#define IMAGE_PRIORITYHIGH		   ":/images/priorityhigh.png"
#define IMAGE_PRIORITYAUTO		   ":/images/priorityauto.png"
#define IMAGE_SEARCH               ":/icons/svg/magnifying-glass.svg"
#define IMAGE_EXPAND               ":/images/edit_add24.png"
#define IMAGE_COLLAPSE             ":/images/edit_remove24.png"
#define IMAGE_LIBRARY              ":/icons/collections.png"
#define IMAGE_COLLCREATE           ":/iconss/png/add.png"
#define IMAGE_COLLMODIF            ":/icons/png/pencil-edit-button.png"
#define IMAGE_COLLVIEW             ":/images/find.png"
#define IMAGE_COLLOPEN             ":/icons/collections.png"
#define IMAGE_FRIENDSFILES         ":/icons/svg/folders.svg"
#define IMAGE_MYFILES              ":icons/svg/folders1.svg"
#define IMAGE_RENAMEFILE           ":images/filecomments.png"
#define IMAGE_STREAMING            ":images/streaming.png"
#define IMAGE_TUNNEL_ANON_E2E      ":/images/blue_lock.png"
#define IMAGE_TUNNEL_ANON          ":/images/blue_lock_open.png"
#define IMAGE_TUNNEL_FRIEND        ":/icons/avatar_128.png"

//#define DEBUG_DOWNLOADLIST 1

Q_DECLARE_METATYPE(FileProgressInfo)

std::ostream& operator<<(std::ostream& o, const QModelIndex& i)
{
	return o << i.row() << "," << i.column() << "," << i.internalPointer() ;
}

class RsDownloadListModel : public QAbstractItemModel
{
	//    Q_OBJECT

public:
	explicit RsDownloadListModel(QObject *parent = NULL) : QAbstractItemModel(parent) {}
	~RsDownloadListModel(){}

	enum Roles{ SortRole = Qt::UserRole+1 };

	int rowCount(const QModelIndex& parent = QModelIndex()) const
	{
		void *ref = (parent.isValid())?parent.internalPointer():NULL ;

		if(!ref)
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "rowCount-1(" << parent << ") : " <<  mDownloads.size() << std::endl;
#endif
			return mDownloads.size() ;
		}

		uint32_t entry = 0 ;
		int source_id ;

		if(!convertRefPointerToTabEntry(ref,entry,source_id) || entry >= mDownloads.size() || source_id > -1)
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "rowCount-2(" << parent << ") : " <<  0 << std::endl;
#endif
			return 0 ;
		}

#ifdef DEBUG_DOWNLOADLIST
		std::cerr << "rowCount-3(" << parent << ") : " <<  mDownloads[entry].peers.size() << std::endl;
#endif
		return mDownloads[entry].peers.size();
	}
	int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const
	{
        return DLListDelegate::COLUMN_COUNT ;
	}
	bool hasChildren(const QModelIndex &parent = QModelIndex()) const
	{
		void *ref = (parent.isValid())?parent.internalPointer():NULL ;
		uint32_t entry = 0;
		int source_id=0 ;

		if(!ref)
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "hasChildren-1(" << parent << ") : " << true << std::endl;
#endif
			return true ;
		}

		if(!convertRefPointerToTabEntry(ref,entry,source_id) || entry >= mDownloads.size() || source_id > -1)
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "hasChildren-2(" << parent << ") : " << false << std::endl;
#endif
			return false ;
		}

#ifdef DEBUG_DOWNLOADLIST
		std::cerr << "hasChildren-3(" << parent << ") : " << !mDownloads[entry].peers.empty() << std::endl;
#endif
		return !mDownloads[entry].peers.empty();
	}

	QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const
	{
        if(row < 0 || column < 0 || column >= DLListDelegate::COLUMN_COUNT)
			return QModelIndex();

		void *parent_ref = (parent.isValid())?parent.internalPointer():NULL ;
		uint32_t entry = 0;
		int source_id=0 ;

		if(!parent_ref)	// top level. The entry is that of a transfer
		{
			void *ref = NULL ;

			if(row >= (int)mDownloads.size() || !convertTabEntryToRefPointer(row,-1,ref))
			{
#ifdef DEBUG_DOWNLOADLIST
				std::cerr << "index-1(" << row << "," << column << " parent=" << parent << ") : " << "NULL" << std::endl;
#endif
				return QModelIndex() ;
			}

#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "index-2(" << row << "," << column << " parent=" << parent << ") : " << createIndex(row,column,ref) << std::endl;
#endif
			return createIndex(row,column,ref) ;
		}

		if(!convertRefPointerToTabEntry(parent_ref,entry,source_id) || entry >= mDownloads.size() || int(mDownloads[entry].peers.size()) <= row)
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "index-5(" << row << "," << column << " parent=" << parent << ") : " << "NULL"<< std::endl ;
#endif
			return QModelIndex() ;
		}

		if(source_id != -1)
			std::cerr << "ERROR: parent.source_id != -1 in index()" << std::endl;

		void *ref = NULL ;

		if(!convertTabEntryToRefPointer(entry,row,ref))
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "index-4(" << row << "," << column << " parent=" << parent << ") : " << "NULL" << std::endl;
#endif
			return QModelIndex() ;
		}

#ifdef DEBUG_DOWNLOADLIST
		std::cerr << "index-3(" << row << "," << column << " parent=" << parent << ") : " << createIndex(row,column,ref) << std::endl;
#endif
		return createIndex(row,column,ref) ;
	}
	QModelIndex parent(const QModelIndex& child) const
	{
		void *child_ref = (child.isValid())?child.internalPointer():NULL ;
		uint32_t entry = 0;
		int source_id=0 ;

		if(!child_ref)
			return QModelIndex() ;

		if(!convertRefPointerToTabEntry(child_ref,entry,source_id) || entry >= mDownloads.size() || int(mDownloads[entry].peers.size()) <= source_id)
			return QModelIndex() ;

		if(source_id < 0)
			return QModelIndex() ;

		void *parent_ref =NULL;

		if(!convertTabEntryToRefPointer(entry,-1,parent_ref))
			return QModelIndex() ;

		return createIndex(entry,child.column(),parent_ref) ;
	}

	QVariant headerData(int section, Qt::Orientation /*orientation*/, int role = Qt::DisplayRole) const
	{
		if(role != Qt::DisplayRole)
			return QVariant();

		switch(section)
		{
		default:
        case DLListDelegate::COLUMN_NAME:         return tr("Name", "i.e: file name");
        case DLListDelegate::COLUMN_SIZE:         return tr("Size", "i.e: file size");
        case DLListDelegate::COLUMN_COMPLETED:    return tr("Completed", "");
        case DLListDelegate::COLUMN_DLSPEED:      return tr("Speed", "i.e: Download speed");
        case DLListDelegate::COLUMN_PROGRESS:     return tr("Progress / Availability", "i.e: % downloaded");
        case DLListDelegate::COLUMN_SOURCES:      return tr("Sources", "i.e: Sources");
        case DLListDelegate::COLUMN_STATUS:       return tr("Status");
        case DLListDelegate::COLUMN_PRIORITY:     return tr("Speed / Queue position");
        case DLListDelegate::COLUMN_REMAINING:    return tr("Remaining");
        case DLListDelegate::COLUMN_DOWNLOADTIME: return tr("Download time", "i.e: Estimated Time of Arrival / Time left");
        case DLListDelegate::COLUMN_ID:           return tr("Hash");
        case DLListDelegate::COLUMN_LASTDL:       return tr("Last Time Seen", "i.e: Last Time Receiced Data");
        case DLListDelegate::COLUMN_PATH:         return tr("Path", "i.e: Where file is saved");
		}
	}

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
	{
		if(!index.isValid())
			return QVariant();

		//int coln = index.column() ;

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
		int source_id=0 ;

#ifdef DEBUG_DOWNLOADLIST
		std::cerr << "data(" << index << ")" ;
#endif

		if(!ref)
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << " [empty]" << std::endl;
#endif
			return QVariant() ;
		}

		if(!convertRefPointerToTabEntry(ref,entry,source_id) || entry >= mDownloads.size())
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
			return QVariant() ;
		}

#ifdef DEBUG_DOWNLOADLIST
		std::cerr << " source_id=" << source_id ;
#endif

		if(source_id >= int(mDownloads[entry].peers.size()))
		{
#ifdef DEBUG_DOWNLOADLIST
			std::cerr << " [empty]" << std::endl;
#endif
			return QVariant() ;
		}

		const FileInfo& finfo(mDownloads[entry]) ;

#ifdef DEBUG_DOWNLOADLIST
		std::cerr << " [ok]" << std::endl;
#endif

		switch(role)
		{
		case Qt::DisplayRole:    return displayRole   (finfo,source_id,index.column()) ;
		case Qt::DecorationRole: return decorationRole(finfo,source_id,index.column()) ;
		case Qt::UserRole:       return userRole      (finfo,source_id,index.column()) ;
		default:
			return QVariant();
		}
	}

	QVariant sizeHintRole(int col) const
	{
		float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

		switch(col)
		{
		default:
        case DLListDelegate::COLUMN_NAME:         return QVariant( QSize(factor * 170, factor*14.0f ));
        case DLListDelegate::COLUMN_SIZE:         return QVariant( QSize(factor * 70 , factor*14.0f ));
        case DLListDelegate::COLUMN_COMPLETED:    return QVariant( QSize(factor * 75 , factor*14.0f ));
        case DLListDelegate::COLUMN_DLSPEED:      return QVariant( QSize(factor * 75 , factor*14.0f ));
        case DLListDelegate::COLUMN_PROGRESS:     return QVariant( QSize(factor * 170, factor*14.0f ));
        case DLListDelegate::COLUMN_SOURCES:      return QVariant( QSize(factor * 90 , factor*14.0f ));
        case DLListDelegate::COLUMN_STATUS:       return QVariant( QSize(factor * 100, factor*14.0f ));
        case DLListDelegate::COLUMN_PRIORITY:     return QVariant( QSize(factor * 100, factor*14.0f ));
        case DLListDelegate::COLUMN_REMAINING:    return QVariant( QSize(factor * 100, factor*14.0f ));
        case DLListDelegate::COLUMN_DOWNLOADTIME: return QVariant( QSize(factor * 100, factor*14.0f ));
        case DLListDelegate::COLUMN_ID:           return QVariant( QSize(factor * 100, factor*14.0f ));
        case DLListDelegate::COLUMN_LASTDL:       return QVariant( QSize(factor * 100, factor*14.0f ));
        case DLListDelegate::COLUMN_PATH:         return QVariant( QSize(factor * 100, factor*14.0f ));
		}
	}


	QVariant displayRole(const FileInfo& fileInfo,int source_id,int col) const
	{
		if(source_id == -1)  // toplevel
			switch(col)
			{
            case DLListDelegate::COLUMN_NAME:           return QVariant(QString::fromUtf8(fileInfo.fname.c_str()));
            case DLListDelegate::COLUMN_COMPLETED:      return QVariant((qlonglong)fileInfo.transfered);
            case DLListDelegate::COLUMN_DLSPEED:        return QVariant((double)((fileInfo.downloadStatus == FT_STATE_DOWNLOADING) ? (fileInfo.tfRate * 1024.0) : 0.0));
            case DLListDelegate::COLUMN_PROGRESS:       return QVariant((float)((fileInfo.size == 0) ? 0 : (fileInfo.transfered * 100.0 / (float)fileInfo.size)));
            case DLListDelegate::COLUMN_STATUS:
			{
				QString status;
				switch (fileInfo.downloadStatus)
				{
				case FT_STATE_FAILED:       status = tr("Failed"); break;
				case FT_STATE_OKAY:         status = tr("Okay"); break;
				case FT_STATE_WAITING:      status = tr("Waiting"); break;
				case FT_STATE_DOWNLOADING:  status = tr("Downloading"); break;
				case FT_STATE_COMPLETE:     status = tr("Complete"); break;
				case FT_STATE_QUEUED:       status = tr("Queued"); break;
				case FT_STATE_PAUSED:       status = tr("Paused"); break;
				case FT_STATE_CHECKING_HASH:status = tr("Checking..."); break;
				default:                    status = tr("Unknown"); break;
				}
				return QVariant(status);
			}

            case DLListDelegate::COLUMN_PRIORITY:
			{
                double priority = DLListDelegate::PRIORITY_NULL;

				if (fileInfo.downloadStatus == FT_STATE_QUEUED)
					priority = fileInfo.queue_position;
				else if (fileInfo.downloadStatus == FT_STATE_COMPLETE)
					priority = 0;
				else
					switch (fileInfo.priority)
					{
                    case SPEED_LOW:     priority = DLListDelegate::PRIORITY_SLOWER; break;
                    case SPEED_NORMAL:  priority = DLListDelegate::PRIORITY_AVERAGE; break;
                    case SPEED_HIGH:    priority = DLListDelegate::PRIORITY_FASTER; break;
                    default:            priority = DLListDelegate::PRIORITY_AVERAGE; break;
					}

				return QVariant(priority);
			}

            case DLListDelegate::COLUMN_REMAINING:       return QVariant((qlonglong)(fileInfo.size - fileInfo.transfered));
            case DLListDelegate::COLUMN_DOWNLOADTIME:    return QVariant((qlonglong)(fileInfo.tfRate > 0)?( (fileInfo.size - fileInfo.transfered) / (fileInfo.tfRate * 1024.0) ) : 0);
            case DLListDelegate::COLUMN_LASTDL:
			{
				qint64 qi64LastDL = fileInfo.lastTS ;

				if (qi64LastDL == 0)	// file is complete, or any raison why the time has not been set properly
				{
					QFileInfo file;

					if (fileInfo.downloadStatus == FT_STATE_COMPLETE)
						file = QFileInfo(QString::fromUtf8(fileInfo.path.c_str()), QString::fromUtf8(fileInfo.fname.c_str()));
					else
						file = QFileInfo(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()), QString::fromUtf8(fileInfo.hash.toStdString().c_str()));

					//Get Last Access on File
					if (file.exists())
						qi64LastDL = file.lastModified().toTime_t();
				}
				return QVariant(qi64LastDL) ;
			}
            case DLListDelegate::COLUMN_PATH:
			{
				QString strPath = QString::fromUtf8(fileInfo.path.c_str());
				QString strPathAfterDL = strPath;
                strPathAfterDL.replace(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()),"[Download Dir]");

				return QVariant(strPathAfterDL);
			}

            case DLListDelegate::COLUMN_SOURCES:
			{
				int active = 0;
				//QString fileHash = QString::fromStdString(fileInfo.hash.toStdString());

				if (fileInfo.downloadStatus != FT_STATE_COMPLETE)
					for (std::vector<TransferInfo>::const_iterator pit = fileInfo.peers.begin() ; pit != fileInfo.peers.end(); ++pit)
					{
						const TransferInfo& transferInfo = *pit;

					//	//unique combination: fileHash + peerId, variant: hash + peerName (too long)
					//	QString hashFileAndPeerId = fileHash + QString::fromStdString(transferInfo.peerId.toStdString());

					//	double peerDlspeed = 0;
					//	if ((uint32_t)transferInfo.status == FT_STATE_DOWNLOADING && fileInfo.downloadStatus != FT_STATE_PAUSED && fileInfo.downloadStatus != FT_STATE_COMPLETE)
					//		peerDlspeed = transferInfo.tfRate * 1024.0;

					//	FileProgressInfo peerpinfo;
					//	peerpinfo.cmap = fcinfo.compressed_peer_availability_maps[transferInfo.peerId];
					//	peerpinfo.type = FileProgressInfo::DOWNLOAD_SOURCE ;
					//	peerpinfo.progress = 0.0;	// we don't display completion for sources.
					//	peerpinfo.nb_chunks = peerpinfo.cmap._map.empty() ? 0 : fcinfo.chunks.size();

						// get the sources (number of online peers)
						if (transferInfo.tfRate > 0 && fileInfo.downloadStatus == FT_STATE_DOWNLOADING)
							++active;
					}

				return QVariant( (float)active + fileInfo.peers.size()/1000.0f );

			}

            case DLListDelegate::COLUMN_SIZE: return QVariant((qlonglong) fileInfo.size);

            case DLListDelegate::COLUMN_ID:   return QVariant(QString::fromStdString(fileInfo.hash.toStdString()));

			default:
				return QVariant("[ TODO ]");
			}
		else
		{
			uint32_t chunk_size = 1024*1024 ;

			switch(col)
			{
			default:
            case DLListDelegate::COLUMN_SOURCES:
            case DLListDelegate::COLUMN_COMPLETED:
            case DLListDelegate::COLUMN_REMAINING:
            case DLListDelegate::COLUMN_LASTDL:
            case DLListDelegate::COLUMN_ID:
            case DLListDelegate::COLUMN_PATH:
            case DLListDelegate::COLUMN_DOWNLOADTIME:
            case DLListDelegate::COLUMN_SIZE:     return QVariant();
            case DLListDelegate::COLUMN_PROGRESS: return QVariant( (fileInfo.size>0)?((fileInfo.peers[source_id].transfered % chunk_size)*100.0/fileInfo.size):0.0) ;
            case DLListDelegate::COLUMN_DLSPEED:
			{
				double peerDlspeed = 0;
				if((uint32_t)fileInfo.peers[source_id].status == FT_STATE_DOWNLOADING && fileInfo.downloadStatus != FT_STATE_PAUSED && fileInfo.downloadStatus != FT_STATE_COMPLETE)
					peerDlspeed = fileInfo.peers[source_id].tfRate * 1024.0;

				return QVariant((double)peerDlspeed) ;
			}
            case DLListDelegate::COLUMN_NAME:
			{
				QString iconName,tooltip;
				return  QVariant(TransfersDialog::getPeerName(fileInfo.peers[source_id].peerId, iconName, tooltip));
			}

            case DLListDelegate::COLUMN_PRIORITY: return QVariant((double)DLListDelegate::PRIORITY_NULL);
			}
		}

		return QVariant("[ERROR]");
	}

	virtual QVariant userRole(const FileInfo& fileInfo,int source_id,int col) const
	{
		if(source_id == -1)
			switch(col)
			{
            case DLListDelegate::COLUMN_PROGRESS:
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

            case DLListDelegate::COLUMN_ID:   return QVariant(QString::fromStdString(fileInfo.hash.toStdString()));


			default:
				return QVariant();
			}
		else
			switch(col)
			{
            case DLListDelegate::COLUMN_PROGRESS:
			{
				FileChunksInfo fcinfo;
				if (!rsFiles->FileDownloadChunksDetails(fileInfo.hash, fcinfo))
					return QVariant();

				RsPeerId pid = fileInfo.peers[source_id].peerId;
				CompressedChunkMap& cmap(fcinfo.compressed_peer_availability_maps[pid]) ;

				FileProgressInfo pinfo;
				pinfo.cmap = cmap;
				pinfo.type = FileProgressInfo::DOWNLOAD_SOURCE;
				pinfo.progress = 0.0; // we dont display completion for sources
				pinfo.nb_chunks = pinfo.cmap._map.empty() ? 0 : fcinfo.chunks.size();

				//std::cerr << "User role of source id " << source_id << std::endl;
				return QVariant::fromValue(pinfo);
			}

            case DLListDelegate::COLUMN_ID: return QVariant(QString::fromStdString(fileInfo.hash.toStdString()) + QString::fromStdString(fileInfo.peers[source_id].peerId.toStdString()));

			default:
				return QVariant();
			}

	}

	QVariant decorationRole(const FileInfo& fileInfo,int source_id,int col) const
	{
        if(col == DLListDelegate::COLUMN_NAME)
		{
			if(source_id == -1)
				return QVariant(FilesDefs::getIconFromFileType(QString::fromUtf8(fileInfo.fname.c_str())));
			else
			{
				QString iconName,tooltip;
				TransfersDialog::getPeerName(fileInfo.peers[source_id].peerId, iconName, tooltip);

				return QVariant(QIcon(iconName));
			}
		}
		else
			return QVariant();
	}

	void update_transfers()
	{
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
			int old_peers_size = fileInfo.peers.size() ;

			rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, fileInfo);

			int new_size = fileInfo.peers.size() ;

			if(old_peers_size < new_size)
			{
				beginInsertRows(index(i,0), old_peers_size, new_size-1);
				insertRows(old_peers_size, new_size - old_peers_size,index(i,0));
#ifdef DEBUG_DOWNLOADLIST
				std::cerr << "called insert rows ( " << old_peers_size << ", " << new_size - old_peers_size << ",index(" << index(i,0)<< "))" << std::endl;
#endif
				endInsertRows();
			}
			else if(new_size < old_peers_size)
			{
				beginRemoveRows(index(i,0), new_size, old_peers_size-1);
				removeRows(new_size, old_peers_size - new_size,index(i,0));
#ifdef DEBUG_DOWNLOADLIST
				std::cerr << "called remove rows ( " << old_peers_size << ", " << old_peers_size - new_size << ",index(" << index(i,0)<< "))" << std::endl;
#endif
				endRemoveRows();
			}

			uint32_t old_status = mDownloads[i].downloadStatus ;

			mDownloads[i] = fileInfo ; // ... because insertRows() calls rowCount() which needs to be consistent with the *old* number of rows.

			if(fileInfo.downloadStatus == FT_STATE_DOWNLOADING || old_status != fileInfo.downloadStatus)
			{
                QModelIndex topLeft = createIndex(i,0), bottomRight = createIndex(i, DLListDelegate::COLUMN_COUNT-1);
			 	emit dataChanged(topLeft, bottomRight);
			}

			// This is apparently not needed.
			//
			// if(!mDownloads.empty())
			// {
            // 	QModelIndex topLeft = createIndex(0,0), bottomRight = createIndex(mDownloads.size()-1, DLListDelegate::COLUMN_COUNT-1);
			// 	emit dataChanged(topLeft, bottomRight);
			// 	mDownloads[i] = fileInfo ;
			// }
		}
	}
private:
	static const uint32_t TRANSFERS_NB_DOWNLOADS_BITS_32BITS   = 22 ;			                            // Means 2^22 simultaneous transfers
	static const uint32_t TRANSFERS_NB_SOURCES_BITS_32BITS     = 10 ;			                            // Means 2^10 simultaneous sources
	static const uint32_t TRANSFERS_NB_SOURCES_BIT_MASK_32BITS = (1 << TRANSFERS_NB_SOURCES_BITS_32BITS)-1 ;// actual bit mask corresponding to previous number of bits

	static bool convertTabEntryToRefPointer(uint32_t entry,int source_id,void *& ref)
	{
		if(source_id < -1)
		{
			std::cerr << "(EE) inconsistent source id = " << source_id << " in convertTabEntryToRefPointer()" << std::endl;
			return false;
		}
		// the pointer is formed the following way:
		//
		//		[ 22 bits   |  10 bits ]
		//
		// This means that the whole software has the following build-in limitation:
		//	  * 4M   simultaenous file transfers
		//	  * 1023 sources

		if(uint32_t(source_id+1) >= (1u<<TRANSFERS_NB_SOURCES_BITS_32BITS) || uint32_t(entry+1) >= (1u<< TRANSFERS_NB_DOWNLOADS_BITS_32BITS))
		{
			std::cerr << "(EE) cannot convert download index " << entry << " and source " << source_id << " to pointer." << std::endl;
			return false ;
		}

		ref = reinterpret_cast<void*>( ( uint32_t(1+entry) << TRANSFERS_NB_SOURCES_BITS_32BITS ) + ( (source_id+1) & TRANSFERS_NB_SOURCES_BIT_MASK_32BITS)) ;

		assert(ref != NULL) ;
		return true;
	}

	static bool convertRefPointerToTabEntry(void *ref,uint32_t& entry,int& source_id)
	{
		assert(ref != NULL) ;

		// we pack the couple (id of DL, id of source) into a single 32-bits pointer that is required by the AbstractItemModel class.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
		uint32_t src = uint32_t(  *reinterpret_cast<uint32_t*>(&ref)   & TRANSFERS_NB_SOURCES_BIT_MASK_32BITS ) ;
		uint32_t ntr =         (  *reinterpret_cast<uint32_t*>(&ref)) >> TRANSFERS_NB_SOURCES_BITS_32BITS ;
#pragma GCC diagnostic pop

		if(ntr == 0)
		{
			std::cerr << "ERROR! ntr=0!"<< std::endl;
			return false ;
		}

		source_id = int(src) - 1 ;
		entry = ntr - 1 ;

		return true;
	}

	std::vector<FileInfo> mDownloads ;	// store the list of downloads, updated from rsFiles.
};

class SortByNameItem : public QStandardItem
{
public:
	explicit SortByNameItem(QHeaderView *header) : QStandardItem()
	{
		this->header = header;
	}

	virtual bool operator<(const QStandardItem &other) const
	{
		QStandardItemModel *m = model();
		if (m == NULL) {
			return QStandardItem::operator<(other);
		}

		QStandardItem *myParent = parent();
		QStandardItem *otherParent = other.parent();

		if (myParent == NULL || otherParent == NULL) {
			return QStandardItem::operator<(other);
		}

        QStandardItem *myName = myParent->child(index().row(), DLListDelegate::COLUMN_NAME);
        QStandardItem *otherName = otherParent->child(other.index().row(), DLListDelegate::COLUMN_NAME);

		if (header == NULL || header->sortIndicatorOrder() == Qt::AscendingOrder) {
			/* Ascending */
			return *myName < *otherName;
		}

		/* Descending, sort peers in ascending order */
		return !(*myName < *otherName);
	}

private:
	QHeaderView *header;
};

class ProgressItem : public SortByNameItem
{
public:
	explicit ProgressItem(QHeaderView *header) : SortByNameItem(header) {}

	virtual bool operator<(const QStandardItem &other) const override
	{
		const int role = model() ? model()->sortRole() : Qt::DisplayRole;

		FileProgressInfo l = data(role).value<FileProgressInfo>();
		FileProgressInfo r = other.data(role).value<FileProgressInfo>();

		if (l < r) {
			return true;
		}
		if (l > r) {
			return false;
		}

		return SortByNameItem::operator<(other);
	}
};

/** Constructor */
TransfersDialog::TransfersDialog(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_bProcessSettings = false;

    connect( ui.downloadList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( downloadListCustomPopupMenu( QPoint ) ) );

	DLListModel = new RsDownloadListModel ;

    // Set Download list model

    DLLFilterModel = new QSortFilterProxyModel(this);
    DLLFilterModel->setSourceModel( DLListModel);
    DLLFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui.downloadList->setModel(DLLFilterModel);

    DLDelegate = new DLListDelegate();
    ui.downloadList->setItemDelegate(DLDelegate);

    QHeaderView *qhvDLList = ui.downloadList->header();
    qhvDLList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(qhvDLList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(downloadListHeaderCustomPopupMenu(QPoint)));
	QHeaderView *qhvULList = ui.uploadsList->header();
	qhvULList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(qhvULList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(uploadsListHeaderCustomPopupMenu(QPoint)));

// Why disable autoscroll ?
// With disabled autoscroll, the treeview doesn't scroll with cursor move
//    ui.downloadList->setAutoScroll(false) ;

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    mShortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.downloadList, 0, 0, Qt::WidgetShortcut);
    connect(mShortcut, SIGNAL(activated()), this, SLOT( cancel ()));

    //Selection Setup
    selection = ui.downloadList->selectionModel();

    ui.downloadList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui.downloadList->setRootIsDecorated(true);

//    /* Set header resize modes and initial section sizes Downloads TreeView*/
    QHeaderView * dlheader = ui.downloadList->header () ;
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_NAME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_SIZE, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_COMPLETED, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_DLSPEED, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_PROGRESS, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_SOURCES, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_STATUS, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_PRIORITY, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_REMAINING, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_DOWNLOADTIME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_ID, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_LASTDL, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(dlheader, DLListDelegate::COLUMN_PATH, QHeaderView::Interactive);

    // set default column and sort order for download
    ui.downloadList->sortByColumn(DLListDelegate::COLUMN_NAME, Qt::AscendingOrder);

    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
    /* Add filter actions */
    QString headerName = DLListModel->headerData(DLListDelegate::COLUMN_NAME, Qt::Horizontal).toString();
    ui.filterLineEdit->addFilter(QIcon(), headerName, DLListDelegate::COLUMN_NAME , QString("%1 %2").arg(tr("Search"), headerName));
    QString headerID = DLListModel->headerData(DLListDelegate::COLUMN_ID, Qt::Horizontal).toString();
    ui.filterLineEdit->addFilter(QIcon(), headerID, DLListDelegate::COLUMN_ID , QString("%1 %2").arg(tr("Search"), headerID));

    connect( ui.uploadsList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( uploadsListCustomPopupMenu( QPoint ) ) );

    // Set Upload list model
    ULListModel = new QStandardItemModel(0,ULListDelegate::COLUMN_UCOUNT);
    ULListModel->setHeaderData(ULListDelegate::COLUMN_UNAME, Qt::Horizontal, tr("Name", "i.e: file name"));
    ULListModel->setHeaderData(ULListDelegate::COLUMN_UPEER, Qt::Horizontal, tr("Peer", "i.e: user name / tunnel id"));
    ULListModel->setHeaderData(ULListDelegate::COLUMN_USIZE, Qt::Horizontal, tr("Size", "i.e: file size"));
    ULListModel->setHeaderData(ULListDelegate::COLUMN_UTRANSFERRED, Qt::Horizontal, tr("Transferred", ""));
    ULListModel->setHeaderData(ULListDelegate::COLUMN_ULSPEED, Qt::Horizontal, tr("Speed", "i.e: upload speed"));
    ULListModel->setHeaderData(ULListDelegate::COLUMN_UPROGRESS, Qt::Horizontal, tr("Progress", "i.e: % uploaded"));
    ULListModel->setHeaderData(ULListDelegate::COLUMN_UHASH, Qt::Horizontal, tr("Hash", ""));

    ui.uploadsList->setModel(ULListModel);

    ULDelegate = new ULListDelegate();
    ui.uploadsList->setItemDelegate(ULDelegate);

// Why disable autoscroll ?
// With disabled autoscroll, the treeview doesn't scroll with cursor move
//    ui.uploadsList->setAutoScroll(false) ;

    //Selection Setup
    selectionUp = ui.uploadsList->selectionModel();

    ui.uploadsList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui.uploadsList->setRootIsDecorated(true);

    /* Set header resize modes and initial section sizes Uploads TreeView*/
    QHeaderView * upheader = ui.uploadsList->header () ;
    QHeaderView_setSectionResizeModeColumn(upheader, ULListDelegate::COLUMN_UNAME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, ULListDelegate::COLUMN_UPEER, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, ULListDelegate::COLUMN_USIZE, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, ULListDelegate::COLUMN_UTRANSFERRED, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, ULListDelegate::COLUMN_ULSPEED, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, ULListDelegate::COLUMN_UPROGRESS, QHeaderView::Interactive);

    upheader->resizeSection ( ULListDelegate::COLUMN_UNAME, 260 );
    upheader->resizeSection ( ULListDelegate::COLUMN_UPEER, 120 );
    upheader->resizeSection ( ULListDelegate::COLUMN_USIZE, 70 );
    upheader->resizeSection ( ULListDelegate::COLUMN_UTRANSFERRED, 75 );
    upheader->resizeSection ( ULListDelegate::COLUMN_ULSPEED, 75 );
    upheader->resizeSection ( ULListDelegate::COLUMN_UPROGRESS, 170 );

    // set default column and sort order for upload
    ui.uploadsList->sortByColumn(ULListDelegate::COLUMN_UNAME, Qt::AscendingOrder);

    QObject::connect(ui.downloadList->selectionModel(),SIGNAL(selectionChanged (const QItemSelection&, const QItemSelection&)),this,SLOT(showFileDetails())) ;

	 ui.tabWidget->insertTab(2,searchDialog = new SearchDialog(), FilesDefs::getIconFromQtResourcePath(IMAGE_SEARCH), tr("Search")) ;
	 ui.tabWidget->insertTab(3,remoteSharedFiles = new RemoteSharedFilesDialog(), FilesDefs::getIconFromQtResourcePath(IMAGE_FRIENDSFILES), tr("Friends files")) ;

	 ui.tabWidget->addTab(localSharedFiles = new LocalSharedFilesDialog(), FilesDefs::getIconFromQtResourcePath(IMAGE_MYFILES), tr("My files")) ;

	 for(int i=0;i<rsPlugins->nbPlugins();++i)
		 if(rsPlugins->plugin(i) != NULL && rsPlugins->plugin(i)->qt_transfers_tab() != NULL)
			 ui.tabWidget->addTab( rsPlugins->plugin(i)->qt_transfers_tab(),QString::fromUtf8(rsPlugins->plugin(i)->qt_transfers_tab_name().c_str()) ) ;

	 ui.tabWidget->setCurrentWidget(ui.uploadsTab);

    /** Setup the actions for the context menu */

	// Actions. Only need to be defined once.
   pauseAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PAUSE), tr("Pause"), this);
   connect(pauseAct, SIGNAL(triggered()), this, SLOT(pauseFileTransfer()));

   resumeAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_RESUME), tr("Resume"), this);
   connect(resumeAct, SIGNAL(triggered()), this, SLOT(resumeFileTransfer()));

   forceCheckAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_CANCEL), tr( "Force Check" ), this );
   connect( forceCheckAct , SIGNAL( triggered() ), this, SLOT( forceCheck() ) );

   cancelAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_CANCEL), tr( "Cancel" ), this );
   connect( cancelAct , SIGNAL( triggered() ), this, SLOT( cancel() ) );

    openFolderAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_OPENFOLDER), tr("Open Folder"), this);
    connect(openFolderAct, SIGNAL(triggered()), this, SLOT(dlOpenFolder()));

    openFileAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_OPENFILE), tr("Open File"), this);
    connect(openFileAct, SIGNAL(triggered()), this, SLOT(dlOpenFile()));

    previewFileAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PREVIEW), tr("Preview File"), this);
    connect(previewFileAct, SIGNAL(triggered()), this, SLOT(dlPreviewFile()));

    detailsFileAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_INFO), tr("Details..."), this);
    connect(detailsFileAct, SIGNAL(triggered()), this, SLOT(showDetailsDialog()));

    clearCompletedAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_CLEARCOMPLETED), tr( "Clear Completed" ), this );
    connect( clearCompletedAct , SIGNAL( triggered() ), this, SLOT( clearcompleted() ) );


    copyLinkAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPYLINK), tr( "Copy RetroShare Link" ), this );
    connect( copyLinkAct , SIGNAL( triggered() ), this, SLOT( dlCopyLink() ) );
    pasteLinkAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PASTELINK), tr( "Paste RetroShare Link" ), this );
    connect( pasteLinkAct , SIGNAL( triggered() ), this, SLOT( pasteLink() ) );
	queueDownAct = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/go-down.png"), tr("Down"), this);
	connect(queueDownAct, SIGNAL(triggered()), this, SLOT(priorityQueueDown()));
	queueUpAct = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/go-up.png"), tr("Up"), this);
	connect(queueUpAct, SIGNAL(triggered()), this, SLOT(priorityQueueUp()));
	queueTopAct = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/go-top.png"), tr("Top"), this);
	connect(queueTopAct, SIGNAL(triggered()), this, SLOT(priorityQueueTop()));
	queueBottomAct = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/go-bottom.png"), tr("Bottom"), this);
	connect(queueBottomAct, SIGNAL(triggered()), this, SLOT(priorityQueueBottom()));
	chunkStreamingAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_STREAMING), tr("Streaming"), this);
	connect(chunkStreamingAct, SIGNAL(triggered()), this, SLOT(chunkStreaming()));
	prioritySlowAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PRIORITYLOW), tr("Slower"), this);
	connect(prioritySlowAct, SIGNAL(triggered()), this, SLOT(speedSlow()));
	priorityMediumAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PRIORITYNORMAL), tr("Average"), this);
	connect(priorityMediumAct, SIGNAL(triggered()), this, SLOT(speedAverage()));
	priorityFastAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PRIORITYHIGH), tr("Faster"), this);
	connect(priorityFastAct, SIGNAL(triggered()), this, SLOT(speedFast()));
	chunkRandomAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PRIORITYAUTO), tr("Random"), this);
	connect(chunkRandomAct, SIGNAL(triggered()), this, SLOT(chunkRandom()));
	chunkProgressiveAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PRIORITYAUTO), tr("Progressive"), this);
	connect(chunkProgressiveAct, SIGNAL(triggered()), this, SLOT(chunkProgressive()));
	playAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PLAY), tr( "Play" ), this );
	connect( playAct , SIGNAL( triggered() ), this, SLOT( dlOpenFile() ) );
	renameFileAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_RENAMEFILE), tr("Rename file..."), this);
	connect(renameFileAct, SIGNAL(triggered()), this, SLOT(renameFile()));
	specifyDestinationDirectoryAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_SEARCH),tr("Specify..."),this) ;
	connect(specifyDestinationDirectoryAct,SIGNAL(triggered()),this,SLOT(chooseDestinationDirectory()));
	expandAllDLAct= new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_EXPAND),tr("Expand all"),this);
	connect(expandAllDLAct,SIGNAL(triggered()),this,SLOT(expandAllDL()));
	collapseAllDLAct= new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COLLAPSE),tr("Collapse all"),this);
	connect(collapseAllDLAct,SIGNAL(triggered()),this,SLOT(collapseAllDL()));
	expandAllULAct= new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_EXPAND),tr("Expand all"),this);
	connect(expandAllULAct,SIGNAL(triggered()),this,SLOT(expandAllUL()));
	collapseAllULAct= new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COLLAPSE),tr("Collapse all"),this);
	connect(collapseAllULAct,SIGNAL(triggered()),this,SLOT(collapseAllUL()));
	collCreateAct= new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COLLCREATE), tr("Create Collection..."), this);
	connect(collCreateAct,SIGNAL(triggered()),this,SLOT(collCreate()));
	collModifAct= new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COLLMODIF), tr("Modify Collection..."), this);
	connect(collModifAct,SIGNAL(triggered()),this,SLOT(collModif()));
	collViewAct= new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COLLVIEW), tr("View Collection..."), this);
	connect(collViewAct,SIGNAL(triggered()),this,SLOT(collView()));
	collOpenAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COLLOPEN), tr( "Download from collection file..." ), this );
	connect(collOpenAct, SIGNAL(triggered()), this, SLOT(collOpen()));
	connect(NotifyQt::getInstance(), SIGNAL(downloadComplete(QString)), this, SLOT(collAutoOpen(QString)));

	/** Setup the actions for the download header context menu */
    showDLSizeAct= new QAction(tr("Size"),this);
    showDLSizeAct->setCheckable(true); showDLSizeAct->setToolTip(tr("Show Size Column"));
    connect(showDLSizeAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLSizeColumn(bool))) ;
    showDLCompleteAct= new QAction(tr("Completed"),this);
    showDLCompleteAct->setCheckable(true); showDLCompleteAct->setToolTip(tr("Show Completed Column"));
    connect(showDLCompleteAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLCompleteColumn(bool))) ;
    showDLDLSpeedAct= new QAction(tr("Speed"),this);
    showDLDLSpeedAct->setCheckable(true); showDLDLSpeedAct->setToolTip(tr("Show Speed Column"));
    connect(showDLDLSpeedAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLDLSpeedColumn(bool))) ;
    showDLProgressAct= new QAction(tr("Progress / Availability"),this);
    showDLProgressAct->setCheckable(true); showDLProgressAct->setToolTip(tr("Show Progress / Availability Column"));
    connect(showDLProgressAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLProgressColumn(bool))) ;
    showDLSourcesAct= new QAction(tr("Sources"),this);
    showDLSourcesAct->setCheckable(true); showDLSourcesAct->setToolTip(tr("Show Sources Column"));
    connect(showDLSourcesAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLSourcesColumn(bool))) ;
    showDLStatusAct= new QAction(tr("Status"),this);
    showDLStatusAct->setCheckable(true); showDLStatusAct->setToolTip(tr("Show Status Column"));
    connect(showDLStatusAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLStatusColumn(bool))) ;
    showDLPriorityAct= new QAction(tr("Speed / Queue position"),this);
    showDLPriorityAct->setCheckable(true); showDLPriorityAct->setToolTip(tr("Show Speed / Queue position Column"));
    connect(showDLPriorityAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLPriorityColumn(bool))) ;
    showDLRemainingAct= new QAction(tr("Remaining"),this);
    showDLRemainingAct->setCheckable(true); showDLRemainingAct->setToolTip(tr("Show Remaining Column"));
    connect(showDLRemainingAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLRemainingColumn(bool))) ;
    showDLDownloadTimeAct= new QAction(tr("Download time"),this);
    showDLDownloadTimeAct->setCheckable(true); showDLDownloadTimeAct->setToolTip(tr("Show Download time Column"));
    connect(showDLDownloadTimeAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLDownloadTimeColumn(bool))) ;
    showDLIDAct= new QAction(tr("Hash"),this);
    showDLIDAct->setCheckable(true); showDLIDAct->setToolTip(tr("Show Hash Column"));
    connect(showDLIDAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLIDColumn(bool))) ;
    showDLLastDLAct= new QAction(tr("Last Time Seen"),this);
    showDLLastDLAct->setCheckable(true); showDLLastDLAct->setToolTip(tr("Show Last Time Seen Column"));
    connect(showDLLastDLAct,SIGNAL(triggered(bool)),this,SLOT(setShowDLLastDLColumn(bool))) ;
    showDLPath= new QAction(tr("Path"),this);
    showDLPath->setCheckable(true); showDLPath->setToolTip(tr("Show Path Column"));
    connect(showDLPath,SIGNAL(triggered(bool)),this,SLOT(setShowDLPath(bool))) ;

	/** Setup the actions for the upload header context menu */
	showULPeerAct= new QAction(tr("Peer"),this);
	showULPeerAct->setCheckable(true); showULPeerAct->setToolTip(tr("Show Peer Column"));
	connect(showULPeerAct,SIGNAL(triggered(bool)),this,SLOT(setShowULPeerColumn(bool))) ;
	showULSizeAct= new QAction(tr("Size"),this);
	showULSizeAct->setCheckable(true); showULSizeAct->setToolTip(tr("Show Peer Column"));
	connect(showULSizeAct,SIGNAL(triggered(bool)),this,SLOT(setShowULSizeColumn(bool))) ;
	showULTransferredAct= new QAction(tr("Transferred"),this);
	showULTransferredAct->setCheckable(true); showULTransferredAct->setToolTip(tr("Show Transferred Column"));
	connect(showULTransferredAct,SIGNAL(triggered(bool)),this,SLOT(setShowULTransferredColumn(bool))) ;
	showULSpeedAct= new QAction(tr("Speed"),this);
	showULSpeedAct->setCheckable(true); showULSpeedAct->setToolTip(tr("Show Speed Column"));
	connect(showULSpeedAct,SIGNAL(triggered(bool)),this,SLOT(setShowULSpeedColumn(bool))) ;
	showULProgressAct= new QAction(tr("Progress"),this);
	showULProgressAct->setCheckable(true); showULProgressAct->setToolTip(tr("Show Progress Column"));
	connect(showULProgressAct,SIGNAL(triggered(bool)),this,SLOT(setShowULProgressColumn(bool))) ;
	showULHashAct= new QAction(tr("Hash"),this);
	showULHashAct->setCheckable(true); showULHashAct->setToolTip(tr("Show Hash Column"));
	connect(showULHashAct,SIGNAL(triggered(bool)),this,SLOT(setShowULHashColumn(bool))) ;

    /** Setup the actions for the upload context menu */
    ulOpenFolderAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_OPENFOLDER), tr("Open Folder"), this);
    connect(ulOpenFolderAct, SIGNAL(triggered()), this, SLOT(ulOpenFolder()));
    ulCopyLinkAct = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPYLINK), tr( "Copy RetroShare Link" ), this );
    connect( ulCopyLinkAct , SIGNAL( triggered() ), this, SLOT( ulCopyLink() ) );

    // load settings
    processSettings(true);

	int H = misc::getFontSizeFactor("HelpButton").height();
	QString help_str = tr(
	    "<h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;"
	    "    File Transfer</h1>"
	    "<p>Retroshare brings two ways of transferring files: direct "
	    "   transfers from your friends, and distant anonymous tunnelled "
	    "   transfers. In addition, file transfer is multi-source and "
	    "   allows swarming (you can be a source while downloading)</p>"
	    "<p>You can share files using the "
	    "   <img src=\":/images/directoryadd_24x24_shadow.png\" width=%2 />"
	    "   icon from the left side bar. These files will be listed in "
	    "   the My Files tab. You can decide for each friend group whether"
	    "   they can or not see these files in their Friends Files tab</p>"
	    "<p>The search tab reports files from your friends' file lists,"
	    "   and distant files that can be reached anonymously using the "
	    "   multi-hop tunnelling system.</p>"
	                     ).arg(QString::number(2*H), QString::number(H)) ;

	registerHelpButton(ui.helpButton,help_str,"TransfersDialog") ;

	mEventHandlerId=0;
    // Do the GUI events in the GUI thread!
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { RsQThreadUtils::postToObject( [this,event]() { handleEvent_main_thread(event); }) ;}, mEventHandlerId, RsEventType::FILE_TRANSFER );
}

void TransfersDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	if(event->mType != RsEventType::FILE_TRANSFER) return;

	const RsFileTransferEvent *fe = dynamic_cast<const RsFileTransferEvent*>(event.get());
	if(!fe)
        return;

 	switch (fe->mFileTransferEventCode)
    {
	case RsFileTransferEventCode::DOWNLOAD_COMPLETE:
    {
        FileInfo nfo ;
        if(!rsFiles->FileDetails(fe->mHash, RS_FILE_HINTS_DOWNLOAD, nfo))
            break;

		SoundManager::play(SOUND_DOWNLOAD_COMPLETE);
		NotifyQt::getInstance()->addToaster(RS_POPUP_DOWNLOAD, fe->mHash.toStdString(), nfo.fname.c_str(),"");
    }
        [[fallthrough]];

	case RsFileTransferEventCode::COMPLETED_FILES_REMOVED:

        getUserNotify()->updateIcon();
    default:
        break;
    }
}

TransfersDialog::~TransfersDialog()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);

    // save settings
    processSettings(false);

    ui.uploadsList->setItemDelegate(nullptr);
    ui.downloadList->setItemDelegate(nullptr);

    delete ULDelegate;
    delete DLDelegate;
}

void TransfersDialog::activatePage(TransfersDialog::Page page)
{
	switch(page)
	{
		case TransfersDialog::SearchTab: ui.tabWidget->setCurrentWidget(searchDialog) ;
													break ;
		case TransfersDialog::LocalSharedFilesTab: ui.tabWidget->setCurrentWidget(localSharedFiles) ;
													break ;
		case TransfersDialog::RemoteSharedFilesTab: ui.tabWidget->setCurrentWidget(remoteSharedFiles) ;
													break ;
		case TransfersDialog::DownloadTab: ui.tabWidget->setCurrentWidget(ui.tab) ;
													break ;
	}
}

UserNotify *TransfersDialog::createUserNotify(QObject *parent)
{
    return new TransferUserNotify(parent);
}

void TransfersDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    QHeaderView *DLHeader = ui.downloadList->header () ;
    QHeaderView *ULHeader = ui.uploadsList->header () ;

    Settings->beginGroup(QString("TransfersDialog"));

    if (bLoad) {
        // load settings

        // state of the lists
        DLHeader->restoreState(Settings->value("downloadList").toByteArray());
        ULHeader->restoreState(Settings->value("uploadList").toByteArray());

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());

        setShowDLSizeColumn(Settings->value("showDLSizeColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_SIZE)).toBool());
        setShowDLCompleteColumn(Settings->value("showDLCompleteColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_COMPLETED)).toBool());
        setShowDLDLSpeedColumn(Settings->value("showDLDLSpeedColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_DLSPEED)).toBool());
        setShowDLProgressColumn(Settings->value("showDLProgressColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PROGRESS)).toBool());
        setShowDLSourcesColumn(Settings->value("showDLSourcesColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_SOURCES)).toBool());
        setShowDLStatusColumn(Settings->value("showDLStatusColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_STATUS)).toBool());
        setShowDLPriorityColumn(Settings->value("showDLPriorityColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PRIORITY)).toBool());
        setShowDLRemainingColumn(Settings->value("showDLRemainingColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_REMAINING)).toBool());
        setShowDLDownloadTimeColumn(Settings->value("showDLDownloadTimeColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_DOWNLOADTIME)).toBool());
        setShowDLIDColumn(Settings->value("showDLIDColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_ID)).toBool());
        setShowDLLastDLColumn(Settings->value("showDLLastDLColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_LASTDL)).toBool());
        setShowDLPath(Settings->value("showDLPath", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PATH)).toBool());

        // selected tab
        ui.tabWidget->setCurrentIndex(Settings->value("selectedTab").toInt());
    } else {
        // save settings

        // state of the lists
        Settings->setValue("downloadList", DLHeader->saveState());
        Settings->setValue("uploadList", ULHeader->saveState());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());

        Settings->setValue("showDLSizeColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_SIZE));
        Settings->setValue("showDLCompleteColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_COMPLETED));
        Settings->setValue("showDLDLSpeedColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_DLSPEED));
        Settings->setValue("showDLProgressColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PROGRESS));
        Settings->setValue("showDLSourcesColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_SOURCES));
        Settings->setValue("showDLStatusColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_STATUS));
        Settings->setValue("showDLPriorityColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PRIORITY));
        Settings->setValue("showDLRemainingColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_REMAINING));
        Settings->setValue("showDLDownloadTimeColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_DOWNLOADTIME));
        Settings->setValue("showDLIDColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_ID));
        Settings->setValue("showDLLastDLColumn", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_LASTDL));
        Settings->setValue("showDLPath", !ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PATH));

        // selected tab
        Settings->setValue("selectedTab", ui.tabWidget->currentIndex());
    }

    Settings->endGroup();
    m_bProcessSettings = false;
}

void TransfersDialog::downloadListCustomPopupMenu( QPoint /*point*/ )
{
	std::set<RsFileHash> items ;
	getDLSelectedItems(&items, NULL) ;

	bool single = (items.size() == 1) ;

	bool atLeastOne_Waiting = false ;
	bool atLeastOne_Downloading = false ;
	bool atLeastOne_Complete = false ;
	bool atLeastOne_Queued = false ;
	bool atLeastOne_Paused = false ;

	bool add_PlayOption = false ;
	bool add_PreviewOption=false ;
	bool add_OpenFileOption = false ;
	bool add_CopyLink = false ;
	bool add_PasteLink = false ;
	bool add_CollActions = false ;

	FileInfo info;

	QMenu priorityQueueMenu(tr("Move in Queue..."), this);
	priorityQueueMenu.setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_PRIORITY));
	priorityQueueMenu.addAction(queueTopAct);
	priorityQueueMenu.addAction(queueUpAct);
	priorityQueueMenu.addAction(queueDownAct);
	priorityQueueMenu.addAction(queueBottomAct);

	QMenu prioritySpeedMenu(tr("Priority (Speed)..."), this);
	prioritySpeedMenu.setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_PRIORITY));
	prioritySpeedMenu.addAction(prioritySlowAct);
	prioritySpeedMenu.addAction(priorityMediumAct);
	prioritySpeedMenu.addAction(priorityFastAct);

	QMenu chunkMenu(tr("Chunk strategy"), this);
	chunkMenu.setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_PRIORITY));
	chunkMenu.addAction(chunkStreamingAct);
	chunkMenu.addAction(chunkProgressiveAct);
	chunkMenu.addAction(chunkRandomAct);

	QMenu collectionMenu(tr("Collection"), this);
	collectionMenu.setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_LIBRARY));
	collectionMenu.addAction(collCreateAct);
	collectionMenu.addAction(collModifAct);
	collectionMenu.addAction(collViewAct);
	collectionMenu.addAction(collOpenAct);

	QMenu contextMnu( this );

	if(!RSLinkClipboard::empty(RetroShareLink::TYPE_FILE))      add_PasteLink=true;
	if(!RSLinkClipboard::empty(RetroShareLink::TYPE_FILE_TREE)) add_PasteLink=true;

	if(!items.empty())
	{
		add_CopyLink = true ;

		//Look for all selected items
		std::set<RsFileHash>::const_iterator it = items.begin();
		std::set<RsFileHash>::const_iterator end = items.end();
		for (; it != end ; ++it) {
			RsFileHash fileHash = *it;

			//Look only for first column == File  List
			//Get Info for current  item
			if (rsFiles->FileDetails(fileHash, RS_FILE_HINTS_DOWNLOAD, info)) {
					/*const uint32_t FT_STATE_FAILED        = 0x0000;
					 *const uint32_t FT_STATE_OKAY          = 0x0001;
					 *const uint32_t FT_STATE_WAITING       = 0x0002;
					 *const uint32_t FT_STATE_DOWNLOADING   = 0x0003;
					 *const uint32_t FT_STATE_COMPLETE      = 0x0004;
					 *const uint32_t FT_STATE_QUEUED        = 0x0005;
					 *const uint32_t FT_STATE_PAUSED        = 0x0006;
					 *const uint32_t FT_STATE_CHECKING_HASH = 0x0007;
					 */
					if (info.downloadStatus == FT_STATE_WAITING)
						atLeastOne_Waiting = true ;

					if (info.downloadStatus == FT_STATE_DOWNLOADING)
						atLeastOne_Downloading=true ;

					if (info.downloadStatus == FT_STATE_COMPLETE) {
						atLeastOne_Complete = true ;
						add_OpenFileOption = single ;
					}
					if (info.downloadStatus == FT_STATE_QUEUED)
						atLeastOne_Queued = true ;

					if (info.downloadStatus == FT_STATE_PAUSED)
						atLeastOne_Paused = true ;

					size_t pos = info.fname.find_last_of('.') ;
					if (pos !=  std::string::npos) {
						// Check if the file is a media file
						if (misc::isPreviewable(info.fname.substr(pos + 1).c_str())) {
							add_PreviewOption = (info.downloadStatus != FT_STATE_COMPLETE) ;
							add_PlayOption = !add_PreviewOption ;
						}
						// Check if the file is a collection
						if (RsCollection::ExtensionString == info.fname.substr(pos + 1).c_str()) {
							add_CollActions = (info.downloadStatus == FT_STATE_COMPLETE);
						}
					}

			}
		}
	}

	if (atLeastOne_Waiting || atLeastOne_Downloading || atLeastOne_Queued || atLeastOne_Paused) {
		contextMnu.addMenu( &prioritySpeedMenu) ;
	}
	if (atLeastOne_Queued) {
		contextMnu.addMenu( &priorityQueueMenu) ;
	}

	if ( (!items.empty())
	     && (atLeastOne_Downloading || atLeastOne_Queued || atLeastOne_Waiting || atLeastOne_Paused)) {
		contextMnu.addMenu(&chunkMenu) ;

		if (single) {
			contextMnu.addAction( renameFileAct) ;
		}

		QMenu *directoryMenu = contextMnu.addMenu(FilesDefs::getIconFromQtResourcePath(IMAGE_OPENFOLDER), tr("Set destination directory")) ;
		directoryMenu->addAction(specifyDestinationDirectoryAct) ;

		// Now get the list of existing  directories.

		std::list< SharedDirInfo> dirs ;
		rsFiles->getSharedDirectories( dirs) ;

		for (std::list<SharedDirInfo>::const_iterator it(dirs.begin());it!=dirs.end();++it){
			// Check for existence of directory name
			QFile directory( QString::fromUtf8((*it).filename.c_str())) ;

			if (!directory.exists()) continue ;
			if (!(directory.permissions() & QFile::WriteOwner)) continue ;

			QAction *act = new QAction(QString::fromUtf8((*it).virtualname.c_str()), directoryMenu) ;
			act->setData(QString::fromUtf8( (*it).filename.c_str() ) ) ;
			connect(act, SIGNAL(triggered()), this, SLOT(setDestinationDirectory())) ;
			directoryMenu->addAction( act) ;
		 }
	 }

	if (atLeastOne_Paused)
		contextMnu.addAction(resumeAct) ;

	if (atLeastOne_Downloading || atLeastOne_Queued || atLeastOne_Waiting)
		contextMnu.addAction(pauseAct) ;

	if (!atLeastOne_Complete && !items.empty()) {
			contextMnu.addAction(forceCheckAct) ;
			contextMnu.addAction(cancelAct) ;
	}
	if (add_PlayOption)
		contextMnu.addAction(playAct) ;

	if (atLeastOne_Paused || atLeastOne_Downloading || atLeastOne_Complete || add_PlayOption)
		contextMnu.addSeparator() ;

	if (single) {
		if (add_OpenFileOption) contextMnu.addAction( openFileAct) ;
		if (add_PreviewOption) contextMnu.addAction( previewFileAct) ;
		contextMnu.addAction( openFolderAct) ;
		contextMnu.addAction( detailsFileAct) ;
		contextMnu.addSeparator() ;//--------------------------------------------
	}

	contextMnu.addAction( clearCompletedAct) ;
	contextMnu.addSeparator() ;

	if (add_CopyLink) {
		contextMnu.addAction( copyLinkAct) ;
	}
	if (add_PasteLink) {
		contextMnu.addAction( pasteLinkAct) ;
	}
	if (add_CopyLink || add_PasteLink) {
		contextMnu.addSeparator() ;
	}

	if (DLLFilterModel->rowCount()>0 ) {
		contextMnu.addAction( expandAllDLAct ) ;
		contextMnu.addAction( collapseAllDLAct ) ;
	}

	contextMnu.addSeparator() ;//-----------------------------------------------

	collCreateAct->setEnabled(true) ;
	collModifAct->setEnabled(single && add_CollActions) ;
	collViewAct->setEnabled(single && add_CollActions) ;
	collOpenAct->setEnabled(true) ;
	contextMnu.addMenu(&collectionMenu) ;

	contextMnu.exec(QCursor::pos()) ;
}

void TransfersDialog::downloadListHeaderCustomPopupMenu( QPoint /*point*/ )
{
    std::cerr << "TransfersDialog::downloadListHeaderCustomPopupMenu()" << std::endl;
    QMenu contextMnu( this );

    showDLSizeAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_SIZE));
    showDLCompleteAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_COMPLETED));
    showDLDLSpeedAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_DLSPEED));
    showDLProgressAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PROGRESS));
    showDLSourcesAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_SOURCES));
    showDLStatusAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_STATUS));
    showDLPriorityAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PRIORITY));
    showDLRemainingAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_REMAINING));
    showDLDownloadTimeAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_DOWNLOADTIME));
    showDLIDAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_ID));
    showDLLastDLAct->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_LASTDL));
    showDLPath->setChecked(!ui.downloadList->isColumnHidden(DLListDelegate::COLUMN_PATH));

    QMenu *menu = contextMnu.addMenu(tr("Columns"));
    menu->addAction(showDLSizeAct);
    menu->addAction(showDLCompleteAct);
    menu->addAction(showDLDLSpeedAct);
    menu->addAction(showDLProgressAct);
    menu->addAction(showDLSourcesAct);
    menu->addAction(showDLStatusAct);
    menu->addAction(showDLPriorityAct);
    menu->addAction(showDLRemainingAct);
    menu->addAction(showDLDownloadTimeAct);
    menu->addAction(showDLIDAct);
    menu->addAction(showDLLastDLAct);
    menu->addAction(showDLPath);

    contextMnu.exec(QCursor::pos());

}

void TransfersDialog::uploadsListCustomPopupMenu( QPoint /*point*/ )
{
	std::set<RsFileHash> items;
	getULSelectedItems(&items, NULL);

	bool single = (items.size() == 1);

	bool add_CopyLink = !items.empty();

	QMenu contextMnu( this );
	if(single)
		contextMnu.addAction( ulOpenFolderAct);

	if (add_CopyLink)
		contextMnu.addAction( ulCopyLinkAct);

	if (ULListModel->rowCount()>0 ) {
		if(single || add_CopyLink)
			contextMnu.addSeparator() ;//-----------------------------------------------

		contextMnu.addAction( expandAllULAct ) ;
		contextMnu.addAction( collapseAllULAct ) ;
	}

	contextMnu.exec(QCursor::pos());
}

void TransfersDialog::uploadsListHeaderCustomPopupMenu( QPoint /*point*/ )
{
	std::cerr << "TransfersDialog::uploadsListHeaderCustomPopupMenu()" << std::endl;
	QMenu contextMnu( this );

    showULPeerAct->setChecked(!ui.uploadsList->isColumnHidden(ULListDelegate::COLUMN_UPEER));
    showULSizeAct->setChecked(!ui.uploadsList->isColumnHidden(ULListDelegate::COLUMN_USIZE));
    showULTransferredAct->setChecked(!ui.uploadsList->isColumnHidden(ULListDelegate::COLUMN_UTRANSFERRED));
    showULSpeedAct->setChecked(!ui.uploadsList->isColumnHidden(ULListDelegate::COLUMN_ULSPEED));
    showULProgressAct->setChecked(!ui.uploadsList->isColumnHidden(ULListDelegate::COLUMN_UPROGRESS));
    showULHashAct->setChecked(!ui.uploadsList->isColumnHidden(ULListDelegate::COLUMN_UHASH));

	QMenu *menu = contextMnu.addMenu(tr("Columns"));
	menu->addAction(showULPeerAct);
	menu->addAction(showULSizeAct);
	menu->addAction(showULTransferredAct);
	menu->addAction(showULSpeedAct);
	menu->addAction(showULProgressAct);
	menu->addAction(showULHashAct);

	contextMnu.exec(QCursor::pos());
}

void TransfersDialog::chooseDestinationDirectory()
{
	QString dest_dir = QFileDialog::getExistingDirectory(this,tr("Choose directory")) ;

	if(dest_dir.isNull())
		return ;

    std::set<RsFileHash> items ;
	getDLSelectedItems(&items, NULL);

    for(std::set<RsFileHash>::const_iterator it(items.begin());it!=items.end();++it)
	{
		std::cerr << "Setting new directory " << dest_dir.toUtf8().data() << " to file " << *it << std::endl;
        rsFiles->setDestinationDirectory(*it,dest_dir.toUtf8().data() ) ;
	}
}
void TransfersDialog::setDestinationDirectory()
{
	std::string dest_dir(qobject_cast<QAction*>(sender())->data().toString().toUtf8().data()) ;

    std::set<RsFileHash> items ;
	getDLSelectedItems(&items, NULL);

    for(std::set<RsFileHash>::const_iterator it(items.begin());it!=items.end();++it)
	{
		std::cerr << "Setting new directory " << dest_dir << " to file " << *it << std::endl;
		rsFiles->setDestinationDirectory(*it,dest_dir) ;
	}
}

int TransfersDialog::addULItem(int row, const FileInfo &fileInfo)
{
	if (fileInfo.peers.empty())
		return -1; //No Peers, nothing to do.

	QString fileHash  = QString::fromStdString(fileInfo.hash.toStdString());

	RsPeerId ownId = rsPeers->getOwnId();

	QString fileName  = QString::fromUtf8(fileInfo.fname.c_str());
	qlonglong fileSize 	= fileInfo.size;

	if(row < 0 )
	{
		row = ULListModel->rowCount();
		ULListModel->insertRow(row);

		// change progress column to own class for sorting
		//ULListModel->setItem(row, COLUMN_UPROGRESS, new ProgressItem(NULL));

        ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UNAME), fileName);
        ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UNAME), FilesDefs::getIconFromFileType(fileName), Qt::DecorationRole);
        ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UHASH), fileHash);
        ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UHASH), fileHash, Qt::UserRole);
	}

    ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_USIZE), QVariant((qlonglong)fileSize));

	//Reset Parent info if child present
    ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UPEER),        QVariant(QString(tr("%1 tunnels").arg(fileInfo.peers.size()))) );
    ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UPEER),        QIcon(), Qt::DecorationRole);
    ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UPEER),        QVariant(), Qt::ToolTipRole);
    ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UTRANSFERRED), QVariant());
    ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UPROGRESS),    QVariant());

	QStandardItem *ulItem = ULListModel->item(row);
	std::set<int> used_rows ;
	double peerULSpeedTotal = 0;
	bool bOnlyOne = ( fileInfo.peers.size() == 1 );

	for(std::vector<TransferInfo>::const_iterator pit = fileInfo.peers.begin() ; pit != fileInfo.peers.end(); ++pit)
	{
		const TransferInfo &transferInfo = *pit;

		if (transferInfo.peerId == ownId) //don't display transfer to ourselves
			continue ;

		//unique combination: fileHash + peerId, variant: hash + peerName (too long)
		QString hashFileAndPeerId = fileHash + QString::fromStdString(transferInfo.peerId.toStdString());

		double peerULSpeed = transferInfo.tfRate * 1024.0;

		FileProgressInfo peerpinfo ;
		if(!rsFiles->FileUploadChunksDetails(fileInfo.hash, transferInfo.peerId, peerpinfo.cmap) )
			continue ;

		// Estimate the completion. We need something more accurate, meaning that we need to
		// transmit the completion info.
		//
		uint32_t chunk_size = 1024*1024 ;
		uint32_t nb_chunks = (uint32_t)((fileInfo.size + (uint64_t)chunk_size - 1) / (uint64_t)(chunk_size)) ;

		uint32_t filled_chunks = peerpinfo.cmap.filledChunks(nb_chunks) ;
		peerpinfo.type = FileProgressInfo::UPLOAD_LINE ;
		peerpinfo.nb_chunks = peerpinfo.cmap._map.empty()?0:nb_chunks ;

		qlonglong completed = 0;
		if(filled_chunks > 0 && nb_chunks > 0)
		{
			completed = peerpinfo.cmap.computeProgress(fileInfo.size,chunk_size) ;
			peerpinfo.progress = completed / (float)fileInfo.size * 100.0f ;
		}
		else
		{
			completed = transferInfo.transfered % chunk_size ;	// use the position with respect to last request.
			peerpinfo.progress = (fileInfo.size>0)?((transferInfo.transfered % chunk_size)*100.0/fileInfo.size):0 ;
		}

		if (bOnlyOne)
		{
			//Only one peer so update parent
			QString iconName;
			QString tooltip;
            ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UPEER),        QVariant(getPeerName(transferInfo.peerId, iconName, tooltip)));
            ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UPEER),        QIcon(iconName), Qt::DecorationRole);
            ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UPEER),        QVariant(tooltip), Qt::ToolTipRole);
            ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UTRANSFERRED), QVariant(completed));
            ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_UPROGRESS),    QVariant::fromValue(peerpinfo));
		} else {
			int row_id = addPeerToULItem(ulItem, transferInfo.peerId, hashFileAndPeerId, completed, peerULSpeed, peerpinfo);

			used_rows.insert(row_id);
		}
		peerULSpeedTotal += peerULSpeed;

	}

	// Update Parent UpLoad Speed
    ULListModel->setData(ULListModel->index(row, ULListDelegate::COLUMN_ULSPEED), QVariant((double)peerULSpeedTotal));


	// This is not optimal, but we deal with a small number of elements. The reverse order is really important,
	// because rows after the deleted rows change positions !
	//
	for (int r = ulItem->rowCount() - 1; r >= 0; --r) {
		if (used_rows.find(r) == used_rows.end()) {
			ulItem->removeRow(r);
		}
	}

	return row;
}

int TransfersDialog::addPeerToULItem(QStandardItem *ulItem, const RsPeerId& peer_ID, const QString& coreID, qlonglong completed, double ulspeed, const FileProgressInfo& peerInfo)
{
	// try to find the item
	int childRow = -1;

	QStandardItem *childId = NULL;
    for (int count = 0; (childId = ulItem->child(count, ULListDelegate::COLUMN_UHASH)) != NULL; ++count) {
		if (childId->data(Qt::UserRole).toString() == coreID) {
			childRow = count;
			break;
		}
	}

	if (childRow == -1) {
		// set this false if you want to expand on double click
		ulItem->setEditable(false);

		QHeaderView *header = ui.uploadsList->header();

		QStandardItem *iName  = new QStandardItem(); //COLUMN_UNAME
		QStandardItem *iPeer  = new QStandardItem(); //COLUMN_UPEER
		QStandardItem *iSize  = new SortByNameItem(header); //COLUMN_USIZE
		QStandardItem *iTransferred  = new SortByNameItem(header); //COLUMN_UTRANSFERRED
		QStandardItem *iULSpeed  = new SortByNameItem(header); //COLUMN_ULSPEED
		QStandardItem *iProgress  = new ProgressItem(header); //COLUMN_UPROGRESS
		QStandardItem *iHash  = new SortByNameItem(header); //COLUMN_UHASH

		QList<QStandardItem*> items;
		iName->setData(     QVariant(QString()), Qt::DisplayRole);
		QString iconName;
		QString tooltip;
		iPeer->setData(     QVariant(getPeerName(peer_ID, iconName, tooltip)), Qt::DisplayRole);
		iPeer->setData(     QIcon(iconName), Qt::DecorationRole);
		iPeer->setData(     QVariant(tooltip), Qt::ToolTipRole);
		iSize->setData(     QVariant(QString()), Qt::DisplayRole);
		iTransferred->setData(QVariant((qlonglong)completed), Qt::DisplayRole);
		iULSpeed->setData(  QVariant((double)ulspeed), Qt::DisplayRole);
		iProgress->setData( QVariant::fromValue(peerInfo), Qt::DisplayRole);
		iHash->setData(     QVariant(), Qt::DisplayRole);
		iHash->setData(     QVariant(coreID), Qt::UserRole);

		items.append(iName);
		items.append(iPeer);
		items.append(iSize);
		items.append(iTransferred);
		items.append(iULSpeed);
		items.append(iProgress);
		items.append(iHash);
		ulItem->appendRow(items);

		childRow = ulItem->rowCount() - 1;
	} else {
		// just update the child (peer)
        ulItem->child(childRow, ULListDelegate::COLUMN_ULSPEED)->setData(QVariant((double)ulspeed), Qt::DisplayRole);
        ulItem->child(childRow, ULListDelegate::COLUMN_UTRANSFERRED)->setData(QVariant((qlonglong)completed), Qt::DisplayRole);
        ulItem->child(childRow, ULListDelegate::COLUMN_UPROGRESS)->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);
	}

	return childRow;
}

/* get the list of Transfers from the RsIface.  **/
void TransfersDialog::updateDisplay()
{
	insertTransfers();
	updateDetailsDialog ();
}

void TransfersDialog::insertTransfers() 
{
	// Since downloads use an AstractItemModel, we just need to update it, while saving the selected and expanded items.

	DLListModel->update_transfers();

	// Now show upload hashes. Here we use the "old" way, since the number of uploads is generally not so large.
	//

	/* disable for performance issues, enable after insert all transfers */
    ui.uploadsList->setSortingEnabled(false);

	/* get the upload lists */
	std::list<RsFileHash> upHashes;
	rsFiles->FileUploads(upHashes);

	/* build set for quick search */
	std::set<RsFileHash> hashs;

	for(std::list<RsFileHash>::iterator it = upHashes.begin(); it != upHashes.end(); ++it) {
		hashs.insert(*it);
	}

	/* add uploads, first iterate all rows in list */

	int rowCount = ULListModel->rowCount();

	for (int row = 0; row < rowCount; ) {
        RsFileHash hash ( ULListModel->item(row, ULListDelegate::COLUMN_UHASH)->data(Qt::UserRole).toString().toStdString());

		std::set<RsFileHash>::iterator hashIt = hashs.find(hash);
		if (hashIt == hashs.end()) {
			// remove not existing uploads
			ULListModel->removeRow(row);
			rowCount = ULListModel->rowCount();
			continue;
		}

		FileInfo fileInfo;
		if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_UPLOAD, fileInfo)) {
			ULListModel->removeRow(row);
			rowCount = ULListModel->rowCount();
			continue;
		}

		hashs.erase(hashIt);

		if (addULItem(row, fileInfo) < 0) {
			ULListModel->removeRow(row);
			rowCount = ULListModel->rowCount();
			continue;
		}

		++row;
	}

	/* then add new uploads to the list */

	for (std::set<RsFileHash>::iterator hashIt = hashs.begin()
	     ; hashIt != hashs.end(); ++hashIt)
	{
		FileInfo fileInfo;
		if (!rsFiles->FileDetails(*hashIt, RS_FILE_HINTS_UPLOAD, fileInfo)) {
			continue;
		}

		addULItem(-1, fileInfo);
	}

	ui.uploadsList->setSortingEnabled(true);
	
	downloads = tr("Downloads") + " (" + QString::number(DLListModel->rowCount()) + ")";
	uploads   = tr("Uploads")   + " (" + QString::number(ULListModel->rowCount()) + ")" ;

	ui.tabWidget->setTabText(0, downloads);
	ui.tabWidget_UL->setTabText(0, uploads);

}

QString TransfersDialog::getPeerName(const RsPeerId& id, QString &iconName, QString &tooltip)
{
	QString res = QString::fromUtf8(rsPeers->getPeerName(id).c_str()) ;

	// csoler 2009-06-03: This is because turtle tunnels have no name (I didn't want to bother with
	// connect mgr). In such a case their id can suitably hold for a name.
	//
	if(res == "")
	{
		res = QString::fromUtf8(rsTurtle->getPeerNameForVirtualPeerId(id).c_str());
		if(rsFiles->isEncryptedSource(id))
		{
			iconName = IMAGE_TUNNEL_ANON_E2E;
			tooltip = tr("Anonymous end-to-end encrypted tunnel 0x")+QString::fromStdString(id.toStdString()).left(8);
			return tr("Tunnel") + " via " + res ;
		}

		iconName = IMAGE_TUNNEL_ANON;
		tooltip = tr("Anonymous tunnel 0x")+QString::fromStdString(id.toStdString()).left(8);
		return tr("Tunnel") + " via " + res ;
	}

	iconName = IMAGE_TUNNEL_FRIEND;
	tooltip = res;
	return res ;
}

void TransfersDialog::forceCheck()
{
	if (!controlTransferFile(RS_FILE_CTRL_FORCE_CHECK))
		std::cerr << "resumeFileTransfer(): can't force check file transfer" << std::endl;
}

void TransfersDialog::cancel()
{
	bool first = true;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); ++it) {
		if (first) {
			first = false;
			QString queryWrn2;
			queryWrn2.clear();
			queryWrn2.append(tr("Are you sure that you want to cancel and delete these files?"));

			if ((QMessageBox::question(this, tr("RetroShare"),queryWrn2,QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::No) {
				break;
			}
		}

		rsFiles->FileCancel(*it);
	}
}

//void TransfersDialog::handleDownloadRequest(const QString& url)
//{
//    RetroShareLink link(url);
//
//    if (!link.valid ())
//	 {
//		 QMessageBox::critical(NULL,"Link error","This link could not be parsed. This is a bug. Please contact the developers.") ;
//		 return;
//	 }
//
//    QVector<RetroShareLinkData> linkList;
//    analyzer.getFileInformation (linkList);
//
//    std::list<std::string> srcIds;
//
//    for (int i = 0, n = linkList.size (); i < n; ++i)
//    {
//        const RetroShareLinkData& linkData = linkList[i];
//
//        rsFiles->FileRequest (linkData.getName ().toStdString (), linkData.getHash ().toStdString (),
//            linkData.getSize ().toInt (), "", 0, srcIds);
//    }
//}

void TransfersDialog::dlCopyLink ()
{
	QList<RetroShareLink> links ;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); ++it) {
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) {
			continue;
		}

		RetroShareLink link = RetroShareLink::createFile(QString::fromUtf8(info.fname.c_str()), info.size, QString::fromStdString(info.hash.toStdString()));
		if (link.valid()) {
			links.push_back(link) ;
		}
	}

	RSLinkClipboard::copyLinks(links) ;
}

void TransfersDialog::ulCopyLink ()
{
    QList<RetroShareLink> links ;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
    getULSelectedItems(&items, NULL);

    for (it = items.begin(); it != items.end(); ++it) {
        FileInfo info;
        if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_UPLOAD, info)) {
            continue;
        }

        RetroShareLink link = RetroShareLink::createFile(QString::fromUtf8(info.fname.c_str()), info.size, QString::fromStdString(info.hash.toStdString()));
        if (link.valid()) {
            links.push_back(link) ;
        }
    }

    RSLinkClipboard::copyLinks(links) ;
}

DetailsDialog *TransfersDialog::detailsDialog()
{
	static DetailsDialog *detailsdlg = new DetailsDialog ;

	 return detailsdlg ;
}

void TransfersDialog::showDetailsDialog()
{
    updateDetailsDialog ();

    detailsDialog()->show();
}

void TransfersDialog::updateDetailsDialog()
{
	std::set<RsFileHash> items;
	getDLSelectedItems(&items, NULL);

	if (!items.empty())
		detailsDialog()->setFileHash(*items.begin());
}

void TransfersDialog::pasteLink()
{
	QList<RetroShareLink> links ;

	// We want to capture and process all links at once here, because we're possibly pasting a large collection of files. So we first
	// merge all links into a single RsCollection and then process it.

	RsCollection col ;
	RSLinkClipboard::pasteLinks(links,RetroShareLink::TYPE_FILE_TREE);

	for(auto &it: links)
	{
		auto ft = RsFileTree::fromRadix64(it.radix().toStdString());
		col.merge_in(*ft);
	}
	links.clear();
	RSLinkClipboard::pasteLinks(links,RetroShareLink::TYPE_FILE);

	for(auto &it : links)
		col.merge_in(it.name(),it.size(),RsFileHash(it.hash().toStdString())) ;

	col.downloadFiles();
}

void TransfersDialog::getDLSelectedItems(std::set<RsFileHash> *ids, std::set<int> *rows)
{
	if (ids == NULL && rows == NULL) {
		return;
	}

	if (ids) ids->clear();
	if (rows) rows->clear();

    QModelIndexList selectedRows = selection->selectedRows(DLListDelegate::COLUMN_ID);

	int i, imax = selectedRows.count();
	for (i = 0; i < imax; ++i) {
		QModelIndex index = selectedRows.at(i);
		if (index.parent().isValid() && index.model())
            index = index.model()->index(index.parent().row(), DLListDelegate::COLUMN_ID);

		if (ids) {
			ids->insert(RsFileHash(index.data(Qt::DisplayRole).toString().toStdString()));
			ids->insert(RsFileHash(index.data(Qt::UserRole   ).toString().toStdString()));
		}

		if (rows) {
			rows->insert(index.row());
		}

	}
}

void TransfersDialog::getULSelectedItems(std::set<RsFileHash> *ids, std::set<int> *rows)
{
    if (ids == NULL && rows == NULL) {
        return;
    }

    if (ids) ids->clear();
    if (rows) rows->clear();

    QModelIndexList indexes = selectionUp->selectedIndexes();
    QModelIndex index;

    foreach(index, indexes) {
        if (ids) {
            QStandardItem *id = ULListModel->item(index.row(), ULListDelegate::COLUMN_UHASH);
            ids->insert(RsFileHash(id->data(Qt::DisplayRole).toString().toStdString()));
        }
        if (rows) {
            rows->insert(index.row());
        }

    }

}

bool TransfersDialog::controlTransferFile(uint32_t flags)
{
	bool result = true;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); ++it) {
		result &= rsFiles->FileControl(*it, flags);
	}

	return result;
}

void TransfersDialog::pauseFileTransfer()
{
	if (!controlTransferFile(RS_FILE_CTRL_PAUSE))
	{
		std::cerr << "pauseFileTransfer(): can't pause file transfer" << std::endl;
	}
}

void TransfersDialog::resumeFileTransfer()
{
	if (!controlTransferFile(RS_FILE_CTRL_START))
	{
		std::cerr << "resumeFileTransfer(): can't resume file transfer" << std::endl;
	}
}

void TransfersDialog::dlOpenFolder()
{
	FileInfo info;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); ++it) {
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

    openFolder(info);
}

void TransfersDialog::openFolder(const FileInfo& info)
{
	/* make path for downloaded or downloading files */
    QDir directory;

    if (info.downloadStatus == FT_STATE_COMPLETE)
        directory = QFileInfo(QString::fromStdString(info.path)).absoluteDir().path();
     else
        directory = QDir(QString::fromStdString(rsFiles->getPartialsDirectory()));

	/* open folder with a suitable application */

    if (directory.exists() && !RsUrlHandler::openUrl(QUrl::fromLocalFile(directory.path())))
            std::cerr << "dlOpenFolder(): can't open folder " << directory.path().toStdString() << std::endl;
}

void TransfersDialog::ulOpenFolder()
{
    FileInfo info;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
    getULSelectedItems(&items, NULL);
    for (it = items.begin(); it != items.end(); ++it) {
        if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_UPLOAD, info)) continue;
        break;
    }

    openFolder(info);
}

void TransfersDialog::dlPreviewFile()
{
	FileInfo info;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); ++it) {
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	QFileInfo fileNameInfo(QString::fromUtf8(info.fname.c_str()));

	/* check if the file is a media file */
	if (!misc::isPreviewable(fileNameInfo.suffix())) return;

	/* make path for downloaded or downloading files */
	QFileInfo fileInfo;
	if (info.downloadStatus == FT_STATE_COMPLETE) {
        fileInfo = QFileInfo(QString::fromUtf8(info.path.c_str()));
	} else {
        fileInfo = QFileInfo(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()), QString::fromUtf8(info.hash.toStdString().c_str()));

		QDir temp;
#ifdef WINDOWS_SYS
		/* the symbolic link must be created on the same drive like the real file, use partial directory */
		temp = fileInfo.absoluteDir();
#else
		temp = QDir::temp();
#endif

		QString linkName = QFileInfo(temp, fileNameInfo.fileName()).absoluteFilePath();
		if (RsFile::CreateLink(fileInfo.absoluteFilePath(), linkName)) {
			fileInfo.setFile(linkName);
		} else {
			std::cerr << "previewTransfer(): can't create link for file " << fileInfo.absoluteFilePath().toStdString() << std::endl;
			QMessageBox::warning(this, tr("File preview"), tr("Can't create link for file %1.").arg(fileInfo.absoluteFilePath()));
			return;
		}
	}

	bool previewStarted = false;
	/* open or preview them with a suitable application */
	if (fileInfo.exists() && RsUrlHandler::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()))) {
		previewStarted = true;
	} else {
		QMessageBox::warning(this, tr("File preview"), tr("File %1 preview failed.").arg(fileInfo.absoluteFilePath()));
		std::cerr << "previewTransfer(): can't preview file " << fileInfo.absoluteFilePath().toStdString() << std::endl;
	}

	if (info.downloadStatus != FT_STATE_COMPLETE) {
		if (previewStarted) {
			/* wait for the file to open then remove the link */
			QMessageBox::information(this, tr("File preview"), tr("Click OK when program terminates!"));
		}
		/* try to delete the preview file */
		forever {
			if (QFile::remove(fileInfo.absoluteFilePath())) {
				/* preview file could be removed */
				break;
			}
			/* ask user to try it again */
			if (QMessageBox::question(this, tr("File preview"), QString("%1\n\n%2\n\n%3").arg(tr("Could not delete preview file"), fileInfo.absoluteFilePath(), tr("Try it again?")),  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
				break;
			}
		}
	}
}

void TransfersDialog::dlOpenFile()
{
	FileInfo info;

	std::set<RsFileHash> items ;
	std::set<RsFileHash>::iterator it ;
	getDLSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); ++it) {
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	/* make path for downloaded or downloading files */
	std::string path;
	if (info.downloadStatus == FT_STATE_COMPLETE) {
        path = info.path ;

		/* open file with a suitable application */
		QFileInfo qinfo;
		qinfo.setFile(QString::fromUtf8(path.c_str()));
		if (qinfo.exists()) {
			if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
				std::cerr << "openTransfer(): can't open file " << path << std::endl;
			}
		}
	} else {
		/* rise a message box for incompleted download file */
		QMessageBox::information(this, tr("Open Transfer"),
								 tr("File %1 is not completed. If it is a media file, try to preview it.").arg(QString::fromUtf8(info.fname.c_str())));
	}
}

/* clear download or all queue - for pending dwls */
//void TransfersDialog::clearQueuedDwl()
//{
//	std::set<QStandardItem *> items;
//	std::set<QStandardItem *>::iterator it;
//	getSelectedItems(&items, NULL);
//
//	for (it = items.begin(); it != items.end(); ++it) {
//		std::string hash = (*it)->data(Qt::DisplayRole).toString().toStdString();
//		rsFiles->clearDownload(hash);
//	}
//}
//void TransfersDialog::clearQueue()
//{
//	rsFiles->clearQueue();
//}

void TransfersDialog::chunkStreaming()
{
	setChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_STREAMING) ;
}
void TransfersDialog::chunkRandom()
{
#ifdef WINDOWS_SYS
    if(QMessageBox::Yes != QMessageBox::warning(nullptr,tr("Warning"),tr("On Windows systems, writing in the middle of large empty files may hang the software for several seconds. Do you want to use this option anyway?"),QMessageBox::Yes,QMessageBox::No))
        return;
#endif
	setChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_RANDOM) ;
}
void TransfersDialog::chunkProgressive()
{
	setChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE) ;
}
void TransfersDialog::setChunkStrategy(FileChunksInfo::ChunkStrategy s)
{
    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); ++it) {
		rsFiles->setChunkStrategy(*it, s);
	}
}
/* modify download priority actions */
void TransfersDialog::speedSlow()
{
	changeSpeed(0);
}
void TransfersDialog::speedAverage()
{
	changeSpeed(1);
}
void TransfersDialog::speedFast()
{
	changeSpeed(2);
}

void TransfersDialog::priorityQueueUp()
{
	changeQueuePosition(QUEUE_UP);
}
void TransfersDialog::priorityQueueDown()
{
	changeQueuePosition(QUEUE_DOWN);
}
void TransfersDialog::priorityQueueTop()
{
	changeQueuePosition(QUEUE_TOP);
}
void TransfersDialog::priorityQueueBottom()
{
	changeQueuePosition(QUEUE_BOTTOM);
}

void TransfersDialog::changeSpeed(int speed)
{
    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); ++it) 
	{
		rsFiles->changeDownloadSpeed(*it, speed);
	}
}
static bool checkFileName(const QString& name)
{
	if(name.contains('/')) return false ;
	if(name.contains('\\')) return false ;
	if(name.contains('|')) return false ;
	if(name.contains(':')) return false ;
	if(name.contains('?')) return false ;
	if(name.contains('>')) return false ;
	if(name.contains('<')) return false ;
	if(name.contains('*')) return false ;

	if(name.length() == 0)
		return false ;
	if(name.length() > 255)
		return false ;

	return true ;
}

void TransfersDialog::renameFile()
{
    std::set<RsFileHash> items;
	getDLSelectedItems(&items, NULL);

	if(items.size() != 1)
	{
		std::cerr << "Can't rename more than one file. This should not be called." << std::endl;
		return ;
	}

    RsFileHash hash = *(items.begin()) ;

	FileInfo info ;
	if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, info)) 
		return ;

	bool ok = true ;
	bool first = true ;
	QString new_name ;

	do
	{
		new_name = QInputDialog::getText(NULL,tr("Change file name"),first?tr("Please enter a new file name"):tr("Please enter a new--and valid--filename"),QLineEdit::Normal,QString::fromUtf8(info.fname.c_str()),&ok) ;

		if(!ok)
			return ;
		first = false ;
	}
	while(!checkFileName(new_name)) ;

	rsFiles->setDestinationName(hash, new_name.toUtf8().data());
}

void TransfersDialog::changeQueuePosition(QueueMove mv)
{
	//	std::cerr << "In changeQueuePosition (gui)"<< std::endl ;
    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); ++it) 
	{
		rsFiles->changeQueuePosition(*it, mv);
	}
}

void TransfersDialog::clearcompleted()
{
//	std::cerr << "TransfersDialog::clearcompleted()" << std::endl;
	rsFiles->FileClearCompleted();
}

void TransfersDialog::showFileDetails()
{
	std::set<RsFileHash> items ;
	getDLSelectedItems(&items, NULL) ;
	if(items.size() != 1)
		detailsDialog()->setFileHash(RsFileHash());
	else
		detailsDialog()->setFileHash(*items.begin()) ;

	updateDetailsDialog ();
}

double TransfersDialog::getProgress(int , QStandardItemModel *)
{
//	return model->data(model->index(row, PROGRESS), Qt::DisplayRole).toDouble();
return 0.0 ;
}

double TransfersDialog::getSpeed(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, DLListDelegate::COLUMN_DLSPEED), Qt::DisplayRole).toDouble();
}

QString TransfersDialog::getFileName(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, DLListDelegate::COLUMN_NAME), Qt::DisplayRole).toString();
}

QString TransfersDialog::getStatus(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, DLListDelegate::COLUMN_STATUS), Qt::DisplayRole).toString();
}

QString TransfersDialog::getID(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, DLListDelegate::COLUMN_ID), Qt::UserRole).toString().left(40); // gets only the "hash" part of the name
}

QString TransfersDialog::getID(int row, QSortFilterProxyModel *filter)
{
    QModelIndex index = filter->mapToSource(filter->index(row, DLListDelegate::COLUMN_ID));

	return filter->sourceModel()->data(index, Qt::UserRole).toString().left(40); // gets only the "hash" part of the name
}

QString TransfersDialog::getPriority(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, DLListDelegate::COLUMN_PRIORITY), Qt::DisplayRole).toString();
}

qlonglong TransfersDialog::getFileSize(int row, QStandardItemModel *model)
{
	bool ok = false;
    return model->data(model->index(row, DLListDelegate::COLUMN_SIZE), Qt::DisplayRole).toULongLong(&ok);
}

qlonglong TransfersDialog::getTransfered(int row, QStandardItemModel *model)
{
	bool ok = false;
    return model->data(model->index(row, DLListDelegate::COLUMN_COMPLETED), Qt::DisplayRole).toULongLong(&ok);
}

qlonglong TransfersDialog::getRemainingTime(int row, QStandardItemModel *model)
{
	bool ok = false;
    return model->data(model->index(row, DLListDelegate::COLUMN_REMAINING), Qt::DisplayRole).toULongLong(&ok);
}

qlonglong TransfersDialog::getDownloadTime(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, DLListDelegate::COLUMN_DOWNLOADTIME), Qt::DisplayRole).toULongLong();
}

qlonglong TransfersDialog::getLastDL(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, DLListDelegate::COLUMN_LASTDL), Qt::DisplayRole).toULongLong();
}

qlonglong TransfersDialog::getPath(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, DLListDelegate::COLUMN_PATH), Qt::DisplayRole).toULongLong();
}

QString TransfersDialog::getSources(int row, QStandardItemModel *model)
{
    double dblValue =  model->data(model->index(row, DLListDelegate::COLUMN_SOURCES), Qt::DisplayRole).toDouble();
	QString temp = QString("%1 (%2)").arg((int)dblValue).arg((int)((fmod(dblValue,1)*1000)+0.5));

	return temp;
}

void TransfersDialog::collCreate()
{
	std::vector <DirDetails> dirVec;

	std::set<RsFileHash> items ;
	std::set<RsFileHash>::iterator it ;
	getDLSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); ++it)
	{
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;

		DirDetails details;
		details.name = info.fname;
		details.hash = info.hash;
        details.size = info.size;
		details.type = DIR_TYPE_FILE;

		dirVec.push_back(details);
	}

	RsCollection(dirVec,RS_FILE_HINTS_LOCAL).openNewColl(this);
}

void TransfersDialog::collModif()
{
	FileInfo info;

	std::set<RsFileHash> items ;
	std::set<RsFileHash>::iterator it ;
	getDLSelectedItems(&items, NULL);

	if (items.size() != 1) return;
	it = items.begin();
	RsFileHash hash = *it;
	if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, info)) return;

	/* make path for downloaded files */
	if (info.downloadStatus == FT_STATE_COMPLETE) {
		std::string path;
		path = info.path + "/" + info.fname;

		/* open collection */
		QFileInfo qinfo;
		qinfo.setFile(QString::fromUtf8(path.c_str()));
		if (qinfo.exists()) {
			if (qinfo.absoluteFilePath().endsWith(RsCollection::ExtensionString)) {
				RsCollection collection;
				collection.openColl(qinfo.absoluteFilePath());
			}
		}
	}
}

void TransfersDialog::collView()
{
	FileInfo info;

	std::set<RsFileHash> items;
	std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);

	if (items.size() != 1) return;
	it = items.begin();
	RsFileHash hash = *it;
	if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, info)) return;

	/* make path for downloaded files */
	if (info.downloadStatus == FT_STATE_COMPLETE) {
		std::string path;
		path = info.path + "/" + info.fname;

		/* open collection  */
		QFileInfo qinfo;
		qinfo.setFile(QString::fromUtf8(path.c_str()));
		if (qinfo.exists()) {
			if (qinfo.absoluteFilePath().endsWith(RsCollection::ExtensionString)) {
				RsCollection collection;
				collection.openColl(qinfo.absoluteFilePath(), true);
			}
		}
	}
}

void TransfersDialog::collOpen()
{
	FileInfo info;

	std::set<RsFileHash> items;
	std::set<RsFileHash>::iterator it;
	getDLSelectedItems(&items, NULL);

	if (items.size() == 1) {
		it = items.begin();
		RsFileHash hash = *it;
		if (rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, info)) {

			/* make path for downloaded files */
			if (info.downloadStatus == FT_STATE_COMPLETE) {
				std::string path;
				path = info.path + "/" + info.fname;

				/* open file with a suitable application */
				QFileInfo qinfo;
				qinfo.setFile(QString::fromUtf8(path.c_str()));
				if (qinfo.exists()) {
					if (qinfo.absoluteFilePath().endsWith(RsCollection::ExtensionString)) {
						RsCollection collection;
						if (collection.load(qinfo.absoluteFilePath())) {
							collection.downloadFiles();
							return;
						}
					}
				}
			}
		}
	}

	RsCollection collection;
	if (collection.load(this)) {
		collection.downloadFiles();
	}
}

void TransfersDialog::collAutoOpen(const QString &fileHash)
{
	if (Settings->valueFromGroup("Transfer","AutoDLColl").toBool())
	{
		RsFileHash hash = RsFileHash(fileHash.toStdString());
		FileInfo info;
		if (rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, info)) {

			/* make path for downloaded files */
			if (info.downloadStatus == FT_STATE_COMPLETE) {
				std::string path;
				path = info.path + "/" + info.fname;

				/* open file with a suitable application */
				QFileInfo qinfo;
				qinfo.setFile(QString::fromUtf8(path.c_str()));
				if (qinfo.exists()) {
					if (qinfo.absoluteFilePath().endsWith(RsCollection::ExtensionString)) {
						RsCollection collection;
						if (collection.load(qinfo.absoluteFilePath(), false)) {
							collection.autoDownloadFiles();
						}
					}
				}
			}
		}
	}
}

void TransfersDialog::setShowDLSizeColumn        (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_SIZE,         !show); }
void TransfersDialog::setShowDLCompleteColumn    (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_COMPLETED,    !show); }
void TransfersDialog::setShowDLDLSpeedColumn     (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_DLSPEED,      !show); }
void TransfersDialog::setShowDLProgressColumn    (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_PROGRESS,     !show); }
void TransfersDialog::setShowDLSourcesColumn     (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_SOURCES,      !show); }
void TransfersDialog::setShowDLStatusColumn      (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_STATUS,       !show); }
void TransfersDialog::setShowDLPriorityColumn    (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_PRIORITY,     !show); }
void TransfersDialog::setShowDLRemainingColumn   (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_REMAINING,    !show); }
void TransfersDialog::setShowDLDownloadTimeColumn(bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_DOWNLOADTIME, !show); }
void TransfersDialog::setShowDLIDColumn          (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_ID,           !show); }
void TransfersDialog::setShowDLLastDLColumn      (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_LASTDL,       !show); }
void TransfersDialog::setShowDLPath              (bool show) { ui.downloadList->setColumnHidden(DLListDelegate::COLUMN_PATH,         !show); }

void TransfersDialog::setShowULPeerColumn       (bool show) { ui.uploadsList->setColumnHidden(ULListDelegate::COLUMN_UPEER,        !show); }
void TransfersDialog::setShowULSizeColumn       (bool show) { ui.uploadsList->setColumnHidden(ULListDelegate::COLUMN_USIZE,        !show); }
void TransfersDialog::setShowULTransferredColumn(bool show) { ui.uploadsList->setColumnHidden(ULListDelegate::COLUMN_UTRANSFERRED, !show); }
void TransfersDialog::setShowULSpeedColumn      (bool show) { ui.uploadsList->setColumnHidden(ULListDelegate::COLUMN_ULSPEED,      !show); }
void TransfersDialog::setShowULProgressColumn   (bool show) { ui.uploadsList->setColumnHidden(ULListDelegate::COLUMN_UPROGRESS,    !show); }
void TransfersDialog::setShowULHashColumn       (bool show) { ui.uploadsList->setColumnHidden(ULListDelegate::COLUMN_UHASH,        !show); }

void TransfersDialog::expandAllDL()
{
	ui.downloadList->expandAll();
}
void TransfersDialog::collapseAllDL()
{
	ui.downloadList->collapseAll();
}

void TransfersDialog::expandAllUL()
{
	ui.uploadsList->expandAll();
}
void TransfersDialog::collapseAllUL()
{
	ui.uploadsList->collapseAll();
}

void TransfersDialog::filterChanged(const QString& /*text*/)
{
	int filterColumn = ui.filterLineEdit->currentFilter();
	QString text = ui.filterLineEdit->text();
	DLLFilterModel->setFilterKeyColumn(filterColumn);
	DLLFilterModel->setFilterRegExp(text);
}
