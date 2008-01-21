/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2008, crypton
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

#ifndef _LANGUAGEDIALOG_H
#define _LANGUAGEDIALOG_H
#include <QtGui>
#include <QFileDialog>
#include <QStyleFactory>
#include <QLineEdit>

#include <config/rsharesettings.h>
#include <lang/languagesupport.h>

#include "ui_LanguageDialog.h"

class LanguageDialog : public QWidget
{
  Q_OBJECT

public:
  /** Default Constructor */
  LanguageDialog(QWidget *parent = 0);
  /** Default Destructor */ 
  ~LanguageDialog();
  /** Saves the changes on this page */
  bool save(QString &errmsg);
  /** Loads the settings for this page */
  void load();


private slots:
  void cancellanguage();

  
private:
  /** A RetroShare Settings object used for saving/loading settings */
  RshareSettings* _settings;
  

  
  /** Qt Designer generated object */
  Ui::LanguageDialog ui;
};

#endif

