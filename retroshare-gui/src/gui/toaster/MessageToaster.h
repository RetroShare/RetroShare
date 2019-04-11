/*******************************************************************************
 * gui/toaster/MessageToaster.h                                                *
 *                                                                             *
 * Copyright (C) 2007 - 2008 Xesc & Technology                                 *
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

#ifndef MESSAGETOASTER_H
#define MESSAGETOASTER_H

#include "ui_MessageToaster.h"

class MessageToaster : public QWidget
{
	Q_OBJECT

public:
	MessageToaster(const std::string &peerId, const QString &title, const QString &message);

private slots:
	void openmessageClicked();

private:
	/** Qt Designer generated object */
	Ui::MessageToaster ui;
};

#endif
