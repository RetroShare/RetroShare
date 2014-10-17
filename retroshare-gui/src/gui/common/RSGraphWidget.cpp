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

#ifndef WINDOWS_SYS
#include <sys/times.h>
#endif

#include <iostream>
#include <math.h>
#include <QtGlobal>
#include <QPainter>
#include <QDateTime>
#include <QTimer>

#include "RSGraphWidget.h"

RSGraphSource::RSGraphSource()
{
    _time_limit_msecs    = 10*1000 ;
    _update_period_msecs =  1*1000 ;
    _time_orig_msecs     = QDateTime::currentMSecsSinceEpoch() ;
    _timer = new QTimer ;
    _digits = 2 ;

    QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(update())) ;
}

RSGraphSource::~RSGraphSource()
{
    stop() ;
    delete _timer ;
}
void RSGraphSource::clear()
{
    _points.clear() ;
}
void RSGraphSource::stop()
{
    _timer->stop() ;
}
void RSGraphSource::start()
{
    _timer->stop();
    _timer->start((int)(_update_period_msecs)) ;
}

int RSGraphSource::n_values() const { return _points.size() ; }

QString RSGraphSource::displayName(int i) const
{
    if(_points.empty())
        return QString() ;

    std::map<std::string,std::list<std::pair<qint64,float> > >::const_iterator it = _points.begin();

    int n=0;
    for(it = _points.begin();it!=_points.end() && n<i;++it,++n) ;

    if(n != i)
        return QString("[error]");

    return QString::fromStdString(it->first) ;
}

QString RSGraphSource::displayValue(float v) const
{
    return QString::number(v,'f',_digits) + " " + unitName() ;
}

void RSGraphSource::getCurrentValues(std::vector<float>& vals) const
{
    std::map<std::string,std::list<std::pair<qint64,float> > >::const_iterator it = _points.begin();

    for(it = _points.begin();it!=_points.end();++it)
        vals.push_back(it->second.back().second) ;
}

QString RSGraphSource::legend(int i,float v) const
{
    return displayName(i) + " (" + displayValue(v) + " )";
}

void RSGraphSource::getDataPoints(int index,std::vector<QPointF>& pts) const
{
    pts.clear() ;
    qint64 now = getTime() ;

    std::map<std::string,std::list<std::pair<qint64,float> > >::const_iterator it = _points.begin();

    int n=0;
    for(it = _points.begin();it!=_points.end() && n<index;++it,++n) ;

    if(n != index)
        return ;

    for(std::list<std::pair<qint64,float> >::const_iterator it2=it->second.begin();it2!=it->second.end();++it2)
        pts.push_back(QPointF( (now - (*it2).first)/1000.0f,(*it2).second)) ;
}

void RSGraphWidget::setShowEntry(uint32_t entry,bool b)
{
    if(b)
    {
        std::set<std::string>::iterator it = _masked_entries.find(_source->displayName(entry).toStdString()) ;
        if(it != _masked_entries.end())
            _masked_entries.erase(it) ;
    }
    else
        _masked_entries.insert(_source->displayName(entry).toStdString()) ;
}

void RSGraphWidget::addSource(RSGraphSource *gs)
{
if(_source != NULL)
    delete _source ;

    _source = gs ;
}

qint64 RSGraphSource::getTime() const
{
    return QDateTime::currentMSecsSinceEpoch() - _time_orig_msecs ;
}

void RSGraphSource::update()
{
    std::map<std::string,float> vals ;
    getValues(vals) ;

    qint64 ms = getTime() ;

    for(std::map<std::string,float>::iterator it=vals.begin();it!=vals.end();++it)
    {
        std::list<std::pair<qint64,float> >& lst(_points[it->first]) ;

        lst.push_back(std::make_pair(ms,it->second)) ;

        for(std::list<std::pair<qint64,float> >::iterator it2=lst.begin();it2!=lst.end();)
            if( ms - (*it2).first > _time_limit_msecs)
            {
                //std::cerr << "  removing old value with time " << (*it).first/1000.0f << std::endl;
                it2 = lst.erase(it2) ;
            }
            else
                break ;
    }

    // remove empty lists

    for(std::map<std::string,std::list<std::pair<qint64,float> > >::iterator it=_points.begin();it!=_points.end();)
        if(it->second.empty())
    {
        std::map<std::string,std::list<std::pair<qint64,float> > >::iterator tmp(it) ;
        ++tmp;
        _points.erase(it) ;
        it=tmp ;
    }
        else
            ++it ;
}
void RSGraphSource::reset()
{
    _points.clear() ;
}

void RSGraphSource::setCollectionTimeLimit(qint64 s) { _time_limit_msecs = s ; }
void RSGraphSource::setCollectionTimePeriod(qint64 s) { _update_period_msecs = s ; }

