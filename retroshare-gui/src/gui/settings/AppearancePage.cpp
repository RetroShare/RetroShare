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

#include <QStyleFactory>
#include <QFileInfo>
#include <QDir>

#include "lang/languagesupport.h"
#include <rshare.h>
#include "AppearancePage.h"
#include "rsharesettings.h"
#include "gui/MainWindow.h"
#include "gui/statusbar/SoundStatus.h"
#include "gui/statusbar/ToasterDisable.h"

/** Constructor */
AppearancePage::AppearancePage(QWidget * parent, Qt::WindowFlags flags)
	: ConfigPage(parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	MainWindow *pMainWindow = MainWindow::getInstance();
	connect(ui.cmboStyleSheet, SIGNAL(activated(int)), this, SLOT(loadStyleSheet(int)));
	connect(ui.checkBoxStatusCompactMode, SIGNAL(toggled(bool)), pMainWindow, SLOT(setCompactStatusMode(bool)));
	connect(ui.checkBoxHideSoundStatus, SIGNAL(toggled(bool)), pMainWindow->soundStatusInstance(), SLOT(setHidden(bool)));
	connect(ui.checkBoxHideToasterDisable, SIGNAL(toggled(bool)), pMainWindow->toasterDisableInstance(), SLOT(setHidden(bool)));

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

	/* Hide platform specific features */
#ifdef Q_WS_WIN
#endif
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
	}

	/* Set to new style */
	Rshare::setStyle(ui.cmboStyle->currentText());

	Settings->setValueToGroup("StatusBar", "CompactMode", QVariant(ui.checkBoxStatusCompactMode->isChecked()));
	Settings->setValueToGroup("StatusBar", "HideSound", QVariant(ui.checkBoxHideSoundStatus->isChecked()));
	Settings->setValueToGroup("StatusBar", "HideToaster", QVariant(ui.checkBoxHideToasterDisable->isChecked()));

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
	}

	ui.checkBoxStatusCompactMode->setChecked(Settings->valueFromGroup("StatusBar", "CompactMode", QVariant(false)).toBool());
	ui.checkBoxHideSoundStatus->setChecked(Settings->valueFromGroup("StatusBar", "HideSound", QVariant(false)).toBool());
	ui.checkBoxHideToasterDisable->setChecked(Settings->valueFromGroup("StatusBar", "HideToaster", QVariant(false)).toBool());

}

void AppearancePage::loadStyleSheet(int index)
{
	Rshare::loadStyleSheet(ui.cmboStyleSheet->itemData(index).toString());
}
