/*******************************************************************************
 * gui/ShareManager.cpp                                                        *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
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

#include <QContextMenuEvent>
#include <QMenu>
#include <QCheckBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QUrl>
#include <QMimeData>

#include <retroshare/rsfiles.h>
#include <retroshare/rstypes.h>
#include <retroshare/rspeers.h>

#include "ShareManager.h"
#include "settings/rsharesettings.h"
#include "gui/common/GroupFlagsWidget.h"
#include "gui/common/GroupSelectionBox.h"
#include "gui/common/GroupDefs.h"
#include "gui/notifyqt.h"
#include "util/QtVersion.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"

/* Images for context menu icons */
#define IMAGE_CANCEL               ":/images/delete.png"
#define IMAGE_EDIT                 ":/icons/png/pencil-edit-button.png"

#define COLUMN_PATH         0
#define COLUMN_VIRTUALNAME  1
#define COLUMN_SHARE_FLAGS  2
#define COLUMN_GROUPS       3

ShareManager *ShareManager::_instance = NULL ;

/** Default constructor */
ShareManager::ShareManager()
  : QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/fileshare64.png"));
    ui.headerFrame->setHeaderText(tr("Share Manager"));

    isLoading = false;

    Settings->loadWidgetInformation(this);

    connect(ui.addButton, SIGNAL(clicked( bool ) ), this , SLOT( addShare() ) );
    connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyAndClose()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));

    connect(ui.shareddirList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(shareddirListCustomPopupMenu(QPoint)));
    connect(ui.shareddirList, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(doubleClickedCell(int,int)));
    connect(ui.shareddirList, SIGNAL(cellChanged(int,int)), this, SLOT(handleCellChange(int,int)));

    connect(NotifyQt::getInstance(), SIGNAL(groupsChanged(int)), this, SLOT(reload()));

    QHeaderView* header = ui.shareddirList->horizontalHeader();
    QHeaderView_setSectionResizeModeColumn(header, COLUMN_PATH, QHeaderView::Stretch);

    header->setHighlightSections(false);

    setAcceptDrops(true);
    setAttribute(Qt::WA_DeleteOnClose, true);

    reload();
}

void ShareManager::handleCellChange(int row,int column)
{
    if(isLoading)
        return ;

    if(column == COLUMN_VIRTUALNAME)
    {
        // check if the thing already exists

        for(uint32_t i=0;i<mDirInfos.size();++i)
            if(i != (uint32_t)row && (mDirInfos[row].virtualname == std::string(ui.shareddirList->item(i,COLUMN_VIRTUALNAME)->text().toUtf8())))
            {
                ui.shareddirList->item(row,COLUMN_VIRTUALNAME)->setText(QString::fromUtf8(mDirInfos[row].virtualname.c_str())) ;
                return ;
            }

        mDirInfos[row].virtualname = std::string(ui.shareddirList->item(row,COLUMN_VIRTUALNAME)->text().toUtf8()) ;
    }
}

void ShareManager::editShareDirectory()
{
    QTableWidget *listWidget = ui.shareddirList;
    int row = listWidget->currentRow();
    int col = listWidget->currentColumn();

    if(col == COLUMN_VIRTUALNAME)
    {
        QModelIndex index = ui.shareddirList->model()->index(row,col,QModelIndex());
        ui.shareddirList->edit(index);
    }
	else
		doubleClickedCell(row,col) ;
}

void ShareManager::doubleClickedCell(int row,int column)
{
    if(column == COLUMN_PATH)
    {
		QString dirname = misc::getExistingDirectory(nullptr,tr("Choose directory"),QString());

        if(!dirname.isNull())
        {
            std::string new_name( dirname.toUtf8() );

            for(uint32_t i=0;i<mDirInfos.size();++i)
                if(mDirInfos[row].filename == new_name)
                    return ;

            mDirInfos[row].filename = new_name ;
            load();
        }
    }
    else if(column == COLUMN_GROUPS)
    {
        std::list<RsNodeGroupId> selected_groups = GroupSelectionDialog::selectGroups(mDirInfos[row].parent_groups) ;

        mDirInfos[row].parent_groups = selected_groups ;
        load();
    }
}

