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

#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QTreeView>
#include <QShortcut>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <gui/common/RsUrlHandler.h>
#include <gui/common/RsCollectionFile.h>
#include <gui/common/FilesDefs.h>

#include <algorithm>
#include <limits>
#include <math.h>

#include "TransfersDialog.h"
#include <gui/RetroShareLink.h>
#include "DetailsDialog.h"
#include "DLListDelegate.h"
#include "ULListDelegate.h"
#include "FileTransferInfoWidget.h"
#include <gui/FileTransfer/SearchDialog.h>
#include <gui/SharedFilesDialog.h>
#include "xprogressbar.h"
#include <gui/settings/rsharesettings.h>
#include "util/misc.h"
#include <gui/common/RsCollectionFile.h>
#include "TransferUserNotify.h"
#include "util/QtVersion.h"
#include "util/RsFile.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsplugin.h>

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
#define IMAGE_PRIORITYLOW			     ":/images/prioritylow.png"
#define IMAGE_PRIORITYNORMAL			 ":/images/prioritynormal.png"
#define IMAGE_PRIORITYHIGH			   ":/images/priorityhigh.png"
#define IMAGE_PRIORITYAUTO			   ":/images/priorityauto.png"
#define IMAGE_SEARCH               ":/images/filefind.png"
#define IMAGE_EXPAND               ":/images/edit_add24.png"
#define IMAGE_COLLAPSE             ":/images/edit_remove24.png"
#define IMAGE_LIBRARY              ":/images/library.png"
#define IMAGE_COLLCREATE           ":/images/library_add.png"
#define IMAGE_COLLMODIF            ":/images/library_edit.png"
#define IMAGE_COLLVIEW             ":/images/library_view.png"
#define IMAGE_COLLOPEN             ":/images/library.png"
#define IMAGE_FRIENDSFILES         ":/images/fileshare16.png"
#define IMAGE_MYFILES              ":images/my_documents_16.png"
#define IMAGE_RENAMEFILE           ":images/filecomments.png"
#define IMAGE_STREAMING             ":images/streaming.png"

Q_DECLARE_METATYPE(FileProgressInfo) 

class SortByNameItem : public QStandardItem
{
public:
	SortByNameItem(QHeaderView *header) : QStandardItem()
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

        QStandardItem *myName = myParent->child(index().row(), COLUMN_NAME);
        QStandardItem *otherName = otherParent->child(other.index().row(), COLUMN_NAME);

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
	ProgressItem(QHeaderView *header) : SortByNameItem(header) {}

	virtual bool operator<(const QStandardItem &other) const
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

class PriorityItem : public SortByNameItem
{
	public:
		PriorityItem(QHeaderView *header) : SortByNameItem(header) {}

		virtual bool operator<(const QStandardItem &other) const
		{
			const int role = model() ? model()->sortRole() : Qt::DisplayRole;

			QString l = data(role).value<QString>();
			QString r = other.data(role).value<QString>();

			bool bl,br ;
			int nl = l.toInt(&bl) ;
			int nr = r.toInt(&br) ;

			if(bl && br)
				return nl < nr ;

			if(bl ^ br)
				return br ;

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

    // Set Download list model
    DLListModel = new QStandardItemModel(0,COLUMN_COUNT);
    DLListModel->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name", "i.e: file name"));
    DLListModel->setHeaderData(COLUMN_SIZE, Qt::Horizontal, tr("Size", "i.e: file size"));
    DLListModel->setHeaderData(COLUMN_COMPLETED, Qt::Horizontal, tr("Completed", ""));
    DLListModel->setHeaderData(COLUMN_DLSPEED, Qt::Horizontal, tr("Speed", "i.e: Download speed"));
    DLListModel->setHeaderData(COLUMN_PROGRESS, Qt::Horizontal, tr("Progress / Availability", "i.e: % downloaded"));
    DLListModel->setHeaderData(COLUMN_SOURCES, Qt::Horizontal, tr("Sources", "i.e: Sources"));
    DLListModel->setHeaderData(COLUMN_STATUS, Qt::Horizontal, tr("Status"));
    DLListModel->setHeaderData(COLUMN_PRIORITY, Qt::Horizontal, tr("Speed / Queue position"));
    DLListModel->setHeaderData(COLUMN_REMAINING, Qt::Horizontal, tr("Remaining"));
    DLListModel->setHeaderData(COLUMN_DOWNLOADTIME, Qt::Horizontal, tr("Download time", "i.e: Estimated Time of Arrival / Time left"));
    DLListModel->setHeaderData(COLUMN_ID, Qt::Horizontal, tr("Hash"));
    DLListModel->setHeaderData(COLUMN_LASTDL, Qt::Horizontal, tr("Last Time Seen", "i.e: Last Time Receiced Data"));
    DLListModel->setHeaderData(COLUMN_PATH, Qt::Horizontal, tr("Path", "i.e: Where file is saved"));
    ui.downloadList->setModel(DLListModel);
    //ui.downloadList->hideColumn(ID);
    DLDelegate = new DLListDelegate();
    ui.downloadList->setItemDelegate(DLDelegate);

    QHeaderView *qhvDLList = ui.downloadList->header();
    qhvDLList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(qhvDLList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(downloadListHeaderCustomPopupMenu(QPoint)));

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


    /* Set header resize modes and initial section sizes Downloads TreeView*/
    QHeaderView * _header = ui.downloadList->header () ;
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_NAME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_SIZE, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_COMPLETED, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_DLSPEED, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PROGRESS, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_SOURCES, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_STATUS, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PRIORITY, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_REMAINING, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_DOWNLOADTIME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_ID, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_LASTDL, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_header, COLUMN_PATH, QHeaderView::Interactive);

    _header->resizeSection ( COLUMN_NAME, 170 );
    _header->resizeSection ( COLUMN_SIZE, 70 );
    _header->resizeSection ( COLUMN_COMPLETED, 75 );
    _header->resizeSection ( COLUMN_DLSPEED, 75 );
    _header->resizeSection ( COLUMN_PROGRESS, 170 );
    _header->resizeSection ( COLUMN_SOURCES, 90 );
    _header->resizeSection ( COLUMN_STATUS, 100 );
    _header->resizeSection ( COLUMN_PRIORITY, 100 );
    _header->resizeSection ( COLUMN_REMAINING, 100 );
    _header->resizeSection ( COLUMN_DOWNLOADTIME, 100 );
    _header->resizeSection ( COLUMN_ID, 100 );
    _header->resizeSection ( COLUMN_LASTDL, 100 );
    _header->resizeSection ( COLUMN_PATH, 100 );

    // set default column and sort order for download
    ui.downloadList->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);
    
    connect( ui.uploadsList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( uploadsListCustomPopupMenu( QPoint ) ) );

    // Set Upload list model
    ULListModel = new QStandardItemModel(0,COLUMN_UCOUNT);
    ULListModel->setHeaderData(COLUMN_UNAME, Qt::Horizontal, tr("Name", "i.e: file name"));
    ULListModel->setHeaderData(COLUMN_USIZE, Qt::Horizontal, tr("Size", "i.e: file size"));
    ULListModel->setHeaderData(COLUMN_USERNAME, Qt::Horizontal, tr("Peer", "i.e: user name"));
    ULListModel->setHeaderData(COLUMN_UPROGRESS, Qt::Horizontal, tr("Progress", "i.e: % uploaded"));
    ULListModel->setHeaderData(COLUMN_ULSPEED, Qt::Horizontal, tr("Speed", "i.e: upload speed"));
    ULListModel->setHeaderData(COLUMN_USTATUS, Qt::Horizontal, tr("Status"));
    ULListModel->setHeaderData(COLUMN_UTRANSFERRED, Qt::Horizontal, tr("Transferred", ""));
    ULListModel->setHeaderData(COLUMN_UHASH, Qt::Horizontal, tr("Hash", ""));
    ULListModel->setHeaderData(COLUMN_UUSERID, Qt::Horizontal, tr("UserID", ""));
    ui.uploadsList->setModel(ULListModel);
    //ULListModel->insertColumn(COLUMN_UUSERID);
    //ui.uploadsList->hideColumn(COLUMN_UUSERID);
    ULDelegate = new ULListDelegate();
    ui.uploadsList->setItemDelegate(ULDelegate);

// Why disable autoscroll ?
// With disabled autoscroll, the treeview doesn't scroll with cursor move
//    ui.uploadsList->setAutoScroll(false) ;

    ui.uploadsList->setRootIsDecorated(false);


