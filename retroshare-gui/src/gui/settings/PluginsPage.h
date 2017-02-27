/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#pragma once

#include <retroshare-gui/configpage.h>
#include "ui_PluginsPage.h"

class PluginsPage : public ConfigPage
{
	Q_OBJECT

	public:
		PluginsPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
		~PluginsPage();

		/** Loads the settings for this page */
		virtual void load();

		virtual QPixmap iconPixmap() const { return QPixmap(":/icons/settings/plugins.svg") ; }
		virtual QString pageName() const { return tr("Plugins") ; }
		virtual QString helpText() const ;


	public slots:
		void enablePlugin(const QString&) ;
		void disablePlugin(const QString&) ;

		void configurePlugin(int i) ;
		void aboutPlugin(int i) ;
		void toggleEnableAll(bool) ;

	private:
			Ui::PluginsPage ui;
};