ShareManager::~ShareManager()
{
    _instance = NULL;

    Settings->saveWidgetInformation(this);
}

void ShareManager::cancel()
{
    close();
}
void ShareManager::applyAndClose()
{
    // This is the only place where we change things.

    std::list<SharedDirInfo> infos ;

    for(uint32_t i=0;i<mDirInfos.size();++i)
        infos.push_back(mDirInfos[i]) ;

    rsFiles->setSharedDirectories(infos) ;

	close() ;
}

void ShareManager::shareddirListCustomPopupMenu( QPoint /*point*/ )
{
    QMenu contextMnu( this );

    int col = ui.shareddirList->currentColumn();
    QString edit_text ;

    switch(col)
    {
    	case COLUMN_GROUPS: edit_text = tr("Change group visibility...") ; break ;
		case COLUMN_PATH:   edit_text = tr("Choose directory to share...") ; break;
		case COLUMN_VIRTUALNAME:   edit_text = tr("Choose visible name...") ; break;
    default:
		case COLUMN_SHARE_FLAGS:   return ;
    }
    QAction *editAct = new QAction(QIcon(IMAGE_EDIT), edit_text, &contextMnu );
    connect( editAct , SIGNAL( triggered() ), this, SLOT( editShareDirectory() ) );

    QAction *removeAct = new QAction(QIcon(IMAGE_CANCEL), tr( "Remove" ), &contextMnu );
    connect( removeAct , SIGNAL( triggered() ), this, SLOT( removeShareDirectory() ) );

    contextMnu.addAction( editAct );
    contextMnu.addAction( removeAct );

    contextMnu.exec(QCursor::pos());
}

void ShareManager::reload()
{
    std::list<SharedDirInfo> dirs ;
    rsFiles->getSharedDirectories(dirs) ;

    mDirInfos.clear();

    for(std::list<SharedDirInfo>::const_iterator it(dirs.begin());it!=dirs.end();++it)
        mDirInfos.push_back(*it) ;

    load();
}

/** Loads the settings for this page */
void ShareManager::load()
{
    if(isLoading)
        return ;

    isLoading = true;

    /* get a link to the table */
    QTableWidget *listWidget = ui.shareddirList;

    /* set new row count */
    listWidget->setRowCount(mDirInfos.size());

    for(uint32_t row=0;row<mDirInfos.size();++row)
    {
        listWidget->setItem(row, COLUMN_PATH, new QTableWidgetItem(QString::fromUtf8(mDirInfos[row].filename.c_str())));

		if(mDirInfos[row].virtualname.empty())
			listWidget->setItem(row, COLUMN_VIRTUALNAME, new QTableWidgetItem(tr("[Unset] (Double click to change)"))) ;
		else
			listWidget->setItem(row, COLUMN_VIRTUALNAME, new QTableWidgetItem(QString::fromUtf8(mDirInfos[row].virtualname.c_str())));

        GroupFlagsWidget *widget = new GroupFlagsWidget(NULL,mDirInfos[row].shareflags);

        listWidget->setRowHeight(row, 32 * QFontMetricsF(font()).height()/14.0);
        listWidget->setCellWidget(row, COLUMN_SHARE_FLAGS, widget);

        listWidget->setItem(row, COLUMN_GROUPS, new QTableWidgetItem()) ;
        listWidget->item(row,COLUMN_GROUPS)->setBackgroundColor(QColor(183,236,181)) ;

        connect(widget,SIGNAL(flagsChanged(FileStorageFlags)),this,SLOT(updateFlags())) ;

        listWidget->item(row,COLUMN_GROUPS)->setToolTip(tr("Double click to select which groups of friends can see the files")) ;
		listWidget->item(row,COLUMN_VIRTUALNAME)->setToolTip(tr("Double click to change the name that friends will see.")) ;

        listWidget->item(row,COLUMN_GROUPS)->setText(getGroupString(mDirInfos[row].parent_groups));

		//TODO (Phenom): Add qproperty for these text colors in stylesheets
		// As palette is not updated by stylesheet
		QFont font = listWidget->item(row,COLUMN_GROUPS)->font();
		font.setBold(mDirInfos[row].shareflags & DIR_FLAGS_BROWSABLE) ;
		listWidget->item(row,COLUMN_GROUPS)->setData(Qt::ForegroundRole, QColor((mDirInfos[row].shareflags & DIR_FLAGS_BROWSABLE) ? (Qt::black):(Qt::lightGray)) );
		listWidget->item(row,COLUMN_GROUPS)->setFont(font);

		if(QDir(QString::fromUtf8(mDirInfos[row].filename.c_str())).exists())
		{
			listWidget->item(row,COLUMN_PATH)->setData(Qt::ForegroundRole, QColor(Qt::black));
			listWidget->item(row,COLUMN_PATH)->setToolTip(tr("Double click to change shared directory path")) ;
		}
		else
		{
			listWidget->item(row,COLUMN_PATH)->setData(Qt::ForegroundRole, QColor(Qt::lightGray));
			listWidget->item(row,COLUMN_PATH)->setToolTip(tr("Directory does not exist! Double click to change shared directory path")) ;
		}
    }

    listWidget->setColumnWidth(COLUMN_SHARE_FLAGS,132 * QFontMetricsF(font()).height()/14.0) ;

    listWidget->update(); /* update display */
    update();

    isLoading = false ;
}

