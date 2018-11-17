/*******************************************************************************
 * gui/common/rshtml.h                                                         *
 *                                                                             *
 * Copyright (c) 2008, defnax                                                  *
 * Copyright (c) 2008, Matt Edman, Justin Hipple                               *
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

/*
** \file rshtml.h
** \version $Id: rshtml.h 2362 2008-02-29 04:30:11Z edmanm $
** \brief HTML formatting functions
*/

#ifndef _HTML_H
#define _HTML_H

#include <QString>


/** Wraps a string in "<p>" tags, converts "\n" to "<br>" and converts "\n\n"
 * to a new paragraph. */
QString p(QString str);

/** Wraps a string in "<i>" tags. */
QString i(QString str);

/** Wraps a string in "<b>" tags. */
QString b(QString str);

/** Wraps a string in "<tr>" tags. */
QString trow(QString str);

/** Wraps a string in "<td>" tags. */
QString tcol(QString str);

/** Wraps a string in "<th>" tags. */
QString thead(QString str);

/** Escapes "<" and ">" characters in a string, for html. */
QString escape(QString str);

#endif

