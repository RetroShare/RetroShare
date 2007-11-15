/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007,  
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


#ifndef _GRAPHFRAME_H
#define _GRAPHFRAME_H

#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>
#include <QPainter>
#include <QPen>
#include <QList>

#define HOR_SPC       2   /** Space between data points */
#define SCALE_WIDTH   75  /** Width of the scale */
#define MIN_SCALE     10  /** 10 kB/s is the minimum scale */  
#define SCROLL_STEP   4   /** Horizontal change on graph update */

#define BACK_COLOR    Qt::black
#define SCALE_COLOR   Qt::green
#define GRID_COLOR    Qt::darkGreen
#define RECV_COLOR    Qt::cyan
#define SEND_COLOR    Qt::yellow

#define FONT_SIZE     11


class GraphFrame : public QFrame
{
  Q_OBJECT

public:
  /** Bandwidth graph style. */
  enum GraphStyle {
    SolidLine = 0,  /**< Plot bandwidth as solid lines. */
    AreaGraph       /**< Plot bandwidth as alpha blended area graphs. */
  };
  
  /** Default Constructor */
  GraphFrame(QWidget *parent = 0);
  /** Default Destructor */
  ~GraphFrame();

  /** Add data points. */
  void addPoints(qreal recv, qreal send);
  /** Clears the graph. */
  void resetGraph();
  /** Toggles display of data counters. */
  void setShowCounters(bool showRecv, bool showSend);
  /** Sets the graph style used to display bandwidth data. */
  void setGraphStyle(GraphStyle style) { _graphStyle = style; }

protected:
  /** Overloaded QWidget::paintEvent() */
  void paintEvent(QPaintEvent *event);

private:
  /** Gets the width of the desktop, the max # of points. */
  int getNumPoints();
  
  /** Paints an integral and an outline of that integral for each data set
   * (send and/or receive) that is to be displayed. */
  void paintData();
  /** Paints the send/receive totals. */
  void paintTotals();
  /** Paints the scale in the graph. */
  void paintScale();
  /** Returns a formatted string representation of total. */
  QString totalToStr(qreal total);
  /** Returns a list of points on the bandwidth graph based on the supplied set
   * of send or receive values. */
  QVector<QPointF> pointsFromData(QList<qreal>* list);
  /** Paints a line with the data in <b>points</b>. */
  void paintLine(QVector<QPointF> points, QColor color, 
                 Qt::PenStyle lineStyle = Qt::SolidLine);
  /** Paints an integral using the supplied data. */
  void paintIntegral(QVector<QPointF> points, QColor color, qreal alpha = 1.0);

  /** Style with which the bandwidth data will be graphed. */
  GraphStyle _graphStyle;
  /** A QPainter object that handles drawing the various graph elements. */
  QPainter* _painter;
  /** Holds the received data points. */
  QList<qreal> *_recvData;
  /** Holds the sent data points. */
  QList<qreal> *_sendData;
  /** The current dimensions of the graph. */
  QRect _rec;
  /** The maximum data value plotted. */
  qreal _maxValue;
  /** The maximum number of points to store. */
  int _maxPoints;
  /** The total data sent/recv. */
  qreal _totalSend;
  qreal _totalRecv;
  /** Show the respective lines and counters. */
  bool _showRecv;
  bool _showSend;
};

#endif
