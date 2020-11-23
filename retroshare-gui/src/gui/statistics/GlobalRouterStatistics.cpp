/*******************************************************************************
 * gui/statistics/GlobalRouterStatistics.cpp                                   *
 *                                                                             *
 * Copyright (c) 2011 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <QDateTime>
#include <iostream>
#include <QTimer>
#include <QObject>
#include <QFontMetrics>
#include <QWheelEvent>
#include <time.h>

#include <QMenu>
#include <QPainter>
#include <QStylePainter>
#include <QLayout>
#include <QHeaderView>

#include <retroshare/rsgrouter.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include "GlobalRouterStatistics.h"

#include "gui/Identity/IdDetailsDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "util/DateTime.h"
#include "util/QtVersion.h"
#include "util/misc.h"

#define COL_ID                  0
#define COL_NICKNAME            1
#define COL_DESTINATION         2
#define COL_DATASTATUS          3
#define COL_TUNNELSTATUS        4
#define COL_DATASIZE            5
#define COL_DATAHASH            6
#define COL_RECEIVED			7
#define COL_SEND				8
#define COL_DUPLICATION_FACTOR  9
#define COL_RECEIVEDTIME	    10
#define COL_SENDTIME			11

static const int PARTIAL_VIEW_SIZE           = 9 ;
//static const int MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;

static QColor colorScale(float f)
{
	if(f == 0)
		return QColor::fromHsv(0,0,192) ;
	else
		return QColor::fromHsv((int)((1.0-f)*280),200,255) ;
}

GlobalRouterStatistics::GlobalRouterStatistics(QWidget *parent)
    : RsAutoUpdatePage(4000,parent)
{
	setupUi(this) ;
	
	m_bProcessSettings = false;

	_router_F->setWidget( _tst_CW = new GlobalRouterStatisticsWidget() ) ; 
	
	  /* Set header resize modes and initial section sizes Uploads TreeView*/
		QHeaderView_setSectionResizeMode(treeWidget->header(), QHeaderView::ResizeToContents);
		
		connect(treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(CustomPopupMenu(QPoint)));


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

void GlobalRouterStatistics::CustomPopupMenu( QPoint )
{
	QMenu contextMnu( this );
	
	QTreeWidgetItem *item = treeWidget->currentItem();
	if (item) {
    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/info16.png"), tr("Details"), this, SLOT(personDetails()));

  }

	contextMnu.exec(QCursor::pos());
}

