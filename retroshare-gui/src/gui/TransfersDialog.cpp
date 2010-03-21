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

#include <set>

#include "rshare.h"
#include "TransfersDialog.h"
#include "RetroShareLink.h"
#include "DetailsDialog.h"
#include "DLListDelegate.h"
#include "ULListDelegate.h"
#include "FileTransferInfoWidget.h"
#include "TurtleRouterDialog.h"
#include "xprogressbar.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QUrl>

#include <sstream>
#include "rsiface/rsfiles.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsdisc.h"
#include "rsiface/rstypes.h"
#include <algorithm>
#include "util/misc.h"

/* Images for context menu icons */
#define IMAGE_INFO                 ":/images/fileinfo.png"
#define IMAGE_CANCEL               ":/images/delete.png"
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

Q_DECLARE_METATYPE(FileProgressInfo) 

/** Constructor */
TransfersDialog::TransfersDialog(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    connect( ui.downloadList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( downloadListCostumPopupMenu( QPoint ) ) );

    // Set Download list model
    DLListModel = new QStandardItemModel(0,ID + 1);
    DLListModel->setHeaderData(NAME, Qt::Horizontal, tr("Name", "i.e: file name"));
    DLListModel->setHeaderData(SIZE, Qt::Horizontal, tr("Size", "i.e: file size"));
    DLListModel->setHeaderData(COMPLETED, Qt::Horizontal, tr("Completed", ""));
    DLListModel->setHeaderData(DLSPEED, Qt::Horizontal, tr("Speed", "i.e: Download speed"));
    DLListModel->setHeaderData(PROGRESS, Qt::Horizontal, tr("Progress / Availability", "i.e: % downloaded"));
    DLListModel->setHeaderData(SOURCES, Qt::Horizontal, tr("Sources", "i.e: Sources"));
    DLListModel->setHeaderData(STATUS, Qt::Horizontal, tr("Status"));
    DLListModel->setHeaderData(PRIORITY, Qt::Horizontal, tr("Speed / Queue position"));
    DLListModel->setHeaderData(REMAINING, Qt::Horizontal, tr("Remaining"));
    DLListModel->setHeaderData(DOWNLOADTIME, Qt::Horizontal, tr("Download time", "i.e: Estimated Time of Arrival / Time left"));
    DLListModel->setHeaderData(ID, Qt::Horizontal, tr("Core-ID"));
    ui.downloadList->setModel(DLListModel);
    ui.downloadList->hideColumn(ID);
    DLDelegate = new DLListDelegate();
    ui.downloadList->setItemDelegate(DLDelegate);

    ui.downloadList->setAutoScroll(false) ;

  	//Selection Setup
    selection = ui.downloadList->selectionModel();

    ui.downloadList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui.downloadList->setRootIsDecorated(true);


    /* Set header resize modes and initial section sizes Downloads TreeView*/
    QHeaderView * _header = ui.downloadList->header () ;
    _header->setResizeMode (NAME, QHeaderView::Interactive);
    _header->setResizeMode (SIZE, QHeaderView::Interactive);
    _header->setResizeMode (COMPLETED, QHeaderView::Interactive);
    _header->setResizeMode (DLSPEED, QHeaderView::Interactive);
    _header->setResizeMode (PROGRESS, QHeaderView::Interactive);
    _header->setResizeMode (SOURCES, QHeaderView::Interactive);
    _header->setResizeMode (STATUS, QHeaderView::Interactive);
    _header->setResizeMode (PRIORITY, QHeaderView::Interactive);
    _header->setResizeMode (REMAINING, QHeaderView::Interactive);

    _header->resizeSection ( NAME, 170 );
    _header->resizeSection ( SIZE, 70 );
    _header->resizeSection ( COMPLETED, 75 );
    _header->resizeSection ( DLSPEED, 75 );
    _header->resizeSection ( PROGRESS, 170 );
    _header->resizeSection ( SOURCES, 90 );
    _header->resizeSection ( STATUS, 100 );
    _header->resizeSection ( PRIORITY, 100 );
    _header->resizeSection ( REMAINING, 100 );

    // Set Upload list model
    ULListModel = new QStandardItemModel(0,8);
    ULListModel->setHeaderData(UNAME, Qt::Horizontal, tr("Name", "i.e: file name"));
    ULListModel->setHeaderData(USIZE, Qt::Horizontal, tr("Size", "i.e: file size"));
    ULListModel->setHeaderData(USERNAME, Qt::Horizontal, tr("Peer", "i.e: user name"));
    ULListModel->setHeaderData(UPROGRESS, Qt::Horizontal, tr("Progress", "i.e: % uploaded"));
    ULListModel->setHeaderData(ULSPEED, Qt::Horizontal, tr("Speed", "i.e: upload speed"));
    ULListModel->setHeaderData(USTATUS, Qt::Horizontal, tr("Status"));
    ULListModel->setHeaderData(UTRANSFERRED, Qt::Horizontal, tr("Transferred", ""));
    ULListModel->setHeaderData(UHASH, Qt::Horizontal, tr("Hash", ""));
    ui.uploadsList->setModel(ULListModel);
    ULDelegate = new ULListDelegate();
    ui.uploadsList->setItemDelegate(ULDelegate);

    ui.uploadsList->setAutoScroll(false) ;

    ui.uploadsList->setRootIsDecorated(false);


  	//Selection Setup
	  selectionup = ui.uploadsList->selectionModel();
	  ui.uploadsList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    /* Set header resize modes and initial section sizes Uploads TreeView*/
    QHeaderView * upheader = ui.uploadsList->header () ;
    upheader->setResizeMode (UNAME, QHeaderView::Interactive);
    upheader->setResizeMode (USIZE, QHeaderView::Interactive);
    upheader->setResizeMode (UTRANSFERRED, QHeaderView::Interactive);
    upheader->setResizeMode (ULSPEED, QHeaderView::Interactive);
    upheader->setResizeMode (UPROGRESS, QHeaderView::Interactive);
    upheader->setResizeMode (USTATUS, QHeaderView::Interactive);
    upheader->setResizeMode (USERNAME, QHeaderView::Interactive);

    upheader->resizeSection ( UNAME, 170 );
    upheader->resizeSection ( USIZE, 70 );
    upheader->resizeSection ( UTRANSFERRED, 75 );
    upheader->resizeSection ( ULSPEED, 75 );
    upheader->resizeSection ( UPROGRESS, 170 );
    upheader->resizeSection ( USTATUS, 100 );
    upheader->resizeSection ( USERNAME, 75 );

    connect(upheader, SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this, SLOT(saveSortIndicatorUpl(int, Qt::SortOrder)));

    FileTransferInfoWidget *ftiw = new FileTransferInfoWidget();
    ui.fileTransferInfoWidget->setWidget(ftiw);
    ui.fileTransferInfoWidget->setWidgetResizable(true);
    ui.fileTransferInfoWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.fileTransferInfoWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui.fileTransferInfoWidget->viewport()->setBackgroundRole(QPalette::NoRole);
    ui.fileTransferInfoWidget->setFrameStyle(QFrame::NoFrame);
    ui.fileTransferInfoWidget->setFocusPolicy(Qt::NoFocus);

    QObject::connect(ui.downloadList,SIGNAL(clicked(const QModelIndex&)),this,SLOT(showFileDetails())) ;

    TurtleRouterDialog *trdl = new TurtleRouterDialog();
    ui.tunnelInfoWidget->setWidget(trdl);
    ui.tunnelInfoWidget->setWidgetResizable(true);
    ui.tunnelInfoWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.tunnelInfoWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui.tunnelInfoWidget->viewport()->setBackgroundRole(QPalette::NoRole);
    ui.tunnelInfoWidget->setFrameStyle(QFrame::NoFrame);
    ui.tunnelInfoWidget->setFocusPolicy(Qt::NoFocus);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif

	 QObject::connect(ui._showCacheTransfers_CB,SIGNAL(toggled(bool)),this,SLOT(insertTransfers())) ;

}

