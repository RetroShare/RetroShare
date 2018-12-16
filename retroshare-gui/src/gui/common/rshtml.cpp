/*******************************************************************************
 * gui/common/rshtml.cpp                                                       *
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
** \file rshtml.cpp
** \version $Id: rshtml.cpp 2362 2008-02-29 04:30:11Z edmanm $
** \brief HTML formatting functions
*/

#include "rshtml.h"


/** Wraps a string in "<p>" tags, converts "\n" to "<br>" and converts "\n\n"
 * to a new paragraph. */
QString
p(QString str)
{
  str = "<p>" + str + "</p>";
  str.replace("\n\n", "</p><p>");
  str.replace("\n", "<br>");
  return str;
}

/** Wraps a string in "<i>" tags. */
QString
i(QString str)
{
  return QString("<i>%1</i>").arg(str);
}

/** Wraps a string in "<b>" tags. */
QString
b(QString str)
{
  return QString("<b>%1</b>").arg(str);
}

/** Wraps a string in "<tr>" tags. */
QString
trow(QString str)
{
  return QString("<tr>%1</tr>").arg(str);
}

/** Wraps a string in "<td>" tags. */
QString
tcol(QString str)
{
  return QString("<td>%1</td>").arg(str);
}

/** Wraps a string in "<th>" tags. */
QString
thead(QString str)
{
  return QString("<th>%1</th>").arg(str);
}

/** Escapes "<" and ">" characters in the given string. */
QString
escape(QString str)
{
  str.replace("<", "&lt;");
  str.replace(">", "&gt;");
  return str;
}

