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

#ifdef WIN32
        #include <windows.h>
#endif

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

#include "TransfersDialog.h"
#include "RetroShareLink.h"
#include "DetailsDialog.h"
#include "DLListDelegate.h"
#include "ULListDelegate.h"
#include "FileTransferInfoWidget.h"
#include "SearchDialog.h"
#include "SharedFilesDialog.h"
#include "xprogressbar.h"
#include "settings/rsharesettings.h"
#include "util/misc.h"
#include "common/RsCollectionFile.h"
#include "transfers/TransferUserNotify.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsplugin.h>

/****
 * #define SHOW_RTT_STATISTICS		1
 ****/

#ifdef SHOW_RTT_STATISTICS
	#include "VoipStatistics.h"
#endif

/* Images for context menu icons */
#define IMAGE_INFO                 ":/images/fileinfo.png"
#define IMAGE_CANCEL               ":/images/delete.png"
#define IMAGE_LIBRARY               ":/images/library.png"
#define IMAGE_CLEARCOMPLETED       ":/images/deleteall.png"
#define IMAGE_PLAY		             ":/images/player_play.png"
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
#define IMAGE_SEARCH    		      ":/images/filefind.png"
#define IMAGE_EXPAND             ":/images/edit_add24.png"
#define IMAGE_COLLAPSE           ":/images/edit_remove24.png"

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
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.downloadList, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( cancel ()));

  	//Selection Setup
    selection = ui.downloadList->selectionModel();

    ui.downloadList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui.downloadList->setRootIsDecorated(true);


    /* Set header resize modes and initial section sizes Downloads TreeView*/
    QHeaderView * _header = ui.downloadList->header () ;
    _header->setResizeMode (COLUMN_NAME, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_SIZE, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_COMPLETED, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_DLSPEED, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_PROGRESS, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_SOURCES, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_STATUS, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_PRIORITY, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_REMAINING, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_DOWNLOADTIME, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_ID, QHeaderView::Interactive);
    _header->setResizeMode (COLUMN_LASTDL, QHeaderView::Interactive);

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

    // set default column and sort order for download
    ui.downloadList->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);
    
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
    selectionup = ui.uploadsList->selectionModel();
    ui.uploadsList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    /* Set header resize modes and initial section sizes Uploads TreeView*/
    QHeaderView * upheader = ui.uploadsList->header () ;
    upheader->setResizeMode (COLUMN_UNAME, QHeaderView::Interactive);
    upheader->setResizeMode (COLUMN_USIZE, QHeaderView::Interactive);
    upheader->setResizeMode (COLUMN_UTRANSFERRED, QHeaderView::Interactive);
    upheader->setResizeMode (COLUMN_ULSPEED, QHeaderView::Interactive);
    upheader->setResizeMode (COLUMN_UPROGRESS, QHeaderView::Interactive);
    upheader->setResizeMode (COLUMN_USTATUS, QHeaderView::Interactive);
    upheader->setResizeMode (COLUMN_USERNAME, QHeaderView::Interactive);

    upheader->resizeSection ( COLUMN_UNAME, 190 );
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
	 ui.tabWidget->insertTab(3,remoteSharedFiles = new RemoteSharedFilesDialog(), QIcon(IMAGE_SEARCH), tr("Friends files")) ;

	 ui.tabWidget->addTab(localSharedFiles = new LocalSharedFilesDialog(), QIcon(IMAGE_SEARCH), tr("My files")) ;

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

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif

    /** Setup the actions for the context menu */
   toggleShowCacheTransfersAct = new QAction(tr( "Show cache transfers" ), this );
	toggleShowCacheTransfersAct->setCheckable(true) ;
	connect(toggleShowCacheTransfersAct,SIGNAL(triggered()),this,SLOT(toggleShowCacheTransfers())) ;

	openCollectionAct = new QAction(QIcon(IMAGE_LIBRARY), tr( "Download from collection file..." ), this );
	connect(openCollectionAct, SIGNAL(triggered()), this, SLOT(openCollection()));

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

   openfolderAct = new QAction(QIcon(IMAGE_OPENFOLDER), tr("Open Folder"), this);
   connect(openfolderAct, SIGNAL(triggered()), this, SLOT(openFolderTransfer()));

   openfileAct = new QAction(QIcon(IMAGE_OPENFILE), tr("Open File"), this);
   connect(openfileAct, SIGNAL(triggered()), this, SLOT(openTransfer()));

   previewfileAct = new QAction(QIcon(IMAGE_PREVIEW), tr("Preview File"), this);
   connect(previewfileAct, SIGNAL(triggered()), this, SLOT(previewTransfer()));

   detailsfileAct = new QAction(QIcon(IMAGE_INFO), tr("Details..."), this);
   connect(detailsfileAct, SIGNAL(triggered()), this, SLOT(showDetailsDialog()));

	clearcompletedAct = new QAction(QIcon(IMAGE_CLEARCOMPLETED), tr( "Clear Completed" ), this );
	connect( clearcompletedAct , SIGNAL( triggered() ), this, SLOT( clearcompleted() ) );


	copylinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy RetroShare Link" ), this );
	connect( copylinkAct , SIGNAL( triggered() ), this, SLOT( copyLink() ) );
	pastelinkAct = new QAction(QIcon(IMAGE_PASTELINK), tr( "Paste RetroShare Link" ), this );
	connect( pastelinkAct , SIGNAL( triggered() ), this, SLOT( pasteLink() ) );
	queueDownAct = new QAction(QIcon(":/images/go-down.png"), tr("Down"), this);
	connect(queueDownAct, SIGNAL(triggered()), this, SLOT(priorityQueueDown()));
	queueUpAct = new QAction(QIcon(":/images/go-up.png"), tr("Up"), this);
	connect(queueUpAct, SIGNAL(triggered()), this, SLOT(priorityQueueUp()));
	queueTopAct = new QAction(QIcon(":/images/go-top.png"), tr("Top"), this);
	connect(queueTopAct, SIGNAL(triggered()), this, SLOT(priorityQueueTop()));
	queueBottomAct = new QAction(QIcon(":/images/go-bottom.png"), tr("Bottom"), this);
	connect(queueBottomAct, SIGNAL(triggered()), this, SLOT(priorityQueueBottom()));
	chunkStreamingAct = new QAction(QIcon(IMAGE_PRIORITYAUTO), tr("Streaming"), this);
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
	renameFileAct = new QAction(QIcon(IMAGE_PRIORITYNORMAL), tr("Rename file..."), this);
	connect(renameFileAct, SIGNAL(triggered()), this, SLOT(renameFile()));
	specifyDestinationDirectoryAct = new QAction(QIcon(IMAGE_SEARCH),tr("Specify..."),this) ;
	connect(specifyDestinationDirectoryAct,SIGNAL(triggered()),this,SLOT(chooseDestinationDirectory())) ;
	expandAllAct= new QAction(QIcon(IMAGE_EXPAND),tr("Expand all"),this);
	connect(expandAllAct,SIGNAL(triggered()),this,SLOT(expandAll())) ;
	collapseAllAct= new QAction(QIcon(IMAGE_COLLAPSE),tr("Collapse all"),this);
	connect(collapseAllAct,SIGNAL(triggered()),this,SLOT(collapseAll())) ;

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

    // load settings
    processSettings(true);
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
//        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());

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
//        Settings->setValue("Splitter", ui.splitter->saveState());

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
	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);

	bool single = (items.size() == 1) ;

	/* check which item is selected
	 * - if it is completed - play should appear in menu
	 */
	std::cerr << "TransfersDialog::downloadListCustomPopupMenu()" << std::endl;
	
	FileInfo info;

	for (it = items.begin(); it != items.end(); it ++) {
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	bool addPlayOption = false;
	bool addOpenFileOption = false;

  if  (info.downloadStatus == FT_STATE_COMPLETE)
  {
	  std::cerr << "Add Play Option" << std::endl;

	  addOpenFileOption = true;

	  size_t pos = info.fname.find_last_of('.');

	  /* check if the file is a media file */
	  if(pos !=  std::string::npos && misc::isPreviewable(info.fname.substr(pos + 1).c_str()))
		  addPlayOption = true;
  }
		
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

	QMenu contextMnu( this );

	if (addPlayOption)
		contextMnu.addAction(playAct);

	contextMnu.addSeparator();

	if(!items.empty())
	{
		bool all_paused = true ;
		bool all_downld = true ;
		bool all_downloading = true ;
		bool all_queued = true ;

		QModelIndexList lst = ui.downloadList->selectionModel ()->selectedIndexes ();

		for (int i = 0; i < lst.count (); i++)
		{
			if ( lst[i].column() == 0 && info.downloadStatus == FT_STATE_PAUSED )
				all_downld = false ;
			if ( lst[i].column() == 0 && info.downloadStatus == FT_STATE_DOWNLOADING )
				all_paused = false ;

			if ( lst[i].column() == 0)
			{
				if(info.downloadStatus == FT_STATE_QUEUED)
					all_downloading = false ;
				else
					all_queued = false ;
			}
		}

		if(all_downloading)
			contextMnu.addMenu(&prioritySpeedMenu);
		else if(all_queued)
			contextMnu.addMenu(&priorityQueueMenu) ;

		if(all_downloading)
		{
			contextMnu.addMenu( &chunkMenu);

			if(single)
				contextMnu.addAction(renameFileAct) ;

			QMenu *directoryMenu = contextMnu.addMenu(QIcon(IMAGE_OPENFOLDER),tr("Set destination directory")) ;
			directoryMenu->addAction(specifyDestinationDirectoryAct);

			// Now get the list of existing directories.

			std::list<SharedDirInfo> dirs ;
			rsFiles->getSharedDirectories(dirs) ;

			for(std::list<SharedDirInfo>::const_iterator it(dirs.begin());it!=dirs.end();++it)
			{
				// check for existence of directory name
				QFile directory(QString::fromUtf8((*it).filename.c_str())) ;

				if(!directory.exists()) continue ;
				if(!(directory.permissions() & QFile::WriteOwner)) continue ;

				QAction *act = new QAction(QString::fromUtf8((*it).virtualname.c_str()),directoryMenu) ;
				act->setData(QString::fromUtf8((*it).filename.c_str())) ;
				connect(act,SIGNAL(triggered()),this,SLOT(setDestinationDirectory())) ;
				directoryMenu->addAction(act) ;
			}
		}

		if(!all_paused)
			contextMnu.addAction( pauseAct);
		if(!all_downld)
			contextMnu.addAction( resumeAct);

		if(single)
		{	
			if(info.downloadStatus == FT_STATE_PAUSED)
				contextMnu.addAction( resumeAct);
			else if(info.downloadStatus != FT_STATE_COMPLETE)
				contextMnu.addAction( pauseAct);
		}

		if(info.downloadStatus != FT_STATE_COMPLETE)
		{
//#ifdef USE_NEW_CHUNK_CHECKING_CODE
			contextMnu.addAction( forceCheckAct);
//#endif
			contextMnu.addAction( cancelAct);
		}

		contextMnu.addSeparator();
	}

	if(single)
	{
		if (addOpenFileOption)
			contextMnu.addAction( openfileAct);

		contextMnu.addAction( previewfileAct);
		contextMnu.addAction( openfolderAct);
		contextMnu.addAction( detailsfileAct);
		contextMnu.addSeparator();
	}

	contextMnu.addAction( clearcompletedAct);
	contextMnu.addSeparator();

	if(!items.empty())
		contextMnu.addAction( copylinkAct);

	if(!RSLinkClipboard::empty(RetroShareLink::TYPE_FILE)) {
		pastelinkAct->setEnabled(true);
	} else {
		pastelinkAct->setDisabled(true);
	}
	contextMnu.addAction( pastelinkAct);

	contextMnu.addSeparator();

	if (DLListModel->rowCount()>0 ) {
		contextMnu.addAction( expandAllAct ) ;
		contextMnu.addAction( collapseAllAct ) ;
	}

	contextMnu.addSeparator();//-----------------------------------------------

	contextMnu.addAction( toggleShowCacheTransfersAct ) ;
	toggleShowCacheTransfersAct->setChecked(_show_cache_transfers) ;
	contextMnu.addAction( openCollectionAct ) ;
	
	contextMnu.addSeparator();

	contextMnu.exec(QCursor::pos());
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

    contextMnu.exec(QCursor::pos());

}