void TransfersDialog::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Delete)
	{
		cancel() ;
		e->accept() ;
	}
	else
		RsAutoUpdatePage::keyPressEvent(e) ;
}

void TransfersDialog::downloadListCostumPopupMenu( QPoint point )
{
	QMenu contextMnu( this );
	QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

	std::set<QStandardItem *> items;
	std::set<QStandardItem *>::iterator it;
	getIdOfSelectedItems(items);

	bool single = (items.size() == 1) ;

	/* check which item is selected
	 * - if it is completed - play should appear in menu
	 */
	std::cerr << "TransfersDialog::downloadListCostumPopupMenu()" << std::endl;
	
	FileInfo info;

	for (it = items.begin(); it != items.end(); it ++) {
		if (!rsFiles->FileDetails((*it)->data(Qt::DisplayRole).toString().toStdString(), RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	bool addPlayOption = false;
	bool addOpenFileOption = false;

  if  (info.downloadStatus == FT_STATE_COMPLETE)
	{
		std::cerr << "Add Play Option" << std::endl;
		
		addOpenFileOption = true;
				
		size_t pos = info.fname.find_last_of('.');

    if (pos ==  std::string::npos) return;	/* can't identify type of file */
    
		/* check if the file is a media file */
    if (misc::isPreviewable(info.fname.substr(pos + 1).c_str()))
    {
		addPlayOption = true;
		}
		

	}
		

	QAction *playAct = NULL;
	if (addPlayOption)
	{
		playAct = new QAction(QIcon(IMAGE_PLAY), tr( "Play" ), this );
		connect( playAct , SIGNAL( triggered() ), this, SLOT( openTransfer() ) );
	}

        pauseAct = new QAction(QIcon(IMAGE_PAUSE), tr("Pause"), this);
        connect(pauseAct, SIGNAL(triggered()), this, SLOT(pauseFileTransfer()));

        resumeAct = new QAction(QIcon(IMAGE_RESUME), tr("Resume"), this);
        connect(resumeAct, SIGNAL(triggered()), this, SLOT(resumeFileTransfer()));

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

#ifndef RS_RELEASE_VERSION
	copylinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy retroshare Link" ), this );
	connect( copylinkAct , SIGNAL( triggered() ), this, SLOT( copyLink() ) );
#endif

	pastelinkAct = new QAction(QIcon(IMAGE_PASTELINK), tr( "Paste retroshare Link" ), this );
	connect( pastelinkAct , SIGNAL( triggered() ), this, SLOT( pasteLink() ) );

	QMenu *viewMenu = new QMenu( tr("View"), this );

//	clearQueueAct = new QAction(QIcon(), tr("Remove all queued"), this);
//	connect(clearQueueAct, SIGNAL(triggered()), this, SLOT(clearQueue()));

	queueDownAct = new QAction(QIcon(":/images/go-down.png"), tr("Down"), this);
	connect(queueDownAct, SIGNAL(triggered()), this, SLOT(priorityQueueDown()));
	queueUpAct = new QAction(QIcon(":/images/go-up.png"), tr("Up"), this);
	connect(queueUpAct, SIGNAL(triggered()), this, SLOT(priorityQueueUp()));
	queueTopAct = new QAction(QIcon(":/images/go-top.png"), tr("Top"), this);
	connect(queueTopAct, SIGNAL(triggered()), this, SLOT(priorityQueueTop()));
	queueBottomAct = new QAction(QIcon(":/images/go-bottom.png"), tr("Bottom"), this);
	connect(queueBottomAct, SIGNAL(triggered()), this, SLOT(priorityQueueBottom()));

	prioritySlowAct = new QAction(QIcon(IMAGE_PRIORITYLOW), tr("Slower"), this);
	connect(prioritySlowAct, SIGNAL(triggered()), this, SLOT(speedSlow()));
	priorityMediumAct = new QAction(QIcon(IMAGE_PRIORITYNORMAL), tr("Average"), this);
	connect(priorityMediumAct, SIGNAL(triggered()), this, SLOT(speedAverage()));
	priorityFastAct = new QAction(QIcon(IMAGE_PRIORITYHIGH), tr("Faster"), this);
	connect(priorityFastAct, SIGNAL(triggered()), this, SLOT(speedFast()));

	QMenu *priorityQueueMenu = new QMenu(tr("Move in Queue..."), this);
	priorityQueueMenu->setIcon(QIcon(IMAGE_PRIORITY));
	priorityQueueMenu->addAction(queueTopAct);
	priorityQueueMenu->addAction(queueUpAct);
	priorityQueueMenu->addAction(queueDownAct);
	priorityQueueMenu->addAction(queueBottomAct);

	QMenu *prioritySpeedMenu = new QMenu(tr("Priority (Speed)..."), this);
	prioritySpeedMenu->setIcon(QIcon(IMAGE_PRIORITY));
	prioritySpeedMenu->addAction(prioritySlowAct);
	prioritySpeedMenu->addAction(priorityMediumAct);
	prioritySpeedMenu->addAction(priorityFastAct);

	chunkStreamingAct = new QAction(QIcon(IMAGE_PRIORITYAUTO), tr("Streaming"), this);
	connect(chunkStreamingAct, SIGNAL(triggered()), this, SLOT(chunkStreaming()));
	chunkRandomAct = new QAction(QIcon(IMAGE_PRIORITYAUTO), tr("Random"), this);
	connect(chunkRandomAct, SIGNAL(triggered()), this, SLOT(chunkRandom()));

	QMenu *chunkMenu = new QMenu(tr("Chunk strategy"), this);
	chunkMenu->setIcon(QIcon(IMAGE_PRIORITY));
	chunkMenu->addAction(chunkStreamingAct);
	chunkMenu->addAction(chunkRandomAct);

	contextMnu.clear();

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
			contextMnu.addMenu(prioritySpeedMenu);
		else if(all_queued)
			contextMnu.addMenu(priorityQueueMenu) ;

		if(all_downloading)
			contextMnu.addMenu( chunkMenu);

	if(single)
	{
		if(info.downloadStatus == FT_STATE_PAUSED)
			contextMnu.addAction( resumeAct);
		else if(info.downloadStatus != FT_STATE_COMPLETE)
			contextMnu.addAction( pauseAct);
	}		

		if(info.downloadStatus != FT_STATE_COMPLETE)
			contextMnu.addAction( cancelAct);

		contextMnu.addSeparator();
	}

	if(single)
	{
		if (addOpenFileOption)
			contextMnu.addAction( openfileAct);

#ifndef RS_RELEASE_VERSION		
		contextMnu.addAction( previewfileAct);
#endif		
		contextMnu.addAction( openfolderAct);
		contextMnu.addAction( detailsfileAct);
		contextMnu.addSeparator();
	}

	contextMnu.addAction( clearcompletedAct);
	contextMnu.addSeparator();
#ifndef RS_RELEASE_VERSION
	if(single)
		contextMnu.addAction( copylinkAct);
#endif
	if(!RSLinkClipboard::empty())
		contextMnu.addAction( pastelinkAct);

	contextMnu.addSeparator();
//	contextMnu.addAction( clearQueueAct);
	contextMnu.addSeparator();
	contextMnu.addMenu( viewMenu);

	contextMnu.exec( mevent->globalPos() );
}

TransfersDialog::~TransfersDialog()
{
	;
}


int TransfersDialog::addItem(const QString&, const QString& name, const QString& coreID, qlonglong fileSize, const FileProgressInfo& pinfo, double dlspeed,
		const QString& sources,  const QString& status, const QString& priority, qlonglong completed, qlonglong remaining, qlonglong downloadtime)
{
    int row;
    QList<QStandardItem *> list = DLListModel->findItems(coreID, Qt::MatchExactly, ID);
    if (list.size() > 0) {
        row = list.front()->row();
    } else {
        row = DLListModel->rowCount();
        DLListModel->insertRow(row);
    }

    DLListModel->setData(DLListModel->index(row, NAME), QVariant(name));
    DLListModel->setData(DLListModel->index(row, SIZE), QVariant((qlonglong)fileSize));
    DLListModel->setData(DLListModel->index(row, COMPLETED), QVariant((qlonglong)completed));
    DLListModel->setData(DLListModel->index(row, DLSPEED), QVariant((double)dlspeed));
    DLListModel->setData(DLListModel->index(row, PROGRESS), QVariant::fromValue(pinfo));
    DLListModel->setData(DLListModel->index(row, SOURCES), QVariant((QString)sources));
    DLListModel->setData(DLListModel->index(row, STATUS), QVariant((QString)status));
    DLListModel->setData(DLListModel->index(row, PRIORITY), QVariant((QString)priority));
    DLListModel->setData(DLListModel->index(row, REMAINING), QVariant((qlonglong)remaining));
    DLListModel->setData(DLListModel->index(row, DOWNLOADTIME), QVariant((qlonglong)downloadtime));
    DLListModel->setData(DLListModel->index(row, ID), QVariant((QString)coreID));

    QString ext = QFileInfo(name).suffix();
    if (ext == "jpg" || ext == "jpeg" || ext == "tif" || ext == "tiff" || ext == "JPG"|| ext == "png" || ext == "gif"
    || ext == "bmp" || ext == "ico" || ext == "svg") {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypePicture.png")), Qt::DecorationRole);
    } else if (ext == "avi" ||ext == "AVI" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "divx" || ext == "ts"
    || ext == "mkv" || ext == "mp4" || ext == "flv" || ext == "mov" || ext == "asf" || ext == "xvid"
    || ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp" || ext == "mpeg" || ext == "ogm") {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeVideo.png")), Qt::DecorationRole);
    } else if (ext == "ogg" || ext == "mp3" || ext == "MP3"  || ext == "mp1" || ext == "mp2" || ext == "wav" || ext == "wma") {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeAudio.png")), Qt::DecorationRole);
    } else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "gz" || ext == "7z" || ext == "msi"
    || ext == "rar" || ext == "rpm" || ext == "ace" || ext == "jar" || ext == "tgz" || ext == "lha"
    || ext == "cab" || ext == "cbz"|| ext == "cbr" || ext == "alz" || ext == "sit" || ext == "arj" || ext == "deb") {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeArchive.png")), Qt::DecorationRole);
    }else if (ext == "app" || ext == "bat" || ext == "cgi" || ext == "com"
    || ext == "exe" || ext == "js" || ext == "pif"
    || ext == "py" || ext == "pl" || ext == "sh" || ext == "vb" || ext == "ws") {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeProgram.png")), Qt::DecorationRole);
    } else if (ext == "iso" || ext == "nrg" || ext == "mdf" || ext == "img" || ext == "dmg" || ext == "bin" ) {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeCDImage.png")), Qt::DecorationRole);
    } else if (ext == "txt" || ext == "cpp" || ext == "c" || ext == "h") {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")), Qt::DecorationRole);
    } else if (ext == "doc" || ext == "rtf" || ext == "sxw" || ext == "xls" || ext == "pps" || ext == "xml"
    || ext == "sxc" || ext == "odt" || ext == "ods" || ext == "dot" || ext == "ppt" || ext == "css"  ) {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")), Qt::DecorationRole);
    } else if (ext == "html" || ext == "htm" || ext == "php") {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")), Qt::DecorationRole);
    } else {
        DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeAny.png")), Qt::DecorationRole);
    }

    return row;
}

