/*******************************************************************************
 * gui/mainpagestack.cpp                                                       *
 *                                                                             *
 * Copyright (c) 2006-2007, crypton    <retroshare.project@gmail.com>          *
 * Copyright (c) 2006, Matt Edman, Justin Hipple                               *
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

/* ccr . 2015 Aug 01 . Resize Main page and Main window.
 *
 * On very small legacy CRTs of about 15" (38 cm) RetroShare pages are
 * initially too deep (too tall) to fit the screen at 1024x768
 * resolution.  Some can be shrunk down manually from their initial
 * size.  Others cannot.  This patch tries to allow each page to be
 * shrunk somewhat.  Then code that runs elsewhere to fit the logical
 * Main window into the physical screen will have a better chance of
 * success.  Notably, on Linux -- Gnome3 -- X11 systems, only when the
 * Main window first fits entirely into the physical screen, can it
 * then be maximized.
 *
 * This code is borrowed from a Stack Overflow post:
 *
 * o Darkgaze. "Resize QStackedWidget to the Page Which Is Opened." 23
 * Jan. 2013. Online posting. Stack Overflow. 1 Aug. 2015
 * <https://stackoverflow.com/questions/14480696/resize-qstackedwidget-to-the-page-which-is-opened>.
 *
 */

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
  page->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);  /* 2015 Aug 01 */
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

/** Adjusts the size of the Main page and the Main window. */
void
MainPageStack::onCurrentChanged(int index)  /* 2015 Aug 01 */
{
   QWidget* pWidget = widget(index);
   Q_ASSERT(pWidget);
   pWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   pWidget->adjustSize();
   adjustSize();
}

