/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 20011, RetroShare Team
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

#include <iostream>
#include <QTimer>
#include <QObject>

#include <QPainter>
#include <QStylePainter>

#include <retroshare/rsrtt.h>
#include <retroshare/rspeers.h>
#include "RttStatistics.h"
#include "time.h"

#include "gui/settings/rsharesettings.h"

#define PLOT_HEIGHT	100
#define PLOT_WIDTH	500

#define MAX_DISPLAY_PERIOD	300

double convertDtToPixels(double refTs, double minTs, double ts)
{
	double dt = refTs - ts;
	double maxdt = refTs - minTs;
	double pix = PLOT_WIDTH - dt / maxdt * PLOT_WIDTH;
	return pix;
}


double convertRttToPixels(double maxRTT, double rtt)
{
	double pix = rtt / maxRTT * PLOT_HEIGHT;
	return PLOT_HEIGHT - pix;
}

class RttPlot
{
	public:
		RttPlot(const std::map<std::string, std::list<RsRttPongResult> > &info, 
			double refTS, double maxRTT, double minTS, double maxTS)
			:mInfo(info), mRefTS(refTS), mMaxRTT(maxRTT), mMinTS(minTS), mMaxTS(maxTS) {}


		QColor colorScale(float f)
		{
			if(f == 0)
				return QColor::fromHsv(0,0,192) ;
			else
				return QColor::fromHsv((int)((1.0-f)*280),200,255) ;
		}

		virtual void draw(QPainter *painter,int& ox,int& oy,const QString& title) 
		{
			static const int cellx = 7 ;
			static const int celly = 12 ;

			//int save_ox = ox ;
			painter->setPen(QColor::fromRgb(0,0,0)) ;
			painter->drawText(2+ox,celly+oy,title) ;
			oy+=2+2*celly ;

			painter->drawRect(ox, oy, PLOT_WIDTH, PLOT_HEIGHT);

			if(mInfo.empty())
				return ;

			double maxdt = mRefTS - mMinTS;
			if (maxdt > MAX_DISPLAY_PERIOD)
			{
				mMinTS = mRefTS - MAX_DISPLAY_PERIOD;
			}

			/* draw a different line for each peer */
			std::map<std::string, std::list<RsRttPongResult> >::const_iterator mit;
			int i = 0;
			int nLines = mInfo.size();
			for(mit = mInfo.begin(); mit != mInfo.end(); mit++, i++)
			{
				QPainterPath path;
				std::list<RsRttPongResult>::const_iterator it = mit->second.begin();
				if (it != mit->second.end())
				{
					double x = convertDtToPixels(mRefTS, mMinTS, it->mTS);
					double y = convertRttToPixels(mMaxRTT, it->mRTT);
					path.moveTo(ox + x, oy + y);
					it++;
				}
				
				for(; it != mit->second.end(); it++)
				{
					/* skip old elements */
					if (it->mTS < mMinTS)
					{
						continue;
					}

					double x = convertDtToPixels(mRefTS, mMinTS, it->mTS);
					double y = convertRttToPixels(mMaxRTT, it->mRTT);

					path.lineTo(ox + x, oy + y);
				}

				/* draw line */
				painter->setPen(QColor::fromRgb(((255.0 * i) / (nLines-1)),0, 255 - (255.0 * i) / (nLines-1))) ;
				painter->drawPath(path);

				/* draw name */
			}

			painter->setPen(QColor::fromRgb(0,0,0)) ;
			painter->drawText(ox+PLOT_WIDTH + cellx ,oy + celly / 2, QString::number(mMaxRTT)+" "+QObject::tr("secs")) ;
			oy += PLOT_HEIGHT / 2;
			painter->drawText(ox+PLOT_WIDTH + cellx ,oy + celly / 2, QString::number(mMaxRTT / 2.0)+" "+QObject::tr("secs")) ;
			oy += PLOT_HEIGHT / 2;
			painter->drawText(ox+PLOT_WIDTH + cellx ,oy + celly / 2, QString::number(0.0)+" "+QObject::tr("secs")) ;
			oy += celly;
			painter->drawText(ox ,oy, QObject::tr("Old"));
			painter->drawText(ox + PLOT_WIDTH - cellx ,oy, QObject::tr("Now"));
			oy += celly;

			// Now do names.
			i = 0;
			for(mit = mInfo.begin(); mit != mInfo.end(); mit++, i++)
			{
				painter->fillRect(ox,oy,cellx,celly,
					QColor::fromRgb(((255.0 * i) / (nLines-1)),0, 255 - (255.0 * i) / (nLines-1))) ;

				painter->setPen(QColor::fromRgb(0,0,0)) ;
				painter->drawRect(ox,oy,cellx,celly) ;
				painter->drawText(ox + cellx + 4,oy + celly / 2,RttStatistics::getPeerName(mit->first));

				oy += 2 * celly;
			}
		}

	private:
		const std::map<std::string, std::list<RsRttPongResult> > &mInfo;
		double mRefTS;
		double mMaxRTT;
		double mMinTS;
		double mMaxTS;

};