  	//Selection Setup
    selectionUp = ui.uploadsList->selectionModel();
    ui.uploadsList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    /* Set header resize modes and initial section sizes Uploads TreeView*/
    QHeaderView * upheader = ui.uploadsList->header () ;
    QHeaderView_setSectionResizeModeColumn(upheader, COLUMN_UNAME, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, COLUMN_USIZE, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, COLUMN_UTRANSFERRED, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, COLUMN_ULSPEED, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, COLUMN_UPROGRESS, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, COLUMN_USTATUS, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(upheader, COLUMN_USERNAME, QHeaderView::Interactive);

    upheader->resizeSection ( COLUMN_UNAME, 260 );
    upheader->resizeSection ( COLUMN_USIZE, 70 );
    upheader->resizeSection ( COLUMN_UTRANSFERRED, 75 );
    upheader->resizeSection ( COLUMN_ULSPEED, 75 );
    upheader->resizeSection ( COLUMN_UPROGRESS, 170 );
    upheader->resizeSection ( COLUMN_USTATUS, 100 );
    upheader->resizeSection ( COLUMN_USERNAME, 120 );

    // set default column and sort order for upload
    ui.uploadsList->sortByColumn(COLUMN_UNAME, Qt::AscendingOrder);
	
    // FileTransferInfoWidget *ftiw = new FileTransferInfoWidget();
    // ui.fileTransferInfoWidget->setWidget(ftiw);
    // ui.fileTransferInfoWidget->setWidgetResizable(true);
    // ui.fileTransferInfoWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // ui.fileTransferInfoWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    // ui.fileTransferInfoWidget->viewport()->setBackgroundRole(QPalette::NoRole);
    // ui.fileTransferInfoWidget->setFrameStyle(QFrame::NoFrame);
    // ui.fileTransferInfoWidget->setFocusPolicy(Qt::NoFocus);

    QObject::connect(ui.downloadList->selectionModel(),SIGNAL(selectionChanged (const QItemSelection&, const QItemSelection&)),this,SLOT(showFileDetails())) ;

	 ui.tabWidget->insertTab(2,searchDialog = new SearchDialog(), QIcon(IMAGE_SEARCH), tr("Search")) ;
	 ui.tabWidget->insertTab(3,remoteSharedFiles = new RemoteSharedFilesDialog(), QIcon(IMAGE_FRIENDSFILES), tr("Friends files")) ;

	 ui.tabWidget->addTab(localSharedFiles = new LocalSharedFilesDialog(), QIcon(IMAGE_MYFILES), tr("My files")) ;

	 //ui.tabWidget->addTab( new TurtleRouterStatistics(), tr("Router Statistics")) ;
	 //ui.tabWidget->addTab( new TurtleRouterDialog(), tr("Router Requests")) ;

	 for(int i=0;i<rsPlugins->nbPlugins();++i)
		 if(rsPlugins->plugin(i) != NULL && rsPlugins->plugin(i)->qt_transfers_tab() != NULL)
			 ui.tabWidget->addTab( rsPlugins->plugin(i)->qt_transfers_tab(),QString::fromUtf8(rsPlugins->plugin(i)->qt_transfers_tab_name().c_str()) ) ;

	 ui.tabWidget->setCurrentWidget(ui.uploadsTab);

//    TurtleRouterDialog *trdl = new TurtleRouterDialog();
//    ui.tunnelInfoWidget->setWidget(trdl);
//    ui.tunnelInfoWidget->setWidgetResizable(true);
//    ui.tunnelInfoWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    ui.tunnelInfoWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
//    ui.tunnelInfoWidget->viewport()->setBackgroundRole(QPalette::NoRole);
//    ui.tunnelInfoWidget->setFrameStyle(QFrame::NoFrame);
//    ui.tunnelInfoWidget->setFocusPolicy(Qt::NoFocus);

    /** Setup the actions for the context menu */
   toggleShowCacheTransfersAct = new QAction(tr( "Show file list transfers" ), this );
	toggleShowCacheTransfersAct->setCheckable(true) ;
	connect(toggleShowCacheTransfersAct,SIGNAL(triggered()),this,SLOT(toggleShowCacheTransfers())) ;

	// Actions. Only need to be defined once.
   pauseAct = new QAction(QIcon(IMAGE_PAUSE), tr("Pause"), this);
   connect(pauseAct, SIGNAL(triggered()), this, SLOT(pauseFileTransfer()));

   resumeAct = new QAction(QIcon(IMAGE_RESUME), tr("Resume"), this);
   connect(resumeAct, SIGNAL(triggered()), this, SLOT(resumeFileTransfer()));

//#ifdef USE_NEW_CHUNK_CHECKING_CODE
	// *********WARNING**********
	// csoler: this has been suspended because it needs the file transfer to consider a file as complete only if all chunks are
	// 			verified by hash. As users are goign to slowly switch to new checking code, this will not be readily available.
	//
   forceCheckAct = new QAction(QIcon(IMAGE_CANCEL), tr( "Force Check" ), this );
   connect( forceCheckAct , SIGNAL( triggered() ), this, SLOT( forceCheck() ) );
//#endif

   cancelAct = new QAction(QIcon(IMAGE_CANCEL), tr( "Cancel" ), this );
   connect( cancelAct , SIGNAL( triggered() ), this, SLOT( cancel() ) );

    openFolderAct = new QAction(QIcon(IMAGE_OPENFOLDER), tr("Open Folder"), this);
    connect(openFolderAct, SIGNAL(triggered()), this, SLOT(openFolderTransfer()));

    openFileAct = new QAction(QIcon(IMAGE_OPENFILE), tr("Open File"), this);
    connect(openFileAct, SIGNAL(triggered()), this, SLOT(openTransfer()));

    previewFileAct = new QAction(QIcon(IMAGE_PREVIEW), tr("Preview File"), this);
    connect(previewFileAct, SIGNAL(triggered()), this, SLOT(previewTransfer()));

    detailsFileAct = new QAction(QIcon(IMAGE_INFO), tr("Details..."), this);
    connect(detailsFileAct, SIGNAL(triggered()), this, SLOT(showDetailsDialog()));

    clearCompletedAct = new QAction(QIcon(IMAGE_CLEARCOMPLETED), tr( "Clear Completed" ), this );
    connect( clearCompletedAct , SIGNAL( triggered() ), this, SLOT( clearcompleted() ) );


    copyLinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy RetroShare Link" ), this );
    connect( copyLinkAct , SIGNAL( triggered() ), this, SLOT( copyLink() ) );
    pasteLinkAct = new QAction(QIcon(IMAGE_PASTELINK), tr( "Paste RetroShare Link" ), this );
    connect( pasteLinkAct , SIGNAL( triggered() ), this, SLOT( pasteLink() ) );
	queueDownAct = new QAction(QIcon(":/images/go-down.png"), tr("Down"), this);
	connect(queueDownAct, SIGNAL(triggered()), this, SLOT(priorityQueueDown()));
	queueUpAct = new QAction(QIcon(":/images/go-up.png"), tr("Up"), this);
	connect(queueUpAct, SIGNAL(triggered()), this, SLOT(priorityQueueUp()));
	queueTopAct = new QAction(QIcon(":/images/go-top.png"), tr("Top"), this);
	connect(queueTopAct, SIGNAL(triggered()), this, SLOT(priorityQueueTop()));
	queueBottomAct = new QAction(QIcon(":/images/go-bottom.png"), tr("Bottom"), this);
	connect(queueBottomAct, SIGNAL(triggered()), this, SLOT(priorityQueueBottom()));
	chunkStreamingAct = new QAction(QIcon(IMAGE_STREAMING), tr("Streaming"), this);
	connect(chunkStreamingAct, SIGNAL(triggered()), this, SLOT(chunkStreaming()));
	prioritySlowAct = new QAction(QIcon(IMAGE_PRIORITYLOW), tr("Slower"), this);
	connect(prioritySlowAct, SIGNAL(triggered()), this, SLOT(speedSlow()));
	priorityMediumAct = new QAction(QIcon(IMAGE_PRIORITYNORMAL), tr("Average"), this);
	connect(priorityMediumAct, SIGNAL(triggered()), this, SLOT(speedAverage()));
	priorityFastAct = new QAction(QIcon(IMAGE_PRIORITYHIGH), tr("Faster"), this);
	connect(priorityFastAct, SIGNAL(triggered()), this, SLOT(speedFast()));
	chunkRandomAct = new QAction(QIcon(IMAGE_PRIORITYAUTO), tr("Random"), this);
	connect(chunkRandomAct, SIGNAL(triggered()), this, SLOT(chunkRandom()));
	chunkProgressiveAct = new QAction(QIcon(IMAGE_PRIORITYAUTO), tr("Progressive"), this);
	connect(chunkProgressiveAct, SIGNAL(triggered()), this, SLOT(chunkProgressive()));
	playAct = new QAction(QIcon(IMAGE_PLAY), tr( "Play" ), this );
	connect( playAct , SIGNAL( triggered() ), this, SLOT( openTransfer() ) );
		renameFileAct = new QAction(QIcon(IMAGE_RENAMEFILE), tr("Rename file..."), this);
		connect(renameFileAct, SIGNAL(triggered()), this, SLOT(renameFile()));
		specifyDestinationDirectoryAct = new QAction(QIcon(IMAGE_SEARCH),tr("Specify..."),this) ;
		connect(specifyDestinationDirectoryAct,SIGNAL(triggered()),this,SLOT(chooseDestinationDirectory()));
		expandAllAct= new QAction(QIcon(IMAGE_EXPAND),tr("Expand all"),this);
		connect(expandAllAct,SIGNAL(triggered()),this,SLOT(expandAll()));
		collapseAllAct= new QAction(QIcon(IMAGE_COLLAPSE),tr("Collapse all"),this);
		connect(collapseAllAct,SIGNAL(triggered()),this,SLOT(collapseAll()));
		collCreateAct= new QAction(QIcon(IMAGE_COLLCREATE), tr("Create Collection..."), this);
		connect(collCreateAct,SIGNAL(triggered()),this,SLOT(collCreate()));
		collModifAct= new QAction(QIcon(IMAGE_COLLMODIF), tr("Modify Collection..."), this);
		connect(collModifAct,SIGNAL(triggered()),this,SLOT(collModif()));
		collViewAct= new QAction(QIcon(IMAGE_COLLVIEW), tr("View Collection..."), this);
		connect(collViewAct,SIGNAL(triggered()),this,SLOT(collView()));
		collOpenAct = new QAction(QIcon(IMAGE_COLLOPEN), tr( "Download from collection file..." ), this );
		connect(collOpenAct, SIGNAL(triggered()), this, SLOT(collOpen()));

    /** Setup the actions for the header context menu */
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

    /** Setup the actions for the upload context menu */
    ulOpenFolderAct = new QAction(QIcon(IMAGE_OPENFOLDER), tr("Open Folder"), this);
    connect(ulOpenFolderAct, SIGNAL(triggered()), this, SLOT(ulOpenFolder()));
    ulCopyLinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy RetroShare Link" ), this );
    connect( ulCopyLinkAct , SIGNAL( triggered() ), this, SLOT( ulCopyLink() ) );

    // load settings
    processSettings(true);

    int S = QFontMetricsF(font()).height();
  QString help_str = tr(
    " <h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;File Transfer</h1>                                                         \
    <p>Retroshare brings two ways of transferring files: direct transfers from your friends, and                                     \
    distant anonymous tunnelled transfers. In addition, file transfer is multi-source and allows swarming                                      \
    (you can be a source while downloading)</p>                                     \
    <p>You can share files using the <img src=\":/images/directoryadd_24x24_shadow.png\" width=%2 /> icon from the left side bar. \
    These files will be listed in the My Files tab. You can decide for each friend group whether they can or not see these files \
    in their Friends Files tab</p>\
    <p>The search tab reports files from your friends' file lists, and distant files that can be reached \
    anonymously using the multi-hop tunnelling system.</p> \
    ").arg(QString::number(2*S)).arg(QString::number(S)) ;


	 registerHelpButton(ui.helpButton,help_str) ;
}

TransfersDialog::~TransfersDialog()
{
    // save settings
    processSettings(false);
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
	}
}

