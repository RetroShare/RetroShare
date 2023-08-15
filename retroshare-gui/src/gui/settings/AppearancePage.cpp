/*******************************************************************************
 * gui/settings/AppearancePage.cpp                                             *
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

#include "AppearancePage.h"

#include "rshare.h"
#include "gui/MainWindow.h"
#include "gui/notifyqt.h"
#include "gui/common/FilesDefs.h"
#include "gui/settings/rsharesettings.h"
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
#include "lang/languagesupport.h"
#include "util/misc.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QStyleFactory>

/** Constructor */
AppearancePage::AppearancePage(QWidget * parent, Qt::WindowFlags flags)
	: ConfigPage(parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	connect(ui.grpStatus,                     SIGNAL(toggled(bool)), this /* pMainWindow->statusBar(),              */, SLOT(switch_status_grpStatus(bool)));
	connect(ui.checkBoxStatusCompactMode,     SIGNAL(toggled(bool)), this /* pMainWindow,                           */, SLOT(switch_status_compactMode(bool)));
	connect(ui.checkBoxDisableSysTrayToolTip, SIGNAL(toggled(bool)), this /* pMainWindow,                           */, SLOT(switch_status_showToolTip(bool)));
	connect(ui.checkBoxShowStatusStatus,      SIGNAL(toggled(bool)), this /* pMainWindow->statusComboBoxInstance(), */, SLOT(switch_status_ShowStatus(bool)));
	connect(ui.checkBoxShowPeerStatus,        SIGNAL(toggled(bool)), this /* pMainWindow->peerstatusInstance(),     */, SLOT(switch_status_ShowPeer(bool)));
	connect(ui.checkBoxShowNATStatus,         SIGNAL(toggled(bool)), this /* pMainWindow->natstatusInstance(),      */, SLOT(switch_status_ShowNAT(bool)));
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
		ui.cmboLanguage->addItem(FilesDefs::getIconFromQtResourcePath(":/images/flags/" + code + ".png"), LanguageSupport::languageName(code), code);
	}
	foreach (QString style, QStyleFactory::keys()) {
        if(style.toLower() != "gtk2" || (getenv("QT_QPA_PLATFORMTHEME")!=nullptr && !strcmp(getenv("QT_QPA_PLATFORMTHEME"),"gtk2")))	// make sure that if style is gtk2, the system has the correct environment variable set.
            ui.cmboStyle->addItem(style, style.toLower());
	}

	QMap<QString, QString> styleSheets;
	Rshare::getAvailableStyleSheets(styleSheets);

	foreach (QString name, styleSheets.keys()) {
		ui.cmboStyleSheet->addItem(name, styleSheets[name]);
	}

	connect(ui.cmboTollButtonsSize,           SIGNAL(currentIndexChanged(int)), this, SLOT(updateCmboToolButtonSize() ));
//	connect(ui.cmboListItemSize,              SIGNAL(currentIndexChanged(int)), this, SLOT(updateCmboListItemSize()   ));
	connect(ui.cmboTollButtonsStyle,          SIGNAL(currentIndexChanged(int)), this, SLOT(updateCmboToolButtonStyle()));
	connect(ui.cmboLanguage,                  SIGNAL(currentIndexChanged(int)), this, SLOT(updateLanguageCode()       ));
	connect(ui.cmboStyle,                     SIGNAL(currentIndexChanged(int)), this, SLOT(updateInterfaceStyle()     ));
	connect(ui.cmboStyleSheet,                SIGNAL(currentIndexChanged(int)), this, SLOT(updateSheetName()          ));
	connect(ui.cmboStyleSheet,                SIGNAL(activated(int))          , this, SLOT(loadStyleSheet(int)        ));
	connect(ui.checkBoxDisableSysTrayToolTip, SIGNAL(toggled(bool))           , this, SLOT(updateStatusToolTip()      ));

	connect(ui.mainPageButtonType_CB,  SIGNAL(currentIndexChanged(int)),           this, SLOT(updateRbtPageOnToolBar()    ));
//	connect(ui.menuItemsButtonType_CB, SIGNAL(currentIndexChanged(int)),           this, SLOT(updateActionButtonLoc()    ));
}

