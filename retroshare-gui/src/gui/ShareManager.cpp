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

#include <QContextMenuEvent>
#include <QMenu>
#include <QCheckBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QUrl>

#include <retroshare/rsfiles.h>

#include "ShareManager.h"
#include "ShareDialog.h"
#include "settings/rsharesettings.h"

/* Images for context menu icons */
#define IMAGE_CANCEL               ":/images/delete.png"
#define IMAGE_EDIT                 ":/images/edit_16.png"

#define COLUMN_PATH         0
#define COLUMN_VIRTUALNAME  1
#define COLUMN_NETWORKWIDE  2
#define COLUMN_BROWSABLE    3
#define COLUMN_COUNT        3

ShareManager *ShareManager::_instance = NULL ;

/** Default constructor */
ShareManager::ShareManager()
  : QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    ui.headerFrame->setHeaderImage(QPixmap(":/images/fileshare64.png"));
    ui.headerFrame->setHeaderText(tr("Share Manager"));

    isLoading = false;
    load();

    Settings->loadWidgetInformation(this);

    connect(ui.addButton, SIGNAL(clicked( bool ) ), this , SLOT( showShareDialog() ) );
    connect(ui.editButton, SIGNAL(clicked( bool ) ), this , SLOT( editShareDirectory() ) );
    connect(ui.removeButton, SIGNAL(clicked( bool ) ), this , SLOT( removeShareDirectory() ) );
    connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(close()));

    connect(ui.shareddirList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(shareddirListCostumPopupMenu(QPoint)));
    connect(ui.shareddirList, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(shareddirListCurrentCellChanged(int,int,int,int)));

    ui.editButton->setEnabled(false);
    ui.removeButton->setEnabled(false);

    QHeaderView* header = ui.shareddirList->horizontalHeader();
    header->setResizeMode( COLUMN_PATH, QHeaderView::Stretch);

    header->setResizeMode(COLUMN_NETWORKWIDE, QHeaderView::Fixed);
    header->setResizeMode(COLUMN_BROWSABLE, QHeaderView::Fixed);

    header->setHighlightSections(false);

    ui.shareddirList->setRangeSelected(QTableWidgetSelectionRange(0, 0, 0, COLUMN_COUNT), true);

    setAcceptDrops(true);

    setAttribute(Qt::WA_DeleteOnClose, true);
}

ShareManager::~ShareManager()
{
    _instance = NULL;

    Settings->saveWidgetInformation(this);
}

void ShareManager::shareddirListCostumPopupMenu( QPoint /*point*/ )
{
    QMenu contextMnu( this );

    QAction *editAct = new QAction(QIcon(IMAGE_EDIT), tr( "Edit" ), &contextMnu );
    connect( editAct , SIGNAL( triggered() ), this, SLOT( editShareDirectory() ) );

    QAction *removeAct = new QAction(QIcon(IMAGE_CANCEL), tr( "Remove" ), &contextMnu );
    connect( removeAct , SIGNAL( triggered() ), this, SLOT( removeShareDirectory() ) );

    contextMnu.addAction( editAct );
    contextMnu.addAction( removeAct );

    contextMnu.exec(QCursor::pos());
}

/** Loads the settings for this page */
void ShareManager::load()
{
    isLoading = true;
    std::cerr << "ShareManager:: In load !!!!!" << std::endl ;

    std::list<SharedDirInfo>::const_iterator it;
    std::list<SharedDirInfo> dirs;
    rsFiles->getSharedDirectories(dirs);

    /* get a link to the table */
    QTableWidget *listWidget = ui.shareddirList;

    /* set new row count */
    listWidget->setRowCount(dirs.size());

    connect(this,SIGNAL(itemClicked(QTableWidgetItem*)),this,SLOT(updateFlags(QTableWidgetItem*))) ;

#ifndef USE_COMBOBOX
    QString ToolTips [2] = { tr("If checked, the share is anonymously shared to anybody."),
                             tr("If checked, the share is browsable by your friends.") };
    int Flags [2] = { RS_FILE_HINTS_NETWORK_WIDE, RS_FILE_HINTS_BROWSABLE };
#endif

    int row=0 ;
    for(it = dirs.begin(); it != dirs.end(); it++,++row)
    {
        listWidget->setItem(row, COLUMN_PATH, new QTableWidgetItem(QString::fromUtf8((*it).filename.c_str())));
        listWidget->setItem(row, COLUMN_VIRTUALNAME, new QTableWidgetItem(QString::fromUtf8((*it).virtualname.c_str())));

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
        int col;
        for (col = 0; col <= 1; col++) {
            QModelIndex index = listWidget->model()->index(row, col + COLUMN_NETWORKWIDE, QModelIndex());
            QWidget* widget = dynamic_cast<QWidget*>(listWidget->indexWidget(index));
            QCheckBox* cb = NULL;
            if (widget) {
                cb = dynamic_cast<QCheckBox*>(widget->children().front());
            }
            if (cb == NULL) {
                QWidget* widget = new QWidget;

                cb = new QCheckBox(widget);
                cb->setToolTip(ToolTips [col]);

                QHBoxLayout* layout = new QHBoxLayout(widget);
                layout->addWidget(cb, 0, Qt::AlignCenter);
                layout->setSpacing(0);
                layout->setContentsMargins(10, 0, 0, 0); // to be centered
                widget->setLayout(layout);

                listWidget->setCellWidget(row, col + COLUMN_NETWORKWIDE, widget);

                QObject::connect(cb, SIGNAL(toggled(bool)), this, SLOT(updateFlags(bool))) ;
            }
            cb->setChecked((*it).shareflags & Flags [col]);
        }
#endif
    }

    //ui.incomingDir->setText(QString::fromStdString(rsFiles->getDownloadDirectory()));

    listWidget->update(); /* update display */
    update();

    isLoading = false ;
}

