/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/TransfersDialog.h                       *
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

#ifndef _TRANSFERSDIALOG_H
#define _TRANSFERSDIALOG_H

#include <set>

#include <retroshare/rstypes.h>
#include "RsAutoUpdatePage.h"

#include "ui_TransfersDialog.h"

#include <QItemSelectionModel>
#include <QSortFilterProxyModel>

#define IMAGE_TRANSFERS      	":/icons/ktorrent_128.png"

class QShortcut;
class DLListDelegate;
class ULListDelegate;
class QStandardItemModel;
class QStandardItem;
class DetailsDialog;
class FileProgressInfo;
class SearchDialog;
class LocalSharedFilesDialog;
class RemoteSharedFilesDialog;
class RsDownloadListModel;

class TransfersDialog : public RsAutoUpdatePage
{
Q_OBJECT

public:
			 enum Page {
						 /* Fixed numbers for load and save the last page */
			 				SearchTab              = 0,  /** Network page. */
							LocalSharedFilesTab    = 1,  /** Network new graph. */
							RemoteSharedFilesTab   = 2,  /** Old group chat page. */
							DownloadTab            = 3
		 };


    /** Default Constructor */
    TransfersDialog(QWidget *parent = 0);
    ~TransfersDialog();

    virtual QIcon iconPixmap() const { return QIcon(IMAGE_TRANSFERS) ; } //MainPage
    virtual QString pageName() const { return tr("Files") ; } //MainPage
    virtual QString helpText() const { return ""; } //MainPage

    virtual UserNotify *getUserNotify(QObject *parent);

	 void activatePage(TransfersDialog::Page page) ;

    virtual void updateDisplay() ;				// derived from RsAutoUpdateWidget

	 static DetailsDialog *detailsDialog() ;

	 SearchDialog *searchDialog ;
	 LocalSharedFilesDialog *localSharedFiles ;
	 RemoteSharedFilesDialog *remoteSharedFiles ;

    static QString getPeerName(const RsPeerId &peer_id, QString &iconName, QString &tooltip) ;
public slots:
    void insertTransfers();

//    void handleDownloadRequest(const QString& url);

private slots:

    /** Create the context popup menu and it's submenus */
    void downloadListCustomPopupMenu( QPoint point );
    void downloadListHeaderCustomPopupMenu( QPoint point );
    void uploadsListCustomPopupMenu( QPoint point );
	void uploadsListHeaderCustomPopupMenu (QPoint point );

    void cancel();
    void forceCheck();
    /** removes finished Downloads**/
    void clearcompleted();

    void dlCopyLink();
    void pasteLink();
    void renameFile();
    void setDestinationDirectory();
    void chooseDestinationDirectory();

    void expandAllDL();
    void collapseAllDL();
    void expandAllUL();
    void collapseAllUL();

    void pauseFileTransfer();
    void resumeFileTransfer();
    void dlOpenFolder();
    void dlOpenFile();
    void dlPreviewFile();

    void ulOpenFolder();
    void ulCopyLink();
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
    void chunkProgressive();
    void chunkStreaming();

    void showDetailsDialog();
    void updateDetailsDialog();

    void collCreate();
    void collModif();
    void collView();
    void collOpen();
    void collAutoOpen(const QString& fileHash);

    void setShowDLSizeColumn(bool show);
    void setShowDLCompleteColumn(bool show);
    void setShowDLDLSpeedColumn(bool show);
    void setShowDLProgressColumn(bool show);
    void setShowDLSourcesColumn(bool show);
    void setShowDLStatusColumn(bool show);
    void setShowDLPriorityColumn(bool show);
    void setShowDLRemainingColumn(bool show);
    void setShowDLDownloadTimeColumn(bool show);
    void setShowDLIDColumn(bool show);
    void setShowDLLastDLColumn(bool show);
    void setShowDLPath(bool show);

	void setShowULPeerColumn(bool show);
	void setShowULSizeColumn(bool show);
	void setShowULTransferredColumn(bool show);
	void setShowULSpeedColumn(bool show);
	void setShowULProgressColumn(bool show);
	void setShowULHashColumn(bool show);

    void filterChanged(const QString &text);

signals:
    void playFiles(QStringList files);

private:

    RsDownloadListModel *DLListModel;
    QSortFilterProxyModel *DLLFilterModel;
    QStandardItemModel *ULListModel;
    QItemSelectionModel *selection;
    QItemSelectionModel *selectionUp;

