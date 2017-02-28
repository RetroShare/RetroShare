/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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

#ifndef _GENERALPAGE_H
#define _GENERALPAGE_H

#include <retroshare-gui/configpage.h>
#include "ui_GeneralPage.h"

class GeneralPage : public ConfigPage
{
	Q_OBJECT

public:
	/** Default Constructor */
	GeneralPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	/** Default Destructor */
	~GeneralPage();

	/** Saves the changes on this page */
	/** Loads the settings for this page */
	virtual void load();

	virtual QPixmap iconPixmap() const { return QPixmap(":/icons/settings/general.svg") ; }
	virtual QString pageName() const { return tr("General") ; }
	virtual QString helpText() const { return ""; }

public slots:
	//void runStartWizard() ;
	void updateAdvancedMode();
	void updateUseLocalServer()   ;
	void updateMaxTimeBeforeIdle();
	void updateStartMinimized()   ;
	void updateDoQuit()           ;
	void updateCloseToTray()      ;
	void updateAutoLogin()        ;
	void updateRunRSOnBoot()      ;
	void updateRegisterRSProtocol();

private:
	/** Qt Designer generated object */
	Ui::GeneralPage ui;
};

#endif

