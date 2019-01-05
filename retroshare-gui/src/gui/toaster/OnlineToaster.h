/*******************************************************************************
 * gui/toaster/OnlineToaster.h                                                 *
 *                                                                             *
 * Copyright (C) 2007 Crypton         <retroshare.project@gmail.com>           *
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

#ifndef ONLINETOASTER_H
#define ONLINETOASTER_H

#include <retroshare/rstypes.h>
#include "ui_OnlineToaster.h"

/**
 * Shows a toaster when friend is Online .
 *
 *
 */
class OnlineToaster : public QWidget
{
	Q_OBJECT

public:
    OnlineToaster(const RsPeerId &peerId);

private slots:
	void chatButtonSlot();

private:
    RsPeerId peerId;

	/** Qt Designer generated object */
	Ui::OnlineToaster ui;
};

#endif	//MESSAGETOASTER_H
