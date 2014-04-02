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

#include <retroshare/rsturtle.h>
#include <retroshare/rspeers.h>
#include "TurtleRouterStatistics.h"

static const int MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;

class TRHistogram
{
	public:
		TRHistogram(const std::vector<TurtleRequestDisplayInfo >& info) :_infos(info) {}

		QColor colorScale(float f)
		{
			if(f == 0)
				return QColor::fromHsv(0,0,192) ;
			else
				return QColor::fromHsv((int)((1.0-f)*280),200,255) ;
		}

		virtual void draw(QPainter *painter,int& ox,int& oy,const QString& title) 
		{
			static const int MaxTime = 61 ;
			static const int MaxDepth = 8 ;
			static const int cellx = 7 ;
			static const int celly = 12 ;

			int save_ox = ox ;
			painter->setPen(QColor::fromRgb(0,0,0)) ;
			painter->drawText(2+ox,celly+oy,title) ;
			oy+=2+2*celly ;

			if(_infos.empty())
				return ;

			ox += 10 ;
			std::map<RsPeerId,std::vector<int> > hits ;
			std::map<RsPeerId,std::vector<int> > depths ;
			std::map<RsPeerId,std::vector<int> >::iterator it ;

			int max_hits = 1;
			int max_depth = 1;

			for(uint32_t i=0;i<_infos.size();++i)
			{
				std::vector<int>& h(hits[_infos[i].source_peer_id]) ;
				std::vector<int>& g(depths[_infos[i].source_peer_id]) ;

				if(h.size() <= _infos[i].age)
					h.resize(MaxTime,0) ;

				if(g.empty())
					g.resize(MaxDepth,0) ;

				if(_infos[i].age < h.size())
				{
					h[_infos[i].age]++ ;
					if(h[_infos[i].age] > max_hits)
						max_hits = h[_infos[i].age] ;
				}
				if(_infos[i].depth < g.size())
				{
					g[_infos[i].depth]++ ;

					if(g[_infos[i].depth] > max_depth)
						max_depth = g[_infos[i].depth] ;
				}
			}

			int max_bi = std::max(max_hits,max_depth) ;
			int p=0 ;

			for(it=depths.begin();it!=depths.end();++it,++p)
				for(int i=0;i<MaxDepth;++i)
					painter->fillRect(ox+MaxTime*cellx+20+i*cellx,oy+p*celly,cellx,celly,colorScale(it->second[i]/(float)max_bi)) ;
			
			painter->setPen(QColor::fromRgb(0,0,0)) ;
			painter->drawRect(ox+MaxTime*cellx+20,oy,MaxDepth*cellx,p*celly) ;

			for(int i=0;i<MaxTime;i+=5)
				painter->drawText(ox+i*cellx,oy+(p+1)*celly+4,QString::number(i)) ;

			p=0 ;
			int great_total = 0 ;

			for(it=hits.begin();it!=hits.end();++it,++p)
			{
				int total = 0 ;

				for(int i=0;i<MaxTime;++i)
				{
					painter->fillRect(ox+i*cellx,oy+p*celly,cellx,celly,colorScale(it->second[i]/(float)max_bi)) ;
					total += it->second[i] ;
				}

				painter->setPen(QColor::fromRgb(0,0,0)) ;
				painter->drawText(ox+MaxDepth*cellx+30+(MaxTime+1)*cellx,oy+(p+1)*celly,TurtleRouterStatistics::getPeerName(it->first)) ;
				painter->drawText(ox+MaxDepth*cellx+30+(MaxTime+1)*cellx+120,oy+(p+1)*celly,"("+QString::number(total)+")") ;
				great_total += total ;
			}

			painter->drawRect(ox,oy,MaxTime*cellx,p*celly) ;

			for(int i=0;i<MaxTime;i+=5)
				painter->drawText(ox+i*cellx,oy+(p+1)*celly+4,QString::number(i)) ;
			for(int i=0;i<MaxDepth;i++)
				painter->drawText(ox+MaxTime*cellx+20+i*cellx,oy+(p+1)*celly+4,QString::number(i)) ;
			painter->setPen(QColor::fromRgb(255,130,80)) ;
			painter->drawText(ox+MaxDepth*cellx+30+(MaxTime+1)*cellx+120,oy+(p+1)*celly+4,"("+QString::number(great_total)+")");

			oy += (p+1)*celly+6 ;

			painter->setPen(QColor::fromRgb(0,0,0)) ;
			painter->drawText(ox,oy+celly,"("+QApplication::translate("TurtleRouterStatistics", "Age in seconds")+")");
			painter->drawText(ox+MaxTime*cellx+20,oy+celly,"("+QApplication::translate("TurtleRouterStatistics", "Depth")+")");

			painter->drawText(ox+MaxDepth*cellx+30+(MaxTime+1)*cellx+120,oy+celly,"("+QApplication::translate("TurtleRouterStatistics", "total")+")");

			oy += 3*celly ;

			// now, draw a scale

			int last_hts = -1 ;
			int cellid = 0 ;

			for(int i=0;i<=10;++i)
			{
				int hts = (int)(max_bi*i/10.0) ;

				if(hts > last_hts)
				{
					painter->fillRect(ox+cellid*(cellx+22),oy,cellx,celly,colorScale(i/10.0f)) ;
					painter->setPen(QColor::fromRgb(0,0,0)) ;
					painter->drawRect(ox+cellid*(cellx+22),oy,cellx,celly) ;
					painter->drawText(ox+cellid*(cellx+22)+cellx+4,oy+celly,QString::number(hts)) ;
					last_hts = hts ;
					++cellid ;
				}
			}

			oy += celly*2 ;

			ox = save_ox ;
		}

