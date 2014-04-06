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
#include <QLayout>

#include <retroshare/rsgrouter.h>
#include <retroshare/rspeers.h>
#include <grouter/p3grouter.h>
#include "GlobalRouterStatistics.h"

static const int MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;

static QColor colorScale(float f)
{
	if(f == 0)
		return QColor::fromHsv(0,0,192) ;
	else
		return QColor::fromHsv((int)((1.0-f)*280),200,255) ;
}

GlobalRouterStatistics::GlobalRouterStatistics(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	_router_F->setWidget( _tst_CW = new GlobalRouterStatisticsWidget() ) ; 
}

GlobalRouterStatistics::~GlobalRouterStatistics()
{
}

void GlobalRouterStatistics::setGlobalRouter(const RsGRouter *grouter)
{
    _grouter = const_cast<RsGRouter*>(grouter);
    updateDisplay() ;
}
void GlobalRouterStatistics::updateDisplay()
{
    _tst_CW->updateContent(_grouter) ;
}

QString GlobalRouterStatistics::getPeerName(const RsPeerId &peer_id)
{
	static std::map<RsPeerId, QString> names ;

	std::map<RsPeerId,QString>::const_iterator it = names.find(peer_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsPeerDetails detail ;
		if(!rsPeers->getPeerDetails(peer_id,detail))
			return tr("Unknown Peer");

		return (names[peer_id] = QString::fromUtf8(detail.name.c_str())) ;
	}
}

GlobalRouterStatisticsWidget::GlobalRouterStatisticsWidget(QWidget *parent)
	: QWidget(parent)
{
	maxWidth = 400 ;
	maxHeight = 0 ;
}

void GlobalRouterStatisticsWidget::updateContent(RsGRouter *grouter)
{
	std::vector<RsGRouter::GRouterRoutingCacheInfo> cache_infos ;
	RsGRouter::GRouterRoutingMatrixInfo matrix_info ;

    grouter->getRoutingCacheInfo(cache_infos) ;
    grouter->getRoutingMatrixInfo(matrix_info) ;

	// What do we need to draw?
	//
	// Routing matrix
	// 	Key         [][][][][][][][][][]
	//
	// ->	each [] shows a square (one per friend location) that is the routing probabilities for all connected friends
	// 	computed using the "computeRoutingProbabilitites()" method.
	//
	// Own key ids
	// 	key			service id			description
	//
	// Data items
	// 	Msg id				Local origin				Destination				Time           Status
	//
	static const int cellx = 6 ;
	static const int celly = 10+4 ;

	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(this, 0, 0);
	setFixedHeight(maxHeight);

	QPainter painter(&tmppixmap);
	painter.initFrom(this);
	painter.setPen(QColor::fromRgb(0,0,0)) ;

	maxHeight = 500 ;

	// std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
	// draw...
	int ox=5,oy=5 ;

	painter.drawText(ox,oy+celly,tr("Pending packets")+":" + QString::number(cache_infos.size())) ; oy += celly*2 ;

	for(uint32_t i=0;i<cache_infos.size();++i)
	{
		QString packet_string ;
		packet_string += QString::number(cache_infos[i].mid,16)  ;
		packet_string += tr(" by ")+QString::fromStdString(cache_infos[i].local_origin.toStdString()) ;
		packet_string += tr(" to ")+QString::fromStdString(cache_infos[i].destination.toStdString()) ;
		packet_string += tr(" Status ")+QString::number(cache_infos[i].status) ;

		painter.drawText(ox+2*cellx,oy+celly,packet_string ) ; oy += celly ;
	}

	oy += celly ;

	painter.drawText(ox,oy+celly,tr("Managed keys")+":" + QString::number(matrix_info.published_keys.size())) ; oy += celly*2 ;

	for(std::map<GRouterKeyId,RsGRouter::GRouterPublishedKeyInfo>::const_iterator it(matrix_info.published_keys.begin());it!=matrix_info.published_keys.end();++it)
	{
		QString packet_string ;
		packet_string += QString::fromStdString(it->first.toStdString())  ;
		packet_string += tr(" : Service ID = ")+QString::number(it->second.service_id,16) ;
		packet_string += "  \""+QString::fromUtf8(it->second.description_string.c_str()) + "\"" ;

		painter.drawText(ox+2*cellx,oy+celly,packet_string ) ; oy += celly ;
	}
	oy += celly ;

	QString prob_string ;
	
	painter.drawText(ox+0*cellx,oy+celly,tr("Routing matrix  (")) ; 

	// draw scale
	
	for(int i=0;i<100;++i)
	{
		painter.setPen(colorScale(i/100.0)) ;
		painter.drawLine(ox+120+i,oy+celly+2,ox+120+i,oy+2) ;
	}
	painter.setPen(QColor::fromRgb(0,0,0)) ;

	painter.drawText(ox+230,oy+celly,")") ; 
	
	oy += celly ;
	oy += celly ;

	static const int MaxKeySize = 20 ;

	for(std::map<GRouterKeyId,std::vector<float> >::const_iterator it(matrix_info.per_friend_probabilities.begin());it!=matrix_info.per_friend_probabilities.end();++it)
	{
		painter.drawText(ox+2*cellx,oy+celly,QString::fromStdString(it->first.toStdString())+" : ") ; 

		for(uint32_t i=0;i<matrix_info.friend_ids.size();++i)
 			painter.fillRect(ox+(MaxKeySize + i)*cellx+200,oy,cellx,celly,colorScale(it->second[i])) ;
		
		oy += celly ;
	}

	oy += celly ;
	oy += celly ;

	// update the pixmap
	//
	pixmap = tmppixmap;
	maxHeight = oy ;
}

QString GlobalRouterStatisticsWidget::speedString(float f)
{
	if(f < 1.0f) 
		return QString("0 B/s") ;
	if(f < 1024.0f)
		return QString::number((int)f)+" B/s" ;

	return QString::number(f/1024.0,'f',2) + " KB/s";
}

void GlobalRouterStatisticsWidget::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void GlobalRouterStatisticsWidget::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height() ;

	 QWidget::resizeEvent(event);
//	 updateContent();
}

