/*******************************************************************************
 * gui/statistics/StatisticsWindow.h                                           *
 *                                                                             *
 * Copyright (c) 2011 Robert Fernier  <retroshare.project@gmail.com>           *
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

#ifndef RSSTATS_WINDOW_H
#define RSSTATS_WINDOW_H

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
class GxsTransportStatistics;
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
  GxsTransportStatistics *gxsdlg;
  BwCtrlWindow *bwdlg;
  TurtleRouterStatistics *trsdlg;
  RttStatistics *rttdlg;


public slots:
  void setNewPage(int page);
	
protected:
    void changeEvent(QEvent *e);
	void closeEvent (QCloseEvent * event);
	
private:
    void initStackedPage();
    
    Ui::StatisticsWindow *ui;

    static StatisticsWindow *mInstance;
    
    /** Creates a new action for a Main page. */
    QAction* createPageAction(const QIcon &icon, const QString &text, QActionGroup *group);    

};

#endif // RSDHT_WINDOW_H

