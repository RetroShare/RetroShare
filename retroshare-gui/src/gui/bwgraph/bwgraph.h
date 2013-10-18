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


#ifndef _BWGRAPH_H
#define _BWGRAPH_H

#include <QDateTime>
#include <QEvent>
#include <QTimer>

#include <gui/common/rwindow.h>

#include "ui_bwgraph.h"

/** Redraw graph every BWGRAPH_REFRESH_RATE ms **/
#define BWGRAPH_REFRESH_RATE    5113


class BandwidthGraph : public RWindow
{
  Q_OBJECT

public:
  /** Default constructor */
  BandwidthGraph(QWidget *parent = 0, Qt::WindowFlags flags = 0);

public slots:
  /** Overloaded QWidget.show */
  void showWindow();

protected:
  /** Called to deliver a bandwidth update event from Tor. */
  void customEvent(QEvent *event);
  void timerEvent(QTimerEvent*);
  
private slots:
  /** Adds new data to the graph */
  void updateGraph(qreal bytesRead, qreal bytesWritten);
  /** Called when settings button is toggled */
  void showSettingsFrame(bool show);
  /** Called when the settings button is toggled */
  void setOpacity(int value);
  /** Called when the user saves settings */
  void saveChanges();
  /** Called when the user cancels changes settings */
  void cancelChanges();
  /** Called when the reset button is pressed */
  void reset();

  void updategraphstatus();

  
private:
  /** Create and bind actions to events **/
  void createActions();
  /** Loads the saved Bandwidth Graph settings */
  void loadSettings();

  /** Qt Designer generated object */
  Ui::BandwidthGraph ui;
};

#endif