UserNotify *TransfersDialog::getUserNotify(QObject *parent)
{
    return new TransferUserNotify(parent);
}

void TransfersDialog::toggleShowCacheTransfers()
{
	_show_cache_transfers = !_show_cache_transfers ;
	insertTransfers() ;
}

void TransfersDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    QHeaderView *DLHeader = ui.downloadList->header () ;
    QHeaderView *ULHeader = ui.uploadsList->header () ;

    Settings->beginGroup(QString("TransfersDialog"));

    if (bLoad) {
        // load settings

        // state of checks
        _show_cache_transfers = Settings->value("showCacheTransfers", false).toBool();

        // state of the lists
        DLHeader->restoreState(Settings->value("downloadList").toByteArray());
        ULHeader->restoreState(Settings->value("uploadList").toByteArray());

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());

        setShowDLSizeColumn(Settings->value("showDLSizeColumn", !ui.downloadList->isColumnHidden(COLUMN_SIZE)).toBool());
        setShowDLCompleteColumn(Settings->value("showDLCompleteColumn", !ui.downloadList->isColumnHidden(COLUMN_COMPLETED)).toBool());
        setShowDLDLSpeedColumn(Settings->value("showDLDLSpeedColumn", !ui.downloadList->isColumnHidden(COLUMN_DLSPEED)).toBool());
        setShowDLProgressColumn(Settings->value("showDLProgressColumn", !ui.downloadList->isColumnHidden(COLUMN_PROGRESS)).toBool());
        setShowDLSourcesColumn(Settings->value("showDLSourcesColumn", !ui.downloadList->isColumnHidden(COLUMN_SOURCES)).toBool());
        setShowDLStatusColumn(Settings->value("showDLStatusColumn", !ui.downloadList->isColumnHidden(COLUMN_STATUS)).toBool());
        setShowDLPriorityColumn(Settings->value("showDLPriorityColumn", !ui.downloadList->isColumnHidden(COLUMN_PRIORITY)).toBool());
        setShowDLRemainingColumn(Settings->value("showDLRemainingColumn", !ui.downloadList->isColumnHidden(COLUMN_REMAINING)).toBool());
        setShowDLDownloadTimeColumn(Settings->value("showDLDownloadTimeColumn", !ui.downloadList->isColumnHidden(COLUMN_DOWNLOADTIME)).toBool());
        setShowDLIDColumn(Settings->value("showDLIDColumn", !ui.downloadList->isColumnHidden(COLUMN_ID)).toBool());
        setShowDLLastDLColumn(Settings->value("showDLLastDLColumn", !ui.downloadList->isColumnHidden(COLUMN_LASTDL)).toBool());
        setShowDLPath(Settings->value("showDLPath", !ui.downloadList->isColumnHidden(COLUMN_PATH)).toBool());

        // selected tab
        ui.tabWidget->setCurrentIndex(Settings->value("selectedTab").toInt());
    } else {
        // save settings

        // state of checks
        Settings->setValue("showCacheTransfers", _show_cache_transfers);

        // state of the lists
        Settings->setValue("downloadList", DLHeader->saveState());
        Settings->setValue("uploadList", ULHeader->saveState());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());

        Settings->setValue("showDLSizeColumn", !ui.downloadList->isColumnHidden(COLUMN_SIZE));
        Settings->setValue("showDLCompleteColumn", !ui.downloadList->isColumnHidden(COLUMN_COMPLETED));
        Settings->setValue("showDLDLSpeedColumn", !ui.downloadList->isColumnHidden(COLUMN_DLSPEED));
        Settings->setValue("showDLProgressColumn", !ui.downloadList->isColumnHidden(COLUMN_PROGRESS));
        Settings->setValue("showDLSourcesColumn", !ui.downloadList->isColumnHidden(COLUMN_SOURCES));
        Settings->setValue("showDLStatusColumn", !ui.downloadList->isColumnHidden(COLUMN_STATUS));
        Settings->setValue("showDLPriorityColumn", !ui.downloadList->isColumnHidden(COLUMN_PRIORITY));
        Settings->setValue("showDLRemainingColumn", !ui.downloadList->isColumnHidden(COLUMN_REMAINING));
        Settings->setValue("showDLDownloadTimeColumn", !ui.downloadList->isColumnHidden(COLUMN_DOWNLOADTIME));
        Settings->setValue("showDLIDColumn", !ui.downloadList->isColumnHidden(COLUMN_ID));
        Settings->setValue("showDLLastDLColumn", !ui.downloadList->isColumnHidden(COLUMN_LASTDL));
        Settings->setValue("showDLPath", !ui.downloadList->isColumnHidden(COLUMN_PATH));

        // selected tab
        Settings->setValue("selectedTab", ui.tabWidget->currentIndex());
    }

    Settings->endGroup();
    m_bProcessSettings = false;
}

