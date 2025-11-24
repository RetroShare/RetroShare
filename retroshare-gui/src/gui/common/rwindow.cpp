/*******************************************************************************
 * gui/common/rwindow.cpp                                                      *
 *                                                                             *
 * Copyright (c) 2006-2007, crypton                                            *
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

#include <QPoint>
#include <QSize>
#include <QShortcut>
#include <QByteArray>
#include <QKeySequence>
#include <rshare.h>
#include "rwindow.h"


/** Default constructor. */
RWindow::RWindow(QString name, QWidget *parent, Qt::WindowFlags flags)
 : QMainWindow(parent, flags)
{
  _name     = name;
  m_bSaveStateOnClose = false;
}

/** Destructor. */
RWindow::~RWindow()
{
  saveWindowState();
}

/** Associates a shortcut key sequence with a slot. */
void
RWindow::setShortcut(QString shortcut, const char *slot)
{
  rApp->createShortcut(QKeySequence(shortcut), this, this, slot);
}

/** Saves the size and location of the window. */
void
RWindow::saveWindowState()
{
  if (m_bSaveStateOnClose == false) {
    // nothing to save
    return;
  }

#if QT_VERSION >= 0x040200
  saveSetting("Geometry", saveGeometry());
#else
  saveSetting("Size", size());
  saveSetting("Position", pos());
#endif
}

/** Restores the last size and location of the window. */
void
RWindow::restoreWindowState()
{
    m_bSaveStateOnClose = true; // now we save the window state on close

#if QT_VERSION >= 0x040200
  QByteArray geo = getSetting("Geometry", QByteArray()).toByteArray();
  if (geo.isEmpty())
  {
    adjustSize();
    QRect rect = geometry();
    int h = fontMetrics().height()*40;
    if (rect.height()<h)
    {
      rect.setHeight(h);
      setGeometry(rect);
    }
  }
  else
    restoreGeometry(geo);
#else
  QRect screen = QDesktopWidget().availableGeometry();

  /* Restore the window size. */
  QSize size = getSetting("Size", QSize()).toSize();
  if (!size.isEmpty()) {
    size = size.boundedTo(screen.size());
    resize(size);
  }

  /* Restore the window position. */
  QPoint pos = getSetting("Position", QPoint()).toPoint();
  if (!pos.isNull() && screen.contains(pos)) {
    move(pos);
  }
#endif
}

/** Gets the saved value of a property associated with this window object.
 * If no value was saved, the default value is returned. */
QVariant
RWindow::getSetting(QString setting, QVariant defaultValue)
{
  RSettings settings(_name);
  return settings.value(setting, defaultValue);
}

/** Saves a value associated with a property name for this window object. */
void
RWindow::saveSetting(QString prop, QVariant value)
{
  RSettings settings(_name);
  settings.setValue(prop, value);
}

/** Overloaded QWidget::setVisible(). If this window is already visible and
 * <b>visible</b> is true, this window will be brought to the top and given 
 * focus. If <b>visible</b> is false, then the window state will be saved and
 * this window will be hidden. */
void
RWindow::setVisible(bool visible)
{
  if (visible) {
    /* Bring the window to the top, if it's already open. Otherwise, make the
     * window visible. */
    if (isVisible()) {
      activateWindow();
      setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
      raise();
    } else {
      restoreWindowState();
    }
  } else {
    /* Save the last size and position of this window. */
    saveWindowState();
  }
  QMainWindow::setVisible(visible);
}

