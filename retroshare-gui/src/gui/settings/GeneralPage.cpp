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

#include <QMessageBox>
#include <iostream>
#include <rshare.h>
#include "retroshare/rsinit.h"
#include "retroshare/rsconfig.h"

#include "GeneralPage.h"
#include <util/stringutil.h>
#include <QSystemTrayIcon>
#include "rsharesettings.h"
#include <gui/QuickStartWizard.h>

/** Constructor */
GeneralPage::GeneralPage(QWidget * parent, Qt::WindowFlags flags)
: ConfigPage(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    /* Connect signals */
    connect(ui.runStartWizard_PB,SIGNAL(clicked()), this,SLOT(runStartWizard())) ;

    /* Hide platform specific features */
#ifdef Q_OS_WIN

#ifdef QT_DEBUG
    ui.chkRunRetroshareAtSystemStartup->setEnabled(false);
    ui.chkRunRetroshareAtSystemStartupMinimized->setEnabled(false);
#endif
    ui.desktopFileMissingLabel->setVisible(false);

#else
    ui.chkRunRetroshareAtSystemStartup->setVisible(false);
    ui.chkRunRetroshareAtSystemStartupMinimized->setVisible(false);
    ui.registerRetroShareProtocol->setVisible(false);
    ui.adminLabel->setVisible(false);
#endif

    if (Settings->canSetRetroShareProtocol()) {
        ui.registerRetroShareProtocol->setEnabled(true);
#ifdef Q_OS_WIN
        ui.adminLabel->setEnabled(true);
        ui.adminLabel->setToolTip(tr("You have enough right."));
#else
        ui.desktopFileMissingLabel->setVisible(false);
#endif
    } else {
        ui.registerRetroShareProtocol->setEnabled(false);
#ifdef Q_OS_WIN
        ui.adminLabel->setEnabled(false);
        ui.adminLabel->setToolTip(tr("You don't have enough right. Run RetroShare as Admin to change this setting."));
#else
        ui.desktopFileMissingLabel->setVisible(true);
#endif
    }
    ui.useLocalServer->setEnabled(true);
}

/** Destructor */
GeneralPage::~GeneralPage()
{
}
void GeneralPage::runStartWizard()
{
    QuickStartWizard(this).exec();
}

/** Saves the changes on this page */
bool GeneralPage::save(QString &/*errmsg*/)
{
#ifdef Q_OS_WIN
#ifndef QT_DEBUG
  Settings->setRunRetroshareOnBoot(ui.chkRunRetroshareAtSystemStartup->isChecked(), ui.chkRunRetroshareAtSystemStartupMinimized->isChecked());
#endif
#endif

  Settings->setStartMinimized(ui.checkStartMinimized->isChecked());

  if (ui.checkAdvanced->isChecked())
  {
    std::string opt("YES");
    rsConfig->setConfigurationOption(RS_CONFIG_ADVANCED, opt);
  }
  else
  {
    std::string opt("NO");
    rsConfig->setConfigurationOption(RS_CONFIG_ADVANCED, opt);
  }

  Settings->setValue("doQuit", ui.checkQuit->isChecked());
  Settings->setCloseToTray(ui.checkCloseToTray->isChecked());
  RsInit::setAutoLogin(ui.autoLogin->isChecked());

  if (ui.registerRetroShareProtocol->isChecked() != Settings->getRetroShareProtocol()) {
    QString error ="";
    if (Settings->setRetroShareProtocol(ui.registerRetroShareProtocol->isChecked(), error) == false) {
        if (ui.registerRetroShareProtocol->isChecked()) {
            QMessageBox::critical(this, tr("Error"), tr("Could not add retroshare:// as protocol.").append("\n").append(error));
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Could not remove retroshare:// protocol.").append("\n").append(error));
        }
    }
  }

  Settings->setUseLocalServer(ui.useLocalServer->isChecked());

  Settings->setMaxTimeBeforeIdle(ui.idleSpinBox->value());

  return true;
}

/** Loads the settings for this page */
void GeneralPage::load()
{
#ifdef Q_OS_WIN
  bool minimized;
  ui.chkRunRetroshareAtSystemStartup->setChecked(Settings->runRetroshareOnBoot(minimized));
  ui.chkRunRetroshareAtSystemStartupMinimized->setChecked(minimized);
#endif

  ui.checkStartMinimized->setChecked(Settings->getStartMinimized());

  bool advancedmode = false;
  std::string advsetting;
  if (rsConfig->getConfigurationOption(RS_CONFIG_ADVANCED, advsetting) && (advsetting == "YES"))
  {
    advancedmode = true;
  }
  ui.checkAdvanced->setChecked(advancedmode);

  ui.checkQuit->setChecked(Settings->value("doQuit", false).toBool());
  ui.checkCloseToTray->setChecked(Settings->getCloseToTray());
  ui.autoLogin->setChecked(RsInit::getAutoLogin());
  ui.registerRetroShareProtocol->setChecked(Settings->getRetroShareProtocol());
  ui.useLocalServer->setChecked(Settings->getUseLocalServer());

  ui.idleSpinBox->setValue(Settings->getMaxTimeBeforeIdle());
}
