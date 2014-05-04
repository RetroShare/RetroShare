/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

#ifndef GXSMESSAGEFRAMEWIDGET_H
#define GXSMESSAGEFRAMEWIDGET_H

#include "gui/gxs/RsGxsUpdateBroadcastWidget.h"

class RsGxsIfaceHelper;

class GxsMessageFrameWidget : public RsGxsUpdateBroadcastWidget
{
	Q_OBJECT

public:
	explicit GxsMessageFrameWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent = NULL);

	virtual RsGxsGroupId groupId() = 0;
	virtual void setGroupId(const RsGxsGroupId &groupId) = 0;
	virtual QString groupName(bool withUnreadCount) = 0;
	virtual QIcon groupIcon() = 0;
	virtual void setAllMessagesRead(bool read) = 0;

signals:
	void groupChanged(QWidget *widget);
};

#endif // GXSMESSAGEFRAMEWIDGET_H