	private:
		const std::vector<TurtleRequestDisplayInfo>& _infos ;
};

TurtleRouterStatistics::TurtleRouterStatistics(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	_turtle = NULL ;

	_tunnel_statistics_F->setWidget( _tst_CW = new TurtleRouterStatisticsWidget() ) ; 
	_tunnel_statistics_F->setWidgetResizable(true);
	_tunnel_statistics_F->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_tunnel_statistics_F->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	_tunnel_statistics_F->viewport()->setBackgroundRole(QPalette::NoRole);
	_tunnel_statistics_F->setFrameStyle(QFrame::NoFrame);
	_tunnel_statistics_F->setFocusPolicy(Qt::NoFocus);
}

TurtleRouterStatistics::~TurtleRouterStatistics()
{
}

void TurtleRouterStatistics::updateDisplay()
{
	if(_turtle == NULL)
		return ;

	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<TurtleRequestDisplayInfo > search_reqs_info ;
	std::vector<TurtleRequestDisplayInfo > tunnel_reqs_info ;

	_turtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

	//updateTunnelRequests(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;
	_tst_CW->updateTunnelStatistics(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info,_turtle) ;
	_tst_CW->update();
}

QString TurtleRouterStatistics::getPeerName(const RsPeerId& peer_id)
{
	static std::map<RsPeerId, QString> names ;

	std::map<RsPeerId,QString>::const_iterator it = names.find(peer_id) ;

	if( it != names.end())
		return it->second ;
	else
        return (names[peer_id] = QString::fromStdString(peer_id.toStdString())) ;
}

TurtleRouterStatisticsWidget::TurtleRouterStatisticsWidget(QWidget *parent)
	: QWidget(parent)
{
	maxWidth = 200 ;
	maxHeight = 0 ;
}

void TurtleRouterStatisticsWidget::updateTunnelStatistics(const std::vector<std::vector<std::string> >& /*hashes_info*/,
																const std::vector<std::vector<std::string> >& /*tunnels_info*/,
																const std::vector<TurtleRequestDisplayInfo >& search_reqs_info, 
																const std::vector<TurtleRequestDisplayInfo >& tunnel_reqs_info,
																const RsTurtle *turtle)

{
	static const int cellx = 6 ;
	static const int celly = 10+4 ;

	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(this, 0, 0);
	setFixedHeight(maxHeight);

	QPainter painter(&tmppixmap);
	painter.initFrom(this);

	maxHeight = 500 ;

	// std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
	// draw...
	int ox=5,oy=5 ;

	TRHistogram(search_reqs_info).draw(&painter,ox,oy,tr("Search requests repartition") + ":") ;

	painter.setPen(QColor::fromRgb(70,70,70)) ;
	painter.drawLine(0,oy,maxWidth,oy) ;
	oy += celly ;

	TRHistogram(tunnel_reqs_info).draw(&painter,ox,oy,tr("Tunnel requests repartition") + ":") ;

	// now give information about turtle traffic.
	//
	TurtleTrafficStatisticsInfo info ;
	turtle->getTrafficStatistics(info) ;

	painter.setPen(QColor::fromRgb(70,70,70)) ;
	painter.drawLine(0,oy,maxWidth,oy) ;
	oy += celly ;

	painter.drawText(ox,oy+celly,tr("Turtle router traffic")+":") ; oy += celly*2 ;
	painter.drawText(ox+2*cellx,oy+celly,tr("Tunnel requests Up")+"\t: " + speedString(info.tr_up_Bps) ) ; oy += celly ;
	painter.drawText(ox+2*cellx,oy+celly,tr("Tunnel requests Dn")+"\t: " + speedString(info.tr_dn_Bps) ) ; oy += celly ;
	painter.drawText(ox+2*cellx,oy+celly,tr("Incoming file data")+"\t: " + speedString(info.data_dn_Bps) ) ; oy += celly ;
	painter.drawText(ox+2*cellx,oy+celly,tr("Outgoing file data")+"\t: " + speedString(info.data_up_Bps) ) ; oy += celly ;
	painter.drawText(ox+2*cellx,oy+celly,tr("Forwarded data    ")+"\t: " + speedString(info.unknown_updn_Bps) ) ; oy += celly ;

	QString prob_string ;

	for(uint i=0;i<info.forward_probabilities.size();++i)
		prob_string += QString::number(info.forward_probabilities[i],'g',2) + " (" + QString::number(i) + ") " ;

	painter.drawText(ox+2*cellx,oy+celly,tr("TR Forward probabilities")+"\t: " + prob_string ) ; 
	oy += celly ;
	oy += celly ;

	// update the pixmap
	//
	pixmap = tmppixmap;
	maxHeight = oy ;
}

QString TurtleRouterStatisticsWidget::speedString(float f)
{
	if(f < 1.0f) 
		return QString("0 B/s") ;
	if(f < 1024.0f)
		return QString::number((int)f)+" B/s" ;

	return QString::number(f/1024.0,'f',2) + " KB/s";
}

void TurtleRouterStatisticsWidget::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void TurtleRouterStatisticsWidget::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height() ;

	 QWidget::resizeEvent(event);
	 update();
}
