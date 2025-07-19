/*******************************************************************************
 * util/DateTime.h                                                             *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team  <retroshare.project@gmail.com>          *
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

#ifndef _DATETIME_H
#define _DATETIME_H

#include <QString>

class QDateTime;
class QDate;
class QTime;

class DateTime
{
public:
	/* format long date and time e.g. "September 30, 2012" */
	static QString formatLongDate(time_t dateValue);
	static QString formatLongDate(const QDate &dateValue);

	/* format long date and time e.g. "September 30, 2012 01:05 PM" */
	static QString formatLongDateTime(time_t datetimeValue);
	static QString formatLongDateTime(const QDateTime &datetimeValue);

	/* format date e.g. "9/30/12", "30.09.12" */
	static QString formatDate(time_t dateValue);
	static QString formatDate(const QDate &dateValue);

	/* format time e.g. "13:05:12" */
	static QString formatTime(time_t timeValue);
	static QString formatTime(const QTime &timeValue);

	/* format date and time (see formatDate & formatTime) */
	static QString formatDateTime(time_t datetimeValue);
	static QString formatDateTime(const QDateTime &datetimeValue);

	/* Convert time_t to QDateTime */
	static QDateTime DateTimeFromTime_t(time_t timeValue);
};

#endif
