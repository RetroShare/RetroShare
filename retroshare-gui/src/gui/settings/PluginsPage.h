/*******************************************************************************
 * gui/settings/PluginsPage.h                                                  *
 *                                                                             *
 * Copyright 2006, Crypton         <retroshare.project@gmail.com>              *
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

#include "retroshare-gui/configpage.h"
#include "ui_PluginsPage.h"
#include "gui/common/FilesDefs.h"

namespace settings {

class PluginsPage : public ConfigPage
{
	Q_OBJECT

	public:
		PluginsPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
		~PluginsPage();

		/** Loads the settings for this page */
		virtual void load();

        virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/settings/plugins.svg") ; }
		virtual QString pageName() const { return tr("Plugins") ; }
		virtual QString helpText() const ;


	public slots:
		void enablePlugin(const QString&) ;
		void disablePlugin(const QString&) ;

		void configurePlugin(int i) ;
		void aboutPlugin(int i) ;
		void toggleEnableAll(bool) ;

	void enableWire();
	void enableWiki() ;
	void enablePhotos() ;

	private:
			Ui::PluginsPage ui;
};

} // namespace settings
