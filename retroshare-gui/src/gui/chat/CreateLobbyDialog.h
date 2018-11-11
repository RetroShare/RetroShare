/*******************************************************************************
 * gui/chat/CreateLobbyDialog.h                                                *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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
#ifndef CREATELOBBYDIALOG_H
#define CREATELOBBYDIALOG_H

#include <QDialog>

#include "ui_CreateLobbyDialog.h"
#include <retroshare/rstypes.h>

class CreateLobbyDialog : public QDialog {
	Q_OBJECT

public:
	/*
	 *@param chanId The channel id to send request for
	 */
    CreateLobbyDialog(const std::set<RsPeerId>& friends_list, int privacyLevel = 0, QWidget *parent = 0);
	~CreateLobbyDialog();

protected:
	void changeEvent(QEvent *e);

private:
	Ui::CreateLobbyDialog *ui;

private slots:
	void createLobby();
	void checkTextFields();
};

#endif // CREATELOBBYDIALOG_H
