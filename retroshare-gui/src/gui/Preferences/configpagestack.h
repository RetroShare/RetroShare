/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#ifndef _CONFIGPAGESTACK_H
#define _CONFIGPAGESTACK_H

#include <QStackedWidget>
#include <QHash>

#include "configpage.h"


class ConfigPageStack : public QStackedWidget
{
  Q_OBJECT

public:
  /** Constructor. */
  ConfigPageStack(QWidget *parent = 0);

  /** Adds a configuration page to the stack. */
  void add(ConfigPage *page, QAction *action);
  /** Sets the current config page and checks its action. */
  void setCurrentPage(ConfigPage *page);
  /** Sets the current config page index and checks its action. */
  void setCurrentIndex(int index);

  /** Returns a list of all pages in the stack. */
  QList<ConfigPage*> pages() { return _pages.values(); }
  
public slots:
  /** Displays the page associated with the activated action. */
  void showPage(QAction *pageAction);
  
private:
  /** Maps an action to a config page. */
  QHash<QAction*, ConfigPage*> _pages;
};

#endif

