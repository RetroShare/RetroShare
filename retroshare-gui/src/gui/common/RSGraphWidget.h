/*******************************************************************************
 * gui/common/RSGraphWidget.h                                                  *
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

#pragma once

#include <map>
#include <set>

#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>

#include <stdint.h>

#define GRAPH_BASE     2  /** Position of the 0 of the scale, in count of the text height */
#define SCALE_WIDTH   75  /** Width of the scale */
#define MINUSER_SCALE 2000  /** 2000 users is the minimum scale */
#define SCROLL_STEP   4   /** Horizontal change on graph update */

#define BACK_COLOR    Qt::white
#define SCALE_COLOR   Qt::black
#define GRID_COLOR    Qt::lightGray
#define RSDHT_COLOR   Qt::magenta
#define ALLDHT_COLOR  Qt::yellow

struct ZeroInitFloat
{
	ZeroInitFloat() { v=0; }
	ZeroInitFloat(float f) { v=f; }

	float operator()() const { return v ; }
	float& operator()() { return v ; }

	float v ;
};

// This class provides a source value that the graph can retrieve on demand.
// In order to use your own source, derive from RSGraphSource and overload the value() method.
//
class RSGraphSource: public QObject
{
    Q_OBJECT

public:
    RSGraphSource();
    virtual ~RSGraphSource() ;

    void start() ;
    void stop() ;
    void clear() ;
    void reset() ;

    virtual QString unitName() const { return "" ; }// overload to give your own unit name (KB/s, Users, etc)

    int n_values() const ;

    // Might be overloaded in order to show a fancy digit number with adaptive units.
    // The default is to return v + unitName()
    virtual QString displayValue(float v) const ;

    // return the vector of last values up to date
    virtual void getCurrentValues(std::vector<QPointF>& vals) const ;

    // return the vector of cumulated values up to date
	virtual void getCumulatedValues(std::vector<float>& vals) const;

    // returns what to display in the legend. Derive this to show additional info.
    virtual QString legend(int i, float v, bool show_value=true) const ;

    // Returns the n^th interpolated value at the given time in floating point seconds backward.
    virtual void getDataPoints(int index, std::vector<QPointF>& pts, float filter_factor=0.0f) const ;

    // returns the name to give to the nth entry in the graph
    virtual QString displayName(int index) const ;

    // Sets the maximum time for keeping values. Units=seconds.
    void setCollectionTimeLimit(qint64 msecs) ;

    // Sets the time period for collecting new values. Units=milliseconds.
    void setCollectionTimePeriod(qint64 msecs) ;

    // Enables/disables time filtering of the data
    void setFiltering(bool b) { _filtering_enabled = b; }

    void setDigits(int d) { _digits = d ;}

protected slots:
    // Calls the internal source for a new data points; called by the timer. You might want to overload this
    // if the collection system needs it. Otherwise, the default method will call getValues()
    virtual void update() ;
	 void updateIfPossible() ;

protected:
    virtual void getValues(std::map<std::string,float>& values) const = 0 ;// overload this in your own class to fill in the values you want to display.

#ifdef TO_REMOVE
	void updateTotals();
#endif
    qint64 getTime() const ;						   // returns time in ms since RS has started

    // Storage of collected events. The string is any string used to represent the collected data.

    std::map<std::string, std::list<std::pair<qint64,float> > > _points ;
    std::map<std::string, ZeroInitFloat> _totals ;

    QTimer *_timer ;

    qint64 _time_limit_msecs ;
    qint64 _update_period_msecs ;
    qint64 _time_orig_msecs ;
    int _digits ;
    bool _filtering_enabled ;
};

