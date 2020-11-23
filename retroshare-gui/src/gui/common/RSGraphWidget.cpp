/*******************************************************************************
 * gui/common/RSGraphWidget.cpp                                                *
 *                                                                             *
 * Copyright (C) 2014 RetroShare Team                                          *
 * Copyright (c) 2006-2007, crypton                                            *
 * Copyright (c) 2006, Matt Edman, Justin Hipple                               *
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

#ifndef WINDOWS_SYS
#include <sys/times.h>
#endif

#include <iostream>
#include <math.h>
#include <QtGlobal>
#include <QPainter>
#include <QDateTime>
#include <QWheelEvent>
#include <QTimer>

#include <retroshare-gui/RsAutoUpdatePage.h>
#include "RSGraphWidget.h"

#if QT_VERSION < 0x040700
#include <sys/time.h>

static qint64 getCurrentMSecsSinceEpoch()
{
	timeval tv ;
	gettimeofday(&tv,NULL) ;
	
	return (qint64)tv.tv_sec + (qint64)tv.tv_usec/1000 ;
}
#endif

RSGraphSource::RSGraphSource()
{
    _time_limit_msecs    = 10*1000 ;
    _update_period_msecs =  1*1000 ;
#if QT_VERSION < 0x040700
    _time_orig_msecs     = getCurrentMSecsSinceEpoch() ;
#else
    _time_orig_msecs     = QDateTime::currentMSecsSinceEpoch() ;
#endif
    _timer = new QTimer ;
    _digits = 2 ;
    _filtering_enabled = true;

    QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(updateIfPossible())) ;
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
    int n=0;
    for(std::map<std::string,std::list<std::pair<qint64,float> > >::const_iterator  it = _points.begin();it!=_points.end() ;++it)
        if(n++ == i)
            return QString::fromStdString(it->first) ;

    return QString("[error]");
}

QString RSGraphSource::displayValue(float v) const
{
    return QString::number(v,'f',_digits) + " " + unitName() ;
}

void RSGraphSource::getCumulatedValues(std::vector<float>& vals) const
{
    for(std::map<std::string,ZeroInitFloat>::const_iterator it = _totals.begin();it!=_totals.end();++it)
        vals.push_back(it->second.v) ;
}
void RSGraphSource::getCurrentValues(std::vector<QPointF>& vals) const
{
    std::map<std::string,std::list<std::pair<qint64,float> > >::const_iterator it = _points.begin();
    qint64 now = getTime() ;

    for(it = _points.begin();it!=_points.end();++it)
        vals.push_back(QPointF( (now - it->second.back().first)/1000.0f,it->second.back().second)) ;
}

QString RSGraphSource::legend(int i,float v,bool show_value) const
{
    return displayName(i) + (show_value?(" (" + displayValue(v) + ")"):"");
}

void RSGraphSource::getDataPoints(int index,std::vector<QPointF>& pts,float filter_factor) const
{
    pts.clear() ;
    qint64 now = getTime() ;

	if(!_filtering_enabled)
		filter_factor = 0 ;

    std::map<std::string,std::list<std::pair<qint64,float> > >::const_iterator it = _points.begin();

    int n=0;
    for(it = _points.begin();it!=_points.end() && n<index;++it,++n) ;

    if(n != index)
        return ;

    float last_value = it->second.empty()?0.0f:(it->second.begin()->second) ;
    
    for(std::list<std::pair<qint64,float> >::const_iterator it2=it->second.begin();it2!=it->second.end();++it2)
    {
        float val = (1-filter_factor)*(*it2).second + filter_factor*last_value;
        last_value = val ;
        
        pts.push_back(QPointF( (now - (*it2).first)/1000.0f, val)) ;
    }
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

void RSGraphWidget::setSource(RSGraphSource *gs)
{
	if (_source != NULL)
		delete _source;

	_source = gs;
}

qint64 RSGraphSource::getTime() const
{
#if QT_VERSION < 0x040700
    return getCurrentMSecsSinceEpoch() - _time_orig_msecs ;
#else
    return QDateTime::currentMSecsSinceEpoch() - _time_orig_msecs ;
#endif
}

void RSGraphSource::updateIfPossible()
{
	if(RsAutoUpdatePage::eventsLocked())
		return ;

	update() ;
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

        float& total ( _totals[it->first].v );

		total += it->second ;

        for(std::list<std::pair<qint64,float> >::iterator it2=lst.begin();it2!=lst.end();) // This loop should be very fast, since we only remove the first elements, if applicable.
            if( ms - (*it2).first > _time_limit_msecs)
            {
                //std::cerr << "  removing old value with time " << (*it).first/1000.0f << std::endl;
				total -= (*it2).second ;
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
			_totals.erase(it->first) ;
			_points.erase(it) ;
			it=tmp ;
		}
        else
            ++it ;
}

#ifdef TO_REMOVE
void RSGraphSource::updateTotals()
{
	std::cerr << "RsGraphSource::updateTotals() for " << _points.size() << " values" << std::endl;
    // now compute totals

    _totals.clear();

    for(std::map<std::string,std::list<std::pair<qint64,float> > >::const_iterator it(_points.begin());it!=_points.end();++it)
    {
        float& f = _totals[it->first].v ;

        for(std::list<std::pair<qint64,float> >::const_iterator it2=it->second.begin();it2!=it->second.end();++it2)
			f += (*it2).second ;
    }
}
#endif

void RSGraphSource::reset()
{
    _points.clear();
    _totals.clear();
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

  _linewidthscale = 1.0f;
  _opacity = 0.6 ;
  _flags = 0;
  _time_scale = 5.0f ; // in pixels per second.
  _time_filter = 1.0f ;	
  _timer = new QTimer ;
  QObject::connect(_timer,SIGNAL(timeout()),this,SLOT(updateIfPossible())) ;


  _y_scale = 1.0f ;
  _timer->start(1000);

    float FS = QFontMetricsF(font()).height();
  setMinimumHeight(12*FS);
  _graph_base = FS*GRAPH_BASE;
}

void RSGraphWidget::updateIfPossible()
{
	if(RsAutoUpdatePage::eventsLocked())
		return ;

	if(!isVisible())
		return ;

	update() ;
}

/** Default destructor */
RSGraphWidget::~RSGraphWidget()
{
    delete _painter;
    delete _source ;
}

