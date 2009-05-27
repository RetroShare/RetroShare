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


#include "rshare.h"
#include "TransfersDialog.h"
#include "RetroShareLinkAnalyzer.h"
#include "DLListDelegate.h"
#include "ULListDelegate.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>
#include <QStandardItemModel>

#include <sstream>
#include "rsiface/rsfiles.h"
#include "rsiface/rspeers.h"
#include <algorithm>

/* Images for context menu icons */
#define IMAGE_INFO                 ":/images/fileinfo.png"
#define IMAGE_CANCEL               ":/images/delete.png"
#define IMAGE_CLEARCOMPLETED       ":/images/deleteall.png"
#define IMAGE_PLAY		             ":/images/player_play.png"
#define IMAGE_COPYLINK             ":/images/copyrslink.png"
#define IMAGE_PASTELINK            ":/images/pasterslink.png"

/** Constructor */
TransfersDialog::TransfersDialog(QWidget *parent)
: MainPage(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    connect( ui.downloadList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( downloadListCostumPopupMenu( QPoint ) ) );

    // Set Download list model
    DLListModel = new QStandardItemModel(0,9);
    DLListModel->setHeaderData(NAME, Qt::Horizontal, tr("Name", "i.e: file name"));
    DLListModel->setHeaderData(SIZE, Qt::Horizontal, tr("Size", "i.e: file size"));
    DLListModel->setHeaderData(COMPLETED, Qt::Horizontal, tr("Completed", ""));
    DLListModel->setHeaderData(DLSPEED, Qt::Horizontal, tr("Speed", "i.e: Download speed"));
    DLListModel->setHeaderData(PROGRESS, Qt::Horizontal, tr("Progress", "i.e: % downloaded"));
    DLListModel->setHeaderData(SOURCES, Qt::Horizontal, tr("Sources", "i.e: Sources"));
    DLListModel->setHeaderData(STATUS, Qt::Horizontal, tr("Status"));
    DLListModel->setHeaderData(REMAINING, Qt::Horizontal, tr("Remaining", "i.e: Estimated Time of Arrival / Time left"));
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
    _header->setResizeMode (REMAINING, QHeaderView::Interactive);

    _header->resizeSection ( NAME, 170 );
    _header->resizeSection ( SIZE, 70 );
    _header->resizeSection ( COMPLETED, 75 );
    _header->resizeSection ( DLSPEED, 75 );
    _header->resizeSection ( PROGRESS, 170 );
    _header->resizeSection ( SOURCES, 90 );
    _header->resizeSection ( STATUS, 100 );
    _header->resizeSection ( REMAINING, 100 );

    // Set Upload list model
    ULListModel = new QStandardItemModel(0,7);
    ULListModel->setHeaderData(UNAME, Qt::Horizontal, tr("Name", "i.e: file name"));
    ULListModel->setHeaderData(USIZE, Qt::Horizontal, tr("Size", "i.e: file size"));
    ULListModel->setHeaderData(USERNAME, Qt::Horizontal, tr("User Name", "i.e: user name"));
    ULListModel->setHeaderData(UPROGRESS, Qt::Horizontal, tr("Progress", "i.e: % uploaded"));
    ULListModel->setHeaderData(ULSPEED, Qt::Horizontal, tr("Speed", "i.e: upload speed"));
    ULListModel->setHeaderData(USTATUS, Qt::Horizontal, tr("Status"));
    ULListModel->setHeaderData(UTRANSFERRED, Qt::Horizontal, tr("Transferred", ""));
    ui.uploadsList->setModel(ULListModel);
    ULDelegate = new ULListDelegate();
    ui.uploadsList->setItemDelegate(ULDelegate);

    ui.uploadsList->setAutoScroll(false) ;

    ui.uploadsList->setRootIsDecorated(false);



  	//Selection Setup
	//selection = ui.uploadsList->selectionModel();

	/* Set header resize modes and initial section sizes Uploads TreeView*/
//	QHeaderView * upheader = ui.uploadsList->header () ;
//	upheader->setResizeMode (0, QHeaderView::Interactive); /*Name */
//	upheader->setResizeMode (1, QHeaderView::Interactive); /*Size */
//	upheader->setResizeMode (2, QHeaderView::Interactive); /*User Name*/
//	upheader->setResizeMode (3, QHeaderView::Interactive); /*Progress*/
//	upheader->setResizeMode (4, QHeaderView::Interactive); /*Speed */
//	upheader->setResizeMode (5, QHeaderView::Interactive); /*Status*/
//	upheader->setResizeMode (6, QHeaderView::Interactive); /*Transferred*/
//
//	upheader->resizeSection ( 0, 100 ); /*Name */
//	upheader->resizeSection ( 1, 75 ); /*Size */
//	upheader->resizeSection ( 2, 100 ); /*User Name*/
//	upheader->resizeSection ( 3, 100 ); /*Progress*/
//	upheader->resizeSection ( 4, 75 ); /*Speed */
//	upheader->resizeSection ( 5, 100 ); /*Status*/
//	upheader->resizeSection ( 6, 75 ); /*Transferred*/

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void TransfersDialog::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Delete)
	{
		cancel() ;
		e->accept() ;
	}
	else
		MainPage::keyPressEvent(e) ;
}

