/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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

#ifndef _MAINPAGESTACK_H
#define _MAINPAGESTACK_H

#include <QStackedWidget>
#include <QHash>

#include "retroshare-gui/mainpage.h"


class MainPageStack : public QStackedWidget
{
  Q_OBJECT

public:
  /** Constructor. */
  MainPageStack(QWidget *parent = 0);

  /** Adds a Mainuration page to the stack. */
  void add(MainPage *page, QAction *action);
  /** Sets the current Main page and checks its action. */
  void setCurrentPage(MainPage *page);
  /** Sets the current Main page index and checks its action. */
  void setCurrentIndex(int index);

  /** Returns a list of all pages in the stack. */
  QList<MainPage*> pages() { return _pages.values(); }
  
public slots:
  /** Displays the page associated with the activated action. */
  void showPage(QAction *pageAction);
  /** Adjusts the size of the Main page and the Main window. */
  void onCurrentChanged(int index);
  
private:
  /** Maps an action to a Main page. */
  QHash<QAction*, MainPage*> _pages;
};

#endif

