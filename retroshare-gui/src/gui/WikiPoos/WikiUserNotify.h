/*******************************************************************************
 * gui/WikiPoos/WikiUserNotify.h                                               *
 *                                                                             *
 * Copyright (C) 2024 RetroShare Team <contact@retroshare.cc>                  *
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

#ifndef WIKIUSERNOTIFY_H
#define WIKIUSERNOTIFY_H

#include "gui/gxs/GxsUserNotify.h"
#include "gui/gxs/GxsGroupFrameDialog.h"

class WikiUserNotify : public GxsUserNotify
{
	Q_OBJECT

public:
	explicit WikiUserNotify(RsGxsIfaceHelper *ifaceImpl, const GxsGroupFrameDialog *g, QObject *parent = 0);

	virtual bool hasSetting(QString *name, QString *group) override;

private:
	virtual QIcon getIcon() override;
	virtual QIcon getMainIcon(bool hasNew) override;

	virtual void iconClicked() override;
};

#endif // WIKIUSERNOTIFY_H

