/*******************************************************************************
 * util/EventFilter.cpp                                                        *
 *                                                                             *
 * Copyright (C) 2006, 2007 crypton  <retroshare.project@gmail.com>            *
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

#include <util/EventFilter.h>

#include <QEvent>

EventFilter::EventFilter(QObject * receiver, const char * member)
	: QObject() {
	connect(this, SIGNAL(activate(QEvent *)), receiver, member);
}

void EventFilter::filter(QEvent * event) {
	activate(event);
}

bool EventFilter::eventFilter(QObject * watched, QEvent * event) {
	return QObject::eventFilter(watched, event);
}