void TransfersDialog::chooseDestinationDirectory()
{
	QString dest_dir = QFileDialog::getExistingDirectory(this,tr("Choose directory")) ;

	if(dest_dir.isNull())
		return ;

	std::set<std::string> items ;
	getSelectedItems(&items, NULL);

	for(std::set<std::string>::const_iterator it(items.begin());it!=items.end();++it)
	{
		std::cerr << "Setting new directory " << dest_dir.toUtf8().data() << " to file " << *it << std::endl;
		rsFiles->setDestinationDirectory(*it,dest_dir.toUtf8().data() ) ;
	}
}
void TransfersDialog::setDestinationDirectory()
{
	std::string dest_dir(qobject_cast<QAction*>(sender())->data().toString().toUtf8().data()) ;

	std::set<std::string> items ;
	getSelectedItems(&items, NULL);

	for(std::set<std::string>::const_iterator it(items.begin());it!=items.end();++it)
	{
		std::cerr << "Setting new directory " << dest_dir << " to file " << *it << std::endl;
		rsFiles->setDestinationDirectory(*it,dest_dir) ;
	}
}

int TransfersDialog::addItem(int row, const FileInfo &fileInfo, const std::map<std::string, std::string> &versions)
{
	QString fileHash = QString::fromStdString(fileInfo.hash);
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
	qlonglong downloadtime = (fileInfo.size - fileInfo.transfered) / (fileInfo.tfRate * 1024.0);
    QString strLastDL = tr("File Never Seen");
    QFileInfo file;

    if (fileInfo.downloadStatus == FT_STATE_COMPLETE) {
        file = QFileInfo(QString::fromUtf8(fileInfo.path.c_str()), QString::fromUtf8(fileInfo.fname.c_str()));
    } else {
        file = QFileInfo(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()), QString::fromUtf8(fileInfo.hash.c_str()));
    }
    /*Get Last Access on File */
    if (file.exists()) {
        strLastDL= file.lastModified().toString("yyyy-MM-dd_hh:mm:ss");
    }

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
        DLListModel->setData(DLListModel->index(row, COLUMN_ID), fileHash);
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
    DLListModel->setData(DLListModel->index(row, COLUMN_LASTDL), QVariant(strLastDL));

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
			QString hashFileAndPeerId = fileHash + QString::fromStdString(transferInfo.peerId);
			QString version;
			if (versions.end() != (vit = versions.find(transferInfo.peerId))) {
				version = tr("version: ") + QString::fromStdString(vit->second);
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
				active++;
		}
	}

	QString sources = QString("%1 (%2)").arg(active).arg(fileInfo.peers.size());
    DLListModel->setData(DLListModel->index(row, COLUMN_SOURCES), QVariant(sources));

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
		if (childId->data(Qt::DisplayRole).toString() == coreID) {
			childRow = count;
			break;
		}
	}

	QStandardItem *si1 = NULL;
	QStandardItem *si7 = NULL;

	if (childRow == -1) {
		// set this false if you want to expand on double click
		dlItem->setEditable(false);

		QHeaderView *header = ui.downloadList->header();

		QStandardItem *i1  = new QStandardItem();
		QStandardItem *i2  = new SortByNameItem(header);
		QStandardItem *i3  = new SortByNameItem(header);
		QStandardItem *i4  = new SortByNameItem(header);
		QStandardItem *i5  = new ProgressItem(header);
		QStandardItem *i6  = new SortByNameItem(header);
		QStandardItem *i7  = new SortByNameItem(header);
		QStandardItem *i8  = new SortByNameItem(header);
		QStandardItem *i9  = new SortByNameItem(header);
		QStandardItem *i10 = new SortByNameItem(header);
		QStandardItem *i11 = new SortByNameItem(header);

		si1 = i1;
		si7 = i7;

		QList<QStandardItem*> items;
		i1->setData(QVariant(" " + name), Qt::DisplayRole);
		i2->setData(QVariant(QString()), Qt::DisplayRole);
		i3->setData(QVariant(QString()), Qt::DisplayRole);
		i4->setData(QVariant((double)dlspeed), Qt::DisplayRole);
		i5->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);
		i6->setData(QVariant(QString()), Qt::DisplayRole);

		i8->setData(QVariant(QString()), Qt::DisplayRole);	// blank field for priority
		i9->setData(QVariant(QString()), Qt::DisplayRole);
		i10->setData(QVariant(QString()), Qt::DisplayRole);
		i11->setData(QVariant(coreID), Qt::DisplayRole);

		items.append(i1);
		items.append(i2);
		items.append(i3);
		items.append(i4);
		items.append(i5);
		items.append(i6);
		items.append(i7);
		items.append(i8);
		items.append(i9);
		items.append(i10);
		items.append(i11);
		dlItem->appendRow(items);

		childRow = dlItem->rowCount() - 1;
	} else {
		// just update the child (peer)
        dlItem->child(childRow, COLUMN_DLSPEED)->setData(QVariant((double)dlspeed), Qt::DisplayRole);
        dlItem->child(childRow, COLUMN_PROGRESS)->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);

        si1 = dlItem->child(childRow,COLUMN_NAME);
        si7 = dlItem->child(childRow, COLUMN_STATUS);
	}

	switch (status) {
	case FT_STATE_FAILED:
		si7->setData(QVariant(tr("Failed"))) ;
		si1->setData(QIcon(":/images/Client1.png"), Qt::DecorationRole);
		break ;
	case FT_STATE_OKAY:
		si7->setData(QVariant(tr("Okay")));
		si1->setData(QIcon(":/images/Client2.png"), Qt::DecorationRole);
		break ;
	case FT_STATE_WAITING:
		si7->setData(QVariant(tr("")));
		si1->setData(QIcon(":/images/Client3.png"), Qt::DecorationRole);
		break ;
	case FT_STATE_DOWNLOADING:
		si7->setData(QVariant(tr("Transferring")));
		si1->setData(QIcon(":/images/Client0.png"), Qt::DecorationRole);
		break ;
	case FT_STATE_COMPLETE:
		si7->setData(QVariant(tr("Complete")));
		si1->setData(QIcon(":/images/Client0.png"), Qt::DecorationRole);
		break ;
	default:
		si7->setData(QVariant(tr("")));
		si1->setData(QIcon(":/images/Client4.png"), Qt::DecorationRole);
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
	std::list<std::string> downHashes;
	rsFiles->FileDownloads(downHashes);

	/* get only once */
	std::map<std::string, std::string> versions;
	rsDisc->getDiscVersions(versions);

	/* build set for quick search */
	std::set<std::string> hashs;
	std::list<std::string>::iterator it;
	for (it = downHashes.begin(); it != downHashes.end(); ++it) {
		hashs.insert(*it);
	}

	/* add downloads, first iterate all rows in list */

	int rowCount = DLListModel->rowCount();
	int row ;
	std::set<std::string>::iterator hashIt;

	for (row = 0; row < rowCount; ) {
        std::string hash = DLListModel->item(row, COLUMN_ID)->data(Qt::DisplayRole).toString().toStdString();

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

		if (addItem(row, fileInfo, versions) < 0) {
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

		addItem(-1, fileInfo, versions);
	}

	ui.downloadList->setSortingEnabled(true);

	ui.uploadsList->setSortingEnabled(false);

	// Now show upload hashes
	//
	std::list<std::string> upHashes;
	rsFiles->FileUploads(upHashes);

	std::string ownId = rsPeers->getOwnId();

	std::set<std::string> used_hashes ;

	for(it = upHashes.begin(); it != upHashes.end(); it++) 
	{
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_UPLOAD, info)) 
			continue;
		
		if((info.transfer_info_flags & RS_FILE_REQ_CACHE) && _show_cache_transfers)
			continue ;

		std::list<TransferInfo>::iterator pit;
		for(pit = info.peers.begin(); pit != info.peers.end(); pit++) 
		{
			if (pit->peerId == ownId) //don't display transfer to ourselves
				continue ;

			QString fileHash        = QString::fromStdString(info.hash);
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
			qlonglong remaining   = (info.size - pit->transfered) / (pit->tfRate * 1024.0);

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

			addUploadItem("", fileName, fileHash, fileSize, pinfo, dlspeed, source,QString::fromStdString(pit->peerId),  status, completed, remaining);

			used_hashes.insert(fileHash.toStdString() + pit->peerId) ;
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
			removeIndex++;
	}

	ui.uploadsList->setSortingEnabled(true);
}

