/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#ifndef _HELPDIALOG_H
#define _HELPDIALOG_H

#include <QFileDialog>

#include <retroshare/rstypes.h>

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog 
{
	Q_OBJECT

public:
	/** Default Constructor */
	explicit HelpDialog(QWidget *parent = 0);
	/** Default Destructor */
	virtual ~HelpDialog();

private:
	/** Qt Designer generated object */
	Ui::HelpDialog *ui;
};

#endif
