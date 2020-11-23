/*******************************************************************************
 * idle/idle.h                                                                 *
 *                                                                             *
 * Copyright (C) 2003  Justin Karneges <retroshare.project@gmail.com>          *
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

#include "idle.h"

#include <QCursor>
#include <QDateTime>
#include <QTimer>

static IdlePlatform *platform = 0;
static int platform_ref = 0;

Idle::Idle()
{
	d = new Private;
	d->active = false;
	d->idleTime = 0;

	// try to use platform idle
	if(!platform) {
		IdlePlatform *p = new IdlePlatform;
		if(p->init())
			platform = p;
		else
			delete p;
	}
	if(platform)
		++platform_ref;

	connect(&d->checkTimer, SIGNAL(timeout()), SLOT(doCheck()));
}

Idle::~Idle()
{
	if(platform) {
		--platform_ref;
		if(platform_ref == 0) {
			delete platform;
			platform = 0;
		}
	}
	delete d;
}

bool Idle::isActive() const
{
	return d->active;
}

bool Idle::usingPlatform() const
{
	return (platform ? true: false);
}

void Idle::start()
{
	d->startTime = QDateTime::currentDateTime();

	if(!platform) {
		// generic idle
		d->lastMousePos = QCursor::pos();
		d->idleSince = QDateTime::currentDateTime();
	}

	// poll every second (use a lower value if you need more accuracy)
	d->checkTimer.start(1000);
}

void Idle::stop()
{
	d->checkTimer.stop();
}

int Idle::secondsIdle()
{
	int i;
	if (platform)
		i = platform->secondsIdle();
	else {
		QPoint curMousePos = QCursor::pos();
		QDateTime curDateTime = QDateTime::currentDateTime();
		if(d->lastMousePos != curMousePos) {
			d->lastMousePos = curMousePos;
			d->idleSince = curDateTime;
		}
		i = d->idleSince.secsTo(curDateTime);
	}

	// set 'beginIdle' to the beginning of the idle time (by backtracking 'i' seconds from now)
	QDateTime beginIdle = QDateTime::currentDateTime().addSecs(-i);

	// set 't' to hold the number of seconds between 'beginIdle' and 'startTime'
	int t = beginIdle.secsTo(d->startTime);

	// beginIdle later than (or equal to) startTime?
	if(t <= 0) {
		// scoot ourselves up to the new idle start
		d->startTime = beginIdle;
	}
	// beginIdle earlier than startTime?
	//else if(t > 0) {  //t is always > 0
	//	// do nothing
	//}
	
	// how long have we been idle?
	int idleTime = d->startTime.secsTo(QDateTime::currentDateTime());
	return idleTime;
}

void Idle::doCheck()
{
	emit secondsIdle(secondsIdle());
}
