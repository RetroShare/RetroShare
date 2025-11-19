/*******************************************************************************
 * util/RsAction.cpp                                                           *
 *                                                                             *
 * Copyright (c) 2007 DrBob          <retroshare.project@gmail.com>            *
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

#include "util/RsAction.h"

RsAction::RsAction(QObject * parent, std::string rsid)
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
