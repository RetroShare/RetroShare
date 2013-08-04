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
GeneralPage::GeneralPage(QWidget * parent, Qt::WFlags flags)
: ConfigPage(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    /* Hide platform specific features */
#ifdef Q_WS_WIN
    if (Settings->canSetRetroShareProtocol() == false) {
        ui.enableRetroShareProtocol->setEnabled(false);
    } else {
        ui.adminLabel->setEnabled(false);
        ui.adminLabel->setToolTip("");
    }
#else
    ui.chkRunRetroshareAtSystemStartup->setVisible(false);
    ui.chkRunRetroshareAtSystemStartupMinimized->setVisible(false);
    ui.enableRetroShareProtocol->setVisible(false);
    ui.adminLabel->setVisible(false);
#endif
	 connect(ui.runStartWizard_PB,SIGNAL(clicked()), this,SLOT(runStartWizard())) ;
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
  Settings->setStartMinimized(ui.checkStartMinimized->isChecked());
  Settings->setValue("doQuit", ui.checkQuit->isChecked());
  Settings->setCloseToTray(ui.checkCloseToTray->isChecked());

#ifdef Q_WS_WIN
  Settings->setRunRetroshareOnBoot(ui.chkRunRetroshareAtSystemStartup->isChecked(), ui.chkRunRetroshareAtSystemStartupMinimized->isChecked());

  if (ui.enableRetroShareProtocol->isChecked() != Settings->getRetroShareProtocol()) {
    if (Settings->setRetroShareProtocol(ui.enableRetroShareProtocol->isChecked()) == false) {
        if (ui.enableRetroShareProtocol->isChecked()) {
            QMessageBox::critical(this, tr("Error"), tr("Could not add retroshare:// as protocol."));
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Could not remove retroshare:// protocol."));
        }
    }
  }
#endif

  Settings->setMaxTimeBeforeIdle(ui.spinBox->value());

  RsInit::setAutoLogin(ui.autoLogin->isChecked());

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

  return true;
}

/** Loads the settings for this page */
void GeneralPage::load()
{
#ifdef Q_WS_WIN
  bool minimized;
  ui.chkRunRetroshareAtSystemStartup->setChecked(Settings->runRetroshareOnBoot(minimized));
  ui.chkRunRetroshareAtSystemStartupMinimized->setChecked(minimized);

  ui.enableRetroShareProtocol->setChecked(Settings->getRetroShareProtocol());
#endif

  ui.checkStartMinimized->setChecked(Settings->getStartMinimized());
  ui.checkQuit->setChecked(Settings->value("doQuit", false).toBool());

  ui.checkCloseToTray->setChecked(Settings->getCloseToTray());
  
  ui.spinBox->setValue(Settings->getMaxTimeBeforeIdle());

  ui.autoLogin->setChecked(RsInit::getAutoLogin());

  bool advancedmode = false;
  std::string advsetting;
  if (rsConfig->getConfigurationOption(RS_CONFIG_ADVANCED, advsetting) && (advsetting == "YES"))
  {
	advancedmode = true;
  }
  ui.checkAdvanced->setChecked(advancedmode);

}
