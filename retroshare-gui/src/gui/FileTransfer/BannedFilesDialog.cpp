/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/BannedFilesDialog.cpp                   *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
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

#include <QMenu>
#include <QDateTime>

#include "retroshare/rsfiles.h"

#include "BannedFilesDialog.h"

#define COLUMN_FILE_NAME 0
#define COLUMN_FILE_HASH 1
#define COLUMN_FILE_SIZE 2
#define COLUMN_FILE_TIME 3

BannedFilesDialog::BannedFilesDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
    ui.setupUi(this);

    fillFilesList() ;

	connect(ui.bannedFiles_TW, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(bannedFilesContextMenu(QPoint)));
}

BannedFilesDialog::~BannedFilesDialog() {}

void BannedFilesDialog::unbanFile()
{
    int row = ui.bannedFiles_TW->currentRow();

    QTableWidgetItem *item = ui.bannedFiles_TW->item(row, COLUMN_FILE_HASH);

	if(!item)
	   return ;

    RsFileHash hash(item->data(Qt::UserRole).toString().toStdString()) ;
    rsFiles->unbanFile(hash) ;

    fillFilesList();
}

void BannedFilesDialog::bannedFilesContextMenu(QPoint)
{
	QMenu menu(this);

	menu.addAction(QIcon(":/images/FeedAdd.png"), tr("Remove"), this, SLOT(unbanFile()));

	menu.exec(QCursor::pos());
}

void BannedFilesDialog::fillFilesList()
{
    std::map<RsFileHash,BannedFileEntry> banned_files ;

    rsFiles->getPrimaryBannedFilesList(banned_files);
    int row=0;

    ui.bannedFiles_TW->setRowCount(banned_files.size()) ;

    for(auto it(banned_files.begin());it!=banned_files.end();++it)
    {
		ui.bannedFiles_TW->setItem(row, COLUMN_FILE_NAME, new QTableWidgetItem(QIcon(),QString::fromUtf8(it->second.mFilename.c_str()),0));
		ui.bannedFiles_TW->setItem(row, COLUMN_FILE_HASH, new QTableWidgetItem(QIcon(),QString::fromStdString(it->first.toStdString()),0));
		ui.bannedFiles_TW->setItem(row, COLUMN_FILE_SIZE, new QTableWidgetItem(QIcon(),QString::number(it->second.mSize),0));
		ui.bannedFiles_TW->setItem(row, COLUMN_FILE_TIME, new QTableWidgetItem(QIcon(),QDateTime::fromTime_t(it->second.mBanTimeStamp).toString(),0));

		ui.bannedFiles_TW->item(row, COLUMN_FILE_HASH)->setData(Qt::UserRole, QString::fromStdString(it->first.toStdString()));

		row++;
    }
}
