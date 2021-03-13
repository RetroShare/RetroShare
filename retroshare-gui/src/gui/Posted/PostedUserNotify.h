/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedListWidget.h                            *
 *                                                                             *
 * Copyright (C) 2014 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#ifndef POSTEDUSERNOTIFY_H
#define POSTEDUSERNOTIFY_H

#include "gui/gxs/GxsUserNotify.h"

class PostedUserNotify : public GxsUserNotify
{
	Q_OBJECT

public:
	explicit PostedUserNotify(RsGxsIfaceHelper *ifaceImpl, const GxsGroupFrameDialog *g, QObject *parent = 0);

	virtual bool hasSetting(QString *name, QString *group) override;

private:
	virtual QIcon getIcon() override;
	virtual QIcon getMainIcon(bool hasNew) override;

	virtual QString getTrayMessage(bool plural) override;
	virtual QString getNotifyMessage(bool plural) override;

	virtual void iconClicked() override;
};

#endif // POSTEDUSERNOTIFY_H
