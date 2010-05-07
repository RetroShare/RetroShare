/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2009 RetroShare Team
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

#include <iostream>
#include <rshare.h>
#include <rsiface/rsinit.h>
#include "GeneralPage.h"
#include <util/stringutil.h>
#include <QSystemTrayIcon>

/** Constructor */
GeneralPage::GeneralPage(QWidget * parent, Qt::WFlags flags)
: ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 /* Create RshareSettings object */
  _settings = new RshareSettings();

  connect(ui.autoLogin, SIGNAL(clicked()), this, SLOT(setAutoLogin()));

  /* Hide platform specific features */
#ifndef Q_WS_WIN
  ui.chkRunRetroshareAtSystemStartup->setVisible(false);

#endif

  ui.autoLogin->setChecked(RsInit::getAutoLogin());

}

/** Destructor */
GeneralPage::~GeneralPage()
{
  delete _settings;
}

/** Saves the changes on this page */
bool
GeneralPage::save(QString &errmsg)
{
  _settings->setValue(QString::fromUtf8("StartMinimized"), startMinimized());

  _settings->setValue(QString::fromUtf8("doQuit"), quit());
  
  _settings->setValue(QString::fromUtf8("ClosetoTray"), closetoTray());
  
  _settings->setRunRetroshareOnBoot(
  ui.chkRunRetroshareAtSystemStartup->isChecked());

  return true;
}

/** Loads the settings for this page */
void
GeneralPage::load()
{
  ui.chkRunRetroshareAtSystemStartup->setChecked(
  _settings->runRetroshareOnBoot());

  ui.checkStartMinimized->setChecked(_settings->value(QString::fromUtf8("StartMinimized"), false).toBool());

  ui.checkQuit->setChecked(_settings->value(QString::fromUtf8("doQuit"), false).toBool());
  
  ui.checkClosetoTray->setChecked(_settings->value(QString::fromUtf8("ClosetoTray"), false).toBool());


}

bool GeneralPage::quit() const {
  if(ui.checkQuit->isChecked()) return true;
  return ui.checkQuit->isChecked();
}

bool GeneralPage::startMinimized() const {
  if(ui.checkStartMinimized->isChecked()) return true;
  return ui.checkStartMinimized->isChecked();
}

bool GeneralPage::closetoTray() const {
  if(ui.checkClosetoTray->isChecked()) return true;
  return ui.checkClosetoTray->isChecked();
}


/** Called when the "show on startup" checkbox is toggled. */
void
GeneralPage::toggleShowOnStartup(bool checked)
{
  //RshareSettings _settings;
  _settings->setShowMainWindowAtStart(checked);
}

void GeneralPage::setAutoLogin(){
	RsInit::setAutoLogin(ui.autoLogin->isChecked());
}
