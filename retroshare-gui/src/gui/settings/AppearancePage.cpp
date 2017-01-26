/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
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

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QStatusBar>
#include <QStyleFactory>

#include "lang/languagesupport.h"
#include <rshare.h>
#include "AppearancePage.h"
#include "rsharesettings.h"
#include "gui/MainWindow.h"
#include "gui/statusbar/peerstatus.h"
#include "gui/statusbar/natstatus.h"
#include "gui/statusbar/dhtstatus.h"
#include "gui/statusbar/hashingstatus.h"
#include "gui/statusbar/discstatus.h"
#include "gui/statusbar/ratesstatus.h"
#include "gui/statusbar/OpModeStatus.h"
#include "gui/statusbar/SoundStatus.h"
#include "gui/statusbar/ToasterDisable.h"
#include "gui/statusbar/SysTrayStatus.h"

/** Constructor */
AppearancePage::AppearancePage(QWidget * parent, Qt::WindowFlags flags)
	: ConfigPage(parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	connect(ui.cmboStyleSheet, SIGNAL(activated(int)), this, SLOT(loadStyleSheet(int)));

	connect(ui.grpStatus,                     SIGNAL(toggled(bool)), this /* pMainWindow->statusBar(),				*/, SLOT(switch_status_grpStatus(bool)));
	connect(ui.checkBoxStatusCompactMode,     SIGNAL(toggled(bool)), this /* pMainWindow, 							*/, SLOT(switch_status_compactMode(bool)));
	connect(ui.checkBoxDisableSysTrayToolTip, SIGNAL(toggled(bool)), this /* pMainWindow, 							*/, SLOT(switch_status_showToolTip(bool)));
	connect(ui.checkBoxShowStatusStatus,      SIGNAL(toggled(bool)), this /* pMainWindow->statusComboBoxInstance(), */, SLOT(switch_status_ShowStatus(bool)));
	connect(ui.checkBoxShowPeerStatus,        SIGNAL(toggled(bool)), this /* pMainWindow->peerstatusInstance(),     */, SLOT(switch_status_ShowStatus(bool)));
	connect(ui.checkBoxShowNATStatus,         SIGNAL(toggled(bool)), this /* pMainWindow->natstatusInstance(),      */, SLOT(switch_status_ShowPeer(bool)));
	connect(ui.checkBoxShowDHTStatus,         SIGNAL(toggled(bool)), this /* pMainWindow->dhtstatusInstance(),      */, SLOT(switch_status_ShowDHT(bool)));
	connect(ui.checkBoxShowHashingStatus,     SIGNAL(toggled(bool)), this /* pMainWindow->hashingstatusInstance(),  */, SLOT(switch_status_ShowHashing(bool)));
	connect(ui.checkBoxShowDiscStatus,        SIGNAL(toggled(bool)), this /* pMainWindow->discstatusInstance(),     */, SLOT(switch_status_ShowDisc(bool)));
	connect(ui.checkBoxShowRateStatus,        SIGNAL(toggled(bool)), this /* pMainWindow->ratesstatusInstance(),    */, SLOT(switch_status_ShowRate(bool)));
	connect(ui.checkBoxShowOpModeStatus,      SIGNAL(toggled(bool)), this /* pMainWindow->opModeStatusInstance(),   */, SLOT(switch_status_ShowOpMode(bool)));
	connect(ui.checkBoxShowSoundStatus,       SIGNAL(toggled(bool)), this /* pMainWindow->soundStatusInstance(),    */, SLOT(switch_status_ShowSound(bool)));
	connect(ui.checkBoxShowToasterDisable,    SIGNAL(toggled(bool)), this /* pMainWindow->toasterDisableInstance(), */, SLOT(switch_status_ShowToaster(bool)));
	connect(ui.checkBoxShowSystrayOnStatus,   SIGNAL(toggled(bool)), this /* pMainWindow->sysTrayStatusInstance(),  */, SLOT(switch_status_ShowSystray(bool)));

	/* Populate combo boxes */
	foreach (QString code, LanguageSupport::languageCodes()) {
		ui.cmboLanguage->addItem(QIcon(":/images/flags/" + code + ".png"), LanguageSupport::languageName(code), code);
	}
	foreach (QString style, QStyleFactory::keys()) {
		ui.cmboStyle->addItem(style, style.toLower());
	}

	// add empty entry representing "no style sheet"
	ui.cmboStyleSheet->addItem("", "");

	QMap<QString, QString> styleSheets;
	Rshare::getAvailableStyleSheets(styleSheets);

	foreach (QString name, styleSheets.keys()) {
		ui.cmboStyleSheet->addItem(name, styleSheets[name]);
	}
}

