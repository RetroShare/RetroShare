/*******************************************************************************
 * gui/toaster/DownloadToaster.cpp                                             *
 *                                                                             *
 * Copyright (c) 2010 Retroshare Team <retroshare.project@gmail.com>           *
 * Copyright (C) 2007 - 2010 Xesc & Technology                                 *
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

#include <QMessageBox>
#include <QFileInfo>
#include <QUrl>

#include "DownloadToaster.h"
#include "gui/common/RsUrlHandler.h"

#include <retroshare/rsfiles.h>

DownloadToaster::DownloadToaster(const RsFileHash &hash, const QString &name) : QWidget(NULL)
{
	ui.setupUi(this);

	fileHash = hash;

	/* connect buttons */
	connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(hide()));
	connect(ui.toasterButton, SIGNAL(clicked()), this, SLOT(play()));

	/* set informations */
	ui.textLabel->setText(name);
}

void DownloadToaster::play()
{
	/* look up path */
	FileInfo fi;
	if (!rsFiles->FileDetails(fileHash, RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_DOWNLOAD | RS_FILE_HINTS_SPEC_ONLY, fi)) {
		return;
	}

	std::string filename = fi.path + "/" + fi.fname;

	/* open file with a suitable application */
	QFileInfo qinfo;
	qinfo.setFile(QString::fromUtf8(filename.c_str()));
	if (qinfo.exists()) {
		hide(); // hide here, because the rscollection dialog blocks and the toaster is deleted in that time
		RsUrlHandler::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()));
		// please add no code here, because "this" can be deleted
		return;
	}

	hide();
}
