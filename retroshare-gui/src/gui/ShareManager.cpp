/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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
#include "ShareManager.h"

#include "rsiface/rsfiles.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QCheckBox>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>

#include <QMessageBox>
#include <QComboBox>

/* Images for context menu icons */
#define IMAGE_CANCEL               ":/images/delete.png"

ShareManager *ShareManager::_instance = NULL ;

/** Default constructor */
ShareManager::ShareManager(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);


  connect(ui.addButton, SIGNAL(clicked( bool ) ), this , SLOT( addShareDirectory() ) );
  connect(ui.removeButton, SIGNAL(clicked( bool ) ), this , SLOT( removeShareDirectory() ) );
  connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

  connect( ui.shareddirList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( shareddirListCostumPopupMenu( QPoint ) ) );

	ui.addButton->setToolTip(tr("Add a Share Directory"));
	ui.removeButton->setToolTip(tr("Stop sharing selected Directory"));

	load();

  ui.shareddirList->horizontalHeader()->setResizeMode( 0,QHeaderView::Stretch);
  ui.shareddirList->horizontalHeader()->setResizeMode( 2,QHeaderView::Interactive); 
 
  ui.shareddirList->horizontalHeader()->resizeSection( 0, 360 );
  ui.shareddirList->horizontalHeader()->setStretchLastSection(false);


}

void ShareManager::shareddirListCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      removeAct = new QAction(QIcon(IMAGE_CANCEL), tr( "Remove" ), this );
      connect( removeAct , SIGNAL( triggered() ), this, SLOT( removeShareDirectory() ) );


      contextMnu.clear();
      contextMnu.addAction( removeAct );
      contextMnu.exec( mevent->globalPos() );
}

/** Loads the settings for this page */
void ShareManager::load()
{
	std::cerr << "ShareManager:: In load !!!!!" << std::endl ;

	std::list<SharedDirInfo>::const_iterator it;
	std::list<SharedDirInfo> dirs;
	rsFiles->getSharedDirectories(dirs);

	/* get a link to the table */
	QTableWidget *listWidget = ui.shareddirList;

	/* remove old items ??? */
	listWidget->clearContents() ;
	listWidget->setRowCount(0) ;

	connect(this,SIGNAL(itemClicked(QTableWidgetItem*)),this,SLOT(updateFlags(QTableWidgetItem*))) ;

	int row=0 ;
	for(it = dirs.begin(); it != dirs.end(); it++,++row)
	{
		listWidget->insertRow(row) ;
		listWidget->setItem(row,0,new QTableWidgetItem(QString::fromStdString((*it).filename)));
#ifdef USE_COMBOBOX
		QComboBox *cb = new QComboBox ;
		cb->addItem(QString("Network Wide")) ;
		cb->addItem(QString("Browsable")) ;
		cb->addItem(QString("Universal")) ;

		cb->setToolTip(QString("Decide here whether this directory is\n* Network Wide: \tanonymously shared over the network (including your friends)\n* Browsable: \tbrowsable by your friends\n* Universal: \t\tboth")) ;

		// TODO
		//  - set combobox current value depending on what rsFiles reports.
		//  - use a signal mapper to get the correct row that contains the combo box sending the signal:
		//  		mapper = new SignalMapper(this) ;
		//
		//  		for(all cb)
		//  		{
		//  			signalMapper->setMapping(cb,...)
		//  		}
		//
		int index = 0 ;
		index += ((*it).shareflags & RS_FILE_HINTS_NETWORK_WIDE) > 0 ;
		index += (((*it).shareflags & RS_FILE_HINTS_BROWSABLE) > 0) * 2 ;
		listWidget->setCellWidget(row,1,cb);

		if(index < 1 || index > 3)
			std::cerr << "******* ERROR IN FILE SHARING FLAGS. Flags = " << (*it).shareflags << " ***********" << std::endl ;
		else
			index-- ;

		cb->setCurrentIndex(index) ;
#else
		QCheckBox *cb1 = new QCheckBox ;
		QCheckBox *cb2 = new QCheckBox ;

		cb1->setChecked( (*it).shareflags & RS_FILE_HINTS_NETWORK_WIDE ) ;
		cb2->setChecked( (*it).shareflags & RS_FILE_HINTS_BROWSABLE ) ;

		cb1->setToolTip(QString("If checked, the share is anonymously shared to anybody.")) ;
		cb2->setToolTip(QString("If checked, the share is browsable by your friends.")) ;

		listWidget->setCellWidget(row,1,cb1);
		listWidget->setCellWidget(row,2,cb2);

		QObject::connect(cb1,SIGNAL(toggled(bool)),this,SLOT(updateFlags(bool))) ;
		QObject::connect(cb2,SIGNAL(toggled(bool)),this,SLOT(updateFlags(bool))) ;
#endif
	}

	//ui.incomingDir->setText(QString::fromStdString(rsFiles->getDownloadDirectory()));

	listWidget->update(); /* update display */
	update();
}

