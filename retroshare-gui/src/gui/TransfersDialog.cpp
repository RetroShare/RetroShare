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
#include <QStandardItemModel>
#include <QTreeView>
#include <QShortcut>
#include <QFileInfo>
#include <QMessageBox>
#include <QDesktopServices>

#include <algorithm>

#include "TransfersDialog.h"
#include "RetroShareLink.h"
#include "DetailsDialog.h"
#include "DLListDelegate.h"
#include "ULListDelegate.h"
#include "FileTransferInfoWidget.h"
#include "TurtleRouterDialog.h"
#include "xprogressbar.h"
#include "settings/rsharesettings.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
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

DetailsDialog *TransfersDialog::detailsdlg = NULL;

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

		QStandardItem *myName = myParent->child(index().row(), NAME);
		QStandardItem *otherName = otherParent->child(other.index().row(), NAME);

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

/** Constructor */
TransfersDialog::TransfersDialog(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_bProcessSettings = false;

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

    // set default column and sort order for download
    ui.downloadList->sortByColumn(NAME, Qt::AscendingOrder);
    
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
    ULListModel->insertColumn(UUSERID);
    ui.uploadsList->hideColumn(UUSERID);
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
    upheader->setResizeMode (UNAME, QHeaderView::Interactive);
    upheader->setResizeMode (USIZE, QHeaderView::Interactive);
    upheader->setResizeMode (UTRANSFERRED, QHeaderView::Interactive);
    upheader->setResizeMode (ULSPEED, QHeaderView::Interactive);
    upheader->setResizeMode (UPROGRESS, QHeaderView::Interactive);
    upheader->setResizeMode (USTATUS, QHeaderView::Interactive);
    upheader->setResizeMode (USERNAME, QHeaderView::Interactive);

    upheader->resizeSection ( UNAME, 190 );
    upheader->resizeSection ( USIZE, 70 );
    upheader->resizeSection ( UTRANSFERRED, 75 );
    upheader->resizeSection ( ULSPEED, 75 );
    upheader->resizeSection ( UPROGRESS, 170 );
    upheader->resizeSection ( USTATUS, 100 );
    upheader->resizeSection ( USERNAME, 120 );

    // set default column and sort order for upload
    ui.uploadsList->sortByColumn(UNAME, Qt::AscendingOrder);
	
    FileTransferInfoWidget *ftiw = new FileTransferInfoWidget();
    ui.fileTransferInfoWidget->setWidget(ftiw);
    ui.fileTransferInfoWidget->setWidgetResizable(true);
    ui.fileTransferInfoWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui.fileTransferInfoWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui.fileTransferInfoWidget->viewport()->setBackgroundRole(QPalette::NoRole);
    ui.fileTransferInfoWidget->setFrameStyle(QFrame::NoFrame);
    ui.fileTransferInfoWidget->setFocusPolicy(Qt::NoFocus);

    QObject::connect(ui.downloadList->selectionModel(),SIGNAL(selectionChanged (const QItemSelection&, const QItemSelection&)),this,SLOT(showFileDetails())) ;

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

	 // Actions. Only need to be defined once.
   pauseAct = new QAction(QIcon(IMAGE_PAUSE), tr("Pause"), this);
   connect(pauseAct, SIGNAL(triggered()), this, SLOT(pauseFileTransfer()));

   resumeAct = new QAction(QIcon(IMAGE_RESUME), tr("Resume"), this);
   connect(resumeAct, SIGNAL(triggered()), this, SLOT(resumeFileTransfer()));

   forceCheckAct = new QAction(QIcon(IMAGE_CANCEL), tr( "Force Check" ), this );
   connect( forceCheckAct , SIGNAL( triggered() ), this, SLOT( forceCheck() ) );

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
	playAct = new QAction(QIcon(IMAGE_PLAY), tr( "Play" ), this );
	connect( playAct , SIGNAL( triggered() ), this, SLOT( openTransfer() ) );

    // load settings
    processSettings(true);
}

