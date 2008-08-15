/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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

#ifndef _GENERALDIALOG_H
#define _GENERALDIALOG_H

#include <QtGui>
#include <QFileDialog>
#include <QLineEdit>

#include "rsharesettings.h"

#include "configpage.h"
#include "ui_GeneralDialog.h"


class GeneralDialog : public ConfigPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  GeneralDialog(QWidget *parent = 0);
  /** Default Destructor */ 
  ~GeneralDialog();
  /** Saves the changes on this page */
  bool save(QString &errmsg);
  /** Loads the settings for this page */
  void load();
  bool startMinimized() const;

private slots:

  /** Called when the "show on startup" checkbox is toggled. */
  void toggleShowOnStartup(bool checked);
  
private:
  /** A RetroShare Settings object used for saving/loading settings */
  RshareSettings *_settings;
  

  
  /** Qt Designer generated object */
  Ui::GeneralDialog ui;
};

#endif

