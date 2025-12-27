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

#ifndef _STATISTICDIALOG_H
#define _STATISTICDIALOG_H

#include <QFileDialog>
#include <QDateTime>
#include <QEvent>
#include <QTimer>

#include "gui/mainpage.h"
#include "gui/linetypes.h"

#include "ui_StatisticDialog.h"

/** Redraw graph every 1000ms **/
#define REFRESH_RATE    1000

class StatisticDialog : public MainPage
{
  Q_OBJECT

public:
  /** Default Constructor */
  StatisticDialog(QWidget *parent = 0);

protected:
  /** Called to deliver a bandwidth update event from RetroShare. */
  void customEvent(QEvent *event);
  void timerEvent(QTimerEvent*);


private slots:
  /** Adds new data to the graph */
  void updateGraph(quint64 bytesRead, quint64 bytesWritten);
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


private:
  /** Create and bind actions to events **/
  void createActions();
  /** Loads the saved Bandwidth Graph settings */
  void loadSettings();

  /** Qt Designer generated object */
  Ui::StatisticDialog ui;
};

#endif

