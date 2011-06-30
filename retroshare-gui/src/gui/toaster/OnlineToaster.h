/*
 * RetroShare
 * Copyright (C) 2006 crypton
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

#ifndef ONLINETOASTER_H
#define ONLINETOASTER_H

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
	OnlineToaster(const std::string &peerId, const QString &name, const QPixmap &avatar);

private slots:
	void chatButtonSlot();

private:
	void play();

	std::string peerId;

	/** Qt Designer generated object */
	Ui::OnlineToaster ui;
};

#endif	//MESSAGETOASTER_H
