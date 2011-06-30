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

#ifndef DOWNLOADTOASTER_H
#define DOWNLOADTOASTER_H

#include "ui_DownloadToaster.h"

class DownloadToaster : public QWidget
{
	Q_OBJECT

public:
	DownloadToaster(const std::string &hash, const QString &name);

private slots:
	void play();

private:
	std::string fileHash;

	/** Qt Designer generated object */
	Ui::DownloadToaster ui;
};

#endif
