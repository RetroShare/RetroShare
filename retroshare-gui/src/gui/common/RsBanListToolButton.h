/*******************************************************************************
 * gui/common/RsBanListToolButton.h                                            *
 *                                                                             *
 * Copyright (C) 2015, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef _RSBANLISTTOOLBUTTON_H
#define _RSBANLISTTOOLBUTTON_H

#include <QToolButton>

class RsBanListToolButton : public QToolButton
{
	Q_OBJECT
public:
	enum List
	{
		LIST_WHITELIST, // default
		LIST_BLACKLIST
	};

	enum Mode
	{
		MODE_ADD, // default
		MODE_REMOVE
	};

public:
	explicit RsBanListToolButton(QWidget *parent = 0);

	void setMode(List list, Mode mode);
	bool setIpAddress(const QString &ipAddress);

signals:
	void banListChanged(const QString &ipAddress);

private:
	void updateUi();

private slots:
	void applyIp();

private:
	Mode mMode;
	List mList;
	QString mIpAddress;
};

#endif