// replaced by shortcut
//void TransfersDialog::keyPressEvent(QKeyEvent *e)
//{
//	if(e->key() == Qt::Key_Delete)
//	{
//		cancel() ;
//		e->accept() ;
//	}
//	else
//		RsAutoUpdatePage::keyPressEvent(e) ;
//}

void TransfersDialog::downloadListCustomPopupMenu( QPoint /*point*/ )
{
	std::set<RsFileHash> items ;
	getSelectedItems(&items, NULL) ;

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
	priorityQueueMenu.setIcon(QIcon(IMAGE_PRIORITY));
	priorityQueueMenu.addAction(queueTopAct);
	priorityQueueMenu.addAction(queueUpAct);
	priorityQueueMenu.addAction(queueDownAct);
	priorityQueueMenu.addAction(queueBottomAct);

	QMenu prioritySpeedMenu(tr("Priority (Speed)..."), this);
	prioritySpeedMenu.setIcon(QIcon(IMAGE_PRIORITY));
	prioritySpeedMenu.addAction(prioritySlowAct);
	prioritySpeedMenu.addAction(priorityMediumAct);
	prioritySpeedMenu.addAction(priorityFastAct);

	QMenu chunkMenu(tr("Chunk strategy"), this);
	chunkMenu.setIcon(QIcon(IMAGE_PRIORITY));
	chunkMenu.addAction(chunkStreamingAct);
	chunkMenu.addAction(chunkProgressiveAct);
	chunkMenu.addAction(chunkRandomAct);

	QMenu collectionMenu(tr("Collection"), this);
	collectionMenu.setIcon(QIcon(IMAGE_LIBRARY));
	collectionMenu.addAction(collCreateAct);
	collectionMenu.addAction(collModifAct);
	collectionMenu.addAction(collViewAct);
	collectionMenu.addAction(collOpenAct);

	QMenu contextMnu( this );

	if(!RSLinkClipboard::empty(RetroShareLink::TYPE_FILE)) add_PasteLink=true;

	if(!items.empty())
	{
		add_CopyLink = true ;

		QModelIndexList lst = ui.downloadList->selectionModel ()->selectedIndexes ();

		//Look for all selected items
		for (int i = 0; i < lst.count(); ++i) {
			//Look only for first column == File  List
			if ( lst[i].column() == 0) {
				//Get Info for current  item
				if (rsFiles->FileDetails(RsFileHash(getID(lst[i].row(), DLListModel).toStdString())
				                         , RS_FILE_HINTS_DOWNLOAD, info)) {
					/*const uint32_t FT_STATE_FAILED        = 0x0000;
					 *const uint32_t FT_STATE_OKAY          = 0x0001;
					 *const uint32_t FT_STATE_WAITING       = 0x0002;
					 *const uint32_t FT_STATE_DOWNLOADING   = 0x0003;
					 *const uint32_t FT_STATE_COMPLETE      = 0x0004;
					 *const uint32_t FT_STATE_QUEUED        = 0x0005;
					 *const uint32_t FT_STATE_PAUSED        = 0x0006;
					 *const uint32_t FT_STATE_CHECKING_HASH = 0x0007;
					 */
					if (info.downloadStatus == FT_STATE_WAITING) {
						atLeastOne_Waiting = true ;
					}//if (info.downloadStatus == FT_STATE_WAITING)
					if (info.downloadStatus == FT_STATE_DOWNLOADING) {
						atLeastOne_Downloading=true ;
					}//if (info.downloadStatus == FT_STATE_DOWNLOADING)
					if (info.downloadStatus == FT_STATE_COMPLETE) {
						atLeastOne_Complete = true ;
						add_OpenFileOption = single ;
					}//if (info.downloadStatus == FT_STATE_COMPLETE)
					if (info.downloadStatus == FT_STATE_QUEUED) {
						atLeastOne_Queued = true ;
					}//if(info.downloadStatus == FT_STATE_QUEUED)
					if (info.downloadStatus == FT_STATE_PAUSED) {
						atLeastOne_Paused = true ;
					}//if (info.downloadStatus == FT_STATE_PAUSED)

					size_t pos = info.fname.find_last_of('.') ;
					if (pos !=  std::string::npos) {
						// Check if the file is a media file
						if (misc::isPreviewable(info.fname.substr(pos + 1).c_str())) {
							add_PreviewOption = (info.downloadStatus != FT_STATE_COMPLETE) ;
							add_PlayOption = !add_PreviewOption ;
						}// if (misc::isPreviewable(info.fname.substr(pos + 1).c_str()))
						// Check if the file is a collection
						if (RsCollectionFile::ExtensionString == info.fname.substr(pos + 1).c_str()) {
							add_CollActions = (info.downloadStatus == FT_STATE_COMPLETE);
						}//if (RsCollectionFile::ExtensionString == info
					}// if(pos !=  std::string::npos)

				}// if (rsFiles->FileDetails(lst[i].data(COLUMN_ID), RS_FILE_HINTS_DOWNLOAD, info))
			}// if (lst[i].column() == 0)
		}// for (int i = 0; i < lst.count(); ++i)
	}// if (!items.empty())

	if (atLeastOne_Waiting || atLeastOne_Downloading || atLeastOne_Queued || atLeastOne_Paused) {
		contextMnu.addMenu( &prioritySpeedMenu) ;
	}
	if (atLeastOne_Queued) {
		contextMnu.addMenu( &priorityQueueMenu) ;
	}//if (atLeastOne_Queued)

	if ( (!items.empty())
	     && (atLeastOne_Downloading || atLeastOne_Queued || atLeastOne_Waiting || atLeastOne_Paused)) {
		contextMnu.addMenu(&chunkMenu) ;

		if (single) {
			contextMnu.addAction( renameFileAct) ;
		}//if (single)

		QMenu *directoryMenu = contextMnu.addMenu(QIcon(IMAGE_OPENFOLDER), tr("Set destination directory")) ;
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
		 }//for (std::list<SharedDirInfo>::const_iterator it
	 }//if ( (!items.empty()) &&

	if (atLeastOne_Paused) {
		contextMnu.addAction(resumeAct) ;
	}//if (atLeastOne_Paused)
	if (atLeastOne_Downloading || atLeastOne_Queued || atLeastOne_Waiting) {
		contextMnu.addAction(pauseAct) ;
	}//if (atLeastOne_Downloading || atLeastOne_Queued || atLeastOne_Waiting)

	if (!atLeastOne_Complete && !items.empty()) {
			contextMnu.addAction(forceCheckAct) ;
			contextMnu.addAction(cancelAct) ;
	}//if (!atLeastOne_Complete && !items.empty())
	if (add_PlayOption) {
		contextMnu.addAction(playAct) ;
	}//if (add_PlayOption)

	if (atLeastOne_Paused || atLeastOne_Downloading || atLeastOne_Complete || add_PlayOption) {
		contextMnu.addSeparator() ;//------------------------------------------------
	}//if (atLeastOne_Paused ||

	if (single) {
		if (add_OpenFileOption) contextMnu.addAction( openFileAct) ;
		if (add_PreviewOption) contextMnu.addAction( previewFileAct) ;
		contextMnu.addAction( openFolderAct) ;
		contextMnu.addAction( detailsFileAct) ;
		contextMnu.addSeparator() ;//--------------------------------------------
	}//if (single)

	contextMnu.addAction( clearCompletedAct) ;
	contextMnu.addSeparator() ;//------------------------------------------------

	if (add_CopyLink) {
		contextMnu.addAction( copyLinkAct) ;
	}//if (add_CopyLink)
	if (add_PasteLink) {
		contextMnu.addAction( pasteLinkAct) ;
	}//if (add_PasteLink)
	if (add_CopyLink || add_PasteLink) {
		contextMnu.addSeparator() ;//--------------------------------------------
	}//if (add_CopyLink || add_PasteLink)

	if (DLListModel->rowCount()>0 ) {
		contextMnu.addAction( expandAllAct ) ;
		contextMnu.addAction( collapseAllAct ) ;
	}

	contextMnu.addSeparator() ;//-----------------------------------------------

	contextMnu.addAction( toggleShowCacheTransfersAct ) ;
	toggleShowCacheTransfersAct->setChecked(_show_cache_transfers) ;

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

    showDLSizeAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_SIZE));
    showDLCompleteAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_COMPLETED));
    showDLDLSpeedAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_DLSPEED));
    showDLProgressAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_PROGRESS));
    showDLSourcesAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_SOURCES));
    showDLStatusAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_STATUS));
    showDLPriorityAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_PRIORITY));
    showDLRemainingAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_REMAINING));
    showDLDownloadTimeAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_DOWNLOADTIME));
    showDLIDAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_ID));
    showDLLastDLAct->setChecked(!ui.downloadList->isColumnHidden(COLUMN_LASTDL));
    showDLPath->setChecked(!ui.downloadList->isColumnHidden(COLUMN_PATH));

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
    std::cerr << "TransfersDialog::uploadsListCustomPopupMenu()" << std::endl;

    std::set<RsFileHash> items;
    getULSelectedItems(&items, NULL);

    bool single = (items.size() == 1);

    bool add_CopyLink = false;

    QMenu contextMnu( this );

    if(!items.empty())
    {
        add_CopyLink = true;

    }//if(!items.empty())

    if(single)
    {
        contextMnu.addAction( ulOpenFolderAct);
    }

    if (add_CopyLink)
        contextMnu.addAction( ulCopyLinkAct);

    contextMnu.exec(QCursor::pos());
}

