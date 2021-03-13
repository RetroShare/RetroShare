/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/TransferUserNotify.h                    *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef TRANSFERUSERNOTIFY_H
#define TRANSFERUSERNOTIFY_H

#include "gui/common/UserNotify.h"

class TransferUserNotify : public UserNotify
{
	Q_OBJECT

public:
	TransferUserNotify(QObject *parent = 0);

	virtual bool hasSetting(QString *name, QString *group) override;

private:
	virtual QIcon getIcon() override;
	virtual QIcon getMainIcon(bool hasNew) override;
	virtual unsigned int getNewCount() override;

	virtual QString getTrayMessage(bool plural) override;
	virtual QString getNotifyMessage(bool plural) override;

	virtual void iconClicked() override;

	unsigned int newTransferCount;
};

#endif // TRANSFERUSERNOTIFY_H
