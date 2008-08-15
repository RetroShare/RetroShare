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



#include <QDir>
#include <QCoreApplication>
#include <QStyleFactory>
#include <lang/languagesupport.h>
#include <rshare.h>

#include "rsharesettings.h"

#include <QWidget>
#include <QMainWindow>

#if defined(Q_WS_WIN)
#include <util/registry.h>
#include <util/win32.h>
#endif


/* Retroshare's Settings */
#define SETTING_LANGUAGE            "LanguageCode"
#define SETTING_STYLE               "InterfaceStyle"
#define SETTING_SHEETNAME           "SheetName"

#define SETTING_DATA_DIRECTORY      "DataDirectory"
#define SETTING_SHOW_MAINWINDOW_AT_START  "ShowMainWindowAtStart"
#define SETTING_BWGRAPH_FILTER        "StatisticDialog/BWLineFilter"
#define SETTING_BWGRAPH_OPACITY       "StatisticDialog/Opacity"
#define SETTING_BWGRAPH_ALWAYS_ON_TOP "StatisticDialog/AlwaysOnTop"

/* Default Retroshare Settings */
#define DEFAULT_OPACITY             100


#if defined(Q_OS_WIN32)
#define STARTUP_REG_KEY        "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RETROSHARE_REG_KEY         "RetroShare" 
#endif

/* Default bandwidth graph settings */
#define DEFAULT_BWGRAPH_FILTER          (BWGRAPH_SEND|BWGRAPH_REC)
#define DEFAULT_BWGRAPH_ALWAYS_ON_TOP   false



/** Default Constructor */
RshareSettings::RshareSettings()
{ 
#if defined(Q_WS_MAC)
  setDefault(SETTING_STYLE, "macintosh (aqua)");
#else
  static QStringList styles = QStyleFactory::keys();
#if defined(Q_WS_WIN)
  if (styles.contains("windowsvista", Qt::CaseInsensitive))
    setDefault(SETTING_STYLE, "windowsvista");
  else
#endif
  {
    if (styles.contains("cleanlooks", Qt::CaseInsensitive))
      setDefault(SETTING_STYLE, "cleanlooks");
    else
      setDefault(SETTING_STYLE, "plastique");
  }
#endif

  setDefault(SETTING_LANGUAGE, LanguageSupport::defaultLanguageCode());
  setDefault(SETTING_SHEETNAME, true); 
  setDefault(SETTING_SHOW_MAINWINDOW_AT_START, true);
}


/** Gets the currently preferred language code for Rshare. */
QString RshareSettings::getLanguageCode()
{
  return value(SETTING_LANGUAGE).toString();
}

/** Sets the preferred language code. */
void RshareSettings::setLanguageCode(QString languageCode)
{
  setValue(SETTING_LANGUAGE, languageCode);
}

/** Gets the interface style key (e.g., "windows", "motif", etc.) */
QString RshareSettings::getInterfaceStyle()
{
  return value(SETTING_STYLE).toString();
}

/** Sets the interface style key. */
void RshareSettings::setInterfaceStyle(QString styleKey)
{
  setValue(SETTING_STYLE, styleKey);
}

/** Gets the sheetname.*/
QString RshareSettings::getSheetName()
{
  return value(SETTING_SHEETNAME).toString();
}
/** Sets the sheetname.*/
void RshareSettings::setSheetName(QString sheet)                                  
{ 
    setValue(SETTING_SHEETNAME, sheet);
}

/** Returns the bandwidth line filter. */
uint RshareSettings::getBWGraphFilter()
{
  return value(SETTING_BWGRAPH_FILTER, DEFAULT_BWGRAPH_FILTER).toUInt(); 
}

/** Saves the setting for whether or not the given line will be graphed */
void RshareSettings::setBWGraphFilter(uint line, bool status)
{
  uint filter = getBWGraphFilter();
  filter = (status ? (filter | line) : (filter & ~(line)));
  setValue(SETTING_BWGRAPH_FILTER, filter);
}

