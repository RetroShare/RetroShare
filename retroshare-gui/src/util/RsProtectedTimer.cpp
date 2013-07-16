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

#include <retroshare-gui/RsAutoUpdatePage.h>
#include <iostream>

#include "RsProtectedTimer.h"

#define TIMER_FAST_POLL 499 // unique time in RetroShare

//#define PROTECTED_TIMER_DEBUG

RsProtectedTimer::RsProtectedTimer(QObject *parent)
	: QTimer(parent)
{
	mInterval = 0;
}

void RsProtectedTimer::timerEvent(QTimerEvent *e)
{
	if(RsAutoUpdatePage::eventsLocked())
	{
		if (!mInterval && interval() > TIMER_FAST_POLL) {
			/* Save interval */
			mInterval = interval();
			/* Set fast interval */
			setInterval(TIMER_FAST_POLL); // restart timer
		}

#ifdef PROTECTED_TIMER_DEBUG
		if (isSingleShot()) {
			/* Singleshot timer will be stopped in QTimer::timerEvent */
			std::cerr << "Singleshot timer is blocked!" << std::endl;
		} else {
			std::cerr << "Timer is blocked!" << std::endl;
		}
#endif

		return ;
	}

#ifdef PROTECTED_TIMER_DEBUG
	if (isSingleShot()) {
		std::cerr << "Singleshot timer has passed protection." << std::endl;
	} else {
		std::cerr << "Timer has passed protection." << std::endl;
	}
#endif

	QTimer::timerEvent(e) ;

	if (mInterval) {
		if (interval() == TIMER_FAST_POLL) {
			/* Still fast poll */
			if (!isSingleShot()) {
				/* Restore interval */
				setInterval(mInterval); // restart timer
			}
		}
		mInterval = 0;
	}
}
