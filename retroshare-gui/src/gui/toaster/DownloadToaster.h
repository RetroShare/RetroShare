/*******************************************************************************
 * gui/toaster/DownloadToaster.h                                               *
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

#ifndef DOWNLOADTOASTER_H
#define DOWNLOADTOASTER_H

#include "ui_DownloadToaster.h"
#include <retroshare/rstypes.h>

class DownloadToaster : public QWidget
{
	Q_OBJECT

public:
    DownloadToaster(const RsFileHash &hash, const QString &name);

private slots:
	void play();

private:
    RsFileHash fileHash;

	/** Qt Designer generated object */
	Ui::DownloadToaster ui;
};

#endif
