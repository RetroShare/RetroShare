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

#include <iostream>

#include "PluginsPage.h"
#include "PluginItem.h"
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

	 std::cerr << "PluginsPage: adding plugins" << std::endl;

	 if(rsPlugins->nbPlugins() > 0)
		 for(int i=0;i<rsPlugins->nbPlugins();++i)
		 {
			 std::cerr << "  Adding new page." << std::endl;

			 std::string file_name, file_hash, error_string ;
			 uint32_t status ;

			 rsPlugins->getPluginStatus(i,status,file_name,file_hash,error_string) ;

			 QString status_string ;

			 switch(status)
			 {
				 case PLUGIN_STATUS_UNKNOWN_HASH: status_string = tr("Hash rejected. Add to white list.") ;
															 break ;
				 case PLUGIN_STATUS_DLOPEN_ERROR: status_string = tr("Loading error.") ;
															 break ;
				 case PLUGIN_STATUS_MISSING_SYMBOL: status_string = tr("Missing symbol. Wrong version?") ;
																break ;
				 case PLUGIN_STATUS_NULL_PLUGIN:		status_string = tr("No plugin object") ;
																break ;
				 case PLUGIN_STATUS_LOADED:		status_string = tr("Plugins is loaded.") ;
															break ;
				 default:
															status_string = tr("Unknown status.") ;
			 }

			 QIcon plugin_icon(":images/disabled_plugin_48.png") ;
			 RsPlugin *plugin = rsPlugins->plugin(i) ;
			 QString pluginTitle = tr("Title unavailable") ;
			 QString pluginDescription = tr("Description unavailable") ;

			 if(plugin!=NULL)
			 {
				 if(plugin->qt_icon() != NULL)
					 plugin_icon = *plugin->qt_icon() ;

				 pluginTitle = QString::fromStdString(plugin->getPluginName()) ;
				 pluginDescription = QString::fromStdString(plugin->getShortPluginDescription()) ;
			 }

			 PluginItem *item = new PluginItem(i,pluginTitle,pluginDescription,status_string,
					 						QString::fromStdString(file_name),
						 					QString::fromStdString(file_hash),QString::fromStdString(error_string),
											plugin_icon) ;

			 ui._pluginsLayout->insertWidget(0,item) ;

			 if(plugin == NULL || plugin->qt_config_panel() == NULL)
				 item->_configure_PB->setEnabled(false) ;

			 if(plugin != NULL)
				 item->_enabled_CB->setChecked(true) ;

			 QObject::connect(item,SIGNAL(pluginEnabled(bool,const QString&)),this,SLOT(togglePlugin(bool,const QString&))) ;
			 QObject::connect(item,SIGNAL(pluginConfigure(int)),this,SLOT(configurePlugin(int))) ;
		 }
	 ui._pluginsLayout->update() ;

	 const std::vector<std::string>& dirs(rsPlugins->getPluginDirectories()) ;
	 text = "" ;

	 for(int i=0;i<dirs.size();++i)
		 text += "<b>"+QString::fromStdString(dirs[i]) + "</b><br/>" ;

	 ui._lookupDirectories_TB->setHtml(text) ;
}
void PluginsPage::configurePlugin(int i)
{
	std::cerr << "Launching configuration window for plugin " << i << std::endl;

	if(rsPlugins->plugin(i) != NULL && rsPlugins->plugin(i)->qt_config_panel() != NULL)
		rsPlugins->plugin(i)->qt_config_panel()->show() ;
}
void PluginsPage::togglePlugin(bool b,const QString& hash)
{
	std::cerr << "Switching status of plugin " << hash.toStdString() << " to " << b << std::endl;

	if(b)
		rsPlugins->enablePlugin(hash.toStdString()) ;
	else
		rsPlugins->disablePlugin(hash.toStdString()) ;
}

PluginsPage::~PluginsPage()
{
}

/** Saves the changes on this page */
bool PluginsPage::save(QString &/*errmsg*/)
{
	// nothing to save for now.
    return true;
}

/** Loads the settings for this page */
void PluginsPage::load()
{
}