int TransfersDialog::addPeerToItem(int row, const QString& name, const QString& coreID, double dlspeed, const QString& status, const FileProgressInfo& peerInfo)
{
    QStandardItem *dlItem = DLListModel->item(row);
    if (!dlItem) return -1;

    //try to find the item
    int childRow = -1;
    int count = 0;
	 QStandardItem* childId=NULL ;

    for(count=0; (childId = dlItem->child(count, ID)) != NULL;++count) 
        if (childId->data(Qt::DisplayRole).toString() == coreID) 
		  {
            childRow = count;
            break;
        }

    if (childRow == -1) {
        //set this false if you want to expand on double click
        dlItem->setEditable(false);

        QList<QStandardItem *> items;
        QStandardItem *i1 = new QStandardItem(); i1->setData(QVariant((QString)" "+name), Qt::DisplayRole);
        QStandardItem *i2 = new QStandardItem(); i2->setData(QVariant(QString()), Qt::DisplayRole);
        QStandardItem *i3 = new QStandardItem(); i3->setData(QVariant(QString()), Qt::DisplayRole);
        QStandardItem *i4 = new QStandardItem(); i4->setData(QVariant((double)dlspeed), Qt::DisplayRole);
        QStandardItem *i5 = new QStandardItem(); i5->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);
        QStandardItem *i6 = new QStandardItem(); i6->setData(QVariant(QString()), Qt::DisplayRole);
        QStandardItem *i7 = new QStandardItem(); i7->setData(QVariant((QString)status), Qt::DisplayRole);
        QStandardItem *i8 = new QStandardItem(); i8->setData(QVariant(QString()), Qt::DisplayRole);	// blank field for priority
        QStandardItem *i9 = new QStandardItem(); i9->setData(QVariant(QString()), Qt::DisplayRole);
        QStandardItem *i10 = new QStandardItem(); i10->setData(QVariant(QString()), Qt::DisplayRole);
        QStandardItem *i11 = new QStandardItem(); i11->setData(QVariant((QString)coreID), Qt::DisplayRole);

        /* set status icon in the name field */
        if (status == "Downloading") {
            i1->setData(QIcon(QString::fromUtf8(":/images/Client0.png")), Qt::DecorationRole);
        } else if (status == "Failed") {
            i1->setData(QIcon(QString::fromUtf8(":/images/Client1.png")), Qt::DecorationRole);
        } else if (status == "Okay") {
            i1->setData(QIcon(QString::fromUtf8(":/images/Client2.png")), Qt::DecorationRole);
        } else if (status == "Waiting") {
            i1->setData(QIcon(QString::fromUtf8(":/images/Client3.png")), Qt::DecorationRole);
        } else if (status == "Unknown") {
            i1->setData(QIcon(QString::fromUtf8(":/images/Client4.png")), Qt::DecorationRole);
        } else if (status == "Complete") {
        }

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

		  childRow = dlItem->rowCount()-1 ;
    } else {
        //just update the child (peer)
        dlItem->child(childRow, DLSPEED)->setData(QVariant((double)dlspeed), Qt::DisplayRole);
        dlItem->child(childRow, STATUS)->setData(QVariant((QString)status), Qt::DisplayRole);
        dlItem->child(childRow, PROGRESS)->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);
        
        std::set<QStandardItem *> dlitems;
        std::set<QStandardItem *>::iterator it;
        getIdOfSelectedItems(dlitems);
	
        FileInfo info;
        for (it = dlitems.begin(); it != dlitems.end(); it ++) {
            if (!rsFiles->FileDetails((*it)->data(Qt::DisplayRole).toString().toStdString(), RS_FILE_HINTS_DOWNLOAD, info)) continue;
            break;
        }
        
        /* set status icon in the name field */
        if ( info.downloadStatus == FT_STATE_DOWNLOADING) 
        {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client0.png")), Qt::DecorationRole);
        } 
        else if ( info.downloadStatus == FT_STATE_FAILED) 
        {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client1.png")), Qt::DecorationRole);
        }  
        else if ( info.downloadStatus == FT_STATE_OKAY) 
        {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client2.png")), Qt::DecorationRole);
        } 
        else if ( info.downloadStatus == FT_STATE_WAITING) 
        {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client3.png")), Qt::DecorationRole);
        } 
        else if ( info.downloadStatus == FT_STATE_COMPLETE)
        {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client0.png")), Qt::DecorationRole);
        } 
        else 
        {
            //dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client4.png")), Qt::DecorationRole);
        }
        
        /* set status icon in the name field */
        /*if (status == "Downloading") {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client0.png")), Qt::DecorationRole);
        } else if (status == "Failed") {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client1.png")), Qt::DecorationRole);
        } else if (status == "Okay") {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client2.png")), Qt::DecorationRole);
        } else if (status == "Waiting") {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client3.png")), Qt::DecorationRole);
        } else if (status == "Unknown") {
            dlItem->child(childRow, NAME)->setData(QIcon(QString::fromUtf8(":/images/Client4.png")), Qt::DecorationRole);
        } else if (status == "Complete") {
        }*/
    }

    return childRow;
}