void ShareManager::showYourself()
{
	if(_instance == NULL)
		_instance = new ShareManager(NULL,0) ;

	_instance->show() ;
}

void ShareManager::addShareDirectory()
{

	/* select a dir
	 */


	QString qdir = QFileDialog::getExistingDirectory(this, tr("Select A Folder To Share"), "",
			QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	/* add it to the server */
	std::string dir = qdir.toStdString();
	if (dir != "")
	{
		SharedDirInfo sdi ;
		sdi.filename = dir ;
		sdi.shareflags = RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_BROWSABLE ;

		rsFiles->addSharedDirectory(sdi);

		messageBoxOk(tr("Shared Directory Added!"));
		load();
	}
}

void ShareManager::updateFlags(bool b)
{
	std::cerr << "Updating flags (b=" << b << ") !!!" << std::endl ;

	std::list<SharedDirInfo>::iterator it;
	std::list<SharedDirInfo> dirs;
	rsFiles->getSharedDirectories(dirs);

	int row=0 ;
	for(it = dirs.begin(); it != dirs.end(); it++,++row)
	{
		std::cerr << "Looking for row=" << row << ", file=" << (*it).filename << ", flags=" << (*it).shareflags << std::endl ;
		uint32_t current_flags = 0 ;
		current_flags |= (dynamic_cast<QCheckBox*>(ui.shareddirList->cellWidget(row,1)))->isChecked()? RS_FILE_HINTS_NETWORK_WIDE:0 ;
		current_flags |= (dynamic_cast<QCheckBox*>(ui.shareddirList->cellWidget(row,2)))->isChecked()? RS_FILE_HINTS_BROWSABLE:0 ;

		if( (*it).shareflags ^ current_flags )
		{
			(*it).shareflags = current_flags ;
			rsFiles->updateShareFlags(*it) ;	// modifies the flags

			std::cout << "Updating share flags for directory " << (*it).filename << std::endl ;
		}
	}
}

void ShareManager::removeShareDirectory()
{
	/* id current dir */
	/* ask for removal */
	QTableWidget *listWidget = ui.shareddirList;
	int row = listWidget -> currentRow();
	QTableWidgetItem *qdir = listWidget->item(row,0) ;

	QString queryWrn;
	queryWrn.clear();
	queryWrn.append(tr("Do you really want to stop sharing this directory ? "));

	if (qdir)
	{
		if ((QMessageBox::question(this, tr("Warning!"),queryWrn,QMessageBox::Ok|QMessageBox::No, QMessageBox::Ok))== QMessageBox::Ok)
		{
			rsFiles->removeSharedDirectory( qdir->text().toStdString());
			load();
		}
		else
		return;
	}
}

bool  ShareManager::messageBoxOk(QString msg)
 {
    QMessageBox mb("Share Manager InfoBox!",msg,QMessageBox::Information,QMessageBox::Ok,0,0);
    mb.setButtonText( QMessageBox::Ok, "OK" );
		mb.setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));
    mb.exec();
    return true;
 }

void ShareManager::showEvent(QShowEvent *event)
{
	if (!event->spontaneous())
	{
		load();
	}
}
