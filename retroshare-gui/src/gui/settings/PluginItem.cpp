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

PluginItem::PluginItem(int id, const QString& pluginTitle,const QString& pluginDescription,const QString& status, const QString& file_name, const QString& file_hash, const QString& error_string, const QIcon& icon)
	:QWidget(NULL)
{
	setupUi(this) ;

	_id = id ;
	_statusLabel->setText(status) ;
	_hashLabel->setText(file_hash) ;
	_name_LE->setText(file_name) ;
	_pluginIcon->setIcon(icon) ;
	_pluginIcon->setText(QString()) ;
	msgLabel->setText(pluginDescription) ;
	subjectLabel->setText(pluginTitle) ;

	QObject::connect(_enabled_CB,SIGNAL(toggled(bool)),this,SLOT(togglePlugin(bool))) ;
	QObject::connect(_configure_PB,SIGNAL(clicked()),this,SLOT(configurePlugin())) ;
}

void PluginItem::togglePlugin(bool b)
{
	emit( pluginEnabled(b,_hashLabel->text()) ) ;
}

void PluginItem::configurePlugin()
{
	emit( pluginConfigure(_id) ) ;
}


