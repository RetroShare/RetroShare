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
#include "moreinfo/moreinfo.h"
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
#include "rsiface/rsiface.h"

/* Images for context menu icons */
#define IMAGE_INFO                 ":/images/fileinfo.png"
#define IMAGE_CANCEL               ":/images/delete.png"
#define IMAGE_CLEARCOMPLETED       ":/images/deleteall.png"

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
	
  
  	//Selection Setup
	selection = ui.downloadList->selectionModel();
  
    /* Set header resize modes and initial section sizes Downloads TreeView*/
//	QHeaderView * _header = ui.downloadList->header () ;
//   	_header->setResizeMode (0, QHeaderView::Interactive); /*Name*/
//	_header->setResizeMode (1, QHeaderView::Interactive); /*Size*/
//	_header->setResizeMode (2, QHeaderView::Interactive); /*Progress*/
//	_header->setResizeMode (3, QHeaderView::Interactive); /*Speed*/
//	_header->setResizeMode (4, QHeaderView::Interactive); /*Sources*/
//	_header->setResizeMode (5, QHeaderView::Interactive); /*Status*/
//	_header->setResizeMode (6, QHeaderView::Interactive); /*Completed*/
//	_header->setResizeMode (7, QHeaderView::Interactive); /*Remaining */
//
//    
//	_header->resizeSection ( 0, 100 ); /*Name*/
//	_header->resizeSection ( 1, 75 ); /*Size*/
//	_header->resizeSection ( 2, 170 ); /*Progress*/
//	_header->resizeSection ( 3, 75 ); /*Speed*/
//	_header->resizeSection ( 4, 100 ); /*Sources*/
//	_header->resizeSection ( 5, 100 ); /*Status*/
//	_header->resizeSection ( 6, 75 ); /*Completed*/
//	_header->resizeSection ( 7, 100 ); /*Remaining */
	
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

void TransfersDialog::downloadListCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

//      showdowninfoAct = new QAction(QIcon(IMAGE_INFO), tr( "Details..." ), this );
//      connect( showdowninfoAct , SIGNAL( triggered() ), this, SLOT( showDownInfoWindow() ) );

	  cancelAct = new QAction(QIcon(IMAGE_CANCEL), tr( "Cancel" ), this );
      connect( cancelAct , SIGNAL( triggered() ), this, SLOT( cancel() ) );
      
      clearcompletedAct = new QAction(QIcon(IMAGE_CLEARCOMPLETED), tr( "Clear Completed" ), this );
      connect( clearcompletedAct , SIGNAL( triggered() ), this, SLOT( clearcompleted() ) );

      contextMnu.clear();
      contextMnu.addAction( cancelAct);
//      contextMnu.addSeparator();     
//      contextMnu.addAction( showdowninfoAct);
      contextMnu.addSeparator();
      contextMnu.addAction( clearcompletedAct);
      contextMnu.exec( mevent->globalPos() );
}

/** Shows Downloads Informations */
void TransfersDialog::showDownInfoWindow()
{
    moreinfo *detailsdlg = new moreinfo();
    detailsdlg->show();
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
	DLListModel->setData(DLListModel->index(row, PROGRESS), QVariant((double)progress));
	DLListModel->setData(DLListModel->index(row, DLSPEED), QVariant((double)dlspeed));
	DLListModel->setData(DLListModel->index(row, SOURCES), QVariant((QString)sources));
	DLListModel->setData(DLListModel->index(row, STATUS), QVariant((QString)status));
	DLListModel->setData(DLListModel->index(row, COMPLETED), QVariant((qlonglong)completed));
	DLListModel->setData(DLListModel->index(row, REMAINING), QVariant((qlonglong)remaining));
	DLListModel->setData(DLListModel->index(row, ID), QVariant((QString)coreID));
	return row;
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
	ULListModel->setData(ULListModel->index(row, UTRANSFERRED), QVariant((qlonglong)remaining));

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

	//remove all Items 
	for(int i = DLListModel->rowCount(); i >= 0; i--) 
	{
		delItem(i);
	}

	for(int i = ULListModel->rowCount(); i >= 0; i--) 
	{
		delUploadItem(i);
	}
	
	
	//nun aktuelle DownloadListe hinzufügen
	rsiface->lockData(); /* Lock Interface */

	std::list<FileTransferInfo>::const_iterator it;
	const std::list<FileTransferInfo> &transfers = rsiface->getTransferList();

	for(it = transfers.begin(); it != transfers.end(); it++) 
	{
		
		symbol  	= "";
		coreId		= "";
		name    	= QString::fromStdString(it->fname);
		sources  	= QString::fromStdString(it->source);

		/* Replace ID with HASH -> as thats what we need to cancel! */
		//std::ostringstream out;
		//out << it->id;
		//coreId		= QString::fromStdString(it->hashout.str());
		coreId		= QString::fromStdString(it->hash);

		switch(it->downloadStatus) 
		{
			/* XXX HAND CODED! */
			case 0: /* FAILED */
				status = "Failed";
				break;
			case 1: /* Downloading */
				if (it->tfRate > 0.01)
				{
					status = "Downloading";
				}
				else
				{
					status = "Waiting for Peer";
				}
				break;
		    case 2: /* COMPLETE */
			default:
				status = "Complete";
				break;
		
        	}
        
		dlspeed  	= it->tfRate * 1024.0;
		fileSize 	= it->size;
		completed 	= it->transfered;
		progress 	= it->transfered * 100.0 / it->size;
		remaining   = (it->size - it->transfered) / (it->tfRate * 1024.0);
	
		if (it->download)
		{
			addItem(symbol, name, coreId, fileSize, progress, 
					dlspeed, sources,  status, completed, remaining);
		}
		else
		{
			addUploadItem(symbol, name, coreId, fileSize, progress, 
					dlspeed, sources,  status, completed, remaining);
		}
	}

	rsiface->unlockData(); /* UnLock Interface */
}

void TransfersDialog::cancel()
{
	for(int i = 0; i <= DLListModel->rowCount(); i++) {
		if(selection->isRowSelected(i, QModelIndex())) {
			std::string id = getID(i, DLListModel).toStdString();
			QString  qname = getFileName(i, DLListModel);
			/* XXX -> Should not have to 'trim' filename ... something wrong here..
			 * but otherwise, not exact filename .... BUG
			 */
			std::string name = (qname.trimmed()).toStdString();
			// TODO
			rsicontrol->FileCancel(name, id, 0); /* name, *hash*, size */
			//std::cerr << "TranfersDialog::cancel(): " << name << ":" << id << ":" << 0 << std::endl;
		}
	}
}

void TransfersDialog::clearcompleted()
{
    std::cerr << "TransfersDialog::clearcompleted()" << std::endl;
	rsicontrol->FileClearCompleted();
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