TransfersDialog::~TransfersDialog()
{
    // save settings
    processSettings(false);
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
        ui._showCacheTransfers_CB->setChecked(Settings->value("showCacheTransfers", false).toBool());

        // state of the lists
        DLHeader->restoreState(Settings->value("downloadList").toByteArray());
        ULHeader->restoreState(Settings->value("uploadList").toByteArray());

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of checks
        Settings->setValue("showCacheTransfers", ui._showCacheTransfers_CB->isChecked());

        // state of the lists
        Settings->setValue("downloadList", DLHeader->saveState());
        Settings->setValue("uploadList", ULHeader->saveState());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
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

void TransfersDialog::downloadListCostumPopupMenu( QPoint point )
{
	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);

	bool single = (items.size() == 1) ;

	/* check which item is selected
	 * - if it is completed - play should appear in menu
	 */
	std::cerr << "TransfersDialog::downloadListCostumPopupMenu()" << std::endl;
	
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
			contextMnu.addMenu( &chunkMenu);

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
			contextMnu.addAction( forceCheckAct);
			contextMnu.addAction( cancelAct);
		}

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

	if(!items.empty())
		contextMnu.addAction( copylinkAct);

	if(!RSLinkClipboard::empty(RetroShareLink::TYPE_FILE)) {
		pastelinkAct->setEnabled(true);
	} else {
		pastelinkAct->setDisabled(true);
	}
	contextMnu.addAction( pastelinkAct);

	contextMnu.addSeparator();

        contextMnu.exec(QCursor::pos());
}