int TransfersDialog::addUploadItem(const QString&, const QString& name, const QString& coreID, qlonglong fileSize, const FileProgressInfo& pinfo, double dlspeed, const QString& source,  const QString& status, qlonglong completed, qlonglong)
{
    int row;
    QList<QStandardItem *> list = ULListModel->findItems(coreID, Qt::MatchExactly, UHASH);
    if (list.size() > 0) {
        row = list.front()->row();
    } else {
        row = ULListModel->rowCount();
        ULListModel->insertRow(row);
    }

    ULListModel->setData(ULListModel->index(row, UNAME), QVariant((QString)" "+name), Qt::DisplayRole);
    ULListModel->setData(ULListModel->index(row, USIZE), QVariant((qlonglong)fileSize));
    ULListModel->setData(ULListModel->index(row, UTRANSFERRED), QVariant((qlonglong)completed));
    ULListModel->setData(ULListModel->index(row, ULSPEED), QVariant((double)dlspeed));
    ULListModel->setData(ULListModel->index(row, UPROGRESS), QVariant::fromValue(pinfo));
    ULListModel->setData(ULListModel->index(row, USTATUS), QVariant((QString)status));
    ULListModel->setData(ULListModel->index(row, USERNAME), QVariant((QString)source));
    ULListModel->setData(ULListModel->index(row, UHASH), QVariant((QString)coreID));

    return row;
}


