/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/BannedFilesDialog.h                     *
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

#pragma once

#include "RsAutoUpdatePage.h"
#include "ui_BannedFilesDialog.h"

class BannedFilesDialog: public QDialog
{
	Q_OBJECT

public:
	/** Default Constructor */
	BannedFilesDialog(QWidget *parent = 0);
	/** Default Destructor */
	~BannedFilesDialog();

private slots:
	void unbanFile();

	/** management of the adv search dialog object when switching search modes */
	//void hideEvent(QHideEvent * event);
	void bannedFilesContextMenu(QPoint);

private:
	void fillFilesList();

	/** Qt Designer generated object */
	Ui::BannedFilesDialog ui;
};