int TransfersDialog::addItem(const QString&, const QString& name, const QString& coreID, qlonglong fileSize, const FileProgressInfo& pinfo, double dlspeed,
		const QString& sources,  const QString& status, const QString& priority, qlonglong completed, qlonglong remaining, qlonglong downloadtime)
{
	int rowCount = DLListModel->rowCount();
	int row ;
	for(row=0;row<rowCount;++row)
		if(DLListModel->item(row,ID)->data(Qt::DisplayRole).toString() == coreID)
			break ;

	if(row >= rowCount )
	{
		row = rowCount;
		DLListModel->insertRow(row);

		// change progress column to own class for sorting
		DLListModel->setItem(row, PROGRESS, new ProgressItem (NULL));
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
	DLListModel->setData(DLListModel->index(row,NAME), getIconFromExtension(ext), Qt::DecorationRole);

	return row ;
}

QIcon TransfersDialog::getIconFromExtension(QString ext)
{
	ext = ext.toLower();
	if (ext == "jpg" || ext == "jpeg" || ext == "tif" || ext == "tiff" || ext == "png" || ext == "gif" || ext == "bmp" || ext == "ico" || ext == "svg")
		return QIcon(QString::fromUtf8(":/images/FileTypePicture.png")) ;
	else if (ext == "avi" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "divx" || ext == "ts"
			|| ext == "mkv" || ext == "mp4" || ext == "flv" || ext == "mov" || ext == "asf" || ext == "xvid"
			|| ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp" || ext == "ogm")
		return QIcon(QString::fromUtf8(":/images/FileTypeVideo.png")) ;
	else if (ext == "ogg" || ext == "mp3" || ext == "mp1" || ext == "mp2" || ext == "wav" || ext == "wma" || ext == "m4a" || ext == "flac")
		return QIcon(QString::fromUtf8(":/images/FileTypeAudio.png")) ;
	else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "gz" || ext == "7z" || ext == "msi"
			|| ext == "rar" || ext == "rpm" || ext == "ace" || ext == "jar" || ext == "tgz" || ext == "lha"
			|| ext == "cab" || ext == "cbz"|| ext == "cbr" || ext == "alz" || ext == "sit" || ext == "arj" || ext == "deb")
		return QIcon(QString::fromUtf8(":/images/FileTypeArchive.png")) ;
	else if (ext == "app" || ext == "bat" || ext == "cgi" || ext == "com"
			|| ext == "exe" || ext == "js" || ext == "pif"
			|| ext == "py" || ext == "pl" || ext == "sh" || ext == "vb" || ext == "ws")
		return QIcon(QString::fromUtf8(":/images/FileTypeProgram.png")) ;
	else if (ext == "iso" || ext == "nrg" || ext == "mdf" || ext == "img" || ext == "dmg" || ext == "bin" )
		return QIcon(QString::fromUtf8(":/images/FileTypeCDImage.png")) ;
	else if (ext == "txt" || ext == "cpp" || ext == "c" || ext == "h")
		return QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")) ;
	else if (ext == "doc" || ext == "rtf" || ext == "sxw" || ext == "xls" || ext == "pps" || ext == "xml"
			|| ext == "sxc" || ext == "odt" || ext == "ods" || ext == "dot" || ext == "ppt" || ext == "css"  )
		return QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")) ;
	else if (ext == "html" || ext == "htm" || ext == "php")
		return QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")) ;
	else 
		return QIcon(QString::fromUtf8(":/images/FileTypeAny.png")) ;
}

int TransfersDialog::addPeerToItem(int row, const QString& name, const QString& coreID, double dlspeed, uint32_t status, const FileProgressInfo& peerInfo)
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


	 QStandardItem *si1 = NULL,*si7=NULL;

    if (childRow == -1) 
	 {
        //set this false if you want to expand on double click
        dlItem->setEditable(false);

		  QHeaderView *header = ui.downloadList->header ();

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

		  si1 = i1 ;
		  si7 = i7 ;

		  QList<QStandardItem *> items;
		  i1->setData(QVariant((QString)" "+name), Qt::DisplayRole);
		  i2->setData(QVariant(QString()), Qt::DisplayRole);
		  i3->setData(QVariant(QString()), Qt::DisplayRole);
		  i4->setData(QVariant((double)dlspeed), Qt::DisplayRole);
		  i5->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);
		  i6->setData(QVariant(QString()), Qt::DisplayRole);

		  i8->setData(QVariant(QString()), Qt::DisplayRole);	// blank field for priority
		  i9->setData(QVariant(QString()), Qt::DisplayRole);
		  i10->setData(QVariant(QString()), Qt::DisplayRole);
		  i11->setData(QVariant((QString)coreID), Qt::DisplayRole);

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
    } 
	 else 
	 {
        //just update the child (peer)
        dlItem->child(childRow, DLSPEED)->setData(QVariant((double)dlspeed), Qt::DisplayRole);
        dlItem->child(childRow, PROGRESS)->setData(QVariant::fromValue(peerInfo), Qt::DisplayRole);

		  si1 = dlItem->child(childRow,NAME) ;
        si7 = dlItem->child(childRow, STATUS) ;
    }

	 QString sstatus;
	 switch (status) 
	 {
		 case FT_STATE_FAILED:       si7->setData(QVariant(tr("Failed"))) ;
											  si1->setData(QIcon(QString::fromUtf8(":/images/Client1.png")), Qt::DecorationRole);
											  break ;

		 case FT_STATE_OKAY:         si7->setData(QVariant(tr("Okay"))); 
											  si1->setData(QIcon(QString::fromUtf8(":/images/Client2.png")), Qt::DecorationRole);
											  break ;

		 case FT_STATE_WAITING:      si7->setData(QVariant(tr(""))); 
											  si1->setData(QIcon(QString::fromUtf8(":/images/Client3.png")), Qt::DecorationRole);
											  break ;

		 case FT_STATE_DOWNLOADING:  si7->setData(QVariant(tr("Transferring"))); 
											  si1->setData(QIcon(QString::fromUtf8(":/images/Client0.png")), Qt::DecorationRole);
											  break ;

		 case FT_STATE_COMPLETE:     si7->setData(QVariant(tr("Complete"))); 
											  si1->setData(QIcon(QString::fromUtf8(":/images/Client0.png")), Qt::DecorationRole);
											  break ;

		 default:                    si7->setData(QVariant(tr(""))); 
											  break ;
											  si1->setData(QIcon(QString::fromUtf8(":/images/Client4.png")), Qt::DecorationRole);
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
		if(ULListModel->item(row,UUSERID)->data(Qt::EditRole).toString() == peer_id && ULListModel->item(row,UHASH)->data(Qt::EditRole).toString() == coreID)
			break ;

	if(row >= ULListModel->rowCount() )
	{
		row = ULListModel->rowCount();
		ULListModel->insertRow(row);

		// change progress column to own class for sorting
		ULListModel->setItem(row, UPROGRESS, new ProgressItem(NULL));
	}

	ULListModel->setData(ULListModel->index(row, UNAME),        QVariant((QString)" "+name), Qt::DisplayRole);
	ULListModel->setData(ULListModel->index(row, USIZE),        QVariant((qlonglong)fileSize));
	ULListModel->setData(ULListModel->index(row, UTRANSFERRED), QVariant((qlonglong)completed));
	ULListModel->setData(ULListModel->index(row, ULSPEED),      QVariant((double)dlspeed));
	ULListModel->setData(ULListModel->index(row, UPROGRESS),    QVariant::fromValue(pinfo));
	ULListModel->setData(ULListModel->index(row, USTATUS),      QVariant((QString)status));
	ULListModel->setData(ULListModel->index(row, USERNAME),     QVariant((QString)source));
	ULListModel->setData(ULListModel->index(row, UHASH),        QVariant((QString)coreID));
	ULListModel->setData(ULListModel->index(row, UUSERID),      QVariant((QString)peer_id));

	QString ext = QFileInfo(name).suffix();
	ULListModel->setData(ULListModel->index(row,UNAME), getIconFromExtension(ext), Qt::DecorationRole);

	return row;
}


