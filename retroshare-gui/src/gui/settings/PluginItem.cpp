/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 Cyril Soler  
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

#include "PluginItem.h"

PluginItem::PluginItem(const QString& pluginVersion, int pluginId, const QString& pluginTitle,const QString& pluginDescription,const QString& pluginStatus, const QString& file_name, const QString& file_hash, const QString& error_string, const QIcon& icon)
	:QWidget(NULL)
{
	setupUi(this) ;

	_id = pluginId ;
	status->setText(pluginStatus) ;
	status->setToolTip(error_string);
	hash->setText(file_hash) ;
	name->setText(file_name) ;
	pluginIcon->setIcon(icon) ;
	pluginIcon->setText(QString()) ;
	msgLabel->setText(pluginDescription) ;
	subject->setText(pluginTitle + "  "+ pluginVersion) ;
	info->setText(pluginTitle + " " + tr("will be enabled after your restart RetroShare.")) ;
	info->hide();

	QObject::connect(configureButton,SIGNAL(clicked()),this,SLOT(configurePlugin())) ;
	QObject::connect(aboutButton,SIGNAL(clicked()),this,SLOT(aboutPlugin())) ;
	
	QObject::connect(enableButton,SIGNAL(clicked()),this,SLOT(enablePlugin())) ;
	QObject::connect(disableButton,SIGNAL(clicked()),this,SLOT(disablePlugin())) ;
	
	expandFrame->hide();
}

void PluginItem::enablePlugin()
{
	emit( pluginEnabled(hash->text()) ) ;
	info->show();
	disableButton->show();	
	enableButton->hide();
}

void PluginItem::disablePlugin()
{
	emit( pluginDisabled(hash->text()) ) ;
	info->hide();
	enableButton->show();
	disableButton->hide();	
}

void PluginItem::aboutPlugin()
{
	emit( pluginAbout(_id) ) ;
}

void PluginItem::configurePlugin()
{
	emit( pluginConfigure(_id) ) ;
}

void PluginItem::on_moreinfo_label_linkActivated(QString)
{
	if (expandFrame->isVisible()) {
		expandFrame->hide();
	} else {
		expandFrame->show();
	}
}