RttStatistics::RttStatistics(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	m_bProcessSettings = false;

	_tunnel_statistics_F->setWidget( _tst_CW = new RttStatisticsWidget() ) ; 
	_tunnel_statistics_F->setWidgetResizable(true);
	_tunnel_statistics_F->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_tunnel_statistics_F->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	_tunnel_statistics_F->viewport()->setBackgroundRole(QPalette::NoRole);
	_tunnel_statistics_F->setFrameStyle(QFrame::NoFrame);
	_tunnel_statistics_F->setFocusPolicy(Qt::NoFocus);
	
	// load settings
    processSettings(true);
}

RttStatistics::~RttStatistics()
{

    // save settings
    processSettings(false);
}

void RttStatistics::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("RttStatistics"));

    if (bLoad) {
        // load settings

        // state of splitter
        //splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        //Settings->setValue("Splitter", splitter->saveState());

    }

    Settings->endGroup();
    
    m_bProcessSettings = false;

}


void RttStatistics::updateDisplay()
{
	std::map<std::string, std::list<RsRttPongResult> > info;

	if (!rsRtt)
	{
		return;
	}

	std::list<std::string> idList;
	std::list<std::string>::iterator it;

	rsPeers->getOnlineList(idList);

	time_t now = time(NULL);
	time_t minTS = now;
	time_t maxTS = 0;
	double maxRTT = 0;
	
	for(it = idList.begin(); it != idList.end(); it++)
	{
		std::list<RsRttPongResult> results;
		std::list<RsRttPongResult>::iterator rit;

#define MAX_RESULTS	60
		rsRtt->getPongResults(*it, MAX_RESULTS, results);

		for(rit = results.begin(); rit != results.end(); rit++)
		{
			/* only want maxRTT to include plotted bit */
			double dt = now - rit->mTS;
			if (dt < MAX_DISPLAY_PERIOD)
			{
				if (maxRTT < rit->mRTT)
				{
					maxRTT = rit->mRTT;
				}
			}
			if (minTS > rit->mTS)
			{
				minTS = rit->mTS;
			}
			if (maxTS < rit->mTS)
			{
				maxTS = rit->mTS;
			}
		}

		info[*it] = results;
	}


	_tst_CW->updateRttStatistics(info, maxRTT, minTS, maxTS);
	_tst_CW->update();
}

QString RttStatistics::getPeerName(const std::string& peer_id)
{
	static std::map<std::string, QString> names ;

	std::map<std::string,QString>::const_iterator it = names.find(peer_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsPeerDetails detail ;
		if(!rsPeers->getPeerDetails(peer_id,detail))
			return "unknown peer";

		return (names[peer_id] = QString::fromUtf8(detail.name.c_str())) ;
	}
}

RttStatisticsWidget::RttStatisticsWidget(QWidget *parent)
	: QWidget(parent)
{
	maxWidth = 200 ;
	maxHeight = 0 ;
}

void RttStatisticsWidget::updateRttStatistics(const std::map<std::string, std::list<RsRttPongResult> >& info, 
		double maxRTT, double minTS, double maxTS)
{
	//static const int cellx = 6 ;
	//static const int celly = 10+4 ;

	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(this, 0, 0);
	setFixedHeight(maxHeight);

	QPainter painter(&tmppixmap);
	painter.initFrom(this);

	maxHeight = 500 ;

	//std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
	// draw...
	int ox=5,oy=5 ;

	double refTS = time(NULL);

	//painter.setPen(QColor::fromRgb(70,70,70)) ;
	//painter.drawLine(0,oy,maxWidth,oy) ;
	//oy += celly ;
	//painter.setPen(QColor::fromRgb(0,0,0)) ;

	// round up RTT to nearest	
	double roundedRTT = maxRTT;
	if (maxRTT < 0.15)
	{
		roundedRTT = 0.2;
	}
	else if (maxRTT < 0.4)
	{
		roundedRTT = 0.5;
	}
	else if (maxRTT < 0.8)
	{
		roundedRTT = 1.0;
	}
	else if (maxRTT < 1.8)
	{
		roundedRTT = 2.0;
	}
	else if (maxRTT < 4.5)
	{
		roundedRTT = 5.0;
	}

	RttPlot(info, refTS, roundedRTT, minTS, maxTS).draw(&painter,ox,oy,QObject::tr("Round Trip Time:")) ;

	// update the pixmap
	pixmap = tmppixmap;
	maxHeight = oy; // + PLOT_HEIGHT * 2;
}

QString RttStatisticsWidget::speedString(float f)
{
	if(f < 1.0f) 
		return QString("0 B/s") ;
	if(f < 1024.0f)
		return QString::number((int)f)+" B/s" ;

	return QString::number(f/1024.0,'f',2) + " KB/s";
}

void RttStatisticsWidget::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void RttStatisticsWidget::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height() ;

	 QWidget::resizeEvent(event);
	 update();
}


