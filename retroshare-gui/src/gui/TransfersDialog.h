/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#ifndef _TRANSFERSDIALOG_H
#define _TRANSFERSDIALOG_H

#include <set>

#include <retroshare/rstypes.h>
#include "RsAutoUpdatePage.h"

#include "ui_TransfersDialog.h"


class DLListDelegate;
class ULListDelegate;
class QStandardItemModel;
class QStandardItem;
class DetailsDialog;
class FileProgressInfo;
class SearchDialog;
class LocalSharedFilesDialog;
class RemoteSharedFilesDialog;

class TransfersDialog : public RsAutoUpdatePage
{
Q_OBJECT

public:
			 enum Page {
						 /* Fixed numbers for load and save the last page */
			 				SearchTab              = 0,  /** Network page. */
							LocalSharedFilesTab    = 1,  /** Network new graph. */
							RemoteSharedFilesTab   = 2   /** Old group chat page. */
		 };


    /** Default Constructor */
    TransfersDialog(QWidget *parent = 0);
    ~TransfersDialog();

    virtual UserNotify *getUserNotify(QObject *parent);

	 void activatePage(TransfersDialog::Page page) ;

    virtual void updateDisplay() ;				// derived from RsAutoUpdateWidget

	 static DetailsDialog *detailsDialog() ;

	 SearchDialog *searchDialog ;
	 LocalSharedFilesDialog *localSharedFiles ;
	 RemoteSharedFilesDialog *remoteSharedFiles ;

public slots:
    void insertTransfers();

//    void handleDownloadRequest(const QString& url);

private slots:

    /** Create the context popup menu and it's submenus */
    void downloadListCostumPopupMenu( QPoint point );

    void cancel();
    void forceCheck();
    /** removes finished Downloads**/
    void clearcompleted();

    void copyLink();
    void pasteLink();

//    void rootdecorated();
//    void rootisnotdecorated();

    void pauseFileTransfer();
    void resumeFileTransfer();
    void openFolderTransfer();
    void openTransfer();
    void previewTransfer();

    /** clear download or all queue - for pending dwls */
//    void clearQueue();

    /** modify download priority actions */
    void priorityQueueUp();
    void priorityQueueDown();
    void priorityQueueTop();
    void priorityQueueBottom();

    void speedSlow();
    void speedAverage();
    void speedFast();

    void changeSpeed(int) ;
    void changeQueuePosition(QueueMove) ;

    void chunkRandom();
    void chunkStreaming();

    void showDetailsDialog();
    void updateDetailsDialog();

    void openCollection();

signals:
    void playFiles(QStringList files);

private:
    QString getPeerName(const std::string& peer_id) const ;

    QStandardItemModel *DLListModel;
    QStandardItemModel *ULListModel;
    QItemSelectionModel *selection;
    QItemSelectionModel *selectionup;

    DLListDelegate *DLDelegate;
    ULListDelegate *ULDelegate;

    /** Create the actions on the tray menu or menubar */
    void createActions();

    /** Defines the actions for the context menu */
    QAction* showdowninfoAct;
    QAction* playAct;
    QAction* cancelAct;
    QAction* forceCheckAct;
    QAction* clearcompletedAct;
    QAction* copylinkAct;
    QAction* pastelinkAct;
    QAction* rootisnotdecoratedAct;
    QAction* rootisdecoratedAct;
    QAction *pauseAct;
    QAction *resumeAct;
    QAction *openfolderAct;
    QAction *openfileAct;
    QAction *previewfileAct;
//    QAction *clearQueuedDwlAct;
//    QAction *clearQueueAct;
    QAction *changePriorityAct;
    QAction *prioritySlowAct;
    QAction *priorityMediumAct;
    QAction *priorityFastAct;
    QAction *queueDownAct;
    QAction *queueUpAct;
    QAction *queueTopAct;
    QAction *queueBottomAct;
    QAction *chunkRandomAct;
    QAction *chunkStreamingAct;
    QAction *detailsfileAct;
    QAction *toggleShowCacheTransfersAct;
    QAction *openCollectionAct;

    bool m_bProcessSettings;
    void processSettings(bool bLoad);

    void getSelectedItems(std::set<std::string> *ids, std::set<int> *rows);
    bool controlTransferFile(uint32_t flags);
    void changePriority(int priority);
    void setChunkStrategy(FileChunksInfo::ChunkStrategy s) ;

    QTreeView *downloadList;

    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, const char *slot = 0);

    /** Qt Designer generated object */
    Ui::TransfersDialog ui;

	 bool _show_cache_transfers ;
public slots:
	// these two functions add entries to the transfers dialog, and return the row id of the entry modified/added
	//
    int addItem(int row, const FileInfo &fileInfo, const std::map<std::string, std::string> &versions);
    int addPeerToItem(QStandardItem *dlItem, const QString& name, const QString& coreID, double dlspeed, uint32_t status, const FileProgressInfo& peerInfo);

    int addUploadItem(const QString& symbol, const QString& name, const QString& coreID, qlonglong size, const FileProgressInfo& pinfo, double dlspeed, const QString& sources,const QString& source_id, const QString& status, qlonglong completed, qlonglong remaining);

    void showFileDetails() ;
	 void toggleShowCacheTransfers() ;

    double getProgress(int row, QStandardItemModel *model);
    double getSpeed(int row, QStandardItemModel *model);
    QString getFileName(int row, QStandardItemModel *model);
    QString getStatus(int row, QStandardItemModel *model);
    QString getID(int row, QStandardItemModel *model);
    QString getPriority(int row, QStandardItemModel *model);
    qlonglong getFileSize(int row, QStandardItemModel *model);
    qlonglong getTransfered(int row, QStandardItemModel *model);
    qlonglong getRemainingTime(int row, QStandardItemModel *model);
    qlonglong getDownloadTime(int row, QStandardItemModel *model);
    QString getSources(int row, QStandardItemModel *model);
};

#endif