void TransfersDialog::delItem(int row)
{
	DLListModel->removeRow(row, QModelIndex());
}

void TransfersDialog::delUploadItem(int row)
{
	ULListModel->removeRow(row, QModelIndex());
}

	/* get the list of Transfers from the RsIface.  **/
void TransfersDialog::updateDisplay()
{
	insertTransfers();
}
void TransfersDialog::insertTransfers() 
{
	/* get the download lists */
	std::list<std::string> downHashes;
	rsFiles->FileDownloads(downHashes);

//	std::list<DwlDetails> dwlDetails;
//	rsFiles->getDwlDetails(dwlDetails);



	std::set<std::string> used_hashes ;

	// clear all source peers.

	std::list<std::string>::iterator it;
	for (it = downHashes.begin(); it != downHashes.end(); it++) 
	{
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) {
			//if file transfer is a cache file index file, don't show it
			continue;
		}

		if((info.flags & CB_CODE_CACHE) && !ui._showCacheTransfers_CB->isChecked())
			continue;

		QString fileName = QString::fromUtf8(info.fname.c_str());
		QString fileHash = QString::fromStdString(info.hash);
		qlonglong fileSize    = info.size;
		double fileDlspeed     = (info.downloadStatus==FT_STATE_PAUSED)?0.0:(info.tfRate * 1024.0);

		/* get the sources (number of online peers) */
		int online = 0;
		std::list<TransferInfo>::iterator pit;
		for (pit = info.peers.begin(); pit != info.peers.end(); pit++) {
			if (rsPeers->isOnline(pit->peerId)) {
				online++;
			}
		}
		QString sources     = QString("%1 (%2)").arg(online).arg(info.peers.size());

		QString status;
		switch (info.downloadStatus) {
			case FT_STATE_FAILED:       status = tr("Failed"); break;
			case FT_STATE_OKAY:         status = tr("Okay"); break;
			case FT_STATE_WAITING:      status = tr("Waiting"); break;
			case FT_STATE_DOWNLOADING:  status = tr("Downloading"); break;
			case FT_STATE_COMPLETE:     status = tr("Complete"); break;
			case FT_STATE_QUEUED:       status = tr("Queued"); break;
			case FT_STATE_PAUSED:       status = tr("Paused"); break;
			default:                    status = tr("Unknown"); break;
		}

		QString priority;

		if(info.downloadStatus == FT_STATE_QUEUED)
			priority = QString::number(info.queue_position) ;
		else
			switch (info.priority) 
			{
				case SPEED_LOW:     priority = tr("Slower");break;
				case SPEED_NORMAL:  priority = tr("Average");break;
				case SPEED_HIGH:    priority = tr("Faster");break;
				default:            priority = tr("Average");break;
			}

		qlonglong completed    = info.transfered;
		qlonglong remaining    = info.size - info.transfered;
		qlonglong downloadtime = (info.size - info.transfered) / (info.tfRate * 1024.0);

		FileChunksInfo fcinfo ;
		if(!rsFiles->FileDownloadChunksDetails(*it,fcinfo))
			continue ;

		FileProgressInfo pinfo ;
		pinfo.cmap = fcinfo.chunks ;
		pinfo.type = FileProgressInfo::DOWNLOAD_LINE ;
		pinfo.progress = completed*100.0/info.size ;
		pinfo.nb_chunks = pinfo.cmap._map.empty()?0:fcinfo.chunks.size() ;

		int addedRow = addItem("", fileName, fileHash, fileSize, pinfo, fileDlspeed, sources, status, priority, completed, remaining, downloadtime);
		used_hashes.insert(info.hash) ;

		/* continue to next download item if no peers to add */
		if (!info.peers.size()) continue;

		std::map<std::string, std::string>::iterator vit;
		std::map<std::string, std::string> versions;
		bool retv = rsDisc->getDiscVersions(versions);

		std::set<int> used_rows ;

		if(info.downloadStatus != FT_STATE_COMPLETE)
		for (pit = info.peers.begin(); pit != info.peers.end(); pit++) 
		{
			QString peerName        = getPeerName(pit->peerId);
			//unique combination: fileHash + peerId, variant: hash + peerName (too long)
			QString hashFileAndPeerId      = fileHash + QString::fromStdString(pit->peerId);
			QString version;
			if (retv && versions.end() != (vit = versions.find(pit->peerId))) {
				version = tr("version: ") + QString::fromStdString(vit->second);
			}

			QString status;
			switch (pit->status) {
				case FT_STATE_FAILED:       status = tr("Failed"); break;
				case FT_STATE_OKAY:         status = tr("Okay"); break;
				case FT_STATE_WAITING:      status = tr(""); break;
				case FT_STATE_DOWNLOADING:  status = tr("Transferring"); break;
				case FT_STATE_COMPLETE:     status = tr("Complete"); break;
				default:                    status = tr(""); break;
			}
			double peerDlspeed	= 0;
			if ((uint32_t)pit->status == FT_STATE_DOWNLOADING && info.downloadStatus != FT_STATE_PAUSED && info.downloadStatus != FT_STATE_COMPLETE) 
				peerDlspeed     = pit->tfRate * 1024.0;

			FileProgressInfo peerpinfo ;
			peerpinfo.cmap = fcinfo.compressed_peer_availability_maps[pit->peerId];
			peerpinfo.type = FileProgressInfo::DOWNLOAD_SOURCE ;
			peerpinfo.progress = 0.0 ;	// we don't display completion for sources.
			peerpinfo.nb_chunks = peerpinfo.cmap._map.empty()?0:fcinfo.chunks.size();

			//					 std::cerr << std::endl ;
			//					 std::cerr << "Source " << pit->peerId << " as map " << peerpinfo.cmap._map.size() << " compressed chunks" << std::endl ;
			//						 for(uint j=0;j<peerpinfo.cmap._map.size();++j)
			//							 std::cerr << peerpinfo.cmap._map[j] ;
			//					 std::cerr << std::endl ;
			//					 std::cerr << std::endl ;

//				std::cout << "adding peer " << peerName.toStdString() << " to row " << addedRow << ", hashfile and peerid=" << hashFileAndPeerId.toStdString() << std::endl ;
			int row_id = addPeerToItem(addedRow, peerName, hashFileAndPeerId, peerDlspeed, status, peerpinfo);

			used_rows.insert(row_id) ;
		}

		QStandardItem *dlItem = DLListModel->item(addedRow);

		// This is not optimal, but we deal with a small number of elements. The reverse order is really important,
		// because rows after the deleted rows change positions !
		//
		for(int r=dlItem->rowCount()-1;r>=0;--r)
			if(used_rows.find(r) == used_rows.end())
				dlItem->takeRow(r) ;
	}
	// remove hashes that where not shown
	//first clean the model in case some files are not download anymore
	//remove items that are not fiends anymore
	int removeIndex = 0;
	while (removeIndex < DLListModel->rowCount()) 
	{
		std::string hash = DLListModel->item(removeIndex, ID)->data(Qt::EditRole).toString().toStdString();

		if(used_hashes.find(hash) == used_hashes.end())
			DLListModel->takeRow(removeIndex);
		 else 
			removeIndex++;
	}

	// Now show upload hashes
	//
	std::list<std::string> upHashes;
	rsFiles->FileUploads(upHashes);
	//first clean the model in case some files are not uploaded anymore
	//remove items that are not fiends anymore
	removeIndex = 0;
	while (removeIndex < ULListModel->rowCount()) {
		if (!ULListModel->item(removeIndex, UHASH)) {
			removeIndex++;
			continue;
		}
		std::string hash = ULListModel->item(removeIndex, UHASH)->data(Qt::EditRole).toString().toStdString();
		std::list<std::string>::iterator upHashesIt;
		bool found = false;
		for (upHashesIt =  upHashes.begin(); upHashesIt != upHashes.end(); upHashesIt++) {
			if (hash == *upHashesIt) {
				found = true;
				break;
			}
		}
		if (!found) {
			ULListModel->takeRow(removeIndex);
		} else {
			removeIndex++;
		}
	}

	for(it = upHashes.begin(); it != upHashes.end(); it++) {
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_UPLOAD, info)) {
			continue;
		}
		if((info.flags & CB_CODE_CACHE) && !ui._showCacheTransfers_CB->isChecked())
			continue ;

		std::list<TransferInfo>::iterator pit;
		for(pit = info.peers.begin(); pit != info.peers.end(); pit++) {
			if (pit->peerId == rsPeers->getOwnId()) //don't display transfer to ourselves
				continue ;

			QString fileHash        = QString::fromStdString(info.hash);
			QString fileName    	= QString::fromUtf8(info.fname.c_str());
			QString sources		= getPeerName(pit->peerId);

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
			double completed 	= info.transfered;
			double progress 	= info.transfered * 100.0 / info.size;
			qlonglong remaining   = (info.size - info.transfered) / (info.tfRate * 1024.0);

			// Estimate the completion. We need something more accurate, meaning that we need to
			// transmit the completion info.
			//
			uint32_t chunk_size = 1024*1024 ;
			uint32_t nb_chunks = (uint32_t)(info.size / (uint64_t)(chunk_size) ) ;
			if((info.size % (uint64_t)chunk_size) != 0)
				++nb_chunks ;

			uint32_t filled_chunks = pinfo.cmap.filledChunks(nb_chunks) ;
			pinfo.nb_chunks = pinfo.cmap._map.empty()?0:nb_chunks ;

			if(filled_chunks > 1) {
				pinfo.progress = filled_chunks*100.0/nb_chunks ;
				completed = std::min(info.size,((uint64_t)filled_chunks)*chunk_size) ;
			} else {
				pinfo.progress = progress ;
			}

			addUploadItem("", fileName, fileHash, fileSize, pinfo, dlspeed, sources,  status, completed, remaining);
		}

		if (info.peers.size() == 0) { //it has not been added (maybe only turtle tunnels
			QString fileHash    = QString::fromStdString(info.hash);
			QString fileName    	= QString::fromUtf8(info.fname.c_str());
			QString sources		= tr("");

			QString status;
			switch(info.downloadStatus)
			{
				case FT_STATE_FAILED: status = tr("Failed"); break;
				case FT_STATE_OKAY: status = tr("Okay"); break;
				case FT_STATE_WAITING: status = tr("Waiting"); break;
				case FT_STATE_DOWNLOADING: status = tr("Uploading");break;
				case FT_STATE_COMPLETE:status = tr("Complete"); break;
				default: status = tr("Complete"); break;

			}

			double dlspeed  	= info.tfRate * 1024.0;
			qlonglong fileSize 	= info.size;
			double completed 	= info.transfered;
			double progress 	= info.transfered * 100.0 / info.size;
			qlonglong remaining   = (info.size - info.transfered) / (info.tfRate * 1024.0);

			FileProgressInfo pinfo ;
			pinfo.progress = progress ;
			pinfo.cmap = CompressedChunkMap() ;
			pinfo.type = FileProgressInfo::DOWNLOAD_LINE ;
			pinfo.nb_chunks = 0 ;

			addUploadItem("", fileName, fileHash, fileSize, pinfo, dlspeed, sources,  status, completed, remaining);
		}
	}
}

