/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2007 - 2010 Xesc & Technology
 * Copyright (c) 2010 RetroShare Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************/

#include <QMessageBox>
#include <QFileInfo>
#include <QUrl>

#include "DownloadToaster.h"
#include "gui/common/RsUrlHandler.h"

#include <retroshare/rsfiles.h>

DownloadToaster::DownloadToaster(const std::string &hash, const QString &name) : QWidget(NULL)
{
	ui.setupUi(this);

	fileHash = hash;

	/* connect buttons */
	connect(ui.spbClose, SIGNAL(clicked()), this, SLOT(hide()));
	connect(ui.startButton, SIGNAL(clicked()), this, SLOT(play()));

	/* set informations */
	ui.labelTitle->setText(name);
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
