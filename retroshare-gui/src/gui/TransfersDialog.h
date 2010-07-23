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

#include "rsiface/rstypes.h"
#include "RsAutoUpdatePage.h"

#include "ui_TransfersDialog.h"


class DLListDelegate;
class ULListDelegate;
class QStandardItemModel;
class QStandardItem;
class DetailsDialog;
class FileProgressInfo;

class TransfersDialog : public RsAutoUpdatePage
{
Q_OBJECT

public:
    /** Default Constructor */
    TransfersDialog(QWidget *parent = 0);
    /** Default Destructor */
    ~TransfersDialog();

// replaced by shortcut
//    virtual void keyPressEvent(QKeyEvent *) ;
    virtual void updateDisplay() ;				// derived from RsAutoUpdateWidget

    static DetailsDialog *detailsdlg;

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

    /** save sort indicators for next transfers display */
//    void saveSortIndicatorDwl(int logicalIndex, Qt::SortOrder order);
//    void saveSortIndicatorUpl(int logicalIndex, Qt::SortOrder order);

    void showDetailsDialog();
    void updateDetailsDialog();

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

    //		int _sortColDwl, _sortColUpl;
    //		Qt::SortOrder _sortOrderDwl, _sortOrderUpl;

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

    void getIdOfSelectedItems(std::set<QStandardItem *>& items);
    bool controlTransferFile(uint32_t flags);
    void changePriority(int priority);
    void setChunkStrategy(FileChunksInfo::ChunkStrategy s) ;

    QTreeView *downloadList;

    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, const char *slot = 0);

    /** Qt Designer generated object */
    Ui::TransfersDialog ui;

public slots:
	// these two functions add entries to the transfers dialog, and return the row id of the entry modified/added
	//
    int addItem(const QString& symbol, const QString& name, const QString& coreID, qlonglong size, const FileProgressInfo& pinfo, double dlspeed, const QString& sources, const QString& status, const QString& priority, qlonglong completed, qlonglong remaining, qlonglong downloadtime);
    int addPeerToItem(int row, const QString& name, const QString& coreID, double dlspeed, uint32_t status, const FileProgressInfo& peerInfo);
    void delItem(int row);

    int addUploadItem(const QString& symbol, const QString& name, const QString& coreID, qlonglong size, const FileProgressInfo& pinfo, double dlspeed, const QString& sources, const QString& status, qlonglong completed, qlonglong remaining);
    void delUploadItem(int row);

    void showFileDetails() ;

    double getProgress(int row, QStandardItemModel *model);
    double getSpeed(int row, QStandardItemModel *model);
    QString getFileName(int row, QStandardItemModel *model);
    QString getStatus(int row, QStandardItemModel *model);
    QString getID(int row, QStandardItemModel *model);
    QString getPriority(int row, QStandardItemModel *model);
    qlonglong getFileSize(int row, QStandardItemModel *model);
    qlonglong getTransfered(int row, QStandardItemModel *model);
    qlonglong getRemainingTime(int row, QStandardItemModel *model);
};

#endif