void RSGraphWidget::setFiltering(bool b)
{
    if(_source != NULL)
        _source->setFiltering(b) ;
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
  updateIfPossible();
}

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
  paintScale1();

  /* Plot the data */
  paintData();

  /* Paint the totals */
  paintTotals();

  // part of the scale that needs to write over the data curves.
  paintScale2();

  if(_flags & RSGRAPH_FLAGS_SHOW_LEGEND)
      paintLegend() ;

  /* Stop the painter */
  _painter->end();
}

//QSizeF RSGraphWidget::sizeHint(Qt::SizeHint which, const QSizeF& /* constraint */) const
//{
//    float FS = QFontMetricsF(font()).height();
//    //float fact = FS/14.0 ;
//
//    switch(which)
//    {
//default:
//    case Qt::MinimumSize:
//    case Qt::PreferredSize:
//        return QSizeF(70*FS,12*FS);
//    case Qt::MaximumSize:
//        return QSizeF(700*FS,120*FS);
//    }
//}

QColor RSGraphWidget::getColor(const std::string& name)
{
    uint32_t r = 57 ;
    for(uint32_t i=0;i<name.length();++i)
        r = (113*name[i] + r)^0x93859aeb;

    // shuffle the colors a little bit
    int h = (r*86243)%359 ;

    return QColor::fromHsv(h,255,255) ;
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
          //std::cerr << "time filter = " << _time_filter << ", factor=" << 1./_time_scale*_time_filter/(1+_time_filter/_time_scale) << std::endl;
          source.getDataPoints(i,values,1./_time_scale*_time_filter/(1.0f+_time_filter/_time_scale)) ;

          QVector<QPointF> points ;
          pointsFromData(values,points) ;

          QColor pcolor = getColor(source.displayName(i).toStdString()) ;

          /* Plot the bandwidth as solid lines. If the graph style is currently an area graph, we end up outlining the integrals. */

          if(_flags & RSGRAPH_FLAGS_PAINT_STYLE_DOTS)
			  paintDots(points, pcolor);
          else
			  paintLine(points, pcolor);

          /* Plot the data as area graphs */

		  points.push_front(QPointF( _rec.width(), _rec.height() - _graph_base)) ; // add a point in the lower right corner, to close the path.

          if (_flags & RSGRAPH_FLAGS_PAINT_STYLE_PLAIN)
              paintIntegral(points, pcolor, _opacity);
      }
  if(_maxValue > 0.0f)
  {
      if(_flags & RSGRAPH_FLAGS_LOG_SCALE_Y)
          _y_scale = _rec.height()*0.8 / log(std::max((qreal)2.0,(qreal)_maxValue)) ;
      else
          _y_scale = _rec.height()*0.8/_maxValue ;
  }
}

