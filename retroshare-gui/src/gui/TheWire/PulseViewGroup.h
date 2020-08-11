/*******************************************************************************
 * gui/TheWire/PulseViewGroup.h                                                *
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

#ifndef MRK_PULSE_VIEW_GROUP_H
#define MRK_PULSE_VIEW_GROUP_H

#include "ui_PulseViewGroup.h"

#include "PulseViewItem.h"
#include <retroshare/rswire.h>

class PulseViewGroup : public PulseViewItem, private Ui::PulseViewGroup
{
  Q_OBJECT

public:
	PulseViewGroup(PulseViewHolder *holder, RsWireGroupSPtr group);

private slots:
	void actionFollow();

protected:
	void setup();


protected:
	RsWireGroupSPtr mGroup;
};

#endif
