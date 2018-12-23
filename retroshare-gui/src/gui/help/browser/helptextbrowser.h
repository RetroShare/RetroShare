/*******************************************************************************
 * retroshare-gui/src/gui/help/browser/helptextbrowser.h                       *
 *                                                                             *
 * Copyright (c) 2008, defnax          <retroshare.project@gmail.com>          *
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
** \file helptextbrowser.h
** \version $Id: helptextbrowser.h 2679 2008-06-10 03:07:10Z edmanm $ 
** \brief Displays an HTML-based help document
*/

#ifndef _HELPTEXTBROWSER_H
#define _HELPTEXTBROWSER_H

#include <QTextBrowser>
#include <QVariant>


class HelpTextBrowser : public QTextBrowser
{
  Q_OBJECT

public:
  /** Default constructor. */
  HelpTextBrowser(QWidget *parent = 0);
  /** Loads a resource into the browser. */
  QVariant loadResource(int type, const QUrl &name);

public slots:
  /** Called when the displayed document is changed. If <b>url</b> specifies
   * an external link, then the user will be prompted for whether they want to
   * open the link in their default browser or not. */
  virtual void setSource(const QUrl &url);
};

#endif

