/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/TransferUserNotify.cpp                  *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "retroshare/rsfiles.h"
#include "TransferUserNotify.h"
#include "gui/notifyqt.h"
#include "gui/MainWindow.h"

TransferUserNotify::TransferUserNotify(QObject *parent) :
	UserNotify(parent)
{
	newTransferCount = 0;

//	connect(NotifyQt::getInstance(), SIGNAL(downloadCompleteCountChanged(int)), this, SLOT(downloadCountChanged(int)));
}

bool TransferUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = tr("Download completed");
	if (group) *group = "Transfer";

	return true;
}

QIcon TransferUserNotify::getIcon()
{
	return QIcon(":/icons/png/fileshare.png");
}

QIcon TransferUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? QIcon(":/icons/png/filesharing-notify.png") : QIcon(":/icons/png/fileshare.png");
}

unsigned int TransferUserNotify::getNewCount()
{
    std::list<RsFileHash> hashs;
    rsFiles->FileDownloads(hashs);
    FileInfo info;

    newTransferCount = 0;

    for(auto hash: hashs)
		if(rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, info) && info.downloadStatus==FT_STATE_COMPLETE)
            ++newTransferCount;

    return newTransferCount;
}

QString TransferUserNotify::getTrayMessage(bool plural)
{
	return plural ? tr("You have %1 completed downloads") : tr("You have %1 completed download");
}

QString TransferUserNotify::getNotifyMessage(bool plural)
{
	return plural ? tr("%1 completed downloads") : tr("%1 completed download");
}

void TransferUserNotify::iconClicked()
{
	MainWindow::showWindow(MainWindow::Transfers);
}

