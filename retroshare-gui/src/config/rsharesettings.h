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


//Forward declaration.
class QWidget;
class QTableWidget;
class QToolBar;
class QMainWindow;

/** Handles saving and restoring RShares's settings, such as the
 * location of Tor, the control port, etc.
 *
 * NOTE: Qt 4.1 documentation states that constructing a QSettings object is
 * "very fast", so we shouldn't need to create a global instance of this
 * class.
 */
class RshareSettings : public QSettings
{
  
public:
  /** Default constructor. */
  RshareSettings();

  /** Resets all of Rshare's settings. */
  static void reset();
  
  /** Sets the default value of <b>key</b> to be <b>val</b>. */
  void setDefault(QString key, QVariant val);
  /** Returns the default value for <b>key</b>. */
  QVariant defaultValue(QString key);
  /** Save <b>val</b> to the configuration file for the setting <b>key</b>, if
   * <b>val</b> is different than <b>key</b>'s current value. */
  void setValue(QString key, QVariant val);
  /** Returns the value for <b>key</b>. If no value is currently saved, then
   * the default value for <b>key</b> will be returned. */
  QVariant value(QString key);
  /** Returns the value for <b>key</b>. If no value is currently saved, then
   * <b>defaultValue</b> will be returned. */
  QVariant value(QString key, QVariant defaultValue);

  /** Gets the currently preferred language code for RShare. */
  QString getLanguageCode();
  /** Saves the preferred language code. */
  void setLanguageCode(QString languageCode);
 
  /** Gets the interface style key (e.g., "windows", "motif", etc.) */
  QString getInterfaceStyle();
  /** Sets the interface style key. */
  void setInterfaceStyle(QString styleKey);
  
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
  

        //! Save placement, state and size information of a window.
        void saveWidgetInformation(QWidget *widget);
        
        //! Load placement, state and size information of a window.
        void loadWidgetInformation(QWidget *widget);
        
        //! Method overload. Save window and toolbar information.
        void saveWidgetInformation(QMainWindow *widget, QToolBar *toolBar);

        //! Method overload. Restore window and toolbar information.
        void loadWidgetInformation(QMainWindow *widget, QToolBar *toolBar);


private:
  QHash<QString,QVariant> _defaults;



};

#endif