void ShareManager::showYourself()
{
    if(_instance == NULL)
        _instance = new ShareManager() ;

    _instance->show() ;
    _instance->activateWindow();
}

/*static*/ void ShareManager::postModDirectories(bool update_local)
{
   if (_instance == NULL || _instance->isHidden()) {
       return;
   }

   if (update_local) {
       _instance->load();
   }
}

void ShareManager::updateFlags(bool b)
{
    if(isLoading)
        return ;

    std::cerr << "Updating flags (b=" << b << ") !!!" << std::endl ;

    std::list<SharedDirInfo>::iterator it;
    std::list<SharedDirInfo> dirs;
    rsFiles->getSharedDirectories(dirs);

    int row=0 ;
    for(it = dirs.begin(); it != dirs.end(); it++,++row)
    {
        std::cerr << "Looking for row=" << row << ", file=" << (*it).filename << ", flags=" << (*it).shareflags << std::endl ;
        uint32_t current_flags = 0 ;
        current_flags |= (dynamic_cast<QCheckBox*>(ui.shareddirList->cellWidget(row,COLUMN_NETWORKWIDE)->children().front()))->isChecked()? RS_FILE_HINTS_NETWORK_WIDE:0 ;
        current_flags |= (dynamic_cast<QCheckBox*>(ui.shareddirList->cellWidget(row,COLUMN_BROWSABLE)->children().front()))->isChecked()? RS_FILE_HINTS_BROWSABLE:0 ;

        if( (*it).shareflags ^ current_flags )
        {
            (*it).shareflags = current_flags ;
            rsFiles->updateShareFlags(*it) ;	// modifies the flags

            std::cout << "Updating share flags for directory " << (*it).filename << std::endl ;
        }
    }
}

void ShareManager::editShareDirectory()
{
    /* id current dir */
    int row = ui.shareddirList->currentRow();
    QTableWidgetItem *item = ui.shareddirList->item(row, COLUMN_PATH);

    if (item) {
        std::string filename = item->text().toUtf8().constData();

        std::list<SharedDirInfo> dirs;
        rsFiles->getSharedDirectories(dirs);

        std::list<SharedDirInfo>::const_iterator it;
        for (it = dirs.begin(); it != dirs.end(); it++) {
            if (it->filename == filename) {
                /* file name found, show dialog */
                ShareDialog sharedlg (it->filename, this);
                sharedlg.exec();
                break;
            }
        }
    }
}

void ShareManager::removeShareDirectory()
{
    /* id current dir */
    /* ask for removal */
    QTableWidget *listWidget = ui.shareddirList;
    int row = listWidget -> currentRow();
    QTableWidgetItem *qdir = listWidget->item(row, COLUMN_PATH);

    if (qdir)
    {
        if ((QMessageBox::question(this, tr("Warning!"),tr("Do you really want to stop sharing this directory ?"),QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
        {
            rsFiles->removeSharedDirectory( qdir->text().toUtf8().constData());
            load();
        }
    }
}

void ShareManager::showEvent(QShowEvent *event)
{
    if (!event->spontaneous())
    {
        load();
    }
}

void ShareManager::showShareDialog()
{
    ShareDialog sharedlg ("", this);
    sharedlg.exec();
}

void ShareManager::shareddirListCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);

    if (currentRow >= 0) {
        ui.editButton->setEnabled(true);
        ui.removeButton->setEnabled(true);
    } else {
        ui.editButton->setEnabled(false);
        ui.removeButton->setEnabled(false);
    }
}

void ShareManager::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	}
}

void ShareManager::dropEvent(QDropEvent *event)
{
	if (!(Qt::CopyAction & event->possibleActions())) {
		/* can't do it */
		return;
	}

	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;

	bool errorShown = false;

	if (event->mimeData()->hasUrls()) {
		QList<QUrl> urls = event->mimeData()->urls();
		QList<QUrl>::iterator it;
		for (it = urls.begin(); it != urls.end(); it++) {
			QString localpath = it->toLocalFile();

			if (localpath.isEmpty() == false) {
				QDir dir(localpath);
				if (dir.exists()) {
					SharedDirInfo sdi;
					sdi.filename = localpath.toUtf8().constData();
					sdi.virtualname.clear();

					sdi.shareflags = 0;

					/* add new share */
					rsFiles->addSharedDirectory(sdi);
				} else if (QFile::exists(localpath)) {
					if (errorShown == false) {
						QMessageBox mb(tr("Drop file error."), tr("File can't be dropped, only directories are accepted."), QMessageBox::Information, QMessageBox::Ok, 0, 0, this);
						mb.exec();
						errorShown = true;
					}
				} else {
					QMessageBox mb(tr("Drop file error."), tr("Directory not found or directory name not accepted."), QMessageBox::Information, QMessageBox::Ok, 0, 0, this);
					mb.exec();
				}
			}
		}
	}

	event->setDropAction(Qt::CopyAction);
	event->accept();
}