QString TransfersDialog::getPeerName(const std::string& id) const
{
	QString res = QString::fromUtf8(rsPeers->getPeerName(id).c_str()) ;

	// This is because turtle tunnels have no name (I didn't want to bother with
	// connect mgr). In such a case their id can suitably hold for a name.
	//
	if(res == "")
		return QString::fromStdString(id) ;
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

	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); it ++) {
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

	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); it ++) {
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) {
			continue;
		}

		RetroShareLink link;
		if (link.createFile(QString::fromUtf8(info.fname.c_str()), info.size, QString::fromStdString(info.hash))) {
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
    std::string file_hash ;
    std::set<int> rows;
    std::set<int>::iterator it;
    getSelectedItems(NULL, &rows);

    if (rows.size()) {
        int row = *rows.begin();

        file_hash = getID(row, DLListModel).toStdString();
    }

    detailsDialog()->setFileHash(file_hash);
}

void TransfersDialog::pasteLink()
{
	RSLinkClipboard::process(RetroShareLink::TYPE_FILE);
}

void TransfersDialog::getSelectedItems(std::set<std::string> *ids, std::set<int> *rows)
{
	if (ids == NULL && rows == NULL) {
		return;
	}

	if (ids) ids->clear();
	if (rows) rows->clear();

	int i, imax = DLListModel->rowCount();
	for (i = 0; i < imax; i++) {
		bool isParentSelected = false;
		bool isChildSelected = false;

		QStandardItem *parent = DLListModel->item(i);
		if (!parent) continue;
		QModelIndex pindex = parent->index();
		if (selection->isSelected(pindex)) {
			isParentSelected = true;
		} else {
			int j, jmax = parent->rowCount();
			for (j = 0; j < jmax && !isChildSelected; j++) {
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
				ids->insert(id->data(Qt::DisplayRole).toString().toStdString());
			}
			if (rows) {
				rows->insert(i);
			}
		}
	}
}

