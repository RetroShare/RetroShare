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



#ifndef _RSHARESETTINGS_H
#define _RSHARESETTINGS_H

#include <QHash>
#include <QSettings>

#include <gui/linetypes.h>
#include "rsettings.h"


//Forward declaration.
class QWidget;
class QTableWidget;
class QToolBar;
class QMainWindow;

/** Handles saving and restoring RShares's settings
 *
 * NOTE: Qt 4.1 documentation states that constructing a QSettings object is
 * "very fast", so we shouldn't need to create a global instance of this
 * class.
 */
class RshareSettings : public RSettings
{
  
public:
  /* create settings object */
  static void Create ();

  /** Gets the currently preferred language code for RShare. */
  QString getLanguageCode();
  /** Saves the preferred language code. */
  void setLanguageCode(QString languageCode);
 
  /** Gets the interface style key (e.g., "windows", "motif", etc.) */
  QString getInterfaceStyle();
  /** Sets the interface style key. */
  void setInterfaceStyle(QString styleKey);
  
  /** Sets the stylesheet */
  void setSheetName(QString sheet);
  /** Gets the stylesheet */
  QString getSheetName();
  
  /** Returns true if RetroShare's main window should be visible when the
  * application starts. */
  bool showMainWindowAtStart();
  /** Sets whether to show main window when the application starts. */
  void setShowMainWindowAtStart(bool show);
  
  /** Returns true if RetroShare should start on system boot. */
  bool runRetroshareOnBoot();

  /** Set whether to run RetroShare on system boot. */
  void setRunRetroshareOnBoot(bool run);

  /** Returns the chat avatar. Returns a null image if no avatar is saved. */
  QImage getChatAvatar() const ;

  /** set the chat avatar. Returns a null image if no avatar is saved. */
  void setChatAvatar(const QImage&) ;

  
  /* Get the destination log file. */
  QString getLogFile();
  /** Set the destination log file. */
  void setLogFile(QString file);

  /* Get the bandwidth graph line filter. */
  uint getBWGraphFilter();
  /** Set the bandwidth graph line filter. */
  void setBWGraphFilter(uint line, bool status);

  /** Set the bandwidth graph opacity setting. */
  int getBWGraphOpacity();
  /** Set the bandwidth graph opacity settings. */
  void setBWGraphOpacity(int value);

  /** Gets whether the bandwidth graph is always on top. */
  bool getBWGraphAlwaysOnTop();
  /** Sets whether the bandwidth graph is always on top. */
  void setBWGraphAlwaysOnTop(bool alwaysOnTop);
  
  uint getNewsFeedFlags();
  void setNewsFeedFlags(uint flags);

  uint getChatFlags();
  void setChatFlags(uint flags);

  uint getNotifyFlags();
  void setNotifyFlags(uint flags);

  //! Save placement, state and size information of a window.
  void saveWidgetInformation(QWidget *widget);

  //! Load placement, state and size information of a window.
  void loadWidgetInformation(QWidget *widget);

  //! Method overload. Save window and toolbar information.
  void saveWidgetInformation(QMainWindow *widget, QToolBar *toolBar);

  //! Method overload. Restore window and toolbar information.
  void loadWidgetInformation(QMainWindow *widget, QToolBar *toolBar);

  /* Messages */
  bool getMsgSetToReadOnActivate ();
  void setMsgSetToReadOnActivate (bool bValue);

protected:
  /** Default constructor. */
  RshareSettings();

  void initSettings();
};

// the one and only global settings object
extern RshareSettings *Settings;

#endif

