/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#include <QtGui>

#include "ChanGroupItem.h"
#include "SubFileItem.h"

#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
ChanGroupItem::ChanGroupItem(std::string groupName)
:QWidget(NULL), mGroupName(groupName)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );

  small();
  updateItemStatic();
  updateItem();
}


void ChanGroupItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ChanGroupItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	titleLabel->setText(QString::fromStdString(mGroupName));
}


void ChanGroupItem::updateItem()
{
}


void ChanGroupItem::small()
{
}

void ChanGroupItem::toggle()
{
}