void RSGraphWidget::setTimeScale(float pixels_per_second)
{
    _time_scale =pixels_per_second ;
}

/** Default contructor */
RSGraphWidget::RSGraphWidget(QWidget *parent)
: QFrame(parent)
{
    _source =NULL;
  _painter = new QPainter();
  
  /* Initialize graph values */
  _maxPoints = getNumPoints();  
  _maxValue = MINUSER_SCALE;

  _opacity = 0.6 ;
  _flags = 0;
  _time_scale = 5.0f ; // in pixels per second.
  _timer = new QTimer ;
  QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(update())) ;

  _y_scale = 1.0f ;
  _timer->start(1000);
}

/** Default destructor */
RSGraphWidget::~RSGraphWidget()
{
    delete _painter;
    delete _source ;
}

/** Gets the width of the desktop, which is the maximum number of points 
 * we can plot in the graph. */
int
RSGraphWidget::getNumPoints()
{
  QDesktopWidget *desktop = QApplication::desktop();
  int width = desktop->width();
  return width;
}

/** Clears the graph. */
void
RSGraphWidget::resetGraph()
{
    _source->reset() ;
  _maxValue = MINUSER_SCALE;
  update();
}

/** Toggles display of respective graph lines and counters. */
//void
//DhtGraph::setShowCounters(bool showRSDHT, bool showALLDHT)
//{
//  _showRSDHT = showRSDHT;
//  _showALLDHT = showALLDHT;
//  this->update();
//}

/** Overloads default QWidget::paintEvent. Draws the actual 
 * bandwidth graph. */
void RSGraphWidget::paintEvent(QPaintEvent *)
{
    //std::cerr << "In paint event!" << std::endl;

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

  if(_flags & RSGRAPH_FLAGS_SHOW_LEGEND)
      paintLegend() ;

  /* Stop the painter */
  _painter->end();
}

QColor RSGraphWidget::getColor(int i)
{
    // shuffle the colors a little bit
    int h = (i*86243)%359 ;

    return QColor::fromHsv(h,128+127*(i&1),255) ;
}

void RSGraphWidget::setCurvesOpacity(float f)
{
    _opacity = f ;
}
/** Paints an integral and an outline of that integral for each data set (rsdht
 * and/or alldht) that is to be displayed. The integrals will be drawn first,
 * followed by the outlines, since we want the area of overlapping integrals
 * to blend, but not the outlines of those integrals. */
void RSGraphWidget::paintData()
{
    /* Convert the bandwidth data points to graph points */

  if(_source == NULL)
      return ;

  const RSGraphSource& source(*_source) ;
  _maxValue = 0.0f ;

  for(int i=0;i<source.n_values();++i)
      if( _masked_entries.find(source.displayName(i).toStdString()) == _masked_entries.end() )
      {
          std::vector<QPointF> values ;
          source.getDataPoints(i,values) ;

          QVector<QPointF> points ;
          pointsFromData(values,points) ;

          /* Plot the bandwidth data as area graphs */
          if (_flags & RSGRAPH_FLAGS_PAINT_STYLE_PLAIN)
              paintIntegral(points, getColor(i), _opacity);

          /* Plot the bandwidth as solid lines. If the graph style is currently an
   * area graph, we end up outlining the integrals. */
          paintLine(points, getColor(i));
      }
  if(_maxValue > 0.0f)
      if(_flags & RSGRAPH_FLAGS_LOG_SCALE_Y)
          _y_scale = _rec.height()*0.8 / log(_maxValue) ;
      else
          _y_scale = _rec.height()*0.8/_maxValue ;
}

/** Returns a list of points on the bandwidth graph based on the supplied set
 * of rsdht or alldht values. */
void RSGraphWidget::pointsFromData(const std::vector<QPointF>& values,QVector<QPointF>& points)
{
    points.clear();

    int x = _rec.width();
    int y = _rec.height();

    float time_step = 1.0f ;	// number of seconds per pixel

    /* Translate all data points to points on the graph frame */

    float last = values.back().x() ;

    //std::cerr << "Got " << values.size() << " values for index 0" << std::endl;

    float last_px = SCALE_WIDTH ;
    float last_py = 0.0f ;

    for (uint i = 0; i < values.size(); ++i)
    {
        //std::cerr << "Value: (" << values[i].x() << " , " << values[i].y() << ")" << std::endl;

        // compute point in pixels

        qreal px = x - (values[i].x()-last)*_time_scale ;
    qreal py = y -  valueToPixels(values[i].y()) ;

        if(px >= SCALE_WIDTH && last_px < SCALE_WIDTH)
        {
            float alpha = (SCALE_WIDTH - last_px)/(px - last_px) ;
            float ipx = SCALE_WIDTH ;
            float ipy = (1-alpha)*last_py + alpha*py ;

            points << QPointF(ipx,y) ;
            points << QPointF(ipx,ipy) ;
        }
        else if(i==0)
            points << QPointF(px,y) ;

        last_px = px ;
        last_py = py ;

        if(px < SCALE_WIDTH)
            continue ;

    _maxValue = std::max(_maxValue,values[i].y()) ;

        // remove midle point when 3 consecutive points have the same value.

        if(points.size() > 1 && points[points.size()-2].y() == points.back().y() && points.back().y() == py)
            points.pop_back() ;

        points << QPointF(px,py) ;

        if(i==values.size()-1)
            points << QPointF(px,y) ;
    }
}

