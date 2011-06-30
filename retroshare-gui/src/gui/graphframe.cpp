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



#include <QtGlobal>

#include "graphframe.h"


/** Default contructor */
GraphFrame::GraphFrame(QWidget *parent)
: QFrame(parent)
{
  /* Create Graph Frame related objects */
  _recvData = new QList<qreal>();
  _sendData = new QList<qreal>();
  _painter = new QPainter();
  _graphStyle = SolidLine;
  
  /* Initialize graph values */
  _recvData->prepend(0);
  _sendData->prepend(0);
  _maxPoints = getNumPoints();  
  _showRecv = true;
  _showSend = true;
  _maxValue = MIN_SCALE;
}

/** Default destructor */
GraphFrame::~GraphFrame()
{
  delete _painter;
  delete _recvData;
  delete _sendData;
}

/** Gets the width of the desktop, which is the maximum number of points 
 * we can plot in the graph. */
int
GraphFrame::getNumPoints()
{
  QDesktopWidget *desktop = QApplication::desktop();
  int width = desktop->width();
  return width;
}

/** Adds new data points to the graph. */
void
GraphFrame::addPoints(qreal recv, qreal send)
{
  /* If maximum number of points plotted, remove oldest */
  if (_sendData->size() == _maxPoints) {
    _sendData->removeLast();
    _recvData->removeLast();
  }

  /* Add the points to their respective lists */
  _sendData->prepend(send);
  _recvData->prepend(recv);

  /* Add to the total counters */
  _totalSend += send;
  _totalRecv += recv;
  
  /* Check for a new maximum value */
  if (send > _maxValue) _maxValue = send;
  if (recv > _maxValue) _maxValue = recv;

  this->update();
}

/** Clears the graph. */
void
GraphFrame::resetGraph()
{
  _recvData->clear();
  _sendData->clear();
  _recvData->prepend(0);
  _sendData->prepend(0);
  _maxValue = MIN_SCALE;
  _totalSend = 0;
  _totalRecv = 0;
  this->update();
}

/** Toggles display of respective graph lines and counters. */
void
GraphFrame::setShowCounters(bool showRecv, bool showSend)
{
  _showRecv = showRecv;
  _showSend = showSend;
  this->update();
}

/** Overloads default QWidget::paintEvent. Draws the actual 
 * bandwidth graph. */
void
GraphFrame::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  /* Set current graph dimensions */
  _rec = this->frameRect();
  
  /* Start the painter */
  _painter->begin(this);
  
  /* We want antialiased lines and text */
  _painter->setRenderHint(QPainter::Antialiasing);
  _painter->setRenderHint(QPainter::TextAntialiasing);
  
  /* Fill in the background */
  _painter->fillRect(_rec, QBrush(BACK_COLOR));
  _painter->drawRect(_rec);

  /* Paint the scale */
  paintScale();
  /* Plot the send/receive data */
  paintData();
  /* Paint the send/recv totals */
  paintTotals();

  /* Stop the painter */
  _painter->end();
}

/** Paints an integral and an outline of that integral for each data set (send
 * and/or receive) that is to be displayed. The integrals will be drawn first,
 * followed by the outlines, since we want the area of overlapping integrals
 * to blend, but not the outlines of those integrals. */
void
GraphFrame::paintData()
{
  QVector<QPointF> recvPoints, sendPoints;

  /* Convert the bandwidth data points to graph points */
  recvPoints = pointsFromData(_recvData);
  sendPoints = pointsFromData(_sendData);
  
  if (_graphStyle == AreaGraph) {
    /* Plot the bandwidth data as area graphs */
    if (_showRecv)
      paintIntegral(recvPoints, RECV_COLOR, 0.6);
    if (_showSend)
      paintIntegral(sendPoints, SEND_COLOR, 0.4);
  }
  
  /* Plot the bandwidth as solid lines. If the graph style is currently an
   * area graph, we end up outlining the integrals. */
  if (_showRecv)
    paintLine(recvPoints, RECV_COLOR);
  if (_showSend)
    paintLine(sendPoints, SEND_COLOR);
}

/** Returns a list of points on the bandwidth graph based on the supplied set
 * of send or receive values. */
