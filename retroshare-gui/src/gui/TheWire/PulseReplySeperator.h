/*******************************************************************************
 * gui/TheWire/PulseReplySeperator.h                                           *
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

#ifndef MRK_PULSE_REPLY_SEPERATOR_H
#define MRK_PULSE_REPLY_SEPERATOR_H

#include "ui_PulseReplySeperator.h"
#include "PulseViewItem.h"

class PulseReplySeperator : public PulseViewItem, private Ui::PulseReplySeperator
{
  Q_OBJECT

public:
	PulseReplySeperator();
};

#endif
