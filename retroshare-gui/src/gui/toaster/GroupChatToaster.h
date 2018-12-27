/*******************************************************************************
 * gui/toaster/GroupChatToaster.h                                              *
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

#ifndef GROUPCHATTOASTER_H
#define GROUPCHATTOASTER_H

#include "ui_GroupChatToaster.h"

/**
 * Shows a toaster when friend is GroupChat .
 *
 *
 */
class GroupChatToaster : public QWidget
{
	Q_OBJECT

public:
	GroupChatToaster(const RsPeerId &peerId, const QString &message);

private slots:
	void chatButtonSlot();

private:
	RsPeerId peerId;

	/** Qt Designer generated object */
	Ui::GroupChatToaster ui;
};

#endif	//GROUPCHATTOASTER_H
