/*******************************************************************************
 * gui/toaster/FriendRequestToaster.h                                          *
 *                                                                             *
 * Copyright (c) 2010 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef FRIENREQUESTTOASTER_H
#define FRIENREQUESTTOASTER_H

#include "ui_FriendRequestToaster.h"

/**
 * Shows a toaster when you get from a Known Peer a Incoming Connect Attempt .
 *
 *
 */
class FriendRequestToaster : public QWidget
{
	Q_OBJECT

public:
	FriendRequestToaster(const RsPgpId &gpgId, const QString &sslName, const RsPeerId &peerId);

private slots:
	void friendrequestButtonSlot();

private:
	RsPgpId mGpgId;
	RsPeerId mSslId;
	QString mSslName;

	/** Qt Designer generated object */
	Ui::FriendRequestToaster ui;
};

#endif	//FRIENDREQUESTTOASTER_H
