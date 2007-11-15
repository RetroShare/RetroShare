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

#include <QAction>
#include "mainpagestack.h"

/** Default constructor. */
MainPageStack::MainPageStack(QWidget *parent)
: QStackedWidget(parent)
{
}

/** Adds a page to the stack. */
void
MainPageStack::add(MainPage *page, QAction *action)
{
  _pages.insert(action, page);
  insertWidget(count(), page);
}

/** Sets the current Main page and checks its action. */
void
MainPageStack::setCurrentPage(MainPage *page)
{
  foreach (QAction *action, _pages.keys(page)) {
    action->setChecked(true);
  }
  setCurrentWidget(page);
}

/** Sets the current Main page index and checks its action. */
void
MainPageStack::setCurrentIndex(int index)
{
  setCurrentPage((MainPage *)widget(index));
}

/** Shows the Main page associated with the activated action. */
void
MainPageStack::showPage(QAction *pageAction)
{
  setCurrentWidget(_pages.value(pageAction));
}