void TransfersDialog::chooseDestinationDirectory()
{
	QString dest_dir = QFileDialog::getExistingDirectory(this,tr("Choose directory")) ;

	if(dest_dir.isNull())
		return ;

    std::set<RsFileHash> items ;
	getSelectedItems(&items, NULL);

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
	getSelectedItems(&items, NULL);

    for(std::set<RsFileHash>::const_iterator it(items.begin());it!=items.end();++it)
	{
		std::cerr << "Setting new directory " << dest_dir << " to file " << *it << std::endl;
		rsFiles->setDestinationDirectory(*it,dest_dir) ;
	}
}

int TransfersDialog::addItem(int row, const FileInfo &fileInfo)
{
    QString fileHash = QString::fromStdString(fileInfo.hash.toStdString());
	double fileDlspeed = (fileInfo.downloadStatus == FT_STATE_DOWNLOADING) ? (fileInfo.tfRate * 1024.0) : 0.0;

	QString status;
	switch (fileInfo.downloadStatus) {
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

	QString priority;

	if (fileInfo.downloadStatus == FT_STATE_QUEUED) {
		priority = QString::number(fileInfo.queue_position);
	} else {
		switch (fileInfo.priority) {
			case SPEED_LOW:     priority = tr("Slower");break;
			case SPEED_NORMAL:  priority = tr("Average");break;
			case SPEED_HIGH:    priority = tr("Faster");break;
			default:            priority = tr("Average");break;
		}
	}

	qlonglong completed = fileInfo.transfered;
	qlonglong remaining = fileInfo.size - fileInfo.transfered;

	qlonglong downloadtime = (fileInfo.tfRate > 0)?( (fileInfo.size - fileInfo.transfered) / (fileInfo.tfRate * 1024.0) ) : 0 ;
	qint64 qi64LastDL = fileInfo.lastTS ; //std::numeric_limits<qint64>::max();

	if (qi64LastDL == 0)	// file is complete, or any raison why the time has not been set properly
	{
		QFileInfo file;

		if (fileInfo.downloadStatus == FT_STATE_COMPLETE) 
			file = QFileInfo(QString::fromUtf8(fileInfo.path.c_str()), QString::fromUtf8(fileInfo.fname.c_str()));
		else 
            file = QFileInfo(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()), QString::fromUtf8(fileInfo.hash.toStdString().c_str()));
		
		/*Get Last Access on File */
		if (file.exists()) 
			qi64LastDL = file.lastModified().toTime_t();
	}
	QString strPath = QString::fromUtf8(fileInfo.path.c_str());
	QString strPathAfterDL = strPath;
	strPathAfterDL.replace(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()),"");
	QStringList qslPath = strPathAfterDL.split("/");

	FileChunksInfo fcinfo;
	if (!rsFiles->FileDownloadChunksDetails(fileInfo.hash, fcinfo)) {
		return -1;
	}

	FileProgressInfo pinfo;
	pinfo.cmap = fcinfo.chunks;
	pinfo.type = FileProgressInfo::DOWNLOAD_LINE;
	pinfo.progress = (fileInfo.size == 0) ? 0 : (completed * 100.0 / fileInfo.size);
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

	QString tooltip;

	if (fileInfo.downloadStatus == FT_STATE_CHECKING_HASH) {
		tooltip = tr("If the hash of the downloaded data does\nnot correspond to the hash announced\nby the file source. The data is likely \nto be corrupted.\n\nRetroShare will ask the source a detailed \nmap of the data; it will compare and invalidate\nbad blocks, and download them again\n\nTry to be patient!") ;
	}

	if (row < 0) {
		row = DLListModel->rowCount();
		DLListModel->insertRow(row);

		// change progress column to own class for sorting
        DLListModel->setItem(row, COLUMN_PROGRESS, new ProgressItem(NULL));
        DLListModel->setItem(row, COLUMN_PRIORITY, new PriorityItem(NULL));

        DLListModel->setData(DLListModel->index(row, COLUMN_SIZE), QVariant((qlonglong) fileInfo.size));
        DLListModel->setData(DLListModel->index(row, COLUMN_ID), fileHash, Qt::DisplayRole);
        DLListModel->setData(DLListModel->index(row, COLUMN_ID), fileHash, Qt::UserRole);
	}
	QString fileName = QString::fromUtf8(fileInfo.fname.c_str());

    DLListModel->setData(DLListModel->index(row, COLUMN_NAME), fileName);
    DLListModel->setData(DLListModel->index(row, COLUMN_NAME), FilesDefs::getIconFromFilename(fileName), Qt::DecorationRole);

    DLListModel->setData(DLListModel->index(row, COLUMN_COMPLETED), QVariant((qlonglong)completed));
    DLListModel->setData(DLListModel->index(row, COLUMN_DLSPEED), QVariant((double)fileDlspeed));
    DLListModel->setData(DLListModel->index(row, COLUMN_PROGRESS), QVariant::fromValue(pinfo));
    DLListModel->setData(DLListModel->index(row, COLUMN_STATUS), QVariant(status));
    DLListModel->setData(DLListModel->index(row, COLUMN_PRIORITY), QVariant(priority));
    DLListModel->setData(DLListModel->index(row, COLUMN_REMAINING), QVariant((qlonglong)remaining));
    DLListModel->setData(DLListModel->index(row, COLUMN_DOWNLOADTIME), QVariant((qlonglong)downloadtime));
    DLListModel->setData(DLListModel->index(row, COLUMN_LASTDL), QVariant(qi64LastDL));
    DLListModel->setData(DLListModel->index(row, COLUMN_PATH), QVariant(strPathAfterDL));
    DLListModel->item(row,COLUMN_PATH)->setToolTip(strPath);
    DLListModel->item(row,COLUMN_STATUS)->setToolTip(tooltip);

	QStandardItem *dlItem = DLListModel->item(row);

	std::map<std::string, std::string>::const_iterator vit;

	std::set<int> used_rows ;
	int active = 0;

	if (fileInfo.downloadStatus != FT_STATE_COMPLETE) {
		std::list<TransferInfo>::const_iterator pit;
		for (pit = fileInfo.peers.begin(); pit != fileInfo.peers.end(); ++pit) {
			const TransferInfo &transferInfo = *pit;

			QString peerName = getPeerName(transferInfo.peerId);
			//unique combination: fileHash + peerId, variant: hash + peerName (too long)
            QString hashFileAndPeerId = fileHash + QString::fromStdString(transferInfo.peerId.toStdString());
			QString version;
			std::string rsversion;
			if (rsDisc->getPeerVersion(transferInfo.peerId, rsversion))
			{
				version = tr("version:")+" " + QString::fromStdString(rsversion);
			}

			double peerDlspeed	= 0;
			if ((uint32_t)transferInfo.status == FT_STATE_DOWNLOADING && fileInfo.downloadStatus != FT_STATE_PAUSED && fileInfo.downloadStatus != FT_STATE_COMPLETE)
				peerDlspeed = transferInfo.tfRate * 1024.0;

			FileProgressInfo peerpinfo;
			peerpinfo.cmap = fcinfo.compressed_peer_availability_maps[transferInfo.peerId];
			peerpinfo.type = FileProgressInfo::DOWNLOAD_SOURCE ;
			peerpinfo.progress = 0.0;	// we don't display completion for sources.
			peerpinfo.nb_chunks = peerpinfo.cmap._map.empty() ? 0 : fcinfo.chunks.size();

			int row_id = addPeerToItem(dlItem, peerName, hashFileAndPeerId, peerDlspeed, transferInfo.status, peerpinfo);

			used_rows.insert(row_id);

			/* get the sources (number of online peers) */
			if (transferInfo.tfRate > 0 && fileInfo.downloadStatus == FT_STATE_DOWNLOADING)
				++active;
		}
	}

	float fltSources = active + (float)fileInfo.peers.size()/1000;
	DLListModel->setData(DLListModel->index(row, COLUMN_SOURCES), fltSources);

	// This is not optimal, but we deal with a small number of elements. The reverse order is really important,
	// because rows after the deleted rows change positions !
	//
	for (int r = dlItem->rowCount() - 1; r >= 0; --r) {
		if (used_rows.find(r) == used_rows.end()) {
			dlItem->removeRow(r);
		}
	}

	return row;
	
}