//void TransfersDialog::delItem(int row)
//{
//	DLListModel->removeRow(row, QModelIndex());
//}

//void TransfersDialog::delUploadItem(int row)
//{
//	ULListModel->removeRow(row, QModelIndex());
//}

	/* get the list of Transfers from the RsIface.  **/
void TransfersDialog::updateDisplay()
{
	insertTransfers();
        updateDetailsDialog ();
}

static void QListDelete (const QList <QStandardItem*> &List)
{
    qDeleteAll (List);
}

void TransfersDialog::insertTransfers() 
{
	/* disable for performance issues, enable after insert all transfers */
	ui.downloadList->setSortingEnabled(false);

	/* get the download lists */
	std::list<std::string> downHashes;
	rsFiles->FileDownloads(downHashes);

//	std::list<DwlDetails> dwlDetails;
//	rsFiles->getDwlDetails(dwlDetails);

	bool showCacheTransfers = ui._showCacheTransfers_CB->isChecked();

	/* get online peers only once */
	std::list<std::string> onlineIds;
	rsPeers->getOnlineList(onlineIds);

	/* get also only once */
	std::map<std::string, std::string> versions;
	bool retv = rsDisc->getDiscVersions(versions);

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

		if((info.flags & RS_FILE_HINTS_CACHE) && !showCacheTransfers)
			continue;

		QString fileName = QString::fromUtf8(info.fname.c_str());
		QString fileHash = QString::fromStdString(info.hash);
		qlonglong fileSize    = info.size;
		double fileDlspeed     = (info.downloadStatus==FT_STATE_DOWNLOADING)?(info.tfRate * 1024.0):0.0;

		/* get the sources (number of online peers) */
		int active = 0;
		std::list<TransferInfo>::iterator pit;
		for (pit = info.peers.begin(); pit != info.peers.end(); pit++) 
			if(pit->tfRate > 0 && info.downloadStatus==FT_STATE_DOWNLOADING)
				active++;
		
		QString sources = QString("%1 (%2)").arg(active).arg(info.peers.size());

		QString status;
		switch (info.downloadStatus) {
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
		pinfo.progress = (info.size==0)?0:(completed*100.0/info.size) ;
		pinfo.nb_chunks = pinfo.cmap._map.empty()?0:fcinfo.chunks.size() ;

		for(uint32_t i=0;i<fcinfo.active_chunks.size();++i)
			pinfo.chunks_in_progress.push_back(fcinfo.active_chunks[i].first) ;

		int addedRow = addItem("", fileName, fileHash, fileSize, pinfo, fileDlspeed, sources, status, priority, completed, remaining, downloadtime);
		used_hashes.insert(info.hash) ;

		std::map<std::string, std::string>::iterator vit;

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

			double peerDlspeed	= 0;
			if ((uint32_t)pit->status == FT_STATE_DOWNLOADING && info.downloadStatus != FT_STATE_PAUSED && info.downloadStatus != FT_STATE_COMPLETE) 
				peerDlspeed     = pit->tfRate * 1024.0;

			FileProgressInfo peerpinfo ;
			peerpinfo.cmap = fcinfo.compressed_peer_availability_maps[pit->peerId];
			peerpinfo.type = FileProgressInfo::DOWNLOAD_SOURCE ;
			peerpinfo.progress = 0.0 ;	// we don't display completion for sources.
			peerpinfo.nb_chunks = peerpinfo.cmap._map.empty()?0:fcinfo.chunks.size();

			int row_id = addPeerToItem(addedRow, peerName, hashFileAndPeerId, peerDlspeed, pit->status, peerpinfo);

			used_rows.insert(row_id) ;
		}

		QStandardItem *dlItem = DLListModel->item(addedRow);

		// This is not optimal, but we deal with a small number of elements. The reverse order is really important,
		// because rows after the deleted rows change positions !
		//
		for(int r=dlItem->rowCount()-1;r>=0;--r)
			if(used_rows.find(r) == used_rows.end())
				QListDelete (dlItem->takeRow(r)) ;
	}

	// remove hashes that where not shown
	//first clean the model in case some files are not download anymore
	//remove items that are not fiends anymore
	int removeIndex = 0;
	int rowCount = DLListModel->rowCount();
	while (removeIndex < rowCount)
	{
		std::string hash = DLListModel->item(removeIndex, ID)->data(Qt::DisplayRole).toString().toStdString();

		if(used_hashes.find(hash) == used_hashes.end()) {
			QListDelete (DLListModel->takeRow(removeIndex));
			rowCount = DLListModel->rowCount();
		} else
			removeIndex++;
	}
	
	ui.downloadList->setSortingEnabled(true);

	ui.uploadsList->setSortingEnabled(false);

	// Now show upload hashes
	//
	std::list<std::string> upHashes;
	rsFiles->FileUploads(upHashes);

	std::string ownId = rsPeers->getOwnId();

	used_hashes.clear() ;

	for(it = upHashes.begin(); it != upHashes.end(); it++) 
	{
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_UPLOAD, info)) 
			continue;
		
		if((info.flags & RS_FILE_HINTS_CACHE) && showCacheTransfers)
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
			double progress 	= (info.size > 0)?(pit->transfered * 100.0 / info.size):0.0;
			qlonglong remaining   = (info.size - pit->transfered) / (pit->tfRate * 1024.0);

			// Estimate the completion. We need something more accurate, meaning that we need to
			// transmit the completion info.
			//
			uint32_t chunk_size = 1024*1024 ;
			uint32_t nb_chunks = (uint32_t)(info.size / (uint64_t)(chunk_size) ) ;
			if((info.size % (uint64_t)chunk_size) != 0)
				++nb_chunks ;

			uint32_t filled_chunks = pinfo.cmap.filledChunks(nb_chunks) ;
			pinfo.type = FileProgressInfo::UPLOAD_LINE ;
			pinfo.nb_chunks = pinfo.cmap._map.empty()?0:nb_chunks ;

			if(filled_chunks > 1) {
				pinfo.progress = (nb_chunks==0)?0:(filled_chunks*100.0/nb_chunks) ;
				completed = std::min(info.size,((uint64_t)filled_chunks)*chunk_size) ;
			} 
			else 
				pinfo.progress = progress ;

			addUploadItem("", fileName, fileHash, fileSize, pinfo, dlspeed, source,QString::fromStdString(pit->peerId),  status, completed, remaining);

			used_hashes.insert(fileHash.toStdString() + pit->peerId) ;
		}
	}
	
	// remove hashes that where not shown
	//first clean the model in case some files are not download anymore
	//remove items that are not fiends anymore
	removeIndex = 0;
	rowCount = ULListModel->rowCount();
	while (removeIndex < rowCount)
	{
		std::string hash = ULListModel->item(removeIndex, UHASH)->data(Qt::EditRole).toString().toStdString();
		std::string peer = ULListModel->item(removeIndex, UUSERID)->data(Qt::EditRole).toString().toStdString();

		if(used_hashes.find(hash + peer) == used_hashes.end()) {
			QListDelete (ULListModel->takeRow(removeIndex));
			rowCount = ULListModel->rowCount();
		} else
			removeIndex++;
	}

	ui.uploadsList->setSortingEnabled(true);
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
	std::vector<RetroShareLink> links ;

	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);

	for (it = items.begin(); it != items.end(); it ++) {
		FileInfo info;
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) {
			continue;
		}

		RetroShareLink link(QString::fromStdString(info.fname), info.size, QString::fromStdString(info.hash));
		links.push_back(link) ;
	}

	RSLinkClipboard::copyLinks(links) ;
}

