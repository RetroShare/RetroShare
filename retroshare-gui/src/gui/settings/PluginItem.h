/*******************************************************************************
 * gui/settings/PluginItem.h                                                   *
 *                                                                             *
 * Copyright 2011, Cyril Soler     <retroshare.project@gmail.com>              *
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

#pragma once

#include "ui_PluginItem.h"

class PluginItem: public QWidget, public Ui::PluginItem
{
	Q_OBJECT

	public:
		PluginItem(const QString& pluginVersion, int id,const QString& pluginTitle,const QString& pluginDescription,const QString& status, const QString& file_name, const QString& file_hash, const QString& error_string, const QIcon& icon) ;

	protected slots:
		void configurePlugin() ;
		void aboutPlugin() ;
		void enablePlugin();
		void disablePlugin();

	signals:
		void pluginEnabled(const QString&) ;
		void pluginDisabled(const QString&) ;
		void pluginConfigure(int) ;
		void pluginAbout(int) ;

  private slots:
		void on_moreinfo_label_linkActivated(QString link);

	private:
		int _id ;

};