/** Get the level of opacity for the BandwidthGraph window. */
int RshareSettings::getBWGraphOpacity()
{
  return value(SETTING_BWGRAPH_OPACITY, DEFAULT_OPACITY).toInt();
}

/** Set the level of opacity for the BandwidthGraph window. */
void RshareSettings::setBWGraphOpacity(int value)
{
  setValue(SETTING_BWGRAPH_OPACITY, value);
}

/** Gets whether the bandwidth graph is always on top when displayed. */
bool RshareSettings::getBWGraphAlwaysOnTop()
{
  return value(SETTING_BWGRAPH_ALWAYS_ON_TOP,
               DEFAULT_BWGRAPH_ALWAYS_ON_TOP).toBool();
}

/** Sets whether the bandwidth graph is always on top when displayed. */
void RshareSettings::setBWGraphAlwaysOnTop(bool alwaysOnTop)
{
  setValue(SETTING_BWGRAPH_ALWAYS_ON_TOP, alwaysOnTop);
}

/** Returns true if RetroShare's main window should be visible when the
 * application starts. */
bool
RshareSettings::showMainWindowAtStart()
{
  return value(SETTING_SHOW_MAINWINDOW_AT_START).toBool();
}

/** Sets whether to show RetroShare's main window when the application starts. */
void
RshareSettings::setShowMainWindowAtStart(bool show)
{
  setValue(SETTING_SHOW_MAINWINDOW_AT_START, show);
}

/** Returns true if Vidalia is set to run on system boot. */
bool
RshareSettings::runRetroshareOnBoot()
{
#if defined(Q_WS_WIN)
  if (!win32_registry_get_key_value(STARTUP_REG_KEY, RETROSHARE_REG_KEY).isEmpty()) {
    return true;
  } else {
    return false;
  }
#else
  /* Platforms other than windows aren't supported yet */
  return false;
#endif
}

/** If <b>run</b> is set to true, then Vidalia will run on system boot. */
void
RshareSettings::setRunRetroshareOnBoot(bool run)
{
#if defined(Q_WS_WIN)
  if (run) {
    win32_registry_set_key_value(STARTUP_REG_KEY, RETROSHARE_REG_KEY,
        QString("\"" +
                QDir::convertSeparators(QCoreApplication::applicationFilePath())) +
                "\"");
  } else {
    win32_registry_remove_key(STARTUP_REG_KEY, RETROSHARE_REG_KEY);
  }
#else
  /* Platforms othe rthan windows aren't supported yet */
  Q_UNUSED(run);
  return;
#endif
}

/** Saving Generic Widget Size / Location */

void RshareSettings::saveWidgetInformation(QWidget *widget)
{
    beginGroup("widgetInformation");
    beginGroup(widget->objectName());

    setValue("size", widget->size());
    setValue("pos", widget->pos());

    endGroup();
    endGroup();
}

void RshareSettings::saveWidgetInformation(QMainWindow *widget, QToolBar *toolBar)
{
 beginGroup("widgetInformation");
 beginGroup(widget->objectName());

 setValue("toolBarArea", widget->toolBarArea(toolBar));

 endGroup();
 endGroup();
 
 saveWidgetInformation(widget);
}

void RshareSettings::loadWidgetInformation(QWidget *widget)
{
 beginGroup("widgetInformation");
 beginGroup(widget->objectName());
 
 widget->resize(value("size", widget->size()).toSize());
 widget->move(value("pos", QPoint(200, 200)).toPoint());

 endGroup();
 endGroup();
}

void RshareSettings::loadWidgetInformation(QMainWindow *widget, QToolBar *toolBar)
{
 beginGroup("widgetInformation");
 beginGroup(widget->objectName());
 
 widget->addToolBar((Qt::ToolBarArea) value("toolBarArea", Qt::TopToolBarArea).toInt(),
                    toolBar);
 
 endGroup();
 endGroup();
 
 loadWidgetInformation(widget);
}

