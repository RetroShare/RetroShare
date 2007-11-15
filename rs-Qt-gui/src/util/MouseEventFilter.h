/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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

#ifndef MOUSEEVENTFILTER_H
#define MOUSEEVENTFILTER_H

#include <util/EventFilter.h>

/**
 * Catch MouseMove event.
 *
 * 
 */
class RSQTUTIL_API MouseMoveEventFilter : public EventFilter {
public:

	MouseMoveEventFilter(QObject * receiver, const char * member);

private:

	virtual bool eventFilter(QObject * watched, QEvent * event);
};


/**
 * Catch MouseButtonPress event.
 *
 * 
 */
class RSQTUTIL_API MousePressEventFilter : public EventFilter {
public:

	MousePressEventFilter(QObject * receiver, const char * member, Qt::MouseButton button = Qt::NoButton);

private:

	virtual bool eventFilter(QObject * watched, QEvent * event);

	Qt::MouseButton _button;
};


/**
 * Catch MouseButtonRelease event.
 *
 * 
 */
class RSQTUTIL_API MouseReleaseEventFilter : public EventFilter {
public:

	MouseReleaseEventFilter(QObject * receiver, const char * member, Qt::MouseButton button = Qt::NoButton);

private:

	virtual bool eventFilter(QObject * watched, QEvent * event);

	Qt::MouseButton _button;
};


/**
 * Catch HoverEnter event.
 *
 * 
 */
class RSQTUTIL_API MouseHoverEnterEventFilter : public EventFilter {
public:

	MouseHoverEnterEventFilter(QObject * receiver, const char * member);

private:

	virtual bool eventFilter(QObject * watched, QEvent * event);
};


/**
 * Catch HoverLeave event.
 *
 * 
 */
class RSQTUTIL_API MouseHoverLeaveEventFilter : public EventFilter {
public:

	MouseHoverLeaveEventFilter(QObject * receiver, const char * member);

private:

	virtual bool eventFilter(QObject * watched, QEvent * event);
};

#endif	//MOUSEEVENTFILTER_H