void TransfersDialog::downloadListCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      //showdowninfoAct = new QAction(QIcon(IMAGE_INFO), tr( "Details..." ), this );
      //connect( showdowninfoAct , SIGNAL( triggered() ), this, SLOT( showDownInfoWindow() ) );

      /* check which item is selected
       * - if it is completed - play should appear in menu
       */
	std::cerr << "TransfersDialog::downloadListCostumPopupMenu()" << std::endl;

      	bool addPlayOption = false;
	for(int i = 0; i <= DLListModel->rowCount(); i++) {
		std::cerr << "Row Status :" << getStatus(i, DLListModel).toStdString() << ":" << std::endl;
		if(selection->isRowSelected(i, QModelIndex())) {
			std::cerr << "Selected Row Status :" << getStatus(i, DLListModel).toStdString() << ":" << std::endl;
			QString qstatus = getStatus(i, DLListModel);
			std::string status = (qstatus.trimmed()).toStdString();
			if (status == "Complete")
			{
				std::cerr << "Add Play Option" << std::endl;
				addPlayOption = true;
			}
		}
	}

      	QAction *playAct = NULL;
	if (addPlayOption)
	{
      		playAct = new QAction(QIcon(IMAGE_PLAY), tr( "Play" ), this );
      		connect( playAct , SIGNAL( triggered() ), this, SLOT( playSelectedTransfer() ) );
	}

	    cancelAct = new QAction(QIcon(IMAGE_CANCEL), tr( "Cancel" ), this );
      connect( cancelAct , SIGNAL( triggered() ), this, SLOT( cancel() ) );

      copylinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy retroshare Link" ), this );
      connect( copylinkAct , SIGNAL( triggered() ), this, SLOT( copyLink() ) );

      pastelinkAct = new QAction(QIcon(IMAGE_PASTELINK), tr( "Paste retroshare Link" ), this );
      connect( pastelinkAct , SIGNAL( triggered() ), this, SLOT( pasteLink() ) );

      clearcompletedAct = new QAction(QIcon(IMAGE_CLEARCOMPLETED), tr( "Clear Completed" ), this );
      connect( clearcompletedAct , SIGNAL( triggered() ), this, SLOT( clearcompleted() ) );

      contextMnu.clear();
      if (addPlayOption)
      {
      	contextMnu.addAction(playAct);
      }
      contextMnu.addSeparator();

      contextMnu.addAction( cancelAct);
      contextMnu.addSeparator();
      contextMnu.addAction( copylinkAct);
      contextMnu.addAction( pastelinkAct);
      contextMnu.addSeparator();
      contextMnu.addAction( clearcompletedAct);
      contextMnu.exec( mevent->globalPos() );
}

void TransfersDialog::playSelectedTransfer()
{
	std::cerr << "TransfersDialog::playSelectedTransfer()" << std::endl;

        /* get the shared directories */
	std::string incomingdir = rsFiles->getDownloadDirectory();

	/* create the List of Files */
	QStringList playList;
	for(int i = 0; i <= DLListModel->rowCount(); i++) {
		if(selection->isRowSelected(i, QModelIndex())) {
			QString qstatus = getStatus(i, DLListModel);
			std::string status = (qstatus.trimmed()).toStdString();
			if (status == "Complete")
			{
				QString qname = getFileName(i, DLListModel);
				QString fullpath = QString::fromStdString(incomingdir);
				fullpath += "/";
				fullpath += qname.trimmed();

				playList.push_back(fullpath);

				std::cerr << "PlayFile:" << fullpath.toStdString() << std::endl;

			}
		}
	}
	playFiles(playList);
}