void GlobalRouterStatistics::updateDisplay()
{
	_tst_CW->updateContent() ;
	updateContent();
	
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

void GlobalRouterStatistics::updateContent()
{
    std::vector<RsGRouter::GRouterRoutingCacheInfo> cache_infos ;
    rsGRouter->getRoutingCacheInfo(cache_infos) ;

    treeWidget->clear();

    static const QString data_status_string[6] = { "Unkown","Pending","Sent","Receipt OK","Ongoing","Done" } ;
    static const QString tunnel_status_string[4] = { "Unmanaged", "Pending", "Ready" } ;

    time_t now = time(NULL) ;
    
    groupBox->setTitle(tr("Pending packets")+": " + QString::number(cache_infos.size()) );

    for(uint32_t i=0;i<cache_infos.size();++i)
    {
        //QTreeWidgetItem *item = new QTreeWidgetItem();
		GxsIdRSTreeWidgetItem *item = new GxsIdRSTreeWidgetItem(NULL,GxsIdDetails::ICON_TYPE_AVATAR) ;
        treeWidget->addTopLevelItem(item);
        
        RsIdentityDetails details ;
        rsIdentity->getIdDetails(cache_infos[i].destination,details);
        QString nicknames = QString::fromUtf8(details.mNickname.c_str());
        
        if(nicknames.isEmpty())
          nicknames = tr("Unknown");
	  
	    QDateTime routingtime;
		routingtime.setTime_t(cache_infos[i].routing_time);
		QDateTime senttime;
		senttime.setTime_t(cache_infos[i].last_sent_time);
	  
		item -> setId(cache_infos[i].destination,COL_NICKNAME, false) ;
        item -> setData(COL_ID,           Qt::DisplayRole, QString::number(cache_infos[i].mid,16).rightJustified(16,'0'));
        item -> setData(COL_NICKNAME,     Qt::DisplayRole, nicknames ) ;
        item -> setData(COL_DESTINATION,  Qt::DisplayRole, QString::fromStdString(cache_infos[i].destination.toStdString()));
        item -> setData(COL_DATASTATUS,   Qt::DisplayRole, data_status_string[cache_infos[i].data_status % 6]);
        item -> setData(COL_TUNNELSTATUS, Qt::DisplayRole, tunnel_status_string[cache_infos[i].tunnel_status % 3]);
        item -> setData(COL_DATASIZE,     Qt::DisplayRole, misc::friendlyUnit(cache_infos[i].data_size));
        item -> setData(COL_DATAHASH,     Qt::DisplayRole, QString::fromStdString(cache_infos[i].item_hash.toStdString()));
		item -> setData(COL_RECEIVED, 	  Qt::DisplayRole, DateTime::formatDateTime(routingtime));
        item -> setData(COL_SEND,         Qt::DisplayRole, DateTime::formatDateTime(senttime));
		item -> setData(COL_DUPLICATION_FACTOR, Qt::DisplayRole, QString::number(cache_infos[i].duplication_factor));
		item -> setData(COL_RECEIVEDTIME,     Qt::DisplayRole, QString::number(now - cache_infos[i].routing_time));
        item -> setData(COL_SENDTIME,         Qt::DisplayRole, QString::number(now - cache_infos[i].last_sent_time));
    }
}

void GlobalRouterStatistics::personDetails()
{
    QTreeWidgetItem *item = treeWidget->currentItem();
    std::string id = item->text(COL_DESTINATION).toStdString();

    if (id.empty()) {
        return;
    }
    
    IdDetailsDialog *dialog = new IdDetailsDialog(RsGxsGroupId(id));
    dialog->show();
  
}

GlobalRouterStatisticsWidget::GlobalRouterStatisticsWidget(QWidget *parent)
	: QWidget(parent)
{
    float size = QFontMetricsF(font()).height() ;
    float fact = size/14.0 ;

    maxWidth = 400*fact ;
	maxHeight = 0 ;
    mCurrentN = PARTIAL_VIEW_SIZE/2+1 ;
}

void GlobalRouterStatisticsWidget::updateContent()
{
    RsGRouter::GRouterRoutingMatrixInfo matrix_info ;

    rsGRouter->getRoutingMatrixInfo(matrix_info) ;
    
    mNumberOfKnownKeys = matrix_info.per_friend_probabilities.size() ;

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
    QFont monospace_f("Monospace") ;
    monospace_f.setStyleHint(QFont::TypeWriter) ;
    monospace_f.setPointSize(font().pointSize()) ;

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


    std::map<QString, std::vector<QString> > tos ;
   
    // Now draw the matrix

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

//	//print friends in the same order their prob is shown
//	QString FO = tr("Friend Order  (");
//	RsPeerDetails peer_ssl_details;
//	for(uint32_t i=0;i<matrix_info.friend_ids.size();++i){
//		rsPeers->getPeerDetails(matrix_info.friend_ids[i], peer_ssl_details);
//		QString fn = QString::fromUtf8(peer_ssl_details.name.c_str());
//		FO+=fn;
//		FO+=" ";
//
//	}
//	FO+=")";
//
//	painter.drawText(ox+0*cellx,oy+fm_times.height(),FO) ;
//	oy += celly ;
//	oy += celly ;

    //static const int MaxKeySize = 20*fact ;
    painter.setFont(monospace_f) ;

    int n=0;
    QString ids;
    std::vector<float> current_probs ;
    int current_oy = 0 ;
    
    mMinWheelZoneX = ox+2*cellx ;
    mMinWheelZoneY = oy ;
    
    RsGxsId current_id ;
    float current_width=0 ;
    
    for(std::map<GRouterKeyId,std::vector<float> >::const_iterator it(matrix_info.per_friend_probabilities.begin());it!=matrix_info.per_friend_probabilities.end();++it,++n)
        if(n >= mCurrentN-PARTIAL_VIEW_SIZE/2 && n <= mCurrentN+PARTIAL_VIEW_SIZE/2)
	{
		//bool is_null = false ;

		//for(uint32_t i=0;i<matrix_info.friend_ids.size();++i)
		//	if(it->second[i] > 0.0)
		//		is_null = false ;

		//if(!is_null)
		//{
		ids = QString::fromStdString(it->first.toStdString())+" : " ;
		painter.drawText(ox+2*cellx,oy+celly,ids) ;

		for(uint32_t i=0;i<matrix_info.friend_ids.size();++i)
			painter.fillRect(ox+i*cellx+fm_monospace.width(ids),oy+0.15*celly,cellx,celly,colorScale(it->second[i])) ;

		if(n == mCurrentN)
		{
			current_probs = it->second ;
			current_oy = oy ;
            		current_id = it->first ;
                    	current_width = ox+matrix_info.friend_ids.size()*cellx+fm_monospace.width(ids);
		}

		oy += celly ;
		//}

	}
    mMaxWheelZoneX = ox+matrix_info.friend_ids.size()*cellx + fm_monospace.width(ids);
    
    RsIdentityDetails iddetails ;
    if(rsIdentity->getIdDetails(current_id,iddetails))
        painter.drawText(current_width+cellx, current_oy+celly, QString::fromUtf8(iddetails.mNickname.c_str())) ;
    else
        painter.drawText(current_width+cellx, current_oy+celly, tr("[Unknown identity]")) ;
        
    mMaxWheelZoneY = oy+celly ;
    
    painter.setPen(QColor::fromRgb(0,0,0)) ;
    
    painter.setPen(QColor::fromRgb(127,127,127));
    painter.drawRect(ox+2*cellx,current_oy+0.15*celly,fm_monospace.width(ids)+cellx*matrix_info.friend_ids.size()- 2*cellx,celly) ;

    float total_length = (matrix_info.friend_ids.size()+2)*cellx ;
    
    if(!current_probs.empty())
    for(uint32_t i=0;i<matrix_info.friend_ids.size();++i)
    {
        float x1 = ox+(i+0.5)*cellx+fm_monospace.width(ids) ;
        float y1 = oy+0.15*celly ;
        float y2 = y1+(matrix_info.friend_ids.size()-1-i+1)*celly;
        
	RsPeerDetails peer_ssl_details;
	rsPeers->getPeerDetails(matrix_info.friend_ids[i], peer_ssl_details);
        
        painter.drawLine(x1,y1,x1,y2);
        painter.drawLine(x1,y2,x1 + total_length - i*cellx,y2) ;
	painter.drawText(cellx+ x1 + total_length - i*cellx,y2+(0.35)*celly, QString::fromUtf8(peer_ssl_details.name.c_str()) + " - " + QString::fromUtf8(peer_ssl_details.location.c_str()) + " ("+QString::number(current_probs[i])+")");
    }
    oy += celly * (2+matrix_info.friend_ids.size());

    oy += celly ;
    oy += celly ;

    // update the pixmap
    //
    pixmap = tmppixmap;
    maxHeight = oy ;
}

void GlobalRouterStatisticsWidget::wheelEvent(QWheelEvent *e)
{
    if(e->x() < mMinWheelZoneX || e->x() > mMaxWheelZoneX || e->y() < mMinWheelZoneY || e->y() > mMaxWheelZoneY)
    {
        QWidget::wheelEvent(e) ;
        return ;
    }
    
    if(e->delta() < 0 && mCurrentN+PARTIAL_VIEW_SIZE/2+1 < mNumberOfKnownKeys)
	    mCurrentN++ ;
    
    if(e->delta() > 0 && mCurrentN > PARTIAL_VIEW_SIZE/2+1)
	    mCurrentN-- ;
    
    updateContent();
    update();
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