void AppearancePage::switch_status_grpStatus(bool b)        { switch_status(MainWindow::StatusGrpStatus  ,"ShowStatusBar",         b) ; }
void AppearancePage::switch_status_compactMode(bool b)      { switch_status(MainWindow::StatusCompactMode,"CompactMode",           b) ; }
void AppearancePage::switch_status_showToolTip(bool b)      { switch_status(MainWindow::StatusShowToolTip,"DisableSysTrayToolTip", b) ; }
void AppearancePage::switch_status_ShowStatus(bool b)       { switch_status(MainWindow::StatusShowStatus ,"ShowStatus",            b) ; }
void AppearancePage::switch_status_ShowPeer(bool b)         { switch_status(MainWindow::StatusShowPeer   ,"ShowPeer",              b) ; }
void AppearancePage::switch_status_ShowNAT(bool b)          { switch_status(MainWindow::StatusShowNAT    ,"ShowNAT",               b) ; }
void AppearancePage::switch_status_ShowDHT(bool b)          { switch_status(MainWindow::StatusShowDHT    ,"ShowDHT",               b) ; }
void AppearancePage::switch_status_ShowHashing(bool b)      { switch_status(MainWindow::StatusShowHashing,"ShowHashing",           b) ; }
void AppearancePage::switch_status_ShowDisc(bool b)         { switch_status(MainWindow::StatusShowDisc   ,"ShowDisc",              b) ; }
void AppearancePage::switch_status_ShowRate(bool b)         { switch_status(MainWindow::StatusShowRate   ,"ShowRate",              b) ; }
void AppearancePage::switch_status_ShowOpMode(bool b)       { switch_status(MainWindow::StatusShowOpMode ,"ShowOpMode",            b) ; }
void AppearancePage::switch_status_ShowSound(bool b)        { switch_status(MainWindow::StatusShowSound  ,"ShowSound",             b) ; }
void AppearancePage::switch_status_ShowToaster(bool b)      { switch_status(MainWindow::StatusShowToaster,"ShowToaster",           b) ; }
void AppearancePage::switch_status_ShowSystray(bool b)      { switch_status(MainWindow::StatusShowSystray,"ShowSysTrayOnStatusBar",b) ; }

void AppearancePage::switch_status(MainWindow::StatusElement s,const QString& key, bool b)
{
	MainWindow *pMainWindow = MainWindow::getInstance();

	if (!pMainWindow)
		return;

	Settings->setValueToGroup("StatusBar", key, QVariant(b));

	pMainWindow->switchVisibilityStatus(s,b);
}

void AppearancePage::updateLanguageCode()     { Settings->setLanguageCode(LanguageSupport::languageCode(ui.cmboLanguage->currentText())); }
void AppearancePage::updateInterfaceStyle()
{
#ifndef QT_NO_CURSOR
	QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
	Rshare::setStyle(ui.cmboStyle->currentText());
	Settings->setInterfaceStyle(ui.cmboStyle->currentText());
#ifndef QT_NO_CURSOR
	QApplication::restoreOverrideCursor();
#endif
}
void AppearancePage::updateSheetName()
{
	Settings->setSheetName(ui.cmboStyleSheet->itemData(ui.cmboStyleSheet->currentIndex()).toString());
}

void AppearancePage::loadStyleSheet(int index)
{
#ifndef QT_NO_CURSOR
	QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
	Rshare::loadStyleSheet(ui.cmboStyleSheet->itemData(index).toString());
#ifndef QT_NO_CURSOR
	QApplication::restoreOverrideCursor();
#endif
}

void AppearancePage::updateRbtPageOnToolBar()
{
    Settings->setPageButtonLoc(!ui.mainPageButtonType_CB->currentIndex());
    Settings->setActionButtonLoc(!ui.mainPageButtonType_CB->currentIndex());
	
	int index = ui.mainPageButtonType_CB->currentIndex();
	if (index != 0) {
		ui.cmboTollButtonsStyle->hide();
	}else {
		ui.cmboTollButtonsStyle->show();
	}

    NotifyQt::getInstance()->notifySettingsChanged();
}
void AppearancePage::updateStatusToolTip()    { MainWindow::getInstance()->toggleStatusToolTip(ui.checkBoxDisableSysTrayToolTip->isChecked()); }