int TransfersDialog::addPeerToItem(QStandardItem *dlItem, const QString& name, const QString& coreID, double dlspeed, uint32_t status, const FileProgressInfo& peerInfo)
{
	// try to find the item
	int childRow = -1;
	int count = 0;
	QStandardItem *childId = NULL;

    for (count = 0; (childId = dlItem->child(count, COLUMN_ID)) != NULL; ++count) {
		if (childId->data(Qt::UserRole).toString() == coreID) {
			childRow = count;
			break;
		}
	}

    QStandardItem *siName = NULL;
    QStandardItem *siStatus = NULL;

	if (childRow == -1) {
		// set this false if you want to expand on double click
		dlItem->setEditable(false);

		QHeaderView *header = ui.downloadList->header();

        QStandardItem *iName  = new QStandardItem();
        QStandardItem *iSize  = new SortByNameItem(header);
        QStandardItem *iCompleted  = new SortByNameItem(header);
        QStandardItem *iDlSpeed  = new SortByNameItem(header);
        QStandardItem *iProgress  = new ProgressItem(header);
        QStandardItem *iSource  = new SortByNameItem(header);
        QStandardItem *iStatus  = new SortByNameItem(header);
        QStandardItem *iPriority  = new SortByNameItem(header);
        QStandardItem *iRemaining  = new SortByNameItem(header);
        QStandardItem *iDownloadTime = new SortByNameItem(header);
        QStandardItem *iID = new SortByNameItem(header);

        siName = iName;
        siStatus = iStatus;

		QList<QStandardItem*> items;
        iName->setData(QVariant(" " + name), Qt::DisplayRole);
        iSize->setData(QVariant(QString()), Qt::DisplayRole);
        iCompleted->setData(QVariant(QString()), Qt::DisplayRole);
        iDlSpeed->setData(QVariant((double)dlspeed), Qt::DisplayRole);
        iProgress->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);
        iSource->setData(QVariant(QString()), Qt::DisplayRole);

        iPriority->setData(QVariant(QString()), Qt::DisplayRole);	// blank field for priority
        iRemaining->setData(QVariant(QString()), Qt::DisplayRole);
        iDownloadTime->setData(QVariant(QString()), Qt::DisplayRole);
        iID->setData(QVariant()      , Qt::DisplayRole);
        iID->setData(QVariant(coreID), Qt::UserRole);

        items.append(iName);
        items.append(iSize);
        items.append(iCompleted);
        items.append(iDlSpeed);
        items.append(iProgress);
        items.append(iSource);
        items.append(iStatus);
        items.append(iPriority);
        items.append(iRemaining);
        items.append(iDownloadTime);
        items.append(iID);
		dlItem->appendRow(items);

		childRow = dlItem->rowCount() - 1;
	} else {
		// just update the child (peer)
        dlItem->child(childRow, COLUMN_DLSPEED)->setData(QVariant((double)dlspeed), Qt::DisplayRole);
        dlItem->child(childRow, COLUMN_PROGRESS)->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);

        siName = dlItem->child(childRow,COLUMN_NAME);
        siStatus = dlItem->child(childRow, COLUMN_STATUS);
	}

	switch (status) {
	case FT_STATE_FAILED:
        siStatus->setData(QVariant(tr("Failed"))) ;
        siName->setData(QIcon(":/images/Client1.png"), Qt::DecorationRole);
		break ;
	case FT_STATE_OKAY:
        siStatus->setData(QVariant(tr("Okay")));
        siName->setData(QIcon(":/images/Client2.png"), Qt::DecorationRole);
		break ;
	case FT_STATE_WAITING:
        siStatus->setData(QVariant(tr("")));
        siName->setData(QIcon(":/images/Client3.png"), Qt::DecorationRole);
		break ;
	case FT_STATE_DOWNLOADING:
        siStatus->setData(QVariant(tr("Transferring")));
        siName->setData(QIcon(":/images/Client0.png"), Qt::DecorationRole);
		break ;
	case FT_STATE_COMPLETE:
        siStatus->setData(QVariant(tr("Complete")));
        siName->setData(QIcon(":/images/Client0.png"), Qt::DecorationRole);
		break ;
	default:
        siStatus->setData(QVariant(tr("")));
        siName->setData(QIcon(":/images/Client4.png"), Qt::DecorationRole);
	}

	return childRow;
}

int TransfersDialog::addUploadItem(	const QString&, const QString& name, const QString& coreID, 
												qlonglong fileSize, const FileProgressInfo& pinfo, double dlspeed, 
												const QString& source, const QString& peer_id, const QString& status, qlonglong completed, qlonglong)
{
	// Find items does not work reliably, because it (apparently) needs Qt to flush pending events to work, so we can't call it
	// on a table that was just filled in.
	//
	int row ;
	for(row=0;row<ULListModel->rowCount();++row)
        if(ULListModel->item(row,COLUMN_UUSERID)->data(Qt::EditRole).toString() == peer_id && ULListModel->item(row,COLUMN_UHASH)->data(Qt::EditRole).toString() == coreID)
			break ;

	if(row >= ULListModel->rowCount() )
	{
		row = ULListModel->rowCount();
		ULListModel->insertRow(row);

		// change progress column to own class for sorting
        ULListModel->setItem(row, COLUMN_UPROGRESS, new ProgressItem(NULL));

        ULListModel->setData(ULListModel->index(row, COLUMN_UNAME),    QVariant((QString)" "+name), Qt::DisplayRole);
        ULListModel->setData(ULListModel->index(row, COLUMN_USERNAME), QVariant((QString)source));
        ULListModel->setData(ULListModel->index(row, COLUMN_UHASH),    QVariant((QString)coreID));
        ULListModel->setData(ULListModel->index(row, COLUMN_UUSERID),  QVariant((QString)peer_id));

        ULListModel->setData(ULListModel->index(row,COLUMN_UNAME), FilesDefs::getIconFromFilename(name), Qt::DecorationRole);
	}

    ULListModel->setData(ULListModel->index(row, COLUMN_USIZE),        QVariant((qlonglong)fileSize));
    ULListModel->setData(ULListModel->index(row, COLUMN_UTRANSFERRED), QVariant((qlonglong)completed));
    ULListModel->setData(ULListModel->index(row, COLUMN_ULSPEED),      QVariant((double)dlspeed));
    ULListModel->setData(ULListModel->index(row, COLUMN_UPROGRESS),    QVariant::fromValue(pinfo));
    ULListModel->setData(ULListModel->index(row, COLUMN_USTATUS),      QVariant((QString)status));

	return row;
}


/* get the list of Transfers from the RsIface.  **/
void TransfersDialog::updateDisplay()
{
	insertTransfers();
	updateDetailsDialog ();
}

