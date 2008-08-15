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
#include <QSystemTrayIcon>

/** Constructor */
GeneralDialog::GeneralDialog(QWidget *parent)
: ConfigPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 /* Create RshareSettings object */
  _settings = new RshareSettings();
  
  if (QSystemTrayIcon::isSystemTrayAvailable()){

    /* Check if we are supposed to show our main window on startup */
    ui.chkShowOnStartup->setChecked(_settings->showMainWindowAtStart());
    if (ui.chkShowOnStartup->isChecked())
      show();
  } else {
    /* Don't let people hide the main window, since that's all they have. */
    ui.chkShowOnStartup->hide();
    //show();
  }
  /* Hide platform specific features */
#ifndef Q_WS_WIN
  ui.chkRunRetroshareAtSystemStartup->setVisible(false);
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
  Q_UNUSED(errmsg);
  
  _settings->setValue(QString::fromUtf8("StartMinimized"), startMinimized());

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
