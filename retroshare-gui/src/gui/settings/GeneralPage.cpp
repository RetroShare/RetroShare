/*******************************************************************************
 * gui/settings/GeneralPage.h                                                  *
 *                                                                             *
 * Copyright 2009, Retroshare Team <retroshare.project@gmail.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QMessageBox>
#include <iostream>
#include <rshare.h>
#include "retroshare/rsinit.h"
#include "retroshare/rsconfig.h"

#include "GeneralPage.h"
#include <util/stringutil.h>
#include <util/misc.h>
#include <QSystemTrayIcon>
#include "rsharesettings.h"

/** Constructor */
GeneralPage::GeneralPage(QWidget * parent, Qt::WindowFlags flags) :
    ConfigPage(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

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
        ui.adminLabel->setToolTip(tr("You have sufficient rights."));
#else
        ui.desktopFileMissingLabel->setVisible(false);
#endif
    } else {
        ui.registerRetroShareProtocol->setEnabled(false);
#ifdef Q_OS_WIN
        ui.adminLabel->setEnabled(false);
        ui.adminLabel->setToolTip(tr("You don't have sufficient rights. Run RetroShare as Admin to change this setting."));
#else
        ui.desktopFileMissingLabel->setVisible(true);
#endif
    }
    ui.useLocalServer->setEnabled(true);

#ifdef RS_AUTOLOGIN
	ui.autoLogin->setToolTip(tr("For security reasons the usage of auto-login is discouraged, you can enable it but you are on your own!"));
#else // RS_AUTOLOGIN
	ui.autoLogin->setEnabled(false);
	ui.autoLogin->setToolTip(tr("Your RetroShare build has auto-login disabled."));
#endif // RS_AUTOLOGIN

    ui.checkCloseToTray->setChecked(false) ; // default should be false because some systems to not support this.

    if(!QSystemTrayIcon::isSystemTrayAvailable())
    {
		ui.checkCloseToTray->setChecked(false) ; // default should be false because some systems to not support this.
		ui.checkCloseToTray->setToolTip(tr("No Qt-compatible system tray was found on this system."));
        Settings->setCloseToTray(false);
    }

    /* Connect signals */
    connect(ui.useLocalServer,                              SIGNAL(toggled(bool)),     this,SLOT(updateUseLocalServer())) ;
    connect(ui.idleSpinBox,                                 SIGNAL(valueChanged(int)), this,SLOT(updateMaxTimeBeforeIdle())) ;
    connect(ui.checkStartMinimized,                         SIGNAL(toggled(bool)),     this,SLOT(updateStartMinimized())) ;
    connect(ui.checkQuit,                                   SIGNAL(toggled(bool)),     this,SLOT(updateDoQuit())) ;
    connect(ui.checkCloseToTray,                            SIGNAL(toggled(bool)),     this,SLOT(updateCloseToTray())) ;
    connect(ui.autoLogin,                                   SIGNAL(toggled(bool)),     this,SLOT(updateAutoLogin())) ;
    connect(ui.chkRunRetroshareAtSystemStartup,             SIGNAL(toggled(bool)),     this,SLOT(updateRunRSOnBoot())) ;
    connect(ui.chkRunRetroshareAtSystemStartupMinimized,    SIGNAL(toggled(bool)),     this,SLOT(updateRunRSOnBoot())) ;
    //connect(ui.runStartWizard_PB,                           SIGNAL(clicked()),         this,SLOT(runStartWizard())) ;
    connect(ui.checkAdvanced,                               SIGNAL(toggled(bool)),     this,SLOT(updateAdvancedMode())) ;
    connect(ui.registerRetroShareProtocol,                  SIGNAL(toggled(bool)),     this,SLOT(updateRegisterRSProtocol())) ;

	// hide advanced checkbox, since the option is not used.

	ui.advGBox->hide();
}

/** Destructor */
GeneralPage::~GeneralPage()
{
}

void GeneralPage::updateAdvancedMode()
{
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
}

void GeneralPage::updateUseLocalServer()   { Settings->setUseLocalServer(ui.useLocalServer->isChecked()); }
void GeneralPage::updateMaxTimeBeforeIdle(){ Settings->setMaxTimeBeforeIdle(ui.idleSpinBox->value()); }
void GeneralPage::updateStartMinimized()   { Settings->setStartMinimized(ui.checkStartMinimized->isChecked()); }
void GeneralPage::updateDoQuit()           { Settings->setValue("doQuit", ui.checkQuit->isChecked()); }
void GeneralPage::updateCloseToTray()      { Settings->setCloseToTray(ui.checkCloseToTray->isChecked()); }
void GeneralPage::updateAutoLogin()        { RsInit::setAutoLogin(ui.autoLogin->isChecked());}
void GeneralPage::updateRunRSOnBoot()
{
#ifdef Q_OS_WIN
#ifndef QT_DEBUG
  Settings->setRunRetroshareOnBoot(ui.chkRunRetroshareAtSystemStartup->isChecked(), ui.chkRunRetroshareAtSystemStartupMinimized->isChecked());
#endif
#endif
}

void GeneralPage::updateRegisterRSProtocol()
{
	if (ui.registerRetroShareProtocol->isChecked() != Settings->getRetroShareProtocol())
    {
		QString error ="";
		if (Settings->setRetroShareProtocol(ui.registerRetroShareProtocol->isChecked(), error) == false) {
			if (ui.registerRetroShareProtocol->isChecked()) {
				QMessageBox::critical(this, tr("Error"), tr("Could not add retroshare:// as protocol.").append("\n").append(error));
			} else {
				QMessageBox::critical(this, tr("Error"), tr("Could not remove retroshare:// protocol.").append("\n").append(error));
			}
		}
	}
}

/** Loads the settings for this page */
void GeneralPage::load()
{
#ifdef Q_OS_WIN
  bool minimized;
  ui.chkRunRetroshareAtSystemStartup->setChecked(Settings->runRetroshareOnBoot(minimized));
  ui.chkRunRetroshareAtSystemStartupMinimized->setChecked(minimized);
#endif

  whileBlocking(ui.checkStartMinimized)->setChecked(Settings->getStartMinimized());

  bool advancedmode = false;
  std::string advsetting;
  if (rsConfig->getConfigurationOption(RS_CONFIG_ADVANCED, advsetting) && (advsetting == "YES"))
  {
    advancedmode = true;
  }
  whileBlocking(ui.checkAdvanced)->setChecked(advancedmode);

  whileBlocking(ui.checkQuit)->setChecked(Settings->value("doQuit", false).toBool());
  whileBlocking(ui.checkCloseToTray)->setChecked(Settings->getCloseToTray());
  whileBlocking(ui.autoLogin)->setChecked(RsInit::getAutoLogin());
  whileBlocking(ui.registerRetroShareProtocol)->setChecked(Settings->getRetroShareProtocol());
  whileBlocking(ui.useLocalServer)->setChecked(Settings->getUseLocalServer());

  whileBlocking(ui.idleSpinBox)->setValue(Settings->getMaxTimeBeforeIdle());
}