void TransfersDialog::insertTransfers() 
{
	/* disable for performance issues, enable after insert all transfers */
	ui.downloadList->setSortingEnabled(false);

	/* get the download lists */
    std::list<RsFileHash> downHashes;
	rsFiles->FileDownloads(downHashes);

	/* build set for quick search */
    std::set<RsFileHash> hashs;
    std::list<RsFileHash>::iterator it;
	for (it = downHashes.begin(); it != downHashes.end(); ++it) {
		hashs.insert(*it);
	}

	/* add downloads, first iterate all rows in list */

	int rowCount = DLListModel->rowCount();
	int row ;
    std::set<RsFileHash>::iterator hashIt;

	for (row = 0; row < rowCount; ) {
        RsFileHash hash ( DLListModel->item(row, COLUMN_ID)->data(Qt::UserRole).toString().toStdString());

		hashIt = hashs.find(hash);
		if (hashIt == hashs.end()) {
			// remove not existing downloads
			DLListModel->removeRow(row);
			rowCount = DLListModel->rowCount();
			continue;
		}

		FileInfo fileInfo;
		if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, fileInfo)) {
			DLListModel->removeRow(row);
			rowCount = DLListModel->rowCount();
			continue;
		}

		if ((fileInfo.transfer_info_flags & RS_FILE_REQ_CACHE) && !_show_cache_transfers) {
			// if file transfer is a cache file index file, don't show it
			DLListModel->removeRow(row);
			rowCount = DLListModel->rowCount();
			continue;
		}

		hashs.erase(hashIt);

		if (addItem(row, fileInfo) < 0) {
			DLListModel->removeRow(row);
			rowCount = DLListModel->rowCount();
			continue;
		}

		++row;
	}

	/* then add new downloads to the list */

	for (hashIt = hashs.begin(); hashIt != hashs.end(); ++hashIt) {
		FileInfo fileInfo;
		if (!rsFiles->FileDetails(*hashIt, RS_FILE_HINTS_DOWNLOAD, fileInfo)) {
			continue;
		}

		if ((fileInfo.transfer_info_flags & RS_FILE_REQ_CACHE) && !_show_cache_transfers) {
			//if file transfer is a cache file index file, don't show it
			continue;
		}

		addItem(-1, fileInfo);
	}

	ui.downloadList->setSortingEnabled(true);

	ui.uploadsList->setSortingEnabled(false);

	// Now show upload hashes
	//
    std::list<RsFileHash> upHashes;
	rsFiles->FileUploads(upHashes);

    RsPeerId ownId = rsPeers->getOwnId();

    std::set<std::string> used_hashes ;

	for(it = upHashes.begin(); it != upHashes.end(); ++it)
	{
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_UPLOAD, info)) 
			continue;
		
		if((info.transfer_info_flags & RS_FILE_REQ_CACHE) && _show_cache_transfers)
			continue ;

		std::list<TransferInfo>::iterator pit;
		for(pit = info.peers.begin(); pit != info.peers.end(); ++pit)
		{
			if (pit->peerId == ownId) //don't display transfer to ourselves
				continue ;

            QString fileHash        = QString::fromStdString(info.hash.toStdString());
			QString fileName    	= QString::fromUtf8(info.fname.c_str());
			QString source	= getPeerName(pit->peerId);

			QString status;
			switch(pit->status)
			{
				case FT_STATE_FAILED:   status = tr("Failed"); break;
				case FT_STATE_OKAY:     status = tr("Okay"); break;
				case FT_STATE_WAITING:  status = tr("Waiting"); break;
				case FT_STATE_DOWNLOADING: status = tr("Uploading"); break;
				case FT_STATE_COMPLETE: status = tr("Complete"); break;
				default:                status = tr("Complete"); break;

			}

			FileProgressInfo pinfo ;

			if(!rsFiles->FileUploadChunksDetails(*it,pit->peerId,pinfo.cmap) )
				continue ;

			double dlspeed  	= pit->tfRate * 1024.0;
			qlonglong fileSize 	= info.size;
			qlonglong completed 	= pit->transfered;
//			double progress 	= (info.size > 0)?(pit->transfered * 100.0 / info.size):0.0;
			qlonglong remaining   = (pit->tfRate>0)?((info.size - pit->transfered) / (pit->tfRate * 1024.0)):0;

			// Estimate the completion. We need something more accurate, meaning that we need to
			// transmit the completion info.
			//
			uint32_t chunk_size = 1024*1024 ;
			uint32_t nb_chunks = (uint32_t)((info.size + (uint64_t)chunk_size - 1) / (uint64_t)(chunk_size)) ;

			uint32_t filled_chunks = pinfo.cmap.filledChunks(nb_chunks) ;
			pinfo.type = FileProgressInfo::UPLOAD_LINE ;
			pinfo.nb_chunks = pinfo.cmap._map.empty()?0:nb_chunks ;

			if(filled_chunks > 0 && nb_chunks > 0) 
			{
				completed = pinfo.cmap.computeProgress(info.size,chunk_size) ;
				pinfo.progress = completed / (float)info.size * 100.0f ;
			} 
			else 
			{
				completed = pit->transfered % chunk_size ;	// use the position with respect to last request.
				pinfo.progress = (info.size>0)?((pit->transfered % chunk_size)*100.0/info.size):0 ;
			}

            addUploadItem("", fileName, fileHash, fileSize, pinfo, dlspeed, source,QString::fromStdString(pit->peerId.toStdString()),  status, completed, remaining);

            used_hashes.insert(fileHash.toStdString() + pit->peerId.toStdString()) ;
		}
	}
	
	// remove hashes that where not shown
	//first clean the model in case some files are not download anymore
	//remove items that are not fiends anymore
	int removeIndex = 0;
	rowCount = ULListModel->rowCount();
	while (removeIndex < rowCount)
	{
        std::string hash = ULListModel->item(removeIndex, COLUMN_UHASH)->data(Qt::EditRole).toString().toStdString();
        std::string peer = ULListModel->item(removeIndex, COLUMN_UUSERID)->data(Qt::EditRole).toString().toStdString();

		if(used_hashes.find(hash + peer) == used_hashes.end()) {
			ULListModel->removeRow(removeIndex);
			rowCount = ULListModel->rowCount();
		} else
			++removeIndex;
	}

	ui.uploadsList->setSortingEnabled(true);
	
	downloads = tr("Downloads") + " (" + QString::number(DLListModel->rowCount()) + ")";
	uploads    = tr("Uploads") + " (" + QString::number(ULListModel->rowCount()) + ")" ;

	ui.tabWidget->setTabText(0,  downloads);
	ui.tabWidget_2->setTabText(0, uploads);

}

