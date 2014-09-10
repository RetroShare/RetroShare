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

#include "graphframetunnel.h"


/** Default contructor */
GraphFrameTunnel::GraphFrameTunnel(QWidget *parent)
: QFrame(parent)
{
  /* Create Graph Frame related objects */
  _tunnelrequestdownData = new QList<qreal>();
  _tunnelrequestupData = new QList<qreal>();
  _incomingfileData = new QList<qreal>();
  _outgoingfileData = new QList<qreal>();
  _forwardedData = new QList<qreal>();

  _painter = new QPainter();
  _graphStyle = SolidLine;
  
  /* Initialize graph values */
  _tunnelrequestdownData->prepend(0);
  _tunnelrequestupData->prepend(0);
  _incomingfileData->prepend(0);
  _outgoingfileData->prepend(0);
  _forwardedData->prepend(0);

  _maxPoints = getNumPoints();  
  _showTrdown = true;
  _showTrup = true;
  _showIncoming = true;
  _showOutgoing = true;
  _showForwarded = true;
  _maxValue = MIN_SCALE;
}

/** Default destructor */
GraphFrameTunnel::~GraphFrameTunnel()
{
  delete _painter;
  delete _tunnelrequestdownData;
  delete _tunnelrequestupData;
  delete _incomingfileData;
  delete _outgoingfileData;
  delete _forwardedData;
}

/** Gets the width of the desktop, which is the maximum number of points 
 * we can plot in the graph. */
int
GraphFrameTunnel::getNumPoints()
{
  QDesktopWidget *desktop = QApplication::desktop();
  int width = desktop->width();
  return width;
}

/** Adds new data points to the graph. */
void
GraphFrameTunnel::addPoints(qreal trup, qreal trdown, qreal datadown,qreal dataup, qreal forwardupdown)
{
  /* If maximum number of points plotted, remove oldest */
  if (_tunnelrequestupData->size() == _maxPoints) {
    _tunnelrequestdownData->removeLast();
    _tunnelrequestupData->removeLast();
    _incomingfileData->removeLast();
    _outgoingfileData->removeLast();
    _forwardedData->removeLast();
  }

  /* Add the points to their respective lists */
  _tunnelrequestupData->prepend(trup);
  _tunnelrequestdownData->prepend(trdown);
  _incomingfileData->prepend(datadown);
  _outgoingfileData->prepend(dataup);
  _forwardedData->prepend(forwardupdown);

  /* Add to the total counters */
  /* These are not the real total values, but should be close enough. */
  _totalTrup += GRAPH_REFRESH_RATE * trup / 1000;
  _totalTrdown += GRAPH_REFRESH_RATE * trdown / 1000;
  _totalInFileData += GRAPH_REFRESH_RATE * datadown / 1000;
  _totalOutgoingFileData += GRAPH_REFRESH_RATE * dataup / 1000;
  _totalForwardedData += GRAPH_REFRESH_RATE * forwardupdown / 1000;

  /* Check for a new maximum value */
  if (trup > _maxValue) _maxValue = trup;
  if (trdown > _maxValue) _maxValue = trdown;
  if (datadown > _maxValue) _maxValue = datadown;
  if (dataup > _maxValue) _maxValue = dataup;
  if (forwardupdown > _maxValue) _maxValue = forwardupdown;

  this->update();
}

/** Clears the graph. */
void
GraphFrameTunnel::resetGraph()
{
  _tunnelrequestupData->clear();
  _tunnelrequestdownData->clear();
  _incomingfileData->clear();
  _outgoingfileData->clear();
  _forwardedData->clear();
  
  _tunnelrequestdownData->prepend(0);
  _tunnelrequestupData->prepend(0);
  _incomingfileData->prepend(0);
  _outgoingfileData->prepend(0);
  _forwardedData->prepend(0);
  
  _maxValue = MIN_SCALE;
  
  _totalTrup = 0;
  _totalTrdown = 0;
  _totalInFileData = 0;
  _totalOutgoingFileData = 0;
  _totalForwardedData = 0;
  
  
  this->update();
}

/** Toggles display of respective graph lines and counters. */
void
GraphFrameTunnel::setShowCounters(bool showTrdown, bool showTrup, bool showIncoming, bool showOutgoing)
{
  _showTrdown = showTrdown;
  _showTrup = showTrup;
  _showIncoming = showIncoming;
  _showOutgoing = showOutgoing;
  this->update();
}

/** Overloads default QWidget::paintEvent. Draws the actual 
 * bandwidth graph. */
