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

#ifndef RSETTINGSWIN_HPP_
#define RSETTINGSWIN_HPP_

#include <QDialog>
#include <retroshare-gui/configpage.h>
#include "ui_settingsw.h"
#include "mainpage.h"

class FloatingHelpBrowser;

#define IMAGE_PREFERENCES       ":/icons/png/options.png"

class SettingsPage: public MainPage
{
	Q_OBJECT

public:
	SettingsPage(QWidget *parent = 0);

	enum PageType { LastPage = -1, General = 0, Server, Transfer,Relay,
					Directories, Plugins, Notify, Security, Message, Forum, Chat, Appearance, Sound, Fileassociations };

	static void showYourself(QWidget *parent, PageType page = LastPage);

	void postModDirectories(bool update_local);

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_PREFERENCES) ; } //MainPage
	virtual QString pageName() const { return tr("Preferences") ; } //MainPage

protected:
	~SettingsPage();

	void addPage(ConfigPage*) ;

public slots:
	//! Go to a specific part of the control panel.
	void setNewPage(int page);

private slots:
	void notifySettingsChanged();

	// Called when user clicks "Save Settings"
	//void saveChanges();
	//void dialogFinished(int result);

private:
	void initStackedWidget();

private:
	FloatingHelpBrowser *mHelpBrowser;
	static int lastPage;

	/* UI - from Designer */
	Ui::Settings ui;
};

#endif // !RSETTINGSWIN_HPP_
