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
#include <QFontMetrics>
#include <time.h>

#include <QPainter>
#include <QStylePainter>
#include <QLayout>

#include <retroshare/rsgrouter.h>
#include <retroshare/rspeers.h>
#include "GlobalRouterStatistics.h"

#include "gui/settings/rsharesettings.h"

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
	
	m_bProcessSettings = false;

	_router_F->setWidget( _tst_CW = new GlobalRouterStatisticsWidget() ) ; 

	// load settings
    processSettings(true);
}

GlobalRouterStatistics::~GlobalRouterStatistics()
{

    // save settings
    processSettings(false);
}

void GlobalRouterStatistics::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("GlobalRouterStatistics"));

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

void GlobalRouterStatistics::updateDisplay()
{
	_tst_CW->updateContent() ;
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

void GlobalRouterStatisticsWidget::updateContent()
{
	std::vector<RsGRouter::GRouterRoutingCacheInfo> cache_infos ;
	RsGRouter::GRouterRoutingMatrixInfo matrix_info ;

	rsGRouter->getRoutingCacheInfo(cache_infos) ;
	rsGRouter->getRoutingMatrixInfo(matrix_info) ;

	// What do we need to draw?
	//
	// Routing matrix
	// 	Key         [][][][][][][][][][]
	//
	// ->	each [] shows a square (one per friend node) that is the routing probabilities for all connected friends
	// 	computed using the "computeRoutingProbabilitites()" method.
	//
	// Own key ids
	// 	key			service id			description
	//
	// Data items
	// 	Msg id				Local origin				Destination				Time           Status
	//
	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(this, 0, 0);
	setFixedHeight(maxHeight);

	QPainter painter(&tmppixmap);
	painter.initFrom(this);
	painter.setPen(QColor::fromRgb(0,0,0)) ;

	QFont times_f("Times") ;
	QFont monospace_f("Monospace") ;
	monospace_f.setStyleHint(QFont::TypeWriter) ;

	QFontMetrics fm_monospace(monospace_f) ;
	QFontMetrics fm_times(times_f) ;

	static const int cellx = fm_monospace.width(QString(" ")) ;
	static const int celly = fm_monospace.height() ;

	maxHeight = 500 ;

	// std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
	// draw...
	int ox=5,oy=5 ;
	
	
	painter.setFont(times_f) ;
	painter.drawText(ox,oy+celly,tr("Managed keys")+":" + QString::number(matrix_info.published_keys.size())) ; oy += celly*2 ;

	painter.setFont(monospace_f) ;
    for(std::map<Sha1CheckSum,RsGRouter::GRouterPublishedKeyInfo>::const_iterator it(matrix_info.published_keys.begin());it!=matrix_info.published_keys.end();++it)
	{
		QString packet_string ;
        packet_string += QString::fromStdString(it->second.authentication_key.toStdString())  ;
		packet_string += tr(" : Service ID = ")+QString::number(it->second.service_id,16) ;
		packet_string += "  \""+QString::fromUtf8(it->second.description_string.c_str()) + "\"" ;

		painter.drawText(ox+2*cellx,oy+celly,packet_string ) ; oy += celly ;
	}
	oy += celly ;

	painter.setFont(times_f) ;
	painter.drawText(ox,oy+celly,tr("Pending packets")+":" + QString::number(cache_infos.size())) ; oy += celly*2 ;

	painter.setFont(monospace_f) ;

    static const QString data_status_string[4] = { "UNKOWN","PENDING","SENT","RECEIVED" } ;
    static const QString tunnel_status_string[3] = { "UNMANAGED", "REQUESTED","ACTIVE" } ;

    time_t now = time(NULL) ;
	std::map<QString, std::vector<QString> > tos ;

	for(uint32_t i=0;i<cache_infos.size();++i)
	{
		QString packet_string ;
		packet_string += QString("Id=")+QString::number(cache_infos[i].mid,16)  ;
        //packet_string += tr(" By ")+QString::fromStdString(cache_infos[i].local_origin.toStdString()) ;
        packet_string += tr(" Size: ")+QString::number(cache_infos[i].data_size) ;
        packet_string += tr(" Data status: ")+data_status_string[cache_infos[i].data_status % 4] ;
        packet_string += tr(" Tunnel status: ")+tunnel_status_string[cache_infos[i].tunnel_status % 3] ;
        packet_string += " " + tr("Received: %1 secs ago, Send: %2 secs ago, Tried: %3 secs ago")
                            .arg(now - cache_infos[i].routing_time)
                            .arg(now - cache_infos[i].last_sent_time)
                            .arg(now - cache_infos[i].last_tunnel_attempt_time);

		tos[ QString::fromStdString(cache_infos[i].destination.toStdString()) ].push_back(packet_string) ;
	}

	for(std::map<QString,std::vector<QString> >::const_iterator it(tos.begin());it!=tos.end();++it)
	{
		painter.drawText(ox+2*cellx,oy+celly,tr("To: ")+it->first ) ; oy += celly ;

		for(uint32_t i=0;i<it->second.size();++i)
		{
			painter.drawText(ox+4*cellx,oy+celly,it->second[i] ) ; 
			oy += celly ;
		}
	}
    QString prob_string ;
    painter.setFont(times_f) ;
    QString Q = tr("Routing matrix  (") ;

    painter.drawText(ox+0*cellx,oy+fm_times.height(),Q) ;

    // draw scale

    for(int i=0;i<100;++i)
    {
        painter.setPen(colorScale(i/100.0)) ;
        painter.drawLine(fm_times.width(Q)+i,oy+celly+2,fm_times.width(Q)+i,oy+2) ;
    }
    painter.setPen(QColor::fromRgb(0,0,0)) ;

    painter.drawText(ox+fm_times.width(Q) + 100,oy+celly,")") ;

    oy += celly ;
    oy += celly ;

    static const int MaxKeySize = 20 ;
    painter.setFont(monospace_f) ;

    for(std::map<GRouterKeyId,std::vector<float> >::const_iterator it(matrix_info.per_friend_probabilities.begin());it!=matrix_info.per_friend_probabilities.end();++it)
    {
        QString ids = QString::fromStdString(it->first.toStdString())+" : " ;
        painter.drawText(ox+2*cellx,oy+celly,ids) ;

        for(uint32_t i=0;i<matrix_info.friend_ids.size();++i)
            painter.fillRect(ox+i*cellx+fm_monospace.width(ids),oy,cellx,celly,colorScale(it->second[i])) ;

        oy += celly ;
    }

    oy += celly ;



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
	 updateContent();
}

