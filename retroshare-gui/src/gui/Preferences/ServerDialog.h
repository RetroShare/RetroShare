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

#ifndef _SERVERDIALOG_H
#define _SERVERDIALOG_H

#include <QFileDialog>

#include "rsharesettings.h"

#include "configpage.h"
#include "ui_ServerDialog.h"

class ServerDialog : public ConfigPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  ServerDialog(QWidget *parent = 0);
  /** Default Destructor */

  /** Saves the changes on this page */
  bool save(QString &errmsg);
  /** Loads the settings for this page */
  void load();

public slots:
  void updateStatus();

private slots:
  void saveAddresses();
  void toggleUPnP();
  void toggleIpDetermination(bool) ;

  
private:
  /** A RshareSettings object used for saving/loading settings */
  RshareSettings* _settings;

  /** Qt Designer generated object */
  Ui::ServerDialog ui;
};

#endif