class RSGraphWidget: public QFrame
{
	Q_OBJECT

public:
	static const uint32_t RSGRAPH_FLAGS_AUTO_SCALE_Y    	= 0x0001 ;// automatically adjust Y scale
	static const uint32_t RSGRAPH_FLAGS_LOG_SCALE_Y     	= 0x0002 ;// log scale in Y
	static const uint32_t RSGRAPH_FLAGS_ALWAYS_COLLECT  	= 0x0004 ;// keep collecting while not displayed
	static const uint32_t RSGRAPH_FLAGS_PAINT_STYLE_PLAIN	= 0x0008 ;// use plain / line drawing style
	static const uint32_t RSGRAPH_FLAGS_SHOW_LEGEND         = 0x0010 ;// show legend in the graph
	static const uint32_t RSGRAPH_FLAGS_PAINT_STYLE_FLAT    = 0x0020 ;// do not interpolate, and draw flat colored boxes
	static const uint32_t RSGRAPH_FLAGS_LEGEND_CUMULATED    = 0x0040 ;// show the total in the legend rather than current values
	static const uint32_t RSGRAPH_FLAGS_PAINT_STYLE_DOTS 	= 0x0080 ;// use dots
	static const uint32_t RSGRAPH_FLAGS_LEGEND_INTEGER   	= 0x0100 ;// use integer number in the legend, and move the lines to match integers

	/** Bandwidth graph style. */
	enum GraphStyle 
	{
		SolidLine = 0,  /**< Plot bandwidth as solid lines. */
		AreaGraph = 1   /**< Plot bandwidth as alpha blended area graphs. */
	};

	/** Default Constructor */
	RSGraphWidget(QWidget *parent = 0);
	/** Default Destructor */
	~RSGraphWidget();

	// sets the update interval period.
	//
	void setTimerPeriod(int miliseconds) ;				
	void setSource(RSGraphSource *gs) ;
	void setTimeScale(float pixels_per_second) ;

	/** Add data points. */
	//void addPoints(qreal rsDHT, qreal allDHT);
	/** Clears the graph. */
	void resetGraph();
	/** Toggles display of data counters. */
	//void setShowCounters(bool showRSDHT, bool showALLDHT);

	void setShowEntry(uint32_t entry, bool show) ;
	void setCurvesOpacity(float f) ;

	void setFiltering(bool b) ;

	void setFlags(uint32_t flag) { _flags |= flag ; }
	void resetFlags(uint32_t flag) { _flags &= ~flag ; }
protected:
	/** Overloaded QWidget::paintEvent() */
	void paintEvent(QPaintEvent *event);

//QSize QFrame::sizeHint() const;
//	virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

protected slots:
	void updateIfPossible() ;

	virtual void wheelEvent(QWheelEvent *e);
private:
	/** Gets the width of the desktop, the max # of points. */
	int getNumPoints();

	/** Paints an integral and an outline of that integral for each data set
		 * (rsdht and/or alldht) that is to be displayed. */
	void paintData();
	/** Paints the rsdht/alldht totals. */
	void paintTotals();
	/** Paints the scale in the graph. */
	void paintLegend();
	/** Paints the scale in the graph. */
	void paintScale1();
	void paintScale2();

	QColor getColor(const std::string &name) ;

	/** Returns a formatted string representation of total. */
	QString totalToStr(qreal total);
	/** Returns a list of points on the bandwidth graph based on the supplied set
		 * of rsdht or alldht values. */
	void pointsFromData(const std::vector<QPointF>& values, QVector<QPointF> &points ) ;

	/** Paints a line with the data in <b>points</b>. */
	void paintLine(const QVector<QPointF>& points, QColor color, Qt::PenStyle lineStyle = Qt::SolidLine);

    /** Paint a series of large dots **/
	void paintDots(const QVector<QPointF>& points, QColor color);

	/** Paints an integral using the supplied data. */
	void paintIntegral(const QVector<QPointF>& points, QColor color, qreal alpha = 1.0);

	/** A QPainter object that handles drawing the various graph elements. */
	QPainter* _painter;
	/** The current dimensions of the graph. */
	QRect _rec;
	/** The maximum data value plotted. */
	qreal _maxValue;
	/** The maximum number of points to store. */
	qreal _y_scale ;
	qreal _opacity ;
    qreal _graph_base;

	qreal pixelsToValue(qreal) ;
	qreal valueToPixels(qreal) ;
	int _maxPoints;

	std::set<std::string> _masked_entries ;

	qreal _time_scale ; // horizontal scale in pixels per sec.
        qreal _time_filter ; // time filter. Goes from 0 to infinity. Will be converted into 1-1/(1+f)
        float _linewidthscale ;

	/** Show the respective lines and counters. */
	//bool _showRSDHT;
	//bool _showALLDHT;

	uint32_t _flags ;
	QTimer *_timer ;

	RSGraphSource *_source ;
};

