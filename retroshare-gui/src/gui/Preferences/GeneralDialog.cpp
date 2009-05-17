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

#include "rshare.h"
#include "GeneralDialog.h"
#include <util/stringutil.h>
#include <QSystemTrayIcon>

/** Constructor */
GeneralDialog::GeneralDialog(QWidget *parent)
: ConfigPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 /* Create RshareSettings object */
  _settings = new RshareSettings();
  

  /* Hide platform specific features */
#ifndef Q_WS_WIN
  ui.chkRunRetroshareAtSystemStartup->setVisible(false);
  ui.autologincheckBox->setEnabled(false) ;
  ui.autologincheckBox->setChecked(false) ;
#endif    
}

/** Destructor */
GeneralDialog::~GeneralDialog()
{
  delete _settings;
}

/** Saves the changes on this page */
bool
GeneralDialog::save(QString &errmsg)
{  
  _settings->setValue(QString::fromUtf8("StartMinimized"), startMinimized());
  
  _settings->setRunRetroshareOnBoot(
  ui.chkRunRetroshareAtSystemStartup->isChecked());

  return true;
}
  
/** Loads the settings for this page */
void
GeneralDialog::load()
{  
  ui.chkRunRetroshareAtSystemStartup->setChecked(
  _settings->runRetroshareOnBoot());
 
  ui.checkStartMinimized->setChecked(_settings->value(QString::fromUtf8("StartMinimized"), false).toBool());
  
}
 
bool GeneralDialog::startMinimized() const {
  if(ui.checkStartMinimized->isChecked()) return true;
  return ui.checkStartMinimized->isChecked();
}

/** Called when the "show on startup" checkbox is toggled. */
void
GeneralDialog::toggleShowOnStartup(bool checked)
{
  //RshareSettings _settings;
  _settings->setShowMainWindowAtStart(checked);
}
