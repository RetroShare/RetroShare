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
#include "rsharesettings.h"

/** Constructor */
GeneralPage::GeneralPage(QWidget * parent, Qt::WFlags flags)
: ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

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
}

/** Saves the changes on this page */
bool
GeneralPage::save(QString &errmsg)
{
  Settings->setValue(QString::fromUtf8("StartMinimized"), startMinimized());

  Settings->setValue(QString::fromUtf8("doQuit"), quit());
  
  Settings->setValue(QString::fromUtf8("ClosetoTray"), closetoTray());
  
  Settings->setRunRetroshareOnBoot(
  ui.chkRunRetroshareAtSystemStartup->isChecked());
  
  Settings->setMaxTimeBeforeIdle(ui.spinBox->value());

  return true;
}

/** Loads the settings for this page */
void
GeneralPage::load()
{
  ui.chkRunRetroshareAtSystemStartup->setChecked(Settings->runRetroshareOnBoot());

  ui.checkStartMinimized->setChecked(Settings->value(QString::fromUtf8("StartMinimized"), false).toBool());

  ui.checkQuit->setChecked(Settings->value(QString::fromUtf8("doQuit"), false).toBool());
  
  ui.checkClosetoTray->setChecked(Settings->value(QString::fromUtf8("ClosetoTray"), false).toBool());
  
  ui.spinBox->setValue(Settings->getMaxTimeBeforeIdle());


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
  Settings->setShowMainWindowAtStart(checked);
}

void GeneralPage::setAutoLogin(){
	RsInit::setAutoLogin(ui.autoLogin->isChecked());
}