void ShareManager::showYourself()
{
    if(_instance == NULL)
        _instance = new ShareManager() ;

    _instance->reload() ;
    _instance->show() ;
    _instance->activateWindow();
}

/*static*/ void ShareManager::postModDirectories(bool update_local)
{
   if (_instance == NULL || _instance->isHidden()) {
       return;
   }

   if (update_local) {
       _instance->reload();
   }
}

void ShareManager::updateFlags()
{
    for(int row=0;row<ui.shareddirList->rowCount();++row)
    {
        FileStorageFlags flags = (dynamic_cast<GroupFlagsWidget*>(ui.shareddirList->cellWidget(row,COLUMN_SHARE_FLAGS)))->flags() ;
        mDirInfos[row].shareflags = flags ;
    }
    load() ;				// update the GUI.
}

QString ShareManager::getGroupString(const std::list<RsNodeGroupId>& groups)
{
    if(groups.empty())
        return tr("[All friend nodes]") ;

    int n = 0;
    QString group_string ;

    for (std::list<RsNodeGroupId>::const_iterator it(groups.begin());it!=groups.end();++it,++n)
    {
        if (n>0)
            group_string += ", " ;

        RsGroupInfo groupInfo;
        rsPeers->getGroupInfo(*it, groupInfo);
        group_string += GroupDefs::name(groupInfo);
    }

    return group_string ;
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
            for(uint32_t i=row;i+1<mDirInfos.size();++i)
                mDirInfos[i] = mDirInfos[i+1] ;

            mDirInfos.pop_back() ;
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

void ShareManager::addShare()
{
	QString fname = misc::getExistingDirectory(nullptr,tr("Choose a directory to share"),QString());

    if(fname.isNull())
        return;

    std::string dir_name ( fname.toUtf8() );

    // check that the directory does not already exist

    for(uint32_t i=0;i<mDirInfos.size();++i)
        if(mDirInfos[i].filename == dir_name)
            return ;

    mDirInfos.push_back(SharedDirInfo());
    mDirInfos.back().filename = dir_name ;
    mDirInfos.back().virtualname = std::string();
    mDirInfos.back().shareflags = DIR_FLAGS_ANONYMOUS_DOWNLOAD | DIR_FLAGS_ANONYMOUS_SEARCH;
    mDirInfos.back().parent_groups.clear();

    load();
}

void ShareManager::shareddirListCurrentCellChanged(int /*currentRow*/, int /*currentColumn*/, int /*previousRow*/, int /*previousColumn*/)
{
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
		for (it = urls.begin(); it != urls.end(); ++it) {
			QString localpath = it->toLocalFile();

			if (localpath.isEmpty() == false) {
				QDir dir(localpath);
				if (dir.exists()) {
					SharedDirInfo sdi;
					sdi.filename = localpath.toUtf8().constData();
					sdi.virtualname.clear();

					sdi.shareflags.clear() ;

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

    load();
}
