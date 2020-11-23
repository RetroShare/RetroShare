/*******************************************************************************
 * plugins/FeedReader/gui/AddReaderConfig.h                                    *
 *                                                                             *
 * Copyright (C) 2012 by Thunder                                               *
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

	/** Loads the settings for this page */
	virtual void load();

	virtual QPixmap iconPixmap() const { return QPixmap(":/images/FeedReader.png") ; }
	virtual QString pageName() const { return tr("FeedReader") ; }
	virtual QString helpText() const { return ""; }

private slots:
	void useProxyToggled();
	void updateProxy();

private:
	Ui::FeedReaderConfig *ui;
};

#endif