QVector<QPointF>
GraphFrame::pointsFromData(QList<qreal>* list)
{
  QVector<QPointF> points;
  int x = _rec.width();
  int y = _rec.height();
  qreal scale = (y - (y/10)) / _maxValue;
  qreal currValue;
  
  /* Translate all data points to points on the graph frame */
  points << QPointF(x, y);
  for (int i = 0; i < list->size(); i++) {
    currValue = y - (list->at(i) * scale);
    if (x - SCROLL_STEP < SCALE_WIDTH) {
      points << QPointF(SCALE_WIDTH, currValue);
      break;
    }
    points << QPointF(x, currValue);
    x -= SCROLL_STEP;
  }
  points << QPointF(SCALE_WIDTH, y);
  return points; 
}

/** Plots an integral using the data points in <b>points</b>. The area will be
 * filled in using <b>color</b> and an alpha-blending level of <b>alpha</b>
 * (default is opaque). */
void
GraphFrame::paintIntegral(QVector<QPointF> points, QColor color, qreal alpha)
{
  /* Save the current brush, plot the integral, and restore the old brush */
  QBrush oldBrush = _painter->brush();
  color.setAlphaF(alpha);
  _painter->setBrush(QBrush(color));
  _painter->drawPolygon(points.data(), points.size());
  _painter->setBrush(oldBrush);
}

/** Iterates the input list and draws a line on the graph in the appropriate
 * color. */
void
GraphFrame::paintLine(QVector<QPointF> points, QColor color, Qt::PenStyle lineStyle) 
{
  /* Save the current brush, plot the line, and restore the old brush */
  QPen oldPen = _painter->pen();
  _painter->setPen(QPen(color, lineStyle));
  _painter->drawPolyline(points.data(), points.size());
  _painter->setPen(oldPen);
}

/** Paints selected total indicators on the graph. */
void
GraphFrame::paintTotals()
{
  int x = SCALE_WIDTH + FONT_SIZE, y = 0;
  int rowHeight = FONT_SIZE;

#if !defined(Q_WS_MAC)
  /* On Mac, we don't need vertical spacing between the text rows. */
  rowHeight += 5;
#endif

  /* If total received is selected */
  if (_showRecv) {
    y = rowHeight;
    _painter->setPen(RECV_COLOR);
    _painter->drawText(x, y,
        tr("Recv: ") + totalToStr(_totalRecv) + 
        " ("+tr("%1 KB/s").arg(_recvData->first(), 0, 'f', 2)+")");
  }

  /* If total sent is selected */
  if (_showSend) {
    y += rowHeight;
    _painter->setPen(SEND_COLOR);
    _painter->drawText(x, y,
        tr("Sent: ") + totalToStr(_totalSend) +
        " ("+tr("%1 KB/s").arg(_sendData->first(), 0, 'f', 2)+")");
  }
}

/** Returns a formatted string with the correct size suffix. */
QString
GraphFrame::totalToStr(qreal total)
{
  /* Determine the correct size suffix */
  if (total < 1024) {
    /* Use KB suffix */
    return tr("%1 KB").arg(total, 0, 'f', 2);
  } else if (total < 1048576) {
    /* Use MB suffix */
    return tr("%1 MB").arg(total/1024.0, 0, 'f', 2);
  } else {
    /* Use GB suffix */
    return tr("%1 GB").arg(total/1048576.0, 0, 'f', 2);
  }
}

/** Paints the scale on the graph. */
void
GraphFrame::paintScale()
{
  qreal markStep = _maxValue * .25;
  int top = _rec.y();
  int bottom = _rec.height();
  qreal paintStep = (bottom - (bottom/10)) / 4;
  
  /* Draw the other marks in their correctly scaled locations */
  qreal scale;
  qreal pos;
  for (int i = 1; i < 5; i++) {
    pos = bottom - (i * paintStep);
    scale = i * markStep;
    _painter->setPen(SCALE_COLOR);
    _painter->drawText(QPointF(5, pos+FONT_SIZE), 
                       tr("%1 KB/s").arg(scale, 0, 'f', 2));
    _painter->setPen(GRID_COLOR);
    _painter->drawLine(QPointF(SCALE_WIDTH, pos), 
                       QPointF(_rec.width(), pos));
  }
  
  /* Draw vertical separator */
  _painter->drawLine(SCALE_WIDTH, top, SCALE_WIDTH, bottom);
}

