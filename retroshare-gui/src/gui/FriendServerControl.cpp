/*******************************************************************************
 * gui/FriendServer.cpp                                                        *
 *                                                                             *
 * Copyright (c) 2021 Retroshare Team  <retroshare.project@gmail.com>          *
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

#include "FriendServerControl.h"
//#include <retroshare/rsfriendserver.h>

#include <iostream>

/** Constructor */
FriendServerControl::FriendServerControl(QWidget *parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

//  /* Hide Settings frame */
//  connect( ui.maxFriendLevelSB, SIGNAL(valueChanged(int)), this, SLOT(setMaxFriendLevel(int)));
//  connect( ui.edgeLengthSB, SIGNAL(valueChanged(int)), this, SLOT(setEdgeLength(int)));
//  connect( ui.freezeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setFreezeState(bool)));
//  connect( ui.nameBox, SIGNAL(textChanged(QString)), this, SLOT(setNameSearch(QString)));
}

FriendServerControl::~FriendServerControl()
{
}
