/*******************************************************************************
 * util/EventFilter.h                                                          *
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

#ifndef EVENTFILTER_H
#define EVENTFILTER_H

#include <util/rsqtutildll.h>

#include <util/NonCopyable.h>

#include <QObject>

class QEvent;

/**
 * EventFilter for QObject.
 *
 * Permits to make some special actions on Qt events.
 * Example:
 * <code>
 * QMainWindow * widget = new QMainWindow();
 * CloseEventFilter * closeFilter = new CloseEventFilter(this, SLOT(printHelloWorld()));
 * ResizeEventFilter * resizeFilter = new ResizeEventFilter(this, SLOT(printHelloWorld()));
 * widget->installEventFilter(closeFilter);
 * widget->installEventFilter(resizeFilter);
 * </code>
 *
 * 
 */
class RSQTUTIL_API EventFilter : public QObject, NonCopyable {
	Q_OBJECT
public:

	/**
	 * Filters an event.
	 *
	 * @param receiver object receiver of the filter signal
	 * @param member member of the object called by the filter signal
	 * @param watched watched object the filter is going to be applied on
	 */
	EventFilter(QObject * receiver, const char * member);

protected:

	/**
	 * Emits the filter signal.
	 *
	 * @param event event filtered
	 */
	void filter(QEvent * event);

	/**
	 * Filters the event.
	 *
	 * @param watched watched object
	 * @param event event filtered of the watched object
	 * @return true then stops the event being handled further
	 */
	virtual bool eventFilter(QObject * watched, QEvent * event);

Q_SIGNALS:

	void activate(QEvent * event);
};

#endif	//EVENTFILTER_H