qreal RSGraphWidget::valueToPixels(qreal val)
{
        if(_flags & RSGRAPH_FLAGS_LOG_SCALE_Y)
            return log(std::max((qreal)1.0,val))*_y_scale ;
        else
            return val*_y_scale ;
}

qreal RSGraphWidget::pixelsToValue(qreal val)
{
        if(_flags & RSGRAPH_FLAGS_LOG_SCALE_Y)
            return exp(val / _y_scale) ;
        else
            return val/_y_scale ;
}

/** Plots an integral using the data points in <b>points</b>. The area will be
 * filled in using <b>color</b> and an alpha-blending level of <b>alpha</b>
 * (default is opaque). */
void RSGraphWidget::paintIntegral(const QVector<QPointF>& points, QColor color, qreal alpha)
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
void RSGraphWidget::paintLine(const QVector<QPointF>& points, QColor color, Qt::PenStyle lineStyle)
{
  /* Save the current brush, plot the line, and restore the old brush */
  QPen oldPen = _painter->pen();
  _painter->setPen(QPen(color, lineStyle));
  _painter->drawPolyline(points.data(), points.size());
  _painter->setPen(oldPen);
}

/** Paints selected total indicators on the graph. */
void RSGraphWidget::paintTotals()
{
  int x = SCALE_WIDTH + FONT_SIZE, y = 0;
  int rowHeight = FONT_SIZE;

#if !defined(Q_WS_MAC)
  /* On Mac, we don't need vertical spacing between the text rows. */
  rowHeight += 5;
#endif

//  /* If total received is selected */
//    y = rowHeight;
//    _painter->setPen(RSDHT_COLOR);
//    _painter->drawText(x, y,
//        tr("RetroShare users in DHT: ")+
//        " ("+tr("%1").arg(_rsDHT->first(), 0, 'f', 0)+")");
}

/** Returns a formatted string with the correct size suffix. */
QString RSGraphWidget::totalToStr(qreal total)
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
void RSGraphWidget::paintScale()
{
  int top = _rec.y();
  int bottom = _rec.height();
  qreal paintStep = (bottom - (bottom/10)) / 4;
  
  /* Draw the other marks in their correctly scaled locations */
  qreal scale;
  qreal pos;

  if(_source == NULL)
      return ;

  QString unit_name = _source->unitName() ;

  for (int i = 1; i < 5; i++)
  {
      pos = bottom - (i * paintStep);

      scale = pixelsToValue(i * paintStep);

      QString text = _source->displayValue(scale) ;

      _painter->setPen(SCALE_COLOR);
      _painter->drawText(QPointF(5, pos+0.5*FONT_SIZE),  text);
      _painter->setPen(GRID_COLOR);
      _painter->drawLine(QPointF(SCALE_WIDTH, pos),  QPointF(_rec.width(), pos));
  }
  
  /* Draw vertical separator */
  _painter->drawLine(SCALE_WIDTH, top, SCALE_WIDTH, bottom);
}

void RSGraphWidget::paintLegend()
{
  int bottom = _rec.height();

  std::vector<float> vals ;
  _source->getCurrentValues(vals) ;

    for(uint i=0;i<vals.size();++i)
    {
        qreal paintStep = 4+FONT_SIZE;
        qreal pos = 20+i*paintStep;
        QString text = _source->legend(i,vals[i]) ;

        QPen oldPen = _painter->pen();
        _painter->setPen(QPen(getColor(i), Qt::SolidLine));
        _painter->drawLine(QPointF(SCALE_WIDTH+10.0, pos),  QPointF(SCALE_WIDTH+30.0, pos));
        _painter->setPen(oldPen);

    _painter->setPen(SCALE_COLOR);
        _painter->drawText(QPointF(SCALE_WIDTH + 40,pos + 0.5*FONT_SIZE), text) ;
    }
}