void TransfersDialog::showDetailsDialog()
{
    if (detailsdlg == NULL) {
        // create window
        detailsdlg = new DetailsDialog ();
    }

    updateDetailsDialog ();

    detailsdlg->show();
}

void TransfersDialog::updateDetailsDialog()
{
    if (detailsdlg == NULL) {
        return;
    }

    std::string file_hash ;
    QString fhash;
    QString fsize;
    QString fname;
    QString fstatus;
    QString fpriority;
    QString fsources;

    qulonglong filesize = 0;
    double fdatarate = 0;
    qulonglong fcompleted = 0;
    qulonglong fremaining = 0;

    qulonglong fdownloadtime = 0;

    std::set<int> rows;
    std::set<int>::iterator it;
    getSelectedItems(NULL, &rows);

    if (rows.size()) {
        int row = *rows.begin();

        fhash = getID(row, DLListModel);
        fsize = getFileSize(row, DLListModel);
        fname = getFileName(row, DLListModel);
        fstatus = getStatus(row, DLListModel);
        fpriority = getPriority(row, DLListModel);
        fsources = getSources(row, DLListModel);

        filesize = getFileSize(row, DLListModel);
        fdatarate = getSpeed(row, DLListModel);
        fcompleted = getTransfered(row, DLListModel);
        fremaining = getRemainingTime(row, DLListModel);

        fdownloadtime = getDownloadTime(row, DLListModel);

// maybe show all links in retroshare link(s) Tab
//        int nb_select = 0 ;
//
//        for(int i = 0; i <= DLListModel->rowCount(); i++)
//        if(selection->isRowSelected(i, QModelIndex()))
//        {
//            file_hash = getID(i, DLListModel).toStdString();
//            ++nb_select ;
//        }

        file_hash = getID(row, DLListModel).toStdString();
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
    if (fname.isEmpty()) {
        detailsdlg->setCompleted(misc::friendlyUnit(-1));
        detailsdlg->setRemaining(misc::friendlyUnit(-1));
    } else {
        detailsdlg->setCompleted(misc::friendlyUnit(fcompleted));
        detailsdlg->setRemaining(misc::friendlyUnit(fremaining));
    }

    //Date GroupBox
    if (fname.isEmpty()) {
        detailsdlg->setDownloadtime(misc::userFriendlyDuration(-1));
    } else {
        detailsdlg->setDownloadtime(misc::userFriendlyDuration(fdownloadtime));
    }

    // retroshare link(s) Tab
    if (fname.isEmpty()) {
        detailsdlg->setLink("");
    } else {
        RetroShareLink link(fname, filesize, fhash);
        detailsdlg->setLink(link.toString());
    }

    FileChunksInfo info ;
    if (fhash.isEmpty() == false && rsFiles->FileDownloadChunksDetails(fhash.toStdString(), info)) {
        detailsdlg->setChunkSize(info.chunk_size);
        detailsdlg->setNumberOfChunks(info.chunks.size());
    } else {
        detailsdlg->setChunkSize(0);
        detailsdlg->setNumberOfChunks(0);
    }
}

void TransfersDialog::pasteLink()
{
    RSLinkClipboard::process(RetroShareLink::TYPE_FILE, RSLINK_PROCESS_NOTIFY_ERROR);
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
				QStandardItem *id = DLListModel->item(i, ID);
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

	std::set<std::string> items;
	std::set<std::string>::iterator it;
	getSelectedItems(&items, NULL);
	for (it = items.begin(); it != items.end(); it ++) {
		if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
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


void TransfersDialog::changeQueuePosition(QueueMove mv)
{
	std::cerr << "In changeQueuePosition (gui)"<< std::endl ;
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

        updateDetailsDialog ();
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
	return model->data(model->index(row, ID), Qt::DisplayRole).toString().left(40); // gets only the "hash" part of the name
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

qlonglong TransfersDialog::getDownloadTime(int row, QStandardItemModel *model)
{
	return model->data(model->index(row, DOWNLOADTIME), Qt::DisplayRole).toULongLong();
}

QString TransfersDialog::getSources(int row, QStandardItemModel *model)
{
	return model->data(model->index(row, SOURCES), Qt::DisplayRole).toString();
}
