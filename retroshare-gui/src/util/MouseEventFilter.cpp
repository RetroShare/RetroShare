/*******************************************************************************
 * util/MouseEventFilter.cpp                                                   *
 *                                                                             *
 * Copyright (c) 2006, Crypton          <retroshare.project@gmail.com>         *
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

#include <util/MouseEventFilter.h>

#include <QEvent>
#include <QMouseEvent>

#include <exception>
#include <typeinfo>

MouseMoveEventFilter::MouseMoveEventFilter(QObject * receiver, const char * member)
	: EventFilter(receiver, member) {
}

bool MouseMoveEventFilter::eventFilter(QObject * watched, QEvent * event) {
	if (event->type() == QEvent::MouseMove) {
		filter(event);
		return false;
	}
	return EventFilter::eventFilter(watched, event);
}

MousePressEventFilter::MousePressEventFilter(QObject * receiver, const char * member, Qt::MouseButton button)
	: EventFilter(receiver, member),
	_button(button) {
}

bool MousePressEventFilter::eventFilter(QObject * watched, QEvent * event) {
	if (event->type() == QEvent::MouseButtonPress) {
		try {
			QMouseEvent * mouseEvent = dynamic_cast<QMouseEvent *>(event);

			if ((_button == Qt::NoButton) || (mouseEvent->button() == _button)) {
				filter(event);
				return false;
			}
		} catch (const std::bad_cast&) {
			//LOG_FATAL("exception when casting a QEvent to a QMouseEvent");
		}
	}
	return EventFilter::eventFilter(watched, event);
}

MouseReleaseEventFilter::MouseReleaseEventFilter(QObject * receiver, const char * member, Qt::MouseButton button)
	: EventFilter(receiver, member),
	_button(button) {
}

bool MouseReleaseEventFilter::eventFilter(QObject * watched, QEvent * event) {
	if (event->type() == QEvent::MouseButtonRelease) {
		try {
			QMouseEvent * mouseEvent = dynamic_cast<QMouseEvent *>(event);

			if ((_button == Qt::NoButton) || (mouseEvent->button() == _button)) {
				filter(event);
				return false;
			}
		} catch (const std::bad_cast&) {
			//LOG_FATAL("exception when casting a QEvent to a QMouseEvent");
		}
	}
	return EventFilter::eventFilter(watched, event);
}

MouseHoverEnterEventFilter::MouseHoverEnterEventFilter(QObject * receiver, const char * member)
	: EventFilter(receiver, member) {
}

bool MouseHoverEnterEventFilter::eventFilter(QObject * watched, QEvent * event) {
	if (event->type() == QEvent::HoverEnter) {
		filter(event);
		return false;
	}
	return EventFilter::eventFilter(watched, event);
}

MouseHoverLeaveEventFilter::MouseHoverLeaveEventFilter(QObject * receiver, const char * member)
	: EventFilter(receiver, member) {
}

bool MouseHoverLeaveEventFilter::eventFilter(QObject * watched, QEvent * event) {
	if (event->type() == QEvent::HoverLeave) {
		filter(event);
		return false;
	}
	return EventFilter::eventFilter(watched, event);
}
