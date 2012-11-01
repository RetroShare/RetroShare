/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

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

	/**
	 * Warns the user that autologin is not secure
	 */
	void notSecureWarning();

	void on_labelProfile_linkActivated(QString link);

private:
	/** Qt Designer generated object */
	Ui::StartDialog ui;

	bool reqNewCert;
};

#endif