void AppearancePage::updateCmboToolButtonStyle()
{
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
    NotifyQt::getInstance()->notifySettingsChanged();
}

void AppearancePage::updateCmboToolButtonSize()
{
	switch (ui.cmboTollButtonsSize->currentIndex())
	{
		case 0:
			Settings->setToolButtonSize(8);
			Settings->setListItemIconSize(8);
		break;
		case 1:
			Settings->setToolButtonSize(16);
			Settings->setListItemIconSize(16);
		break;
		case 2:
			Settings->setToolButtonSize(24);
			Settings->setListItemIconSize(24);
		break;
		case 3:
		default:
			Settings->setToolButtonSize(32);
			Settings->setListItemIconSize(32);
        break;
        case 4:
            Settings->setToolButtonSize(64);
			Settings->setListItemIconSize(64);
        break;
        case 5:
            Settings->setToolButtonSize(128);
			Settings->setListItemIconSize(128);
    }
    NotifyQt::getInstance()->notifySettingsChanged();
}
// void AppearancePage::updateCmboListItemSize()
// {
// 	switch (ui.cmboListItemSize->currentIndex())
// 	{
// 		case 0:
// 			Settings->setListItemIconSize(8);
// 		break;
// 		case 1:
// 			Settings->setListItemIconSize(16);
// 		break;
// 		case 2:
// 		default:
// 			Settings->setListItemIconSize(24);
// 		break;
// 		case 3:
// 			Settings->setListItemIconSize(32);
//         break;
//         case 4:
//             Settings->setListItemIconSize(64);
//         break;
//         case 5:
//             Settings->setListItemIconSize(128);
//     }
//     NotifyQt::getInstance()->notifySettingsChanged();
// }

void AppearancePage::updateStyle() { Rshare::setStyle(ui.cmboStyle->currentText()); }

