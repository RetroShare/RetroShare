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
#include <retroshare/rsinit.h>
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

  /* Hide platform specific features */
#ifndef Q_WS_WIN
  ui.chkRunRetroshareAtSystemStartup->setVisible(false);
  ui.chkRunRetroshareAtSystemStartupMinimized->setVisible(false);
#endif
}

/** Destructor */
GeneralPage::~GeneralPage()
{
}

/** Saves the changes on this page */
bool GeneralPage::save(QString &errmsg)
{
  Settings->setStartMinimized(ui.checkStartMinimized->isChecked());
  Settings->setValue("doQuit", ui.checkQuit->isChecked());
  Settings->setCloseToTray(ui.checkClosetoTray->isChecked());

#ifdef Q_WS_WIN
  Settings->setRunRetroshareOnBoot(ui.chkRunRetroshareAtSystemStartup->isChecked(), ui.chkRunRetroshareAtSystemStartupMinimized->isChecked());
#endif

  Settings->setMaxTimeBeforeIdle(ui.spinBox->value());

  RsInit::setAutoLogin(ui.autoLogin->isChecked());

  return true;
}

/** Loads the settings for this page */
void GeneralPage::load()
{
#ifdef Q_WS_WIN
  bool minimized;
  ui.chkRunRetroshareAtSystemStartup->setChecked(Settings->runRetroshareOnBoot(minimized));
  ui.chkRunRetroshareAtSystemStartupMinimized->setChecked(minimized);
#endif

  ui.checkStartMinimized->setChecked(Settings->getStartMinimized());
  ui.checkQuit->setChecked(Settings->value("doQuit", false).toBool());

  ui.checkClosetoTray->setChecked(Settings->getCloseToTray());
  
  ui.spinBox->setValue(Settings->getMaxTimeBeforeIdle());

  ui.autoLogin->setChecked(RsInit::getAutoLogin());
}
