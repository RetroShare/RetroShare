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


#ifndef _CONFCERTDIALOG_H
#define _CONFCERTDIALOG_H

#include <QMainWindow>

#include "ui_ConfCertDialog.h"

class ConfCertDialog : public QMainWindow
{
  Q_OBJECT

public:
  /** Default constructor */
  ConfCertDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default destructor */


void 	loadId(std::string id);

void setInfo(std::string name, 
		std::string trust, 
		std::string org,
		std::string loc,
		std::string country,
		std::string signers);

public slots:
  /** Overloaded QWidget.show */
  void show();

protected:
  void closeEvent (QCloseEvent * event);
  
private slots:

	void closeinfodlg();
	void applyDialog();
  
private:

  /** Loads the saved connectidialog settings */
//  void loadSettings();
 
void 	loadDialog();

std::string mId;
  /** Qt Designer generated object */
  Ui::ConfCertDialog ui;
};

#endif

