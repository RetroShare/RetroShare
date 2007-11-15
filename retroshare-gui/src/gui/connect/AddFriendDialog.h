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


#ifndef _ADDFRIENDDIALOG_H
#define _ADDFRIENDDIALOG_H

#include <QDialog>

#include "ui_AddFriendDialog.h"
class NetworkDialog;

class AddFriendDialog : public QDialog
{
  Q_OBJECT

public:
  /** Default constructor */
  AddFriendDialog(NetworkDialog *cd, QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default destructor */

void setInfo(std::string text);

public slots:
protected:
private slots:

	void donebutton();
	void afcancelbutton();
	void filebutton();
  
private:

  /** Qt Designer generated object */
  Ui::AddFriendDialog ui;
  NetworkDialog *cDialog;
};

#endif

