/*
 * RetroShare
 * Copyright (C) 2012 RetroShare Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
	FriendRequestToaster(const std::string &gpgId, const QString &sslName, const std::string &peerId);

private slots:
	void friendrequestButtonSlot();

private:
	std::string mGpgId;
	std::string mSslId;
	QString mSslName;

	/** Qt Designer generated object */
	Ui::FriendRequestToaster ui;
};

#endif	//FRIENDREQUESTTOASTER_H
