#ifndef RSSTATS_WINDOW_H
#define RSSTATS_WINDOW_H

/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 Robert Fernie
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

#pragma once

#include <QMainWindow>

namespace Ui {
    class StatisticsWindow;
}

class MainPage;
class QActionGroup;

class DhtWindow;
class BwCtrlWindow;
class TurtleRouterStatistics;
class GlobalRouterStatistics;
class RttStatistics;

class StatisticsWindow : public QMainWindow {
    Q_OBJECT
public:

    static void showYourself ();
    static StatisticsWindow* getInstance();
    static void releaseInstance();


    StatisticsWindow(QWidget *parent = 0);
    ~StatisticsWindow();

  DhtWindow *dhtw;
  GlobalRouterStatistics *grsdlg;
  BwCtrlWindow *bwdlg;
  TurtleRouterStatistics *trsdlg;
  RttStatistics *rttdlg;


public slots:
  void setNewPage(int page);
	
protected:
    void changeEvent(QEvent *e);

private:
    void initStackedPage();
    
    Ui::StatisticsWindow *ui;

    static StatisticsWindow *mInstance;
    
    /** Creates a new action for a Main page. */
    QAction* createPageAction(const QIcon &icon, const QString &text, QActionGroup *group);    

};

#endif // RSDHT_WINDOW_H

