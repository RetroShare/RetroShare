/*******************************************************************************
 * gui/StartDialog.h                                                           *
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

#ifndef _STARTDIALOG_H
#define _STARTDIALOG_H

#include "ui_StartDialog.h"

class StartDialog : public QDialog
{
	Q_OBJECT

public:
	/** Default constructor */
	StartDialog(QWidget *parent = 0);

	bool requestedNewCert();

protected:
	void closeEvent (QCloseEvent * event);

private slots:
	void loadPerson();
	void updateSelectedProfile(int);

#ifdef RS_AUTOLOGIN
	/**
	 * Warns the user that autologin is not secure
	 */
	void notSecureWarning();
#endif // RS_AUTOLOGIN

	void on_labelProfile_linkActivated(QString link);

private:
	/** Qt Designer generated object */
	Ui::StartDialog ui;

	bool reqNewCert;
};

#endif