/** Returns a list of points on the bandwidth graph based on the supplied set
 * of rsdht or alldht values. */
void RSGraphWidget::pointsFromData(const std::vector<QPointF>& values,QVector<QPointF>& points)
{
	points.clear();

	int x = _rec.width();
	int y = _rec.height();

	//float time_step = 1.0f ;	// number of seconds per pixel

	/* Translate all data points to points on the graph frame */

	// take 0 as the origin, otherwise the different curves are not aligned properly
	float last = 0;//values.back().x();

	//std::cerr << "Got " << values.size() << " values for index 0" << std::endl;

	float FS = QFontMetricsF(font()).height();
	float fact = FS/14.0 ;

	float last_px = SCALE_WIDTH*fact ;
	float last_py = 0.0f ;

	//   float min_x_no_data_threshold = 1.5 ; // 1.5 sec.

	for (uint i = 0; i < values.size(); ++i)
	{
		//std::cerr << "Value: (" << values[i].x() << " , " << values[i].y() << ")" << std::endl;

		// compute point in pixels

		qreal px = x - (values[i].x()-last)*_time_scale ;
		qreal py = y -  valueToPixels(values[i].y()) ;

		if(!(_flags & RSGRAPH_FLAGS_PAINT_STYLE_DOTS))
		{
			if(px >= SCALE_WIDTH*fact && last_px < SCALE_WIDTH*fact)
			{
				float alpha = (SCALE_WIDTH*fact - last_px)/(px - last_px) ;
				float ipx = SCALE_WIDTH*fact ;
				float ipy = (1-alpha)*last_py + alpha*py ;

				points << QPointF(ipx,y - _graph_base) ;
				points << QPointF(ipx,ipy) ;
			}
			else if(i==0)
			{
				if(px < SCALE_WIDTH*fact)
					points << QPointF(SCALE_WIDTH*fact,py) ;
				else
					points << QPointF(px,y - _graph_base) ;
			}
		}

		if(px < SCALE_WIDTH*fact)
			continue ;

		if((_flags & RSGRAPH_FLAGS_PAINT_STYLE_DOTS) && values[i].y() == 0)
            continue ;

		_maxValue = std::max(_maxValue,values[i].y()) ;

		// remove midle point when 3 consecutive points have the same value.

		if(!(_flags & RSGRAPH_FLAGS_PAINT_STYLE_DOTS))
			if(points.size() > 1 && points[points.size()-2].y() == points.back().y() && points.back().y() == py)
				points.pop_back() ;

		points << QPointF(px,py) ;

		if(!(_flags & RSGRAPH_FLAGS_PAINT_STYLE_DOTS) && (i==values.size()-1))
			points << QPointF(px,py) ;

		last_px = px ;
		last_py = py ;

	}
}

qreal RSGraphWidget::valueToPixels(qreal val)
{
        if(_flags & RSGRAPH_FLAGS_LOG_SCALE_Y)
            return _graph_base + log(std::max((qreal)1.0,val))*_y_scale ;
        else
            return _graph_base + val*_y_scale ;
}