bool TransfersDialog::controlTransferFile(uint32_t flags)
{
	bool result = true;

	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); it ++) {
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

	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); it ++) {
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

void TransfersDialog::previewTransfer()
{
	FileInfo info;

	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); it ++) {
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	/* check if the file is a media file */
	if (!misc::isPreviewable(QFileInfo(QString::fromUtf8(info.fname.c_str())).suffix())) return;

	/* make path for downloaded or downloading files */
	QFileInfo fileInfo;
	if (info.downloadStatus == FT_STATE_COMPLETE) {
		fileInfo = QFileInfo(QString::fromUtf8(info.path.c_str()), QString::fromUtf8(info.fname.c_str()));
	} else {
		fileInfo = QFileInfo(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()), QString::fromUtf8(info.hash.c_str()));

		QString linkName = QFileInfo(QDir::temp(), QString::fromUtf8(info.fname.c_str())).absoluteFilePath();
		if (QFile::link(fileInfo.absoluteFilePath(), linkName)) {
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
		QFile::remove(fileInfo.absoluteFilePath());
	}
}

void TransfersDialog::openTransfer()
{
	FileInfo info;

	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); it ++) {
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
//	for (it = items.begin(); it != items.end(); it ++) {
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
	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); it ++) {
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
	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); it ++) 
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
	std::set<std::string> items;
	getSelectedItems(&items, NULL);

	if(items.size() != 1)
	{
		std::cerr << "Can't rename more than one file. This should not be called." << std::endl;
		return ;
	}

	std::string hash = *(items.begin()) ;

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
	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); it ++) 
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
	std::string file_hash ;
	int nb_select = 0 ;

	for(int i = 0; i <= DLListModel->rowCount(); i++) 
		if(selection->isRowSelected(i, QModelIndex())) 
		{
	        file_hash = getID(i, DLListModel).toStdString();
			  ++nb_select ;
		}
	if(nb_select != 1)
		detailsDialog()->setFileHash("") ;
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
    return model->data(model->index(row, COLUMN_ID), Qt::DisplayRole).toString().left(40); // gets only the "hash" part of the name
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

