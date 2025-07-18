/*******************************************************************************
 * gui/settings/ChannelPage.h                                                  *
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

#ifndef CHANNELPAGE_H
#define CHANNELPAGE_H

#include "retroshare-gui/configpage.h"
#include "ui_ChannelPage.h"
#include "gui/common/FilesDefs.h"

#define Size_Of_1_GB	                            (1024 * 1024 * 1024)	// It is the size of 1 GB in bytes.

class ChannelPage : public ConfigPage
{
	Q_OBJECT

public:
	ChannelPage(QWidget * parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	~ChannelPage();

	/** Loads the settings for this page */
	virtual void load();

    virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/settings/channels.svg") ; }
	virtual QString pageName() const { return tr("Channels") ; }
	virtual QString helpText() const { return ""; }

private slots:
	void updateEmotes();
  
    // Function to update the maximum size allowed for auto download in channels
    void updateMaxAutoDownloadSizeLimit(int value);

private:
	Ui::ChannelPage ui;
};

#endif // !CHANNELPAGE_H

