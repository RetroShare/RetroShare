/*******************************************************************************
 * gui/settings/PluginsPage.cpp                                                *
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

#include <iostream>

#include <QDialog>
#include <QFileInfo>

#include "PluginsPage.h"
#include "PluginItem.h"
#include "rshare.h"
#include "rsharesettings.h"
#include "util/misc.h"

#include <retroshare/rsplugin.h>

#include "../MainWindow.h"

settings::PluginsPage::PluginsPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);

	ui.wireBox->hide();
	ui.wikiBox->hide();
	ui.photosBox->hide();

	 QString text ;

	 std::cerr << "PluginsPage: adding plugins" << std::endl;

	 if(rsPlugins->nbPlugins() > 0)
		 for(int i=0;i<rsPlugins->nbPlugins();++i)
		 {
			 std::cerr << "  Adding new page." << std::endl;

             std::string file_name,  error_string ;
             RsFileHash file_hash ;
			 uint32_t status ;
			 uint32_t svn_revision ;

			 rsPlugins->getPluginStatus(i,status,file_name,file_hash,svn_revision,error_string) ;

			 QString status_string ;
             QString short_status_string;

			 switch(status)
			 {
                 case PLUGIN_STATUS_REJECTED_HASH: status_string = tr("Plugin disabled. Click the enable button and restart Retroshare") ;
                                                   short_status_string = tr("[disabled]");
															  break ;

				 case PLUGIN_STATUS_MISSING_API:   status_string = tr("No API number supplied. Please read plugin development manual.") ;
                                                   short_status_string = tr("[loading problem]");
															  break ;

				 case PLUGIN_STATUS_MISSING_SVN:   status_string = tr("No SVN number supplied. Please read plugin development manual.") ;
                                                   short_status_string = tr("[loading problem]");
															  break ;

				 case PLUGIN_STATUS_DLOPEN_ERROR:  status_string = tr("Loading error.") ;
                                                   short_status_string = tr("[loading problem]");
															  break ;

				 case PLUGIN_STATUS_MISSING_SYMBOL:status_string = tr("Missing symbol. Wrong version?") ;
                                                   short_status_string = tr("[loading problem]");
															  break ;

				 case PLUGIN_STATUS_NULL_PLUGIN:	  status_string = tr("No plugin object") ;
                                                   short_status_string = tr("[loading problem]");
															  break ;

				 case PLUGIN_STATUS_LOADED:		  status_string = tr("Plugins is loaded.") ;
															  break ;
				 default:
															status_string = tr("Unknown status.") ;
			 }

			 QIcon plugin_icon(":images/disabled_plugin_48.png") ;
			 RsPlugin *plugin = rsPlugins->plugin(i) ;
             QString pluginTitle = QFileInfo(QString::fromStdString(file_name)).fileName();
             QString pluginDescription = status_string;
             QString pluginVersion = short_status_string;

			 if(plugin!=NULL)
			 {
				QIcon *icon = plugin->qt_icon();
				if(icon != NULL)
					plugin_icon = *icon ;

				pluginTitle = QString::fromUtf8(plugin->getPluginName().c_str()) ;
				pluginDescription = QString::fromUtf8(plugin->getShortPluginDescription().c_str()) ;

				int major = 0;
				int minor = 0;
				int build = 0;
				int svn_rev = 0;
				plugin->getPluginVersion(major, minor, build, svn_rev);
				pluginVersion = QString("%1.%2.%3.%4").arg(major).arg(minor).arg(build).arg(svn_rev);
			}

			 PluginItem *item = new PluginItem(pluginVersion, i,pluginTitle,pluginDescription,status_string,
					 						QString::fromStdString(file_name),
                                            QString::fromStdString(file_hash.toStdString()),QString::fromStdString(error_string),
											plugin_icon) ;

			 ui._pluginsLayout->insertWidget(0,item) ;


			 if(plugin == NULL || plugin->qt_config_panel() == NULL)
                 item->_configure_PB->hide() ;


			 if(plugin != NULL){
				 item->enableButton->hide();
				 item->disableButton->show();
				 }else{
				 item->enableButton->show();
				 item->disableButton->hide();
                 item->_about_PB->hide();
				 }

			 //if(rsPlugins->getAllowAllPlugins())

			 QObject::connect(item,SIGNAL(pluginEnabled(const QString&)),this,SLOT(enablePlugin(const QString&))) ;
			 QObject::connect(item,SIGNAL(pluginDisabled(const QString&)),this,SLOT(disablePlugin(const QString&))) ;

			 QObject::connect(item,SIGNAL(pluginConfigure(int)),this,SLOT(configurePlugin(int))) ;
			 QObject::connect(item,SIGNAL(pluginAbout(int)),this,SLOT(aboutPlugin(int))) ;
		 }
	 ui._pluginsLayout->update() ;

	 const std::vector<std::string>& dirs(rsPlugins->getPluginDirectories()) ;
	 text = "" ;

	 for(uint i=0;i<dirs.size();++i)
		 text += "<b>"+QString::fromStdString(dirs[i]) + "</b><br>" ;

	 ui._lookupDirectories_TB->setHtml(text) ;

	// todo
	ui.enableAll->setChecked(rsPlugins->getAllowAllPlugins());
	ui.enableAll->setToolTip(tr("Check this for developing plugins. They will not\nbe checked for the hash. However, in normal\ntimes, checking the hash protects you from\nmalicious behavior of crafted plugins."));
    ui.enableAll->setEnabled(false);

	QObject::connect(ui.enableAll,SIGNAL(toggled(bool)),this,SLOT(toggleEnableAll(bool))) ;
	
	connect(ui.wireBox,     SIGNAL(toggled(bool)),          this,SLOT(enableWire()  ));
	connect(ui.wikiBox,     SIGNAL(toggled(bool)),          this,SLOT(enableWiki()  ));
	connect(ui.photosBox,   SIGNAL(toggled(bool)),          this,SLOT(enablePhotos()  ));
	
#ifdef RS_USE_WIRE
	ui.wireBox->show();
#endif
#ifdef RS_USE_WIKI
	ui.wikiBox->show();
#endif
#ifdef RS_USE_PHOTO
	ui.photosBox->show();
#endif
}

void settings::PluginsPage::enableWire()       { Settings->setValueToGroup("Services", "Wire", ui.wireBox->isChecked()); }
void settings::PluginsPage::enableWiki()       { Settings->setValueToGroup("Services", "Wiki", ui.wikiBox->isChecked()); }
void settings::PluginsPage::enablePhotos()       { Settings->setValueToGroup("Services", "Photos", ui.photosBox->isChecked()); }

QString settings::PluginsPage::helpText() const
{
   return tr("<h1><img width=\"24\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Plugins</h1>     \
              <p>Plugins are loaded from the directories listed in the bottom list.</p>         \
              <p>For security reasons, accepted plugins load automatically until                \
              the main Retroshare executable or the plugin library changes. In                  \
              such a case, the user needs to confirm them again.                                \
              After the program is started, you can enable a plugin manually by clicking on the \
              \"Enable\" button and then restart Retroshare.</p>                                \
              <p>If you want to develop your own plugins, contact the developpers team          \
              they will be happy to help you out!</p>") ;
}
void settings::PluginsPage::toggleEnableAll(bool b)
{
	rsPlugins->allowAllPlugins(b) ;
}
void settings::PluginsPage::aboutPlugin(int i)
{
	std::cerr << "Launching about window for plugin " << i << std::endl;

	QDialog *dialog = NULL;
	if(rsPlugins->plugin(i) != NULL && (dialog = rsPlugins->plugin(i)->qt_about_page()) != NULL)
		dialog->exec() ;
}
void settings::PluginsPage::configurePlugin(int i)
{
	std::cerr << "Launching configuration window for plugin " << i << std::endl;

	if(rsPlugins->plugin(i) != NULL && rsPlugins->plugin(i)->qt_config_panel() != NULL)
		rsPlugins->plugin(i)->qt_config_panel()->show() ;
}

void settings::PluginsPage::enablePlugin(const QString& hash)
{
	std::cerr << "Switching status of plugin " << hash.toStdString() << " to  enable" << std::endl;

        rsPlugins->enablePlugin(RsFileHash(hash.toStdString()) );
}

void settings::PluginsPage::disablePlugin(const QString& hash)
{
	std::cerr << "Switching status of plugin " << hash.toStdString() << " to disable " << std::endl;

        rsPlugins->disablePlugin(RsFileHash(hash.toStdString())) ;
}

settings::PluginsPage::~PluginsPage()
{
}

/** Loads the settings for this page */
void settings::PluginsPage::load()
{
    Settings->beginGroup(QString("Services"));

    whileBlocking(ui.wireBox)->setChecked(Settings->value("Wire", true).toBool());
    whileBlocking(ui.wikiBox)->setChecked(Settings->value("Wiki", true).toBool());
    whileBlocking(ui.photosBox)->setChecked(Settings->value("Photos", true).toBool());

    Settings->endGroup();
}
