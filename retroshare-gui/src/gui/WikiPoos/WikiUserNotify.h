/*******************************************************************************
 * retroshare-gui/src/gui/WikiPoos/WikiUserNotify.h                           *
 *                                                                             *
 * Copyright 2014-2026 Retroshare Team <retroshare.project@gmail.com>          *
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

#include "gui/common/UserNotify.h"

class RsGxsIfaceHelper;

class WikiUserNotify : public UserNotify
{
	Q_OBJECT

public:
	explicit WikiUserNotify(RsGxsIfaceHelper *ifaceImpl, QObject *parent = nullptr);

	virtual bool hasSetting(QString *name, QString *group) override;

protected:
	virtual void startUpdate() override;

private:
	virtual QIcon getIcon() override;
	virtual QIcon getMainIcon(bool hasNew) override;
	virtual unsigned int getNewCount() override;

	virtual void iconClicked() override;

	RsGxsIfaceHelper *mInterface;
	unsigned int mNewCount;
};

#endif // WIKIUSERNOTIFY_H