void AppearancePage::switch_status_grpStatus(bool b)        { switch_status(MainWindow::StatusGrpStatus  ,b) ; }
void AppearancePage::switch_status_compactMode(bool b)      { switch_status(MainWindow::StatusCompactMode,b) ; }
void AppearancePage::switch_status_showToolTip(bool b)      { switch_status(MainWindow::StatusShowToolTip,b) ; }
void AppearancePage::switch_status_ShowStatus(bool b)       { switch_status(MainWindow::StatusShowStatus ,b) ; }
void AppearancePage::switch_status_ShowPeer(bool b)         { switch_status(MainWindow::StatusShowPeer   ,b) ; }
void AppearancePage::switch_status_ShowDHT(bool b)          { switch_status(MainWindow::StatusShowDHT    ,b) ; }
void AppearancePage::switch_status_ShowHashing(bool b)      { switch_status(MainWindow::StatusShowHashing,b) ; }
void AppearancePage::switch_status_ShowDisc(bool b)         { switch_status(MainWindow::StatusShowDisc   ,b) ; }
void AppearancePage::switch_status_ShowRate(bool b)         { switch_status(MainWindow::StatusShowRate   ,b) ; }
void AppearancePage::switch_status_ShowOpMode(bool b)       { switch_status(MainWindow::StatusShowOpMode ,b) ; }
void AppearancePage::switch_status_ShowSound(bool b)        { switch_status(MainWindow::StatusShowSound  ,b) ; }
void AppearancePage::switch_status_ShowToaster(bool b)      { switch_status(MainWindow::StatusShowToaster,b) ; }
void AppearancePage::switch_status_ShowSystray(bool b)      { switch_status(MainWindow::StatusShowSystray,b) ; }

void AppearancePage::switch_status(MainWindow::StatusElement s,bool b)
{
	MainWindow *pMainWindow = MainWindow::getInstance();

    if(!pMainWindow)
        return ;

    pMainWindow->switchVisibilityStatus(s,b) ;
}