QString TransfersDialog::getPeerName(const RsPeerId& id) const
{
	QString res = QString::fromUtf8(rsPeers->getPeerName(id).c_str()) ;

	// This is because turtle tunnels have no name (I didn't want to bother with
	// connect mgr). In such a case their id can suitably hold for a name.
	//
	if(res == "")
        return tr("Anonymous tunnel 0x")+QString::fromStdString(id.toStdString()).left(8) ;
	else
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
	getSelectedItems(&items, NULL);
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

void TransfersDialog::copyLink ()
{
	QList<RetroShareLink> links ;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); ++it) {
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) {
			continue;
		}

		RetroShareLink link;
        if (link.createFile(QString::fromUtf8(info.fname.c_str()), info.size, QString::fromStdString(info.hash.toStdString()))) {
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

        RetroShareLink link;
        if (link.createFile(QString::fromUtf8(info.fname.c_str()), info.size, QString::fromStdString(info.hash.toStdString()))) {
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
    RsFileHash file_hash ;
    std::set<int> rows;
    std::set<int>::iterator it;
    getSelectedItems(NULL, &rows);

    if (rows.size()) {
        int row = *rows.begin();

        file_hash = RsFileHash(getID(row, DLListModel).toStdString());
    }

    detailsDialog()->setFileHash(file_hash);
}

void TransfersDialog::pasteLink()
{
	RSLinkClipboard::process(RetroShareLink::TYPE_FILE);
}

void TransfersDialog::getSelectedItems(std::set<RsFileHash> *ids, std::set<int> *rows)
{
	if (ids == NULL && rows == NULL) {
		return;
	}

	if (ids) ids->clear();
	if (rows) rows->clear();

	int i, imax = DLListModel->rowCount();
	for (i = 0; i < imax; ++i) {
		bool isParentSelected = false;
		bool isChildSelected = false;

		QStandardItem *parent = DLListModel->item(i);
		if (!parent) continue;
		QModelIndex pindex = parent->index();
		if (selection->isSelected(pindex)) {
			isParentSelected = true;
		} else {
			int j, jmax = parent->rowCount();
			for (j = 0; j < jmax && !isChildSelected; ++j) {
				QStandardItem *child = parent->child(j);
				if (!child) continue;
				QModelIndex cindex = child->index();
				if (selection->isSelected(cindex)) {
					isChildSelected = true;
				}
			}
		}

		/* if transfered file or it's peers are selected control it*/
		if (isParentSelected || isChildSelected) {
			if (ids) {
                QStandardItem *id = DLListModel->item(i, COLUMN_ID);
                ids->insert(RsFileHash(id->data(Qt::DisplayRole).toString().toStdString()));
                ids->insert(RsFileHash(id->data(Qt::UserRole   ).toString().toStdString()));
			}
			if (rows) {
				rows->insert(i);
			}
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
            QStandardItem *id = ULListModel->item(index.row(), COLUMN_UHASH);
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
	getSelectedItems(&items, NULL);
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

void TransfersDialog::openFolderTransfer()
{
	FileInfo info;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); ++it) {
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	/* make path for downloaded or downloading files */
	QFileInfo qinfo;
	std::string path;
	if (info.downloadStatus == FT_STATE_COMPLETE) {
		path = info.path;
	} else {
		path = rsFiles->getPartialsDirectory();
	}

	/* open folder with a suitable application */
	qinfo.setFile(QString::fromUtf8(path.c_str()));
	if (qinfo.exists() && qinfo.isDir()) {
		if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
			std::cerr << "openFolderTransfer(): can't open folder " << path << std::endl;
		}
	}
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

    /* make path for uploading files */
    QFileInfo qinfo;
    std::string path;
    path = info.path.substr(0,info.path.length()-info.fname.length());

    /* open folder with a suitable application */
    qinfo.setFile(QString::fromUtf8(path.c_str()));
    if (qinfo.exists() && qinfo.isDir()) {
        if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
            std::cerr << "ulOpenFolder(): can't open folder " << path << std::endl;
        }
    }

}

void TransfersDialog::previewTransfer()
{
	FileInfo info;

    std::set<RsFileHash> items;
    std::set<RsFileHash>::iterator it;
	getSelectedItems(&items, NULL);
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
		fileInfo = QFileInfo(QString::fromUtf8(info.path.c_str()), QString::fromUtf8(info.fname.c_str()));
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

void TransfersDialog::openTransfer()
{
	FileInfo info;

	std::set<RsFileHash> items ;
	std::set<RsFileHash>::iterator it ;
	getSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); ++it) {
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	/* make path for downloaded or downloading files */
	std::string path;
	if (info.downloadStatus == FT_STATE_COMPLETE) {
		path = info.path + "/" + info.fname;

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
	getSelectedItems(&items, NULL);

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
	getSelectedItems(&items, NULL);

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
	getSelectedItems(&items, NULL);

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
	getSelectedItems(&items, NULL);

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
    RsFileHash file_hash ;
	int nb_select = 0 ;

	for(int i = 0; i <= DLListModel->rowCount(); ++i)
		if(selection->isRowSelected(i, QModelIndex())) 
		{
	        file_hash = RsFileHash(getID(i, DLListModel).toStdString());
			  ++nb_select ;
		}
	if(nb_select != 1)
        detailsDialog()->setFileHash(RsFileHash()) ;
	else
		detailsDialog()->setFileHash(file_hash) ;
	
	updateDetailsDialog ();
}

double TransfersDialog::getProgress(int , QStandardItemModel *)
{
//	return model->data(model->index(row, PROGRESS), Qt::DisplayRole).toDouble();
return 0.0 ;
}

double TransfersDialog::getSpeed(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_DLSPEED), Qt::DisplayRole).toDouble();
}

QString TransfersDialog::getFileName(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_NAME), Qt::DisplayRole).toString();
}

QString TransfersDialog::getStatus(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_STATUS), Qt::DisplayRole).toString();
}

QString TransfersDialog::getID(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_ID), Qt::UserRole).toString().left(40); // gets only the "hash" part of the name
}

QString TransfersDialog::getPriority(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_PRIORITY), Qt::DisplayRole).toString();
}

qlonglong TransfersDialog::getFileSize(int row, QStandardItemModel *model)
{
	bool ok = false;
    return model->data(model->index(row, COLUMN_SIZE), Qt::DisplayRole).toULongLong(&ok);
}

qlonglong TransfersDialog::getTransfered(int row, QStandardItemModel *model)
{
	bool ok = false;
    return model->data(model->index(row, COLUMN_COMPLETED), Qt::DisplayRole).toULongLong(&ok);
}

qlonglong TransfersDialog::getRemainingTime(int row, QStandardItemModel *model)
{
	bool ok = false;
    return model->data(model->index(row, COLUMN_REMAINING), Qt::DisplayRole).toULongLong(&ok);
}

qlonglong TransfersDialog::getDownloadTime(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_DOWNLOADTIME), Qt::DisplayRole).toULongLong();
}

qlonglong TransfersDialog::getLastDL(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_LASTDL), Qt::DisplayRole).toULongLong();
}

qlonglong TransfersDialog::getPath(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_PATH), Qt::DisplayRole).toULongLong();
}

QString TransfersDialog::getSources(int row, QStandardItemModel *model)
{
	double dblValue =  model->data(model->index(row, COLUMN_SOURCES), Qt::DisplayRole).toDouble();
	QString temp = QString("%1 (%2)").arg((int)dblValue).arg((int)((fmod(dblValue,1)*1000)+0.5));

	return temp;
}

void TransfersDialog::collCreate()
{
	std::vector <DirDetails> dirVec;

	std::set<RsFileHash> items ;
	std::set<RsFileHash>::iterator it ;
	getSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); ++it) {
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;

		DirDetails details;
		details.name = info.fname;
		details.hash = info.hash;
		details.count = info.size;
		details.type = DIR_TYPE_FILE;

		dirVec.push_back(details);
	}//for (it = items.begin();

	RsCollectionFile(dirVec).openNewColl(this);
}

void TransfersDialog::collModif()
{
	FileInfo info;

	std::set<RsFileHash> items ;
	std::set<RsFileHash>::iterator it ;
	getSelectedItems(&items, NULL);

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
			if (qinfo.absoluteFilePath().endsWith(RsCollectionFile::ExtensionString)) {
				RsCollectionFile collection;
				collection.openColl(qinfo.absoluteFilePath());
			}//if (qinfo.absoluteFilePath().endsWith(RsCollectionFile::ExtensionString))
		}//if (qinfo.exists())
	}//if (info.downloadStatus == FT_STATE_COMPLETE)
}

void TransfersDialog::collView()
{
	FileInfo info;

	std::set<RsFileHash> items;
	std::set<RsFileHash>::iterator it;
	getSelectedItems(&items, NULL);

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
			if (qinfo.absoluteFilePath().endsWith(RsCollectionFile::ExtensionString)) {
				RsCollectionFile collection;
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
	getSelectedItems(&items, NULL);

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
					if (qinfo.absoluteFilePath().endsWith(RsCollectionFile::ExtensionString)) {
						RsCollectionFile collection;
                        if (collection.load(qinfo.absoluteFilePath(), true)) {
							collection.downloadFiles();
							return;
                        }
                    }
                }
            }
        }
    }

	RsCollectionFile collection;
	if (collection.load(this)) {
		collection.downloadFiles();
    }
}

void TransfersDialog::setShowDLSizeColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_SIZE)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_SIZE, !show);
    }
}

void TransfersDialog::setShowDLCompleteColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_COMPLETED)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_COMPLETED, !show);
    }
}

void TransfersDialog::setShowDLDLSpeedColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_DLSPEED)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_DLSPEED, !show);
    }
}

void TransfersDialog::setShowDLProgressColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_PROGRESS)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_PROGRESS, !show);
    }
}

void TransfersDialog::setShowDLSourcesColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_SOURCES)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_SOURCES, !show);
    }
}

void TransfersDialog::setShowDLStatusColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_STATUS)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_STATUS, !show);
    }
}

void TransfersDialog::setShowDLPriorityColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_PRIORITY)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_PRIORITY, !show);
    }
}

void TransfersDialog::setShowDLRemainingColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_REMAINING)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_REMAINING, !show);
    }
}

void TransfersDialog::setShowDLDownloadTimeColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_DOWNLOADTIME)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_DOWNLOADTIME, !show);
    }
}

void TransfersDialog::setShowDLIDColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_ID)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_ID, !show);
    }
}

void TransfersDialog::setShowDLLastDLColumn(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_LASTDL)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_LASTDL, !show);
    }
}

void TransfersDialog::setShowDLPath(bool show)
{
    if ( (!ui.downloadList->isColumnHidden(COLUMN_PATH)) != show) {
        ui.downloadList->setColumnHidden(COLUMN_PATH, !show);
    }
}

void TransfersDialog::expandAll()
{
	ui.downloadList->expandAll();
}
void TransfersDialog::collapseAll()
{
	ui.downloadList->collapseAll();
}