QString TransfersDialog::getSources(int row, QStandardItemModel *model)
{
    return model->data(model->index(row, COLUMN_SOURCES), Qt::DisplayRole).toString();
}

void TransfersDialog::openCollection()
{
	RsCollectionFile Collection;
	if (Collection.load(this)) {
		Collection.downloadFiles();
	}
}
void TransfersDialog::setShowDLSizeColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_SIZE) != show) {
        ui.downloadList->setColumnHidden(COLUMN_SIZE, !show);
    }
}

void TransfersDialog::setShowDLCompleteColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_COMPLETED) != show) {
        ui.downloadList->setColumnHidden(COLUMN_COMPLETED, !show);
    }
}

void TransfersDialog::setShowDLDLSpeedColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_DLSPEED) != show) {
        ui.downloadList->setColumnHidden(COLUMN_DLSPEED, !show);
    }
}

void TransfersDialog::setShowDLProgressColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_PROGRESS) != show) {
        ui.downloadList->setColumnHidden(COLUMN_PROGRESS, !show);
    }
}

void TransfersDialog::setShowDLSourcesColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_SOURCES) != show) {
        ui.downloadList->setColumnHidden(COLUMN_SOURCES, !show);
    }
}

void TransfersDialog::setShowDLStatusColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_STATUS) != show) {
        ui.downloadList->setColumnHidden(COLUMN_STATUS, !show);
    }
}

void TransfersDialog::setShowDLPriorityColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_PRIORITY) != show) {
        ui.downloadList->setColumnHidden(COLUMN_PRIORITY, !show);
    }
}

void TransfersDialog::setShowDLRemainingColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_REMAINING) != show) {
        ui.downloadList->setColumnHidden(COLUMN_REMAINING, !show);
    }
}

void TransfersDialog::setShowDLDownloadTimeColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_DOWNLOADTIME) != show) {
        ui.downloadList->setColumnHidden(COLUMN_DOWNLOADTIME, !show);
    }
}

void TransfersDialog::setShowDLIDColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_ID) != show) {
        ui.downloadList->setColumnHidden(COLUMN_ID, !show);
    }
}

void TransfersDialog::setShowDLLastDLColumn(bool show)
{
    if (!ui.downloadList->isColumnHidden(COLUMN_LASTDL) != show) {
        ui.downloadList->setColumnHidden(COLUMN_LASTDL, !show);
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
