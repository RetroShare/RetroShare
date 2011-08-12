/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007 DrBob
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

#include "util/RsAction.h"

RsAction::RsAction(QWidget * parent, std::string rsid)
	: QAction(parent), RsId(rsid) 
{
	connect(this, SIGNAL( triggered( bool ) ), this, SLOT( triggerEvent( bool ) ) );
}


RsAction::RsAction(const QString & text, QObject * parent, std::string rsid)
	: QAction(text, parent), RsId(rsid) 
{
	connect(this, SIGNAL( triggered( bool ) ), this, SLOT( triggerEvent( bool ) ) );
}


RsAction::RsAction(const QIcon & icon, const QString & text, QObject * parent , std::string rsid)
	: QAction(icon, text, parent), RsId(rsid) 
{
	connect(this, SIGNAL( triggered( bool ) ), this, SLOT( triggerEvent( bool ) ) );
}


void	RsAction::triggerEvent( bool /*checked*/ )
{
	triggeredId(RsId);
}

//void triggeredId( std::string rsid );
