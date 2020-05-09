/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostsUserNotify.h              *
 *                                                                             *
 * Copyright 2014 Retroshare Team      <retroshare.project@gmail.com>          *
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

#ifndef GXSCHANNELUSERNOTIFY_H
#define GXSCHANNELUSERNOTIFY_H

#include "gui/gxs/GxsUserNotify.h"
#include "gui/gxs/GxsGroupFrameDialog.h"

class GxsChannelUserNotify : public GxsUserNotify
{
	Q_OBJECT

public:
	GxsChannelUserNotify(RsGxsIfaceHelper *ifaceImpl, const GxsGroupFrameDialog *g, QObject *parent = 0);

	virtual bool hasSetting(QString *name, QString *group);

private:
	virtual QIcon getIcon();
	virtual QIcon getMainIcon(bool hasNew);
	virtual void iconClicked();
};

#endif // GXSCHANNELUSERNOTIFY_H