qreal RSGraphWidget::pixelsToValue(qreal val)
{
        if(_flags & RSGRAPH_FLAGS_LOG_SCALE_Y)
            return exp( (val - _graph_base) / _y_scale) ;
        else
            return (val - _graph_base)/_y_scale ;
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

  QPen newPen(color, lineStyle);
  newPen.setWidth(2.0f*_linewidthscale);
  _painter->setPen(newPen);
  _painter->drawPolyline(points.data(), points.size());
  _painter->setPen(oldPen);
}
void RSGraphWidget::paintDots(const QVector<QPointF>& points, QColor color)
{
  /* Save the current brush, plot the line, and restore the old brush */
  QPen oldPen = _painter->pen();
  _painter->setPen(QPen(color, oldPen.style()));
   QBrush oldBrush = _painter->brush();
  _painter->setBrush(QBrush(color));

  for(int i=0;i<points.size();++i)
	  _painter->drawEllipse(QRect(points[i].x()-2.5*_linewidthscale,points[i].y()-2.5*_linewidthscale,5*_linewidthscale,5*_linewidthscale)) ;

  _painter->setPen(oldPen);
  _painter->setBrush(oldBrush);
}
/** Paints selected total indicators on the graph. */
void RSGraphWidget::paintTotals()
{
    float FS = QFontMetricsF(font()).height();
    //float fact = FS/14.0 ;

  //int x = SCALE_WIDTH*fact + FS, y = 0;
  int rowHeight = FS;

#if !defined(Q_OS_MAC)
  /* On Mac, we don't need vertical spacing between the text rows. */
  rowHeight += 5;
#endif
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
void RSGraphWidget::paintScale1()
{
    float FS = QFontMetricsF(font()).height();
    float fact = FS/14.0 ;

    int top = _rec.y();
	int bottom = _rec.height() - _graph_base;
	qreal paintStep = (bottom - top) / 5;

	/* Draw the other marks in their correctly scaled locations */
	qreal scale;
	qreal pos;

	if(_source == NULL)
		return ;

	QString unit_name = _source->unitName() ;

	for (int i = 0; i < 5; i++)
	{
		pos = bottom - (i * paintStep) ;

		scale = pixelsToValue(_graph_base + i * paintStep);

        // If legend contains integers only the value should be rounded to the nearest integer
		if(_flags & RSGRAPH_FLAGS_LEGEND_INTEGER)
        {
            scale = (int)scale ;
            pos = bottom - (valueToPixels(scale) - _graph_base) ;
        }

		QString text = _source->displayValue(scale) ;

		_painter->setPen(SCALE_COLOR);
        _painter->drawText(QPointF(SCALE_WIDTH*fact - QFontMetricsF(font()).width(text) - 4*fact, pos+0.4*FS),  text);
		_painter->setPen(GRID_COLOR);
        _painter->drawLine(QPointF(SCALE_WIDTH*fact, pos),  QPointF(_rec.width(), pos));
	}

	/* Draw vertical separator */
    _painter->drawLine(SCALE_WIDTH*fact, top, SCALE_WIDTH*fact, bottom);
}

void RSGraphWidget::paintScale2()
{
    // draw time below the graph

    float FS = QFontMetricsF(font()).height();
    float fact = FS/14.0 ;

    //int bottom = _rec.height();
    static const int npix = 100*fact ;

    for(int i=_rec.width();i>SCALE_WIDTH*fact;i-=npix)
    {
        //qreal pos = bottom - FS;

        int seconds = (_rec.width()-i)/_time_scale ;	// pixels / (pixels per second) => seconds
        QString text = QString::number(seconds)+ " secs";

        _painter->setPen(SCALE_COLOR);
        _painter->drawText(QPointF(i, _rec.height()-0.5*FS),  text);
    }
}

void RSGraphWidget::wheelEvent(QWheelEvent *e)
{
    if(e->modifiers() & Qt::ShiftModifier)
	    if(e->delta() > 0)
		    _time_filter *= 1.1 ;
	    else
		    _time_filter /= 1.1 ;
    else if(e->modifiers() & Qt::ControlModifier)
        if(e->delta() > 0)
            _linewidthscale *= 1.2 ;
		else
            _linewidthscale /= 1.2 ;
    else
	    if(e->delta() > 0)
		    _time_scale *= 1.1 ;
	    else
		    _time_scale /= 1.1 ;

    update() ;
}

void RSGraphWidget::paintLegend()
{
  //int bottom = _rec.height();

  std::vector<float> vals ;

  if(_flags & RSGRAPH_FLAGS_LEGEND_CUMULATED)
	  _source->getCumulatedValues(vals) ;
  else
  {
	  std::vector<QPointF> cvals ;
	  _source->getCurrentValues(cvals) ;

      for(uint32_t i=0;i<cvals.size();++i)
          vals.push_back(cvals[i].y()) ;
  }

  int j=0;

    float FS = QFontMetricsF(font()).height();
    float fact = FS/14.0 ;

    for(uint i=0;i<vals.size();++i)
      if( _masked_entries.find(_source->displayName(i).toStdString()) == _masked_entries.end() )
      {
//          if( _rec.width() - (vals[i].x()-0)*_time_scale < SCALE_WIDTH*fact )
//              continue ;

          qreal paintStep = 4*fact+FS;
          qreal pos = 15*fact+j*paintStep;

          QString text = _source->legend(i,vals[i]) ;

          QPen oldPen = _painter->pen();

          QPen pen(getColor(_source->displayName(i).toStdString()), Qt::SolidLine) ;
		  pen.setWidth(_linewidthscale);

          _painter->setPen(pen);
          _painter->drawLine(QPointF(SCALE_WIDTH*fact+10.0*fact, pos+FS/3),  QPointF(SCALE_WIDTH*fact+30.0*fact, pos+FS/3));
          _painter->setPen(oldPen);

          _painter->setPen(SCALE_COLOR);
          _painter->drawText(QPointF(SCALE_WIDTH *fact+ 40*fact,pos + 0.5*FS), text) ;

          ++j ;
      }
}