void
GraphFrameTunnel::paintEvent(QPaintEvent *event)
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
  _painter->fillRect(_rec, QBrush(TBACK_COLOR));
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
GraphFrameTunnel::paintData()
{
  //QVector<QPointF> recvPoints, sendPoints;
  QVector<QPointF> trdownPoints, trupPoints, infilePoints, outfilePoints, forwardedPoints;

  /* Convert the bandwidth data points to graph points */
  trdownPoints = pointsFromData( _tunnelrequestupData);
  trupPoints = pointsFromData( _tunnelrequestdownData);
  infilePoints = pointsFromData(_incomingfileData);
  outfilePoints = pointsFromData(_outgoingfileData);
  forwardedPoints = pointsFromData(_forwardedData);
  
  if (_graphStyle == AreaGraph) {
    /* Plot the bandwidth data as area graphs */
    if (_showTrdown)
      paintIntegral(trdownPoints, TRDOWN_COLOR, 0.6);
    if (_showTrup)
      paintIntegral(trupPoints, TRUP_COLOR, 0.4);
      
      paintIntegral(infilePoints, INFILEDATA_COLOR, 0.6);
      paintIntegral(outfilePoints, OUTFILEDATA_COLOR, 0.4);
      //paintIntegral(forwardedPoints, FORWARDED_COLOR, 0.6);
  }
  
  /* Plot the bandwidth as solid lines. If the graph style is currently an
   * area graph, we end up outlining the integrals. */
  if (_showTrdown)
    paintLine(trdownPoints, TRDOWN_COLOR);
  if (_showTrup)
    paintLine(trupPoints, TRUP_COLOR);  
    
    paintLine(infilePoints, INFILEDATA_COLOR);
    paintLine(outfilePoints, OUTFILEDATA_COLOR);
    //paintLine(forwardedPoints, FORWARDED_COLOR);
}

/** Returns a list of points on the bandwidth graph based on the supplied set
 * of send or receive values. */
QVector<QPointF>
GraphFrameTunnel::pointsFromData(QList<qreal>* list)
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
GraphFrameTunnel::paintIntegral(QVector<QPointF> points, QColor color, qreal alpha)
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
GraphFrameTunnel::paintLine(QVector<QPointF> points, QColor color, Qt::PenStyle lineStyle) 
{
  /* Save the current brush, plot the line, and restore the old brush */
  QPen oldPen = _painter->pen();
  _painter->setPen(QPen(color, lineStyle));
  _painter->drawPolyline(points.data(), points.size());
  _painter->setPen(oldPen);
}

/** Paints selected total indicators on the graph. */
void
GraphFrameTunnel::paintTotals()
{
  int x = SCALE_WIDTH + FONT_SIZE, y = 0;
  int rowHeight = FONT_SIZE;

#if !defined(Q_WS_MAC)
  /* On Mac, we don't need vertical spacing between the text rows. */
  rowHeight += 5;
#endif

  /* If total received is selected */
  if (_showTrdown) {
    y = rowHeight;
    _painter->setPen(TRDOWN_COLOR);
    _painter->drawText(x, y,
        tr("Tunnel requests Down: ") + totalToStr(_totalTrdown) + 
        " ("+tr("%1 KB/s").arg(_tunnelrequestdownData->first(), 0, 'f', 2)+")");
  }

  /* If total sent is selected */
  if (_showTrup) {
    y += rowHeight;
    _painter->setPen(TRUP_COLOR);
    _painter->drawText(x, y,
        tr("Tunnel requests Up: ") + totalToStr(_totalTrup) +
        " ("+tr("%1 KB/s").arg(_tunnelrequestupData->first(), 0, 'f', 2)+")");
  }
  
    /* If total received is selected */
  if (_showIncoming) {
    y += rowHeight;
    _painter->setPen(INFILEDATA_COLOR);
    _painter->drawText(x, y,
        tr("Incoming file data: ") + totalToStr(_totalInFileData) + 
        " ("+tr("%1 KB/s").arg(_incomingfileData->first(), 0, 'f', 2)+")");
  }

  /* If total sent is selected */
  if (_showOutgoing) {
    y += rowHeight;
    _painter->setPen(OUTFILEDATA_COLOR);
    _painter->drawText(x, y,
        tr("Outgoing file data: ") + totalToStr(_totalOutgoingFileData) +
        " ("+tr("%1 KB/s").arg(_outgoingfileData->first(), 0, 'f', 2)+")");
  }
  
  /* If total sent is selected */
  /*if (_showForwarded) {
    y += rowHeight;
    _painter->setPen(FORWARDED_COLOR);
    _painter->drawText(x, y,
        tr("Forwarded data: ") + totalToStr(_totalOutgoingFileData) +
        " ("+tr("%1 KB/s").arg(_outgoingfileData->first(), 0, 'f', 2)+")");
  }*/
}

/** Returns a formatted string with the correct size suffix. */
QString
GraphFrameTunnel::totalToStr(qreal total)
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
GraphFrameTunnel::paintScale()
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
    _painter->setPen(TSCALE_COLOR);
    _painter->drawText(QPointF(5, pos+FONT_SIZE), 
                       tr("%1 KB/s").arg(scale, 0, 'f', 2));
    _painter->setPen(TGRID_COLOR);
    _painter->drawLine(QPointF(SCALE_WIDTH, pos), 
                       QPointF(_rec.width(), pos));
  }
  
  /* Draw vertical separator */
  _painter->drawLine(SCALE_WIDTH, top, SCALE_WIDTH, bottom);
}

