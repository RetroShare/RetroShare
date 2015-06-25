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
    float size = QFontMetricsF(font()).height() ;
    float fact = size/14.0 ;

    maxWidth = 400*fact ;
	maxHeight = 0 ;
}

void GlobalRouterStatisticsWidget::updateContent()
{
    std::vector<RsGRouter::GRouterRoutingCacheInfo> cache_infos ;
    RsGRouter::GRouterRoutingMatrixInfo matrix_info ;

    rsGRouter->getRoutingCacheInfo(cache_infos) ;
    rsGRouter->getRoutingMatrixInfo(matrix_info) ;

    float size = QFontMetricsF(font()).height() ;
    float fact = size/14.0 ;

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
	tmppixmap.fill(Qt::transparent);
    setFixedHeight(maxHeight);

    QPainter painter(&tmppixmap);
    painter.initFrom(this);
    painter.setPen(QColor::fromRgb(0,0,0)) ;

    QFont times_f(font());//"Times") ;
    QFont monospace_f(font());//"Monospace") ;
    monospace_f.setStyleHint(QFont::TypeWriter) ;

    QFontMetricsF fm_monospace(monospace_f) ;
    QFontMetricsF fm_times(times_f) ;

    static const int cellx = fm_monospace.width(QString(" ")) ;
    static const int celly = fm_monospace.height() ;

    maxHeight = 500*fact ;

    // std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
    // draw...
    int ox=5*fact,oy=5*fact ;


    painter.setFont(times_f) ;
    painter.drawText(ox,oy+celly,tr("Managed keys")+":" + QString::number(matrix_info.published_keys.size())) ; oy += celly*2 ;

    painter.setFont(monospace_f) ;
    for(std::map<Sha1CheckSum,RsGRouter::GRouterPublishedKeyInfo>::const_iterator it(matrix_info.published_keys.begin());it!=matrix_info.published_keys.end();++it)
    {
        QString packet_string ;
        packet_string += QString::fromStdString(it->second.authentication_key.toStdString())  ;
        packet_string += tr(" : Service ID =")+" "+QString::number(it->second.service_id,16) ;
        packet_string += "  \""+QString::fromUtf8(it->second.description_string.c_str()) + "\"" ;

        painter.drawText(ox+2*cellx,oy+celly,packet_string ) ; oy += celly ;
    }
    oy += celly ;

    painter.setFont(times_f) ;
    painter.drawText(ox,oy+celly,tr("Pending packets")+":" + QString::number(cache_infos.size())) ; oy += celly*2 ;

    painter.setFont(monospace_f) ;

    static const QString data_status_string[6] = { "Unkown","Pending","Sent","Receipt OK","Ongoing","Done" } ;
    static const QString tunnel_status_string[4] = { "Unmanaged", "Pending", "Ready" } ;

    time_t now = time(NULL) ;
    std::map<QString, std::vector<QString> > tos ;
    static const int nb_fields = 8 ;

    static const QString fname[nb_fields] = {
            tr("Id"),
            tr("Destination"),
            tr("Data status"),
            tr("Tunnel status"),
            tr("Data size"),
            tr("Data hash"),
            tr("Received"),
            tr("Send") } ;

    std::vector<int> max_column_width(nb_fields,0) ;

    for(int i=0;i<cache_infos.size();++i)
    {
        std::vector<QString> strings;
        strings.push_back( QString::number(cache_infos[i].mid,16) ) ;
        strings.push_back( QString::fromStdString(cache_infos[i].destination.toStdString()) ) ;

        //for(std::set<RsPeerId>::const_iterator it(cache_infos[i].local_origin.begin());it!=cache_infos[i].local_origin.end();++it)
        //	packet_string += QString::fromStdString((*it).toStdString()) + " - ";

        strings.push_back( data_status_string[cache_infos[i].data_status % 6]) ;
        strings.push_back( tunnel_status_string[cache_infos[i].tunnel_status % 3]) ;
        strings.push_back( QString::number(cache_infos[i].data_size) );
        strings.push_back( QString::fromStdString(cache_infos[i].item_hash.toStdString())) ;
        strings.push_back( QString::number(now - cache_infos[i].routing_time)) ;
        strings.push_back( QString::number(now - cache_infos[i].last_sent_time)) ;

        tos[ strings[0] ] = strings ;

        // now compute max column sizes

        for(int j=0;j<nb_fields;++j)
            max_column_width[j] = std::max(max_column_width[j],(int)(fm_monospace.boundingRect(strings[j]).width()+cellx+2*fact)) ;
    }

    // compute cumulated sizes

    std::vector<int> cumulated_sizes(nb_fields+1,0) ;

    for(int i=1;i<=nb_fields;++i)
    {
        cumulated_sizes[i] = std::max(max_column_width[i-1],(int)(fm_monospace.boundingRect(fname[i-1]).width()+cellx+2*fact)) ;
        cumulated_sizes[i] += cumulated_sizes[i-1] ;
    }

    // Now draw the matrix

    for(int i=0;i<nb_fields;++i)
        painter.drawText(ox+cumulated_sizes[i]+2,oy+celly,fname[i]) ;

    painter.drawLine(ox,oy,ox+cumulated_sizes.back(),oy) ;

    int top_matrix_oy = oy ;
    oy += celly +2*fact;

    painter.drawLine(ox,oy,ox+cumulated_sizes.back(),oy) ;

    for(std::map<QString,std::vector<QString> >::const_iterator it(tos.begin());it!=tos.end();++it)
    {
        for(int i=0;i<it->second.size();++i)
            painter.drawText(ox+cumulated_sizes[i]+2,oy+celly,it->second[i] ) ;

        oy += celly ;
    }

    oy += 2*fact;

    for(int i=0;i<=nb_fields;++i)
        painter.drawLine(ox+cumulated_sizes[i],top_matrix_oy,ox+cumulated_sizes[i],oy) ;

    painter.drawLine(ox,oy,ox+cumulated_sizes.back(),oy) ;
    oy += celly ;

    QString prob_string ;
    painter.setFont(times_f) ;
    QString Q = tr("Routing matrix  (") ;

    painter.drawText(ox+0*cellx,oy+fm_times.height(),Q) ;

    // draw scale

    for(int i=0;i<100*fact;++i)
    {
        painter.setPen(colorScale(i/100.0/fact)) ;
        painter.drawLine(ox+fm_times.width(Q)+i,oy+fm_times.height()*0.5,ox+fm_times.width(Q)+i,oy+fm_times.height()) ;
    }
    painter.setPen(QColor::fromRgb(0,0,0)) ;

    painter.drawText(ox+fm_times.width(Q) + 102*fact,oy+celly,")") ;

    oy += celly ;
    oy += celly ;

    static const int MaxKeySize = 20*fact ;
    painter.setFont(monospace_f) ;

    for(std::map<GRouterKeyId,std::vector<float> >::const_iterator it(matrix_info.per_friend_probabilities.begin());it!=matrix_info.per_friend_probabilities.end();++it)
    {
        bool is_null = true ;

        for(uint32_t i=0;i<matrix_info.friend_ids.size();++i)
            if(it->second[i] > 0.0)
                is_null = false ;

        if(!is_null)
        {
            QString ids = QString::fromStdString(it->first.toStdString())+" : " ;
            painter.drawText(ox+2*cellx,oy+celly,ids) ;

            for(uint32_t i=0;i<matrix_info.friend_ids.size();++i)
                painter.fillRect(ox+i*cellx+fm_monospace.width(ids),oy,cellx,celly,colorScale(it->second[i])) ;

            oy += celly ;
        }
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
	 updateContent();
}