void TransfersDialog::updateProgress(int value)
{
	for(int i = 0; i <= DLListModel->rowCount(); i++) {
		if(selection->isRowSelected(i, QModelIndex())) {
			editItem(i, PROGRESS, QVariant((double)value));
		}
	}
}

TransfersDialog::~TransfersDialog()
{
	;
}



int TransfersDialog::addItem(QString symbol, QString name, QString coreID, qlonglong fileSize, double progress, double dlspeed, QString sources,  QString status, qlonglong completed, qlonglong remaining)
{
    int row;
    QString sl;
    //QIcon icon(symbol);
    name.insert(0, " ");
    //sl.sprintf("%d / %d", seeds, leechs);
    row = DLListModel->rowCount();
    DLListModel->insertRow(row);

    //DLListModel->setData(DLListModel->index(row, NAME), QVariant((QIcon)icon), Qt::DecorationRole);
    DLListModel->setData(DLListModel->index(row, NAME), QVariant((QString)name), Qt::DisplayRole);
    DLListModel->setData(DLListModel->index(row, SIZE), QVariant((qlonglong)fileSize));
    DLListModel->setData(DLListModel->index(row, COMPLETED), QVariant((qlonglong)completed));
    DLListModel->setData(DLListModel->index(row, DLSPEED), QVariant((double)dlspeed));
    DLListModel->setData(DLListModel->index(row, PROGRESS), QVariant((double)progress));
    DLListModel->setData(DLListModel->index(row, SOURCES), QVariant((QString)sources));
    DLListModel->setData(DLListModel->index(row, STATUS), QVariant((QString)status));
    DLListModel->setData(DLListModel->index(row, REMAINING), QVariant((qlonglong)remaining));
    DLListModel->setData(DLListModel->index(row, ID), QVariant((QString)coreID));


    QString ext = QFileInfo(name).suffix();
    if (ext == "jpg" || ext == "jpeg" || ext == "tif" || ext == "tiff" || ext == "JPG"|| ext == "png" || ext == "gif"
    || ext == "bmp" || ext == "ico" || ext == "svg")
    {
      DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypePicture.png")), Qt::DecorationRole);
    }
    else if (ext == "avi" ||ext == "AVI" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "divx" || ext == "ts"
    || ext == "mkv" || ext == "mp4" || ext == "flv" || ext == "mov" || ext == "asf" || ext == "xvid"
    || ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp" || ext == "mpeg" || ext == "ogm")
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeVideo.png")), Qt::DecorationRole);
    }
    else if (ext == "ogg" || ext == "mp3" || ext == "MP3"  || ext == "mp1" || ext == "mp2" || ext == "wav" || ext == "wma")
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeAudio.png")), Qt::DecorationRole);
    }
    else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "gz" || ext == "7z" || ext == "msi"
    || ext == "rar" || ext == "rpm" || ext == "ace" || ext == "jar" || ext == "tgz" || ext == "lha"
    || ext == "cab" || ext == "cbz"|| ext == "cbr" || ext == "alz" || ext == "sit" || ext == "arj" || ext == "deb")
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeArchive.png")), Qt::DecorationRole);
    }
    else if (ext == "app" || ext == "bat" || ext == "cgi" || ext == "com"
    || ext == "exe" || ext == "js" || ext == "pif"
    || ext == "py" || ext == "pl" || ext == "sh" || ext == "vb" || ext == "ws")
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeProgram.png")), Qt::DecorationRole);
    }
    else if (ext == "iso" || ext == "nrg" || ext == "mdf" || ext == "img" || ext == "dmg" || ext == "bin" )
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/imagees/FileTypeCDImage.png")), Qt::DecorationRole);
    }
    else if (ext == "txt" || ext == "cpp" || ext == "c" || ext == "h")
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")), Qt::DecorationRole);
    }
    else if (ext == "doc" || ext == "rtf" || ext == "sxw" || ext == "xls" || ext == "pps" || ext == "xml"
    || ext == "sxc" || ext == "odt" || ext == "ods" || ext == "dot" || ext == "ppt" || ext == "css"  )
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")), Qt::DecorationRole);
    }
    else if (ext == "html" || ext == "htm" || ext == "php")
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeDocument.png")), Qt::DecorationRole);
    }
    else
    {
    DLListModel->setData(DLListModel->index(row,NAME), QIcon(QString::fromUtf8(":/images/FileTypeAny.png")), Qt::DecorationRole);
    }

	return row;
}