/** Loads the settings for this page */
void AppearancePage::load()
{
	int index = ui.cmboLanguage->findData(Settings->getLanguageCode());
	whileBlocking(ui.cmboLanguage)->setCurrentIndex(index);

	index = ui.cmboStyle->findData(Rshare::style().toLower());
	whileBlocking(ui.cmboStyle)->setCurrentIndex(index);

	index = ui.cmboStyleSheet->findData(Settings->getSheetName());
	if (index == -1) {
		/* set standard "no style sheet" */
		index = ui.cmboStyleSheet->findData("");
	}
	whileBlocking(ui.cmboStyleSheet)->setCurrentIndex(index);
	
	index = ui.mainPageButtonType_CB->findData(Settings->getPageButtonLoc());
	if (index != 0) {
		ui.cmboTollButtonsStyle->hide();
	}else {
		ui.cmboTollButtonsStyle->show();
	}

	whileBlocking(ui.mainPageButtonType_CB)->setCurrentIndex(!Settings->getPageButtonLoc());
//	ui.menuItemsButtonType_CB->setCurrentIndex(!Settings->getActionButtonLoc());

	switch (Settings->getToolButtonStyle())
	{
		case Qt::ToolButtonIconOnly:
			whileBlocking(ui.cmboTollButtonsStyle)->setCurrentIndex(0);
		break;
		case Qt::ToolButtonTextOnly:
			whileBlocking(ui.cmboTollButtonsStyle)->setCurrentIndex(1);
		break;
		case Qt::ToolButtonTextBesideIcon:
			whileBlocking(ui.cmboTollButtonsStyle)->setCurrentIndex(2);
		break;
		case Qt::ToolButtonTextUnderIcon:
		default:
			whileBlocking(ui.cmboTollButtonsStyle)->setCurrentIndex(3);
	}
	switch (Settings->getToolButtonSize())
	{
		case 8:
			whileBlocking(ui.cmboTollButtonsSize)->setCurrentIndex(0);
		break;
		case 16:
			whileBlocking(ui.cmboTollButtonsSize)->setCurrentIndex(1);
		break;
		case 24:
			whileBlocking(ui.cmboTollButtonsSize)->setCurrentIndex(2);
		break;
		case 32:
		default:
			whileBlocking(ui.cmboTollButtonsSize)->setCurrentIndex(3);
        break;
        case 64:
            whileBlocking(ui.cmboTollButtonsSize)->setCurrentIndex(4);
        break;
        case 128:
            whileBlocking(ui.cmboTollButtonsSize)->setCurrentIndex(5);
    }
//	switch (Settings->getListItemIconSize())
//	{
//		case 8:
//			ui.cmboListItemSize->setCurrentIndex(0);
//		break;
//		case 16:
//			ui.cmboListItemSize->setCurrentIndex(1);
//		break;
//		case 24:
//		default:
//			ui.cmboListItemSize->setCurrentIndex(2);
//		break;
//		case 32:
//			ui.cmboListItemSize->setCurrentIndex(3);
//        break;
//        case 64:
//            ui.cmboListItemSize->setCurrentIndex(4);
//        break;
//        case 128:
//            ui.cmboListItemSize->setCurrentIndex(5);
//    }

	whileBlocking(ui.grpStatus)->setChecked(Settings->valueFromGroup("StatusBar", "ShowStatusBar", QVariant(true)).toBool());
	whileBlocking(ui.checkBoxStatusCompactMode)->setChecked(Settings->valueFromGroup("StatusBar", "CompactMode", QVariant(false)).toBool());
	whileBlocking(ui.checkBoxDisableSysTrayToolTip)->setChecked(Settings->valueFromGroup("StatusBar", "DisableSysTrayToolTip", QVariant(false)).toBool());
	whileBlocking(ui.checkBoxShowStatusStatus)->  setChecked(Settings->valueFromGroup("StatusBar", "ShowStatus",  QVariant(true)).toBool());
	whileBlocking(ui.checkBoxShowPeerStatus)->    setChecked(Settings->valueFromGroup("StatusBar", "ShowPeer",    QVariant(true)).toBool());
	if(MainWindow::hiddenmode)	{
		whileBlocking(ui.checkBoxShowNATStatus)->     setChecked(0);
		whileBlocking(ui.checkBoxShowDHTStatus)->     setChecked(0);
		ui.checkBoxShowNATStatus->setVisible(false);
		ui.checkBoxShowDHTStatus->setVisible(false);
	} else {
		whileBlocking(ui.checkBoxShowNATStatus)->     setChecked(Settings->valueFromGroup("StatusBar", "ShowNAT",     QVariant(true)).toBool());
		whileBlocking(ui.checkBoxShowDHTStatus)->     setChecked(Settings->valueFromGroup("StatusBar", "ShowDHT",     QVariant(true)).toBool());
	}
	whileBlocking(ui.checkBoxShowHashingStatus)-> setChecked(Settings->valueFromGroup("StatusBar", "ShowHashing", QVariant(true)).toBool());
	whileBlocking(ui.checkBoxShowDiscStatus)->    setChecked(Settings->valueFromGroup("StatusBar", "ShowDisc",    QVariant(true)).toBool());
	whileBlocking(ui.checkBoxShowRateStatus)->    setChecked(Settings->valueFromGroup("StatusBar", "ShowRate",    QVariant(true)).toBool());
	whileBlocking(ui.checkBoxShowOpModeStatus)->  setChecked(Settings->valueFromGroup("StatusBar", "ShowOpMode",  QVariant(false)).toBool());
	whileBlocking(ui.checkBoxShowSoundStatus)->   setChecked(Settings->valueFromGroup("StatusBar", "ShowSound",   QVariant(true)).toBool());
	whileBlocking(ui.checkBoxShowToasterDisable)->setChecked(Settings->valueFromGroup("StatusBar", "ShowToaster", QVariant(true)).toBool());
	whileBlocking(ui.checkBoxShowSystrayOnStatus)->setChecked(Settings->valueFromGroup("StatusBar", "ShowSysTrayOnStatusBar", QVariant(false)).toBool());

}
