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

#include "PluginsPage.h"
#include "rshare.h"
#include "rsharesettings.h"

#include <retroshare/rsplugin.h>

#include "../MainWindow.h"

PluginsPage::PluginsPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);

	 QString text ;

	 if(rsPlugins->nbPlugins() > 0)
		 for(int i=0;i<rsPlugins->nbPlugins();++i)
		 {
			 text += "<b>"+tr("Plugin")+":</b> \t" + QString::fromStdString(rsPlugins->plugin(i)->getPluginName()) + "<BR/>" ;
			 text += "<b>"+tr("Description")+":</b> \t" + QString::fromStdString(rsPlugins->plugin(i)->getShortPluginDescription()) + "<BR/>" ;
			 text += "<br/>" ;
		 }
	 else
		 text = tr("<h3>No plugins loaded.</h3>") ;

	 ui._loadedPlugins_TB->setHtml(text) ;

	 const std::vector<std::string>& dirs(rsPlugins->getPluginDirectories()) ;
	 text = "" ;

	 for(int i=0;i<dirs.size();++i)
		 text += "<b>"+QString::fromStdString(dirs[i]) + "</b><br/>" ;

	 ui._lookupDirectories_TB->setHtml(text) ;
}

PluginsPage::~PluginsPage()
{
}

/** Saves the changes on this page */
bool PluginsPage::save(QString &errmsg)
{
	// nothing to save for now.
    return true;
}

/** Loads the settings for this page */
void PluginsPage::load()
{
}