bool TransfersDialog::addPeerToItem(int row, QString symbol, QString name, QString coreID, qlonglong fileSize, double progress, double dlspeed, QString sources, QString status, qlonglong completed, qlonglong remaining)
{
    QStandardItem *dlItem = DLListModel->item(row);
    if (!dlItem) return false;

    //set this false if you want to expand on double click
    dlItem->setEditable(false);

    QList<QStandardItem *> items;
    QStandardItem *i1 = new QStandardItem(); i1->setData(QVariant((QString)name), Qt::DisplayRole);
    QStandardItem *i2 = new QStandardItem(); i2->setData(QVariant((qlonglong)fileSize), Qt::DisplayRole);
    QStandardItem *i3 = new QStandardItem(); i3->setData(QVariant((qlonglong)completed), Qt::DisplayRole);
    QStandardItem *i4 = new QStandardItem(); i4->setData(QVariant((double)dlspeed), Qt::DisplayRole);
    QStandardItem *i5 = new QStandardItem(); i5->setData(QVariant((double)progress), Qt::DisplayRole);
    QStandardItem *i6 = new QStandardItem(); i6->setData(QVariant((QString)sources), Qt::DisplayRole);
    QStandardItem *i7 = new QStandardItem(); i7->setData(QVariant((QString)status), Qt::DisplayRole);
    QStandardItem *i8 = new QStandardItem(); i8->setData(QVariant((qlonglong)remaining), Qt::DisplayRole);
    QStandardItem *i9 = new QStandardItem(); i9->setData(QVariant((QString)coreID), Qt::DisplayRole);

    items.append(i1);
    items.append(i2);
    items.append(i3);
    items.append(i4);
    items.append(i5);
    items.append(i6);
    items.append(i7);
    items.append(i8);
    items.append(i9);

    dlItem->appendRow(items);

    return true;
}


