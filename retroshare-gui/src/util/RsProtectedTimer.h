/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013 Cyril Soler
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

#pragma once

#include <QTimer>
#include <retroshare-gui/RsAutoUpdatePage.h>
#include <iostream>

#define PROTECTED_TIMER_DEBUG

class RsProtectedTimer: public QTimer
{
	public:
		RsProtectedTimer(QObject *parent)
			: QTimer(parent)
		{
		}

		virtual void timerEvent(QTimerEvent *e)
		{
			if(RsAutoUpdatePage::eventsLocked())
			{
#ifdef PROTECTED_TIMER_DEBUG
				std::cerr << "Timer is blocked!." << std::endl;
#endif
				return ;
			}

#ifdef PROTECTED_TIMER_DEBUG
			std::cerr << "Timer has passed protection." << std::endl;
#endif
			QTimer::timerEvent(e) ;
		}
};

