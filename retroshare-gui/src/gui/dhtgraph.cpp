/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2014 RetroShare Team
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

#include "dhtgraph.h"


/** Default contructor */
DhtGraph::DhtGraph(QWidget *parent)
: QFrame(parent)
{
  /* Create Graph Frame related objects */
  _allDHT = new QList<qreal>();
  _rsDHT = new QList<qreal>();
  _painter = new QPainter();
  _graphStyle = AreaGraph;
  
  /* Initialize graph values */
  _allDHT->prepend(0);
  _rsDHT->prepend(0);
  _maxPoints = getNumPoints();  
  _showRSDHT = true;
  _showALLDHT = false;
  _maxValue = MINUSER_SCALE;
}

/** Default destructor */
DhtGraph::~DhtGraph()
{
  delete _painter;
  delete _allDHT;
  delete _rsDHT;
}

/** Gets the width of the desktop, which is the maximum number of points 
 * we can plot in the graph. */
int
DhtGraph::getNumPoints()
{
  QDesktopWidget *desktop = QApplication::desktop();
  int width = desktop->width();
  return width;
}

/** Adds new data points to the graph. */
void
DhtGraph::addPoints(qreal rsdht, qreal alldht)
{
  /* If maximum number of points plotted, remove oldest */
  if (_rsDHT->size() == _maxPoints) {
    _rsDHT->removeLast();
    //_allDHT->removeLast();
  }

  /* Add the points to their respective lists */
  //_allDHT->prepend(alldht);
  _rsDHT->prepend(rsdht);


  /* Check for a new maximum value */
  //if (alldht > _maxValue) _maxValue = alldht;
  if (rsdht > _maxValue) _maxValue = rsdht;

  this->update();
}

/** Clears the graph. */
void
DhtGraph::resetGraph()
{
  _allDHT->clear();
  _rsDHT->clear();
  _allDHT->prepend(0);
  _rsDHT->prepend(0);
  _maxValue = MINUSER_SCALE;

  this->update();
}

/** Toggles display of respective graph lines and counters. */
void
DhtGraph::setShowCounters(bool showRSDHT, bool showALLDHT)
{
  _showRSDHT = showRSDHT;
  _showALLDHT = showALLDHT;
  this->update();
}

/** Overloads default QWidget::paintEvent. Draws the actual 
 * bandwidth graph. */
void
DhtGraph::paintEvent(QPaintEvent *event)
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
  /* Plot the rsDHT/allDHT data */
  paintData();
  /* Paint the rsDHT/allDHT totals */
  paintTotals();

  /* Stop the painter */
  _painter->end();
}

/** Paints an integral and an outline of that integral for each data set (rsdht
 * and/or alldht) that is to be displayed. The integrals will be drawn first,
 * followed by the outlines, since we want the area of overlapping integrals
 * to blend, but not the outlines of those integrals. */
void
DhtGraph::paintData()
{
  QVector<QPointF> rsdhtPoints, alldhtPoints;

  /* Convert the bandwidth data points to graph points */
  rsdhtPoints = pointsFromData(_rsDHT);
  alldhtPoints = pointsFromData(_allDHT);
  
  if (_graphStyle == AreaGraph) {
    /* Plot the bandwidth data as area graphs */
    if (_showRSDHT)
      paintIntegral(rsdhtPoints, RSDHT_COLOR, 0.6);
    if (_showALLDHT)
      paintIntegral(alldhtPoints, ALLDHT_COLOR, 0.4);
  }
  
  /* Plot the bandwidth as solid lines. If the graph style is currently an
   * area graph, we end up outlining the integrals. */
  if (_showRSDHT)
    paintLine(rsdhtPoints, RSDHT_COLOR);
  if (_showALLDHT)
    paintLine(alldhtPoints, ALLDHT_COLOR);
}

/** Returns a list of points on the bandwidth graph based on the supplied set
 * of rsdht or alldht values. */
QVector<QPointF>
DhtGraph::pointsFromData(QList<qreal>* list)
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
DhtGraph::paintIntegral(QVector<QPointF> points, QColor color, qreal alpha)
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
DhtGraph::paintLine(QVector<QPointF> points, QColor color, Qt::PenStyle lineStyle) 
{
  /* Save the current brush, plot the line, and restore the old brush */
  QPen oldPen = _painter->pen();
  _painter->setPen(QPen(color, lineStyle));
  _painter->drawPolyline(points.data(), points.size());
  _painter->setPen(oldPen);
}

/** Paints selected total indicators on the graph. */
void
DhtGraph::paintTotals()
{
  int x = SCALE_WIDTH + FONT_SIZE, y = 0;
  int rowHeight = FONT_SIZE;

#if !defined(Q_WS_MAC)
  /* On Mac, we don't need vertical spacing between the text rows. */
  rowHeight += 5;
#endif

  /* If total received is selected */
  if (_showRSDHT) {
    y = rowHeight;
    _painter->setPen(RSDHT_COLOR);
    _painter->drawText(x, y,
        tr("RetroShare users in DHT: ")+ 
        " ("+tr("%1").arg(_rsDHT->first(), 0, 'f', 0)+")");
  }

  /* If total sent is selected */
  if (_showALLDHT) {
    y += rowHeight;
    _painter->setPen(ALLDHT_COLOR);
    _painter->drawText(x, y,
        tr("Total DHT users: ") +  
        " ("+tr("%1").arg(_allDHT->first(), 0, 'f', 0)+")");
  }
}

/** Returns a formatted string with the correct size suffix. */
QString
DhtGraph::totalToStr(qreal total)
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
DhtGraph::paintScale()
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
                       tr("%1 Users").arg(scale, 0, 'f', 0));
    _painter->setPen(GRID_COLOR);
    _painter->drawLine(QPointF(SCALE_WIDTH, pos), 
                       QPointF(_rec.width(), pos));
  }
  
  /* Draw vertical separator */
  _painter->drawLine(SCALE_WIDTH, top, SCALE_WIDTH, bottom);
}