int TransfersDialog::addUploadItem(QString symbol, QString name, QString coreID, qlonglong fileSize, double progress, double dlspeed, QString sources,  QString status, qlonglong completed, qlonglong remaining)
{
	int row;
	QString sl;
	//QIcon icon(symbol);
	name.insert(0, " ");
	row = ULListModel->rowCount();
	ULListModel->insertRow(row);

	ULListModel->setData(ULListModel->index(row, UNAME), QVariant((QString)name), Qt::DisplayRole);
	ULListModel->setData(ULListModel->index(row, USIZE), QVariant((qlonglong)fileSize));
	ULListModel->setData(ULListModel->index(row, USERNAME), QVariant((QString)sources));
	ULListModel->setData(ULListModel->index(row, UPROGRESS), QVariant((double)progress));
	ULListModel->setData(ULListModel->index(row, ULSPEED), QVariant((double)dlspeed));
	ULListModel->setData(ULListModel->index(row, USTATUS), QVariant((QString)status));
	ULListModel->setData(ULListModel->index(row, UTRANSFERRED), QVariant((qlonglong)completed));

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

void TransfersDialog::editItem(int row, int column, QVariant data)
{
	//QIcon *icon;
	switch(column) {
		//case SYMBOL:
		//	icon = new QIcon(data.toString());
		//	DLListModel->setData(DLListModel->index(row, NAME), QVariant((QIcon)*icon), Qt::DecorationRole);
		//	delete icon;
		//	break;
		case NAME:
			DLListModel->setData(DLListModel->index(row, NAME), data, Qt::DisplayRole);
			break;
		case SIZE:
			DLListModel->setData(DLListModel->index(row, SIZE), data);
			break;
		case PROGRESS:
			DLListModel->setData(DLListModel->index(row, PROGRESS), data);
			break;
		case DLSPEED:
			DLListModel->setData(DLListModel->index(row, DLSPEED), data);
			break;
		case SOURCES:
			DLListModel->setData(DLListModel->index(row, SOURCES), data);
			break;
		case STATUS:
			DLListModel->setData(DLListModel->index(row, STATUS), data);
			break;
		case COMPLETED:
			DLListModel->setData(DLListModel->index(row, COMPLETED), data);
			break;
		case REMAINING:
			DLListModel->setData(DLListModel->index(row, REMAINING), data);
			break;
		case ID:
			DLListModel->setData(DLListModel->index(row, ID), data);
			break;
	}
}

	/* get the list of Transfers from the RsIface.  **/
void TransfersDialog::insertTransfers()
{
	QString symbol, name, sources, status, coreId;
	qlonglong fileSize, completed, remaining;
	double progress, dlspeed;


	/* get current selection */
	std::list<std::string> selectedIds;

	for (int i = 0; i < DLListModel->rowCount(); i++) {
	    if (selection->isRowSelected(i, QModelIndex())) {
	        std::string id = getID(i, DLListModel).toStdString();
	        selectedIds.push_back(id);
        }

        QStandardItem *parent = DLListModel->item(i);
        //if (!parent) continue;
        int childs = parent->rowCount();
        for (int j = 0; j < childs; j++) {
            QModelIndex parentIndex = DLListModel->indexFromItem(parent);
            if (selection->isRowSelected(j, parentIndex)) {
                QStandardItem *child = parent->child(j, ID);
                std::string id = child->data(Qt::DisplayRole).toString().toStdString();
                selectedIds.push_back(id);
            }
        }
    }


	/* get expanded donwload items */
	std::list<std::string> expandedIds;

	for (int i = 0; i < DLListModel->rowCount(); i++) {
	    QStandardItem *item = DLListModel->item(i);
	    QModelIndex index = DLListModel->indexFromItem(item);
	    if (ui.downloadList->isExpanded(index)) {
	        std::string id = getID(i, DLListModel).toStdString();
	        expandedIds.push_back(id);
        }
    }

	//remove all Items
	for(int i = DLListModel->rowCount(); i >= 0; i--)
	{
		delItem(i);
	}

	for(int i = ULListModel->rowCount(); i >= 0; i--)
	{
		delUploadItem(i);
	}


	/* get the download and upload lists */
	std::list<std::string> downHashes;
	std::list<std::string> upHashes;

	rsFiles->FileDownloads(downHashes);
	rsFiles->FileUploads(upHashes);

	uint32_t dlCount = 0;
	uint32_t ulCount = 0;

    std::list<std::string>::iterator it;
    for (it = downHashes.begin(); it != downHashes.end(); it++) {
        FileInfo info;
        if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_DOWNLOAD, info)) continue;
        //if file transfer is a cache file index file, don't show it
        if (info.flags & CB_CODE_CACHE) continue;

        symbol      = "";
        name        = QString::fromStdString(info.fname);
        coreId      = QString::fromStdString(info.hash);
        fileSize    = info.size;
        progress    = (info.transfered * 100.0) / info.size;
        dlspeed     = info.tfRate * 1024.0;

        /* get number of online peers */
        int online = 0;
        std::list<TransferInfo>::iterator pit;
        for (pit = info.peers.begin(); pit != info.peers.end(); pit++) {
            if (rsPeers->isOnline(pit->peerId)) {
                online++;
            }
        }

        sources     = QString("%1 (%2)").arg(online).arg(info.peers.size() - online);

        switch (info.downloadStatus) {
            case FT_STATE_FAILED:
                status = tr("Failed"); break;
            case FT_STATE_OKAY:
                status = tr("Okay"); break;
            case FT_STATE_WAITING:
                status = tr("Waiting"); break;
            case FT_STATE_DOWNLOADING:
                status = tr("Downloading"); break;
            case FT_STATE_COMPLETE:
                status = tr("Complete"); break;
            default:
                status = tr("Unknown"); break;
        }

        completed   = info.transfered;
        remaining   = (info.size - info.transfered) / (info.tfRate * 1024.0);

        int addedRow = addItem(symbol, name, coreId, fileSize, progress, dlspeed, sources, status, completed, remaining);

        /* if found in selectedIds -> select again */
        if (selectedIds.end() != std::find(selectedIds.begin(), selectedIds.end(), info.hash)) {
            selection->select(DLListModel->index(dlCount, 0),
                QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }

        /* expand last expanded items */
        if (expandedIds.end() != std::find(expandedIds.begin(), expandedIds.end(), info.hash)) {
            QStandardItem *item = DLListModel->item(dlCount);
            QModelIndex index = DLListModel->indexFromItem(item);
            ui.downloadList->setExpanded(index, true);
        }

        dlCount++;

        /* continue to next download item if no peers to add */
        if (!info.peers.size()) continue;

        int dlPeers = 0;
        for (pit = info.peers.begin(); pit != info.peers.end(); pit++) {
            symbol      = "";
            name        = QString::fromStdString(rsPeers->getPeerName(pit->peerId));
            //unique combination: fileName + peerName, variant: hash + peerName (too long)
            coreId      = QString::fromStdString(info.fname + rsPeers->getPeerName(pit->peerId));
            fileSize    = info.size;
            progress    = (info.transfered * 100.0) / info.size;
            dlspeed     = pit->tfRate * 1024.0;
            //sources     = QString("rShare v %1").arg(tr("0.4.13a"));//TODO: take it from somewhere
            sources     = "";

            if (info.downloadStatus == FT_STATE_COMPLETE) {
                status = tr("Complete");
            } else {
                status = tr("Unknown");
                switch (pit->status) {
                    case FT_STATE_FAILED:
                        status = tr("Failed"); break;
                    case FT_STATE_OKAY:
                        status = tr("Okay"); break;
                    case FT_STATE_WAITING:
                        status = tr("Waiting"); break;
                    case FT_STATE_DOWNLOADING:
                        status = tr("Downloading"); break;
                    case FT_STATE_COMPLETE:
                        status = tr("Complete"); break;
                }
            }

            completed   = info.transfered;
            remaining   = (info.size - info.transfered) / (pit->tfRate * 1024.0);

            if (!addPeerToItem(addedRow, symbol, name, coreId, fileSize, progress, dlspeed, sources, status, completed, remaining))
                continue;

            /* if peers found in selectedIds, select again */
            if (selectedIds.end() != std::find(selectedIds.begin(), selectedIds.end(), info.fname + rsPeers->getPeerName(pit->peerId))) {
                QStandardItem *dlItem = DLListModel->item(addedRow);
                QModelIndex childIndex = DLListModel->indexFromItem(dlItem).child(dlPeers, 0);
                selection->select(childIndex, QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
            }
            dlPeers++;
        }
    }


	for(it = upHashes.begin(); it != upHashes.end(); it++)
	{
	  FileInfo info;
	  if (!rsFiles->FileDetails(*it, RS_FILE_HINTS_UPLOAD, info))
	  {
		continue;
	  }

	  std::list<TransferInfo>::iterator pit;
	  for(pit = info.peers.begin(); pit != info.peers.end(); pit++)
	  {
		symbol  	= "";
		coreId		= QString::fromStdString(info.hash);
		name    	= QString::fromStdString(info.fname);
		sources		= QString::fromStdString(rsPeers->getPeerName(pit->peerId));

		switch(pit->status)
		{
			case FT_STATE_FAILED:
				status = tr("Failed");
				break;
			case FT_STATE_OKAY:
				status = tr("Okay");
				break;
			case FT_STATE_WAITING:
				status = tr("Waiting");
				break;
			case FT_STATE_DOWNLOADING:
				status = tr("Uploading");
				break;
		    	case FT_STATE_COMPLETE:
			default:
				status = tr("Complete");
				break;

        	}

	//	if (info.downloadStatus == FT_STATE_COMPLETE)
	//	{
	//		status = "Complete";
	//	}

		dlspeed  	= pit->tfRate * 1024.0;
		fileSize 	= info.size;
		completed 	= info.transfered;
		progress 	= info.transfered * 100.0 / info.size;
		remaining   = (info.size - info.transfered) / (info.tfRate * 1024.0);

		addUploadItem(symbol, name, coreId, fileSize, progress,
				dlspeed, sources,  status, completed, remaining);
		ulCount++;
	  }

	  if (info.peers.size() == 0)
	  {
		symbol  	= "";
		coreId		= QString::fromStdString(info.hash);
		name    	= QString::fromStdString(info.fname);
		sources		= tr("Unknown");

		switch(info.downloadStatus)
		{
			case FT_STATE_FAILED:
				status = tr("Failed");
				break;
			case FT_STATE_OKAY:
				status = tr("Okay");
				break;
			case FT_STATE_WAITING:
				status = tr("Waiting");
				break;
			case FT_STATE_DOWNLOADING:
				status = tr("Uploading");
				break;
		    	case FT_STATE_COMPLETE:
			default:
				status = tr("Complete");
				break;

        	}

		dlspeed  	= info.tfRate * 1024.0;
		fileSize 	= info.size;
		completed 	= info.transfered;
		progress 	= info.transfered * 100.0 / info.size;
		remaining   = (info.size - info.transfered) / (info.tfRate * 1024.0);

		addUploadItem(symbol, name, coreId, fileSize, progress,
				dlspeed, sources,  status, completed, remaining);
		ulCount++;
	  }
	}
}

void TransfersDialog::cancel()
{
		QString queryWrn2;
    queryWrn2.clear();
    queryWrn2.append(tr("Are you sure that you want to cancel and delete these files?"));

    if ((QMessageBox::question(this, tr("RetroShare"),queryWrn2,QMessageBox::Ok|QMessageBox::No, QMessageBox::Ok))== QMessageBox::Ok)
    {


      for(int i = 0; i <= DLListModel->rowCount(); i++)
      {
        if(selection->isRowSelected(i, QModelIndex()))
        {
        std::string id = getID(i, DLListModel).toStdString();
        QString  qname = getFileName(i, DLListModel);
        /* XXX -> Should not have to 'trim' filename ... something wrong here..
        * but otherwise, not exact filename .... BUG
        */
        std::string name = (qname.trimmed()).toStdString();
        rsFiles->FileCancel(id); /* hash */

        }
      }

    }
    else
    return;
}

void TransfersDialog::handleDownloadRequest(const QString& url){

    RetroShareLinkAnalyzer analyzer (url);

    if (!analyzer.isValid ())
        return;

    QVector<RetroShareLinkData> linkList;
    analyzer.getFileInformation (linkList);

    std::list<std::string> srcIds;

    for (int i = 0, n = linkList.size (); i < n; ++i)
    {
        const RetroShareLinkData& linkData = linkList[i];

        rsFiles->FileRequest (linkData.getName ().toStdString (), linkData.getHash ().toStdString (),
            linkData.getSize ().toInt (), "", 0, srcIds);
    }
}

void TransfersDialog::copyLink ()
{
    QModelIndexList lst = ui.downloadList->selectionModel ()->selectedIndexes ();
    RetroShareLinkAnalyzer analyzer;

    for (int i = 0; i < lst.count (); i++)
    {
        if ( lst[i].column() == 0 )
        {
            QModelIndex & ind = lst[i];
            QString fhash= ind.model ()->data (ind.model ()->index (ind.row (), ID )).toString() ;
            QString fsize= ind.model ()->data (ind.model ()->index (ind.row (), SIZE)).toString() ;
            QString fname= ind.model ()->data (ind.model ()->index (ind.row (), NAME)).toString() ;

            analyzer.setRetroShareLink (fname, fsize, fhash);
        }
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(analyzer.getRetroShareLink ());
}

void TransfersDialog::pasteLink()
{
    QClipboard *clipboard = QApplication::clipboard();
    RetroShareLinkAnalyzer analyzer (clipboard->text ());

    if (!analyzer.isValid ())
        return;

    QVector<RetroShareLinkData> linkList;
    analyzer.getFileInformation (linkList);

    std::list<std::string> srcIds;

    for (int i = 0, n = linkList.size (); i < n; ++i)
    {
        const RetroShareLinkData& linkData = linkList[i];
        //downloadFileRequested(linkData.getName (), linkData.getSize ().toInt (),
        //    linkData.getHash (), "", -1, -1, -1, -1);
        rsFiles->FileRequest (linkData.getName ().toStdString (), linkData.getHash ().toStdString (),
            linkData.getSize ().toInt (), "", 0, srcIds);
    }
}

void TransfersDialog::clearcompleted()
{
    	std::cerr << "TransfersDialog::clearcompleted()" << std::endl;
   	rsFiles->FileClearCompleted();
}

double TransfersDialog::getProgress(int row, QStandardItemModel *model)
{
	return model->data(model->index(row, PROGRESS), Qt::DisplayRole).toDouble();
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