QString TransfersDialog::getPeerName(const std::string& id) const
{
	QString res = QString::fromStdString(rsPeers->getPeerName(id)) ;

	// This is because turtle tunnels have no name (I didn't want to bother with
	// connect mgr). In such a case their id can suitably hold for a name.
	//
	if(res == "")
		return QString::fromStdString(id) ;
	else
		return res ;
}

void TransfersDialog::cancel()
{
	QString queryWrn2;
	queryWrn2.clear();
	queryWrn2.append(tr("Are you sure that you want to cancel and delete these files?"));

	if ((QMessageBox::question(this, tr("RetroShare"),queryWrn2,QMessageBox::Ok|QMessageBox::No, QMessageBox::Ok))== QMessageBox::Ok)
		for(int i = 0; i <= DLListModel->rowCount(); i++)
			if(selection->isRowSelected(i, QModelIndex()))
			{
				std::string id = getID(i, DLListModel).toStdString();

				rsFiles->FileCancel(id); /* hash */
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
	QModelIndexList lst = ui.downloadList->selectionModel ()->selectedIndexes ();

	for (int i = 0; i < lst.count (); i++)
		if ( lst[i].column() == 0 )
		{
			QModelIndex & ind = lst[i];
			QString fhash= ind.model ()->data (ind.model ()->index (ind.row (), ID )).toString() ;
			qulonglong fsize= ind.model ()->data (ind.model ()->index (ind.row (), SIZE)).toULongLong() ;
			QString fname= ind.model ()->data (ind.model ()->index (ind.row (), NAME)).toString() ;

			RetroShareLink link(fname, fsize, fhash);

			QApplication::clipboard()->setText(link.toString());
			break ;
		}
}

void TransfersDialog::showDetailsDialog()
{
    static DetailsDialog *detailsdlg = new DetailsDialog();
    
    QModelIndexList lst = ui.downloadList->selectionModel ()->selectedIndexes ();
    
    std::string file_hash ;

    for (int i = 0; i < lst.count (); i++)
    {
        if (lst[i].column () == 0)
        {
            QModelIndex& ind = lst[i];
            QString fhash = ind.model ()->data (ind.model ()->index (ind.row (), ID )).toString() ;
            QString fsize = ind.model ()->data (ind.model ()->index (ind.row (), SIZE)).toString() ;
            QString fname = ind.model ()->data (ind.model ()->index (ind.row (), NAME)).toString() ;
            QString fstatus = ind.model ()->data (ind.model ()->index (ind.row (), STATUS)).toString() ;
            QString fpriority = ind.model ()->data (ind.model ()->index (ind.row (), PRIORITY)).toString() ;
            QString fsources= ind.model ()->data (ind.model ()->index (ind.row (), SOURCES)).toString() ;
            
            qulonglong filesize = ind.model ()->data (ind.model ()->index (ind.row (), SIZE)).toULongLong() ;
            double fdatarate = ind.model ()->data (ind.model ()->index (ind.row (), DLSPEED)).toDouble() ;            
            qulonglong fcompleted = ind.model ()->data (ind.model ()->index (ind.row (), COMPLETED)).toULongLong() ;
            qulonglong fremaining = ind.model ()->data (ind.model ()->index (ind.row (), REMAINING)).toULongLong() ;
            
            qulonglong fdownloadtime = ind.model ()->data (ind.model ()->index (ind.row (), DOWNLOADTIME)).toULongLong() ;
            
            int nb_select = 0 ;
            
            for(int i = 0; i <= DLListModel->rowCount(); i++) 
            if(selection->isRowSelected(i, QModelIndex())) 
            {
              file_hash = getID(i, DLListModel).toStdString();
              ++nb_select ;
            }
            
            detailsdlg->setFileHash(file_hash);
            
            // Set Details.. Window Title
            detailsdlg->setWindowTitle(tr("Details:") + fname);

            // General GroupBox
            detailsdlg->setHash(fhash);
            detailsdlg->setFileName(fname);
            detailsdlg->setSize(filesize);
            detailsdlg->setStatus(fstatus);
            detailsdlg->setPriority(fpriority);
            detailsdlg->setType(QFileInfo(fname).suffix());
            
            // Transfer GroupBox
            detailsdlg->setSources(fsources);
            detailsdlg->setDatarate(fdatarate);
            detailsdlg->setCompleted(fcompleted);
            detailsdlg->setRemaining(fremaining);
            
            //Date GroupBox
            detailsdlg->setDownloadtime(fdownloadtime);
            
            // retroshare link(s) Tab
            RetroShareLink link(fname, filesize, fhash);
            detailsdlg->setLink(link.toString());
            
            detailsdlg->show();
            break;
        }
    }
}

void TransfersDialog::pasteLink()
{
	const std::vector<RetroShareLink>& links(RSLinkClipboard::pasteLinks()) ;

	for(uint32_t i=0;i<links.size();++i)
		if (links[i].valid())
			if(!rsFiles->FileRequest(links[i].name().toStdString(), links[i].hash().toStdString(),links[i].size(), "", RS_FILE_HINTS_NETWORK_WIDE, std::list<std::string>()))
				QMessageBox::critical(NULL,"Download refused","The file "+links[i].name()+" could not be downloaded. Do you already have it ?") ;
}

void TransfersDialog::getIdOfSelectedItems(std::set<QStandardItem *>& items)
{
	items.clear();

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
			QStandardItem *id = DLListModel->item(i, ID);
			items.insert(id);
		}
	}
}

