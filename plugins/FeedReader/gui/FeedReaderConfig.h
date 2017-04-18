/****************************************************************
 *  RetroShare GUI is distributed under the following license:
 *
 *  Copyright (C) 2012 by Thunder
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

#ifndef _FEEDREADERCONFIG_H
#define _FEEDREADERCONFIG_H

#include "retroshare-gui/configpage.h"

namespace Ui {
class FeedReaderConfig;
}

#define FeedReaderSetting_SetMsgToReadOnActivate() Settings->valueFromGroup("FeedReaderDialog", "SetMsgToReadOnActivate", true).toBool()
#define FeedReaderSetting_OpenAllInNewTab() Settings->valueFromGroup("FeedReaderDialog", "OpenAllInNewTab", true).toBool()

class FeedReaderConfig : public ConfigPage 
{
	Q_OBJECT

public:
	/** Default Constructor */
	FeedReaderConfig(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	/** Default Destructor */
	virtual ~FeedReaderConfig();

	/** Saves the changes on this page */
	virtual bool save(QString &errmsg);
	/** Loads the settings for this page */
	virtual void load();

	virtual QPixmap iconPixmap() const { return QPixmap(":/images/FeedReader.png") ; }
	virtual QString pageName() const { return tr("FeedReader") ; }
	virtual QString helpText() const { return ""; }

private slots:
	void useProxyToggled();

private:
	Ui::FeedReaderConfig *ui;
	//bool loaded; //Already in ConfigPage
};

#endif
