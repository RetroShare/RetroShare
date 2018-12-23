/*******************************************************************************
 * gui/settings/PluginItem.cpp                                                 *
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

#include "PluginItem.h"

PluginItem::PluginItem(const QString& pluginVersion, int id, const QString& pluginTitle,const QString& pluginDescription,const QString& status, const QString& file_name, const QString& file_hash, const QString& error_string, const QIcon& icon)
	:QWidget(NULL)
{
	setupUi(this) ;

	_id = id ;
	_statusLabel->setText(status) ;
	_statusLabel->setToolTip(error_string);
	_hashLabel->setText(file_hash) ;
	_name_LE->setText(file_name) ;
	_pluginIcon->setIcon(icon) ;
	_pluginIcon->setText(QString()) ;
	msgLabel->setText(pluginDescription) ;
	subjectLabel->setText(pluginTitle + "  "+ pluginVersion) ;
	infoLabel->setText(pluginTitle + " " + tr("will be enabled after your restart RetroShare.")) ;
	infoLabel->hide();

	QObject::connect(_configure_PB,SIGNAL(clicked()),this,SLOT(configurePlugin())) ;
	QObject::connect(_about_PB,SIGNAL(clicked()),this,SLOT(aboutPlugin())) ;
	
	QObject::connect(enableButton,SIGNAL(clicked()),this,SLOT(enablePlugin())) ;
	QObject::connect(disableButton,SIGNAL(clicked()),this,SLOT(disablePlugin())) ;
	
	expandFrame->hide();
}

void PluginItem::enablePlugin()
{
	emit( pluginEnabled(_hashLabel->text()) ) ;
	infoLabel->show();
	disableButton->show();	
	enableButton->hide();
}

void PluginItem::disablePlugin()
{
	emit( pluginDisabled(_hashLabel->text()) ) ;
	infoLabel->hide();
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