    DLListDelegate *DLDelegate;
    ULListDelegate *ULDelegate;

    /** Create the actions on the tray menu or menubar */
    void createActions();

    /** Defines the actions for the context menu */
    QAction *showdownInfoAct;
    QAction *playAct;
    QAction *cancelAct;
    QAction *forceCheckAct;
    QAction *clearCompletedAct;
    QAction *copyLinkAct;
    QAction *pasteLinkAct;
    QAction *rootIsNotDecoratedAct;
    QAction *rootIsDecoratedAct;
    QAction *pauseAct;
    QAction *resumeAct;
    QAction *openFolderAct;
    QAction *openFileAct;
    QAction *previewFileAct;
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
    QAction *chunkProgressiveAct;
    QAction *chunkStreamingAct;
    QAction *detailsFileAct;
    QAction *renameFileAct;
    QAction *specifyDestinationDirectoryAct;
    QAction *expandAllDLAct;
    QAction *collapseAllDLAct;
    QAction *expandAllULAct;
    QAction *collapseAllULAct;
    QAction *collCreateAct;
    QAction *collModifAct;
    QAction *collViewAct;
    QAction *collOpenAct;

	/** Defines the actions for the header context menu in download */
    QAction* showDLSizeAct;
    QAction* showDLCompleteAct;
    QAction* showDLDLSpeedAct;
    QAction* showDLProgressAct;
    QAction* showDLSourcesAct;
    QAction* showDLStatusAct;
    QAction* showDLPriorityAct;
    QAction* showDLRemainingAct;
    QAction* showDLDownloadTimeAct;
    QAction* showDLIDAct;
    QAction* showDLLastDLAct;
    QAction* showDLPath;

    /** Defines the actions for the upload context menu */
    QAction* ulOpenFolderAct;
    QAction* ulCopyLinkAct;

	/** Defines the actions for the header context menu in upload*/
	QAction* showULPeerAct;
	QAction* showULSizeAct;
	QAction* showULTransferredAct;
	QAction* showULSpeedAct;
	QAction* showULProgressAct;
	QAction* showULHashAct;

    bool m_bProcessSettings;
    void processSettings(bool bLoad);

    void getDLSelectedItems(std::set<RsFileHash> *ids, std::set<int> *rows);
    void getULSelectedItems(std::set<RsFileHash> *ids, std::set<int> *rows);
    bool controlTransferFile(uint32_t flags);
    void changePriority(int priority);
    void setChunkStrategy(FileChunksInfo::ChunkStrategy s) ;

    QTreeView *downloadList;

    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, const char *slot = 0);
    
      QString downloads;
      QString uploads;

    QShortcut *mShortcut ;

    /** Qt Designer generated object */
    Ui::TransfersDialog ui;

public slots:
    // these four functions add entries to the transfers dialog, and return the row id of the entry modified/added
//    int addDLItem(int row, const FileInfo &fileInfo);
//    int addPeerToDLItem(QStandardItem* dlItem, const RsPeerId &peer_ID, const QString &coreID, double dlspeed, uint32_t status, const FileProgressInfo &peerInfo);
    int addULItem(int row, const FileInfo &fileInfo);
    int addPeerToULItem(QStandardItem* ulItem, const RsPeerId &peer_ID, const QString &coreID, qlonglong completed, double ulspeed, const FileProgressInfo &peerInfo);

    void showFileDetails() ;

    double getProgress(int row, QStandardItemModel *model);
    double getSpeed(int row, QStandardItemModel *model);
    QString getFileName(int row, QStandardItemModel *model);
    QString getStatus(int row, QStandardItemModel *model);
    QString getID(int row, QStandardItemModel *model);
    QString getID(int row, QSortFilterProxyModel *filter);
    QString getPriority(int row, QStandardItemModel *model);
    qlonglong getFileSize(int row, QStandardItemModel *model);
    qlonglong getTransfered(int row, QStandardItemModel *model);
    qlonglong getRemainingTime(int row, QStandardItemModel *model);
    qlonglong getDownloadTime(int row, QStandardItemModel *model);
    qlonglong getLastDL(int row, QStandardItemModel *model);
    qlonglong getPath(int row, QStandardItemModel *model);
    QString getSources(int row, QStandardItemModel *model);
};

#endif

