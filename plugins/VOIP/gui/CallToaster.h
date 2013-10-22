/*
 * RetroShare
 * Copyright (C) 2013 RetroShare Team
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

#ifndef CALLTOASTER_H
#define CALLTOASTER_H

#include "ui_CallToaster.h"

/**
 * Shows a toaster when friend is Calling you .
 *
 *
 */
class CAllToaster : public QWidget
{
	Q_OBJECT

public:
	CallToaster(const std::string &peerId);

private slots:
	void chatButtonSlot();

private:
	std::string peerId;

	/** Qt Designer generated object */
	Ui::CallToaster ui;
};

#endif	//MESSAGETOASTER_H
