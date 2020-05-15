/*******************************************************************************
 * gui/TheWire/PulseMessage.h                                                  *
 *                                                                             *
 * Copyright (c) 2020-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#ifndef MRK_PULSE_MSG_H
#define MRK_PULSE_MSG_H

#include "ui_PulseMessage.h"

#include <retroshare/rswire.h>

class PulseMessage : public QWidget, private Ui::PulseMessage
{
  Q_OBJECT

public:
	PulseMessage(QWidget *parent); 

	void setup(RsWirePulseSPtr pulse);
	void setMessage(QString msg);
};

#endif