/** Saves the changes on this page */
bool AppearancePage::save(QString &errmsg)
{
	Q_UNUSED(errmsg);

	QString languageCode = LanguageSupport::languageCode(ui.cmboLanguage->currentText());

	Settings->setLanguageCode(languageCode);
	Settings->setInterfaceStyle(ui.cmboStyle->currentText());
	Settings->setSheetName(ui.cmboStyleSheet->itemData(ui.cmboStyleSheet->currentIndex()).toString());
	Settings->setPageButtonLoc(ui.rbtPageOnToolBar->isChecked());
	Settings->setActionButtonLoc(ui.rbtActionOnToolBar->isChecked());
	switch (ui.cmboTollButtonsStyle->currentIndex())
	{
		case 0:
			Settings->setToolButtonStyle(Qt::ToolButtonIconOnly);
		break;
		case 1:
			Settings->setToolButtonStyle(Qt::ToolButtonTextOnly);
		break;
		case 2:
			Settings->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		break;
		case 3:
		default:
			Settings->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	}
	switch (ui.cmboTollButtonsSize->currentIndex())
	{
		case 0:
			Settings->setToolButtonSize(8);
		break;
		case 1:
			Settings->setToolButtonSize(16);
		break;
		case 2:
		default:
			Settings->setToolButtonSize(24);
		break;
		case 3:
			Settings->setToolButtonSize(32);
        break;
        case 4:
            Settings->setToolButtonSize(64);
        break;
        case 5:
            Settings->setToolButtonSize(128);
    }
	switch (ui.cmboListItemSize->currentIndex())
	{
		case 0:
			Settings->setListItemIconSize(8);
		break;
		case 1:
			Settings->setListItemIconSize(16);
		break;
		case 2:
		default:
			Settings->setListItemIconSize(24);
		break;
		case 3:
			Settings->setListItemIconSize(32);
        break;
        case 4:
            Settings->setListItemIconSize(64);
        break;
        case 5:
            Settings->setListItemIconSize(128);
    }

	/* Set to new style */
	Rshare::setStyle(ui.cmboStyle->currentText());

	Settings->setValueToGroup("StatusBar", "ShowStatusBar", QVariant(ui.grpStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "CompactMode", QVariant(ui.checkBoxStatusCompactMode->isChecked()));
	Settings->setValueToGroup("StatusBar", "DisableSysTrayToolTip", QVariant(ui.checkBoxDisableSysTrayToolTip->isChecked()));
	MainWindow::getInstance()->toggleStatusToolTip(ui.checkBoxDisableSysTrayToolTip->isChecked());
	Settings->setValueToGroup("StatusBar", "ShowStatus", QVariant(ui.checkBoxShowStatusStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowPeer", QVariant(ui.checkBoxShowPeerStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowNAT", QVariant(ui.checkBoxShowNATStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowDHT", QVariant(ui.checkBoxShowDHTStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowHashing", QVariant(ui.checkBoxShowHashingStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowDisc", QVariant(ui.checkBoxShowDiscStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowRate", QVariant(ui.checkBoxShowRateStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowOpMode", QVariant(ui.checkBoxShowOpModeStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowSound", QVariant(ui.checkBoxShowSoundStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowToaster", QVariant(ui.checkBoxShowToasterDisable->isChecked()));
	Settings->setValueToGroup("StatusBar", "ShowSysTrayOnStatusBar", QVariant(ui.checkBoxShowSystrayOnStatus->isChecked()));

	return true;
}

/** Loads the settings for this page */
void AppearancePage::load()
{
	int index = ui.cmboLanguage->findData(Settings->getLanguageCode());
	ui.cmboLanguage->setCurrentIndex(index);

	index = ui.cmboStyle->findData(Rshare::style().toLower());
	ui.cmboStyle->setCurrentIndex(index);

	index = ui.cmboStyleSheet->findData(Settings->getSheetName());
	if (index == -1) {
		/* set standard "no style sheet" */
		index = ui.cmboStyleSheet->findData("");
	}
	ui.cmboStyleSheet->setCurrentIndex(index);

	ui.rbtPageOnToolBar->setChecked(Settings->getPageButtonLoc());
	ui.rbtPageOnListItem->setChecked(!Settings->getPageButtonLoc());
	ui.rbtActionOnToolBar->setChecked(Settings->getActionButtonLoc());
	ui.rbtActionOnListItem->setChecked(!Settings->getActionButtonLoc());
	switch (Settings->getToolButtonStyle())
	{
		case Qt::ToolButtonIconOnly:
			ui.cmboTollButtonsStyle->setCurrentIndex(0);
		break;
		case Qt::ToolButtonTextOnly:
			ui.cmboTollButtonsStyle->setCurrentIndex(1);
		break;
		case Qt::ToolButtonTextBesideIcon:
			ui.cmboTollButtonsStyle->setCurrentIndex(2);
		break;
		case Qt::ToolButtonTextUnderIcon:
		default:
			ui.cmboTollButtonsStyle->setCurrentIndex(3);
	}
	switch (Settings->getToolButtonSize())
	{
		case 8:
			ui.cmboTollButtonsSize->setCurrentIndex(0);
		break;
		case 16:
			ui.cmboTollButtonsSize->setCurrentIndex(1);
		break;
		case 24:
		default:
			ui.cmboTollButtonsSize->setCurrentIndex(2);
		break;
		case 32:
			ui.cmboTollButtonsSize->setCurrentIndex(3);
        break;
        case 64:
            ui.cmboTollButtonsSize->setCurrentIndex(4);
        break;
        case 128:
            ui.cmboTollButtonsSize->setCurrentIndex(5);
    }
	switch (Settings->getListItemIconSize())
	{
		case 8:
			ui.cmboListItemSize->setCurrentIndex(0);
		break;
		case 16:
			ui.cmboListItemSize->setCurrentIndex(1);
		break;
		case 24:
		default:
			ui.cmboListItemSize->setCurrentIndex(2);
		break;
		case 32:
			ui.cmboListItemSize->setCurrentIndex(3);
        break;
        case 64:
            ui.cmboListItemSize->setCurrentIndex(4);
        break;
        case 128:
            ui.cmboListItemSize->setCurrentIndex(5);
    }

	ui.grpStatus->setChecked(Settings->valueFromGroup("StatusBar", "ShowStatusBar", QVariant(true)).toBool());
	ui.checkBoxStatusCompactMode->setChecked(Settings->valueFromGroup("StatusBar", "CompactMode", QVariant(false)).toBool());
	ui.checkBoxDisableSysTrayToolTip->setChecked(Settings->valueFromGroup("StatusBar", "DisableSysTrayToolTip", QVariant(false)).toBool());
	ui.checkBoxShowStatusStatus->  setChecked(Settings->valueFromGroup("StatusBar", "ShowStatus",  QVariant(true)).toBool());
	ui.checkBoxShowPeerStatus->    setChecked(Settings->valueFromGroup("StatusBar", "ShowPeer",    QVariant(true)).toBool());
	ui.checkBoxShowNATStatus->     setChecked(Settings->valueFromGroup("StatusBar", "ShowNAT",     QVariant(true)).toBool());
	ui.checkBoxShowDHTStatus->     setChecked(Settings->valueFromGroup("StatusBar", "ShowDHT",     QVariant(true)).toBool());
	ui.checkBoxShowHashingStatus-> setChecked(Settings->valueFromGroup("StatusBar", "ShowHashing", QVariant(true)).toBool());
	ui.checkBoxShowDiscStatus->    setChecked(Settings->valueFromGroup("StatusBar", "Show eDisc",  QVariant(true)).toBool());
	ui.checkBoxShowRateStatus->    setChecked(Settings->valueFromGroup("StatusBar", "ShowRate",    QVariant(true)).toBool());
	ui.checkBoxShowOpModeStatus->  setChecked(Settings->valueFromGroup("StatusBar", "ShowOpMode",  QVariant(true)).toBool());
	ui.checkBoxShowSoundStatus->   setChecked(Settings->valueFromGroup("StatusBar", "ShowSound",   QVariant(true)).toBool());
	ui.checkBoxShowToasterDisable->setChecked(Settings->valueFromGroup("StatusBar", "ShowToaster", QVariant(true)).toBool());
	ui.checkBoxShowSystrayOnStatus->setChecked(Settings->valueFromGroup("StatusBar", "ShowSysTrayOnStatusBar", QVariant(false)).toBool());

}

void AppearancePage::loadStyleSheet(int index)
{
	Rshare::loadStyleSheet(ui.cmboStyleSheet->itemData(index).toString());
}
