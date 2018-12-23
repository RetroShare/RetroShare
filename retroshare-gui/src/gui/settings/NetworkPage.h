/*******************************************************************************
 * gui/settings/NetworkPage.h                                                  *
 *                                                                             *
 * Copyright (C) 2006 Crypton <retroshare.project@gmail.com>                   *
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

#ifndef NETWORKPAGE_H
#define NETWORKPAGE_H

#include <retroshare-gui/configpage.h>
#include "ui_NetworkPage.h"

class NetworkPage : public ConfigPage
{
public:
    NetworkPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~NetworkPage() {}

    /** Loads the settings for this page */
    virtual void load();

	 virtual QPixmap iconPixmap() const { return QPixmap(":/icons/settings/network.svg") ; }
	 virtual QString pageName() const { return tr("Network") ; }

private:
    Ui::NetworkPage ui;
};

#endif // !NETWROKPAGE_H

