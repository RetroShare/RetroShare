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

#include <QtGui/QDialog>
#include <retroshare-gui/configpage.h>
#include "ui_settings.h"

class FloatingHelpBrowser;

class RSettingsWin: public QDialog, private Ui::Settings
{
	Q_OBJECT

public:
	enum PageType { LastPage = -1, General = 0, Server, Transfer,Relay,
					Directories, Plugins, Notify, Security, Message, Forum, Chat, Appearance, Sound, Fileassociations };

	static void showYourself(QWidget *parent, PageType page = LastPage);
	static void postModDirectories(bool update_local);

protected:
	RSettingsWin(QWidget *parent = 0);
	~RSettingsWin();

	void addPage(ConfigPage*) ;

public slots:
	//! Go to a specific part of the control panel.
	void setNewPage(int page);

private slots:
	/** Called when user clicks "Save Settings" */
	void saveChanges();
	void dialogFinished(int result);

private:
	void initStackedWidget();

private:
	FloatingHelpBrowser *mHelpBrowser;
	static RSettingsWin *_instance;
	static int lastPage;
};

#endif // !RSETTINGSWIN_HPP_
