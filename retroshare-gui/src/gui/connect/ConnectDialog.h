/****************************************************************
 *  RShare is distributed under the following license:
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


#ifndef _CONNECTDIALOG_H
#define _CONNECTDIALOG_H

#include <gui/Preferences/rsharesettings.h>

#include "ui_ConnectDialog.h"


class ConnectDialog : public QMainWindow
{
  Q_OBJECT

public:
  /** Default constructor */
  ConnectDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default destructor */

bool loadPeer(std::string id);

public slots:
  /** Overloaded QWidget.show */
  void checkAuthCode( const QString &txt );
  void show();

protected:
  void closeEvent (QCloseEvent * event);
  
private slots:

	void closeinfodlg();
        void authAttempt();

  
private:


void setInfo(std::string name, 
		std::string trust, 
		std::string org,
		std::string loc,
		std::string country,
		std::string signers);

void setAuthCode(std::string id, std::string code);

  std::string authCode;
  std::string authId;

  /** A VidaliaSettings object that handles getting/saving settings */
  RshareSettings* _settings;
  
  /** Qt Designer generated object */
  Ui::ConnectDialog ui;
};

#endif