bool TransfersDialog::controlTransferFile(uint32_t flags)
{
	bool result = true;

	std::set<QStandardItem *> items;
	std::set<QStandardItem *>::iterator it;
	getIdOfSelectedItems(items);
	for (it = items.begin(); it != items.end(); it ++) {
		result &= rsFiles->FileControl((*it)->data(Qt::DisplayRole).toString().toStdString(), flags);
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

	std::set<QStandardItem *> items;
	std::set<QStandardItem *>::iterator it;
	getIdOfSelectedItems(items);
	for (it = items.begin(); it != items.end(); it ++) {
		if (!rsFiles->FileDetails((*it)->data(Qt::DisplayRole).toString().toStdString(), RS_FILE_HINTS_DOWNLOAD, info)) continue;
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
	qinfo.setFile(path.c_str());
	if (qinfo.exists() && qinfo.isDir()) {
		if (!QDesktopServices::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
			std::cerr << "openFolderTransfer(): can't open folder " << path << std::endl;
		}
	}
}

void TransfersDialog::previewTransfer()
{
	FileInfo info;

	std::set<QStandardItem *> items;
	std::set<QStandardItem *>::iterator it;
	getIdOfSelectedItems(items);
	for (it = items.begin(); it != items.end(); it ++) {
		if (!rsFiles->FileDetails((*it)->data(Qt::DisplayRole).toString().toStdString(), RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	size_t pos = info.fname.find_last_of('.');
	if (pos == std::string::npos) 
		return;	/* can't identify type of file */

	/* check if the file is a media file */
	if (!misc::isPreviewable(info.fname.substr(pos + 1).c_str())) return;

	/* make path for downloaded or downloading files */
	bool complete = false;
	std::string path;
	if (info.downloadStatus == FT_STATE_COMPLETE) {
		path = info.path + "/" + info.fname;
		complete = true;
	} else {
		path = rsFiles->getPartialsDirectory() + "/" + info.hash;
	}

	/* open or preview them with a suitable application */
	QFileInfo qinfo;
	if (complete) {
		qinfo.setFile(QString::fromStdString(path));
		if (qinfo.exists()) {
			if (!QDesktopServices::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
				std::cerr << "previewTransfer(): can't preview file " << path << std::endl;
			}
		}
	} else {
		QString linkName = QString::fromStdString(path) +
							QString::fromStdString(info.fname.substr(info.fname.find_last_of('.')));
		if (QFile::link(QString::fromStdString(path), linkName)) {
			qinfo.setFile(linkName);
			if (qinfo.exists()) {
				if (!QDesktopServices::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
					std::cerr << "previewTransfer(): can't preview file " << path << std::endl;
				}
			}
			/* wait for the file to open then remove the link */
#ifdef WIN32
			Sleep(2000);
#else
			sleep(2);
#endif
			QFile::remove(linkName);
		} else {
			std::cerr << "previewTransfer(): can't create link for file " << path << std::endl;
		}
	}
}

void TransfersDialog::openTransfer()
{
	FileInfo info;

	std::set<QStandardItem *> items;
	std::set<QStandardItem *>::iterator it;
	getIdOfSelectedItems(items);
	for (it = items.begin(); it != items.end(); it ++) {
		if (!rsFiles->FileDetails((*it)->data(Qt::DisplayRole).toString().toStdString(), RS_FILE_HINTS_DOWNLOAD, info)) continue;
		break;
	}

	/* make path for downloaded or downloading files */
	std::string path;
	if (info.downloadStatus == FT_STATE_COMPLETE) {
		path = info.path + "/" + info.fname;

		/* open file with a suitable application */
		QFileInfo qinfo;
		qinfo.setFile(path.c_str());
		if (qinfo.exists()) {
			if (!QDesktopServices::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
				std::cerr << "openTransfer(): can't open file " << path << std::endl;
			}
		}
	} else {
		/* rise a message box for incompleted download file */
		QMessageBox::information(this, tr("Open Transfer"),
				tr("File %1 is not completed. If it is a media file, try to preview it.").arg(info.fname.c_str()));
	}
}

/* clear download or all queue - for pending dwls */
//void TransfersDialog::clearQueuedDwl()
//{
//	std::set<QStandardItem *> items;
//	std::set<QStandardItem *>::iterator it;
//	getIdOfSelectedItems(items);
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
void TransfersDialog::setChunkStrategy(FileChunksInfo::ChunkStrategy s)
{
	std::set<QStandardItem *> items;
	std::set<QStandardItem *>::iterator it;
	getIdOfSelectedItems(items);

	for (it = items.begin(); it != items.end(); it ++) {
		std::string hash = (*it)->data(Qt::DisplayRole).toString().toStdString();
		rsFiles->setChunkStrategy(hash, s);
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
	std::set<QStandardItem *> items;
	std::set<QStandardItem *>::iterator it;
	getIdOfSelectedItems(items);

	for (it = items.begin(); it != items.end(); it ++) 
	{
		std::string hash = (*it)->data(Qt::DisplayRole).toString().toStdString();
		rsFiles->changeDownloadSpeed(hash, speed);
	}
}


void TransfersDialog::changeQueuePosition(QueueMove mv)
{
	std::cerr << "In changeQueuePosition (gui)"<< std::endl ;
	std::set<QStandardItem *> items;
	std::set<QStandardItem *>::iterator it;
	getIdOfSelectedItems(items);

	for (it = items.begin(); it != items.end(); it ++) 
	{
		std::string hash = (*it)->data(Qt::DisplayRole).toString().toStdString();
		rsFiles->changeQueuePosition(hash, mv);
	}
}

void TransfersDialog::clearcompleted()
{
    	std::cerr << "TransfersDialog::clearcompleted()" << std::endl;
   	rsFiles->FileClearCompleted();
}

void TransfersDialog::showFileDetails()
{
	std::string file_hash ;
	int nb_select = 0 ;

	std::cout << "new selection " << std::endl ;

	for(int i = 0; i <= DLListModel->rowCount(); i++) 
		if(selection->isRowSelected(i, QModelIndex())) 
		{
	        file_hash = getID(i, DLListModel).toStdString();
			  ++nb_select ;
		}
	if(nb_select != 1)
		dynamic_cast<FileTransferInfoWidget*>(ui.fileTransferInfoWidget->widget())->setFileHash("") ;
	else
		dynamic_cast<FileTransferInfoWidget*>(ui.fileTransferInfoWidget->widget())->setFileHash(file_hash) ;
	
	std::cout << "calling update " << std::endl ;
	dynamic_cast<FileTransferInfoWidget*>(ui.fileTransferInfoWidget->widget())->updateDisplay() ;
	std::cout << "done" << std::endl ;
}

double TransfersDialog::getProgress(int , QStandardItemModel *)
{
//	return model->data(model->index(row, PROGRESS), Qt::DisplayRole).toDouble();
return 0.0 ;
}

double TransfersDialog::getSpeed(int row, QStandardItemModel *model)
{
	return model->data(model->index(row, DLSPEED), Qt::DisplayRole).toDouble();
}

QString TransfersDialog::getFileName(int row, QStandardItemModel *model)
{
	return model->data(model->index(row, NAME), Qt::DisplayRole).toString();
}

QString TransfersDialog::getStatus(int row, QStandardItemModel *model)
{
	return model->data(model->index(row, STATUS), Qt::DisplayRole).toString();
}

QString TransfersDialog::getID(int row, QStandardItemModel *model)
{
	return model->data(model->index(row, ID), Qt::DisplayRole).toString();
}

QString TransfersDialog::getPriority(int row, QStandardItemModel *model)
{
	return model->data(model->index(row, PRIORITY), Qt::DisplayRole).toString();
}

qlonglong TransfersDialog::getFileSize(int row, QStandardItemModel *model)
{
	bool ok = false;
	return model->data(model->index(row, SIZE), Qt::DisplayRole).toULongLong(&ok);
}

qlonglong TransfersDialog::getTransfered(int row, QStandardItemModel *model)
{
	bool ok = false;
	return model->data(model->index(row, COMPLETED), Qt::DisplayRole).toULongLong(&ok);
}

qlonglong TransfersDialog::getRemainingTime(int row, QStandardItemModel *model)
{
	bool ok = false;
	return model->data(model->index(row, REMAINING), Qt::DisplayRole).toULongLong(&ok);
}
