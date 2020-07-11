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

#include <time.h>
#include <math.h>

#include <iostream>

#include <QDateTime>
#include <QTimer>
#include <QObject>
#include <QFontMetrics>
#include <QWheelEvent>
#include <QMenu>
#include <QPainter>
#include <QStylePainter>
#include <QLayout>
#include <QHeaderView>

#include <retroshare/rsidentity.h>
#include <retroshare/rsservicecontrol.h>

#include "GxsIdStatistics.h"

#include "util/DateTime.h"
#include "util/QtVersion.h"
#include "util/misc.h"

static QColor colorScale(float f)
{
	if(f == 0)
		return QColor::fromHsv(0,0,192) ;
	else
		return QColor::fromHsv((int)((1.0-f)*280),200,255) ;
}

GxsIdStatistics::GxsIdStatistics(QWidget *parent)
    : RsAutoUpdatePage(4000,parent)
{
	setupUi(this) ;

    _stats_F->setWidget(_tst_CW = new GxsIdStatisticsWidget);
	m_bProcessSettings = false;

	// load settings
	processSettings(true);
}

GxsIdStatistics::~GxsIdStatistics()
{
    // save settings
    processSettings(false);
}

void GxsIdStatistics::processSettings(bool bLoad)
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

void GxsIdStatistics::updateDisplay()
{
	_tst_CW->updateContent() ;
}

static QString getServiceName(uint32_t s)
{
    switch(s)
    {
    default:
	case 0x0011  /* GOSSIP_DISCOVERY  */         : return QObject::tr("Discovery");
	case 0x0012  /* CHAT              */         : return QObject::tr("Chat");
	case 0x0013  /* MSG               */         : return QObject::tr("Messages");
	case 0x0014  /* TURTLE            */         : return QObject::tr("Turtle");
	case 0x0016  /* HEARTBEAT         */         : return QObject::tr("Heartbeat");
	case 0x0017  /* FILE_TRANSFER     */         : return QObject::tr("File transfer");
	case 0x0018  /* GROUTER           */         : return QObject::tr("Global router");
	case 0x0019  /* FILE_DATABASE     */         : return QObject::tr("File database");
	case 0x0020  /* SERVICEINFO       */         : return QObject::tr("Service info");
	case 0x0021  /* BANDWIDTH_CONTROL */         : return QObject::tr("Bandwidth control");
	case 0x0022  /* MAIL              */         : return QObject::tr("Mail");
	case 0x0023  /* DIRECT_MAIL       */         : return QObject::tr("Mail");
	case 0x0024  /* DISTANT_MAIL      */         : return QObject::tr("Distant mail");
	case 0x0026  /* SERVICE_CONTROL   */         : return QObject::tr("Service control");
	case 0x0027  /* DISTANT_CHAT      */         : return QObject::tr("Distant chat");
	case 0x0028  /* GXS_TUNNEL        */         : return QObject::tr("GXS Tunnel");
	case 0x0101  /* BANLIST           */         : return QObject::tr("Ban list");
	case 0x0102  /* STATUS            */         : return QObject::tr("Status");
	case 0x0200  /* NXS               */         : return QObject::tr("NXS");
	case 0x0211  /* GXSID             */         : return QObject::tr("Identities");
	case 0x0212  /* PHOTO             */         : return QObject::tr("GXS Photo");
	case 0x0213  /* WIKI              */         : return QObject::tr("GXS Wiki");
	case 0x0214  /* WIRE              */         : return QObject::tr("GXS TheWire");
	case 0x0215  /* FORUMS            */         : return QObject::tr("Forums");
	case 0x0216  /* POSTED            */         : return QObject::tr("Boards");
	case 0x0217  /* CHANNELS          */         : return QObject::tr("Channels");
	case 0x0218  /* GXSCIRCLE         */         : return QObject::tr("Circles");
	///  entiti not gxs, but used with :dentities.
	case 0x0219  /* REPUTATION        */         : return QObject::tr("Reputation");
	case 0x0220  /* GXS_RECOGN        */         : return QObject::tr("Recogn");
	case 0x0230  /* GXS_TRANS         */         : return QObject::tr("GXS Transport");
	case 0x0240  /* JSONAPI           */         : return QObject::tr("JSon API");

    }
}

static QString getUsageStatisticsName(RsIdentityUsage::UsageCode code)
{
    switch(code)
	{
    default:
		case RsIdentityUsage::UNKNOWN_USAGE                        : return QObject::tr("Unknown");
		case RsIdentityUsage::GROUP_ADMIN_SIGNATURE_CREATION       : return QObject::tr("Group admin signature creation");
		case RsIdentityUsage::GROUP_ADMIN_SIGNATURE_VALIDATION     : return QObject::tr("Group admin signature validation");
		case RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_CREATION      : return QObject::tr("Group author signature creation");
		case RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_VALIDATION    : return QObject::tr("Group author signature validation");
		case RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_CREATION    : return QObject::tr("Message author signature creation");
		case RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_VALIDATION  : return QObject::tr("Message author signature validation");
		case RsIdentityUsage::GROUP_AUTHOR_KEEP_ALIVE              : return QObject::tr("Routine group author signature check.");
		case RsIdentityUsage::MESSAGE_AUTHOR_KEEP_ALIVE            : return QObject::tr("Routine message author signature check");
		case RsIdentityUsage::CHAT_LOBBY_MSG_VALIDATION            : return QObject::tr("Chat room signature validation");
		case RsIdentityUsage::GLOBAL_ROUTER_SIGNATURE_CHECK        : return QObject::tr("Global router message validation");
		case RsIdentityUsage::GLOBAL_ROUTER_SIGNATURE_CREATION     : return QObject::tr("Global router message creation");
		case RsIdentityUsage::GXS_TUNNEL_DH_SIGNATURE_CHECK        : return QObject::tr("DH Key exchange validation for GXS tunnel");
		case RsIdentityUsage::GXS_TUNNEL_DH_SIGNATURE_CREATION     : return QObject::tr("DH Key exchange creation for GXS tunnel");
		case RsIdentityUsage::IDENTITY_NEW_FROM_GXS_SYNC           : return QObject::tr("New identity from GXS sync");
		case RsIdentityUsage::IDENTITY_NEW_FROM_DISCOVERY          : return QObject::tr("New friend identity from discovery");
		case RsIdentityUsage::IDENTITY_NEW_FROM_EXPLICIT_REQUEST   : return QObject::tr("New identity requested from friend node");
		case RsIdentityUsage::IDENTITY_GENERIC_SIGNATURE_CHECK     : return QObject::tr("Generic signature validation");
		case RsIdentityUsage::IDENTITY_GENERIC_SIGNATURE_CREATION  : return QObject::tr("Generic signature creation");
		case RsIdentityUsage::IDENTITY_GENERIC_ENCRYPTION          : return QObject::tr("Generic data decryption");
		case RsIdentityUsage::IDENTITY_GENERIC_DECRYPTION          : return QObject::tr("Generic data encryption");
		case RsIdentityUsage::CIRCLE_MEMBERSHIP_CHECK              : return QObject::tr("Circle membership checking");
	}
}

void GxsIdStatisticsWidget::updateContent()
{
    // get the info, stats, histograms and pass them

    std::list<RsGroupMetaData> ids;
    rsIdentity->getIdentitiesSummaries(ids) ;

    time_t now = time(NULL);
	uint32_t nb_weeks = 52;
	uint32_t nb_hours = 52;

    Histogram publish_date_hist(now - nb_weeks*7*86400,now,nb_weeks);
    Histogram last_used_hist(now - 3600*nb_hours,now,nb_hours);
    uint32_t total_identities = 0;
    std::map<RsIdentityUsage::UsageCode,int> usage_map;
    std::map<uint32_t,int> per_service_usage_map;

    for(auto& meta:ids)
    {
        RsIdentityDetails det;

        if(!rsIdentity->getIdDetails(RsGxsId(meta.mGroupId),det))
            continue;

        publish_date_hist.insert((double)meta.mPublishTs);
        last_used_hist.insert((double)det.mLastUsageTS);

        for(auto it:det.mUseCases)
        {
            auto it2 = usage_map.find(it.first.mUsageCode);
            if(it2 == usage_map.end())
                usage_map[it.first.mUsageCode] = 0 ;

			++usage_map[it.first.mUsageCode];

            uint32_t s = static_cast<uint32_t>(it.first.mServiceId);
            auto it3 = per_service_usage_map.find(s);
            if(it3 == per_service_usage_map.end())
                per_service_usage_map[s] = 0;

            ++per_service_usage_map[s];
        }

        ++total_identities;
    }

#ifdef DEBUG_GXSID_STATISTICS
    std::cerr << "Identities statistics:" << std::endl;

    std::cerr << "  Usage map:" << std::endl;
    for(auto it:usage_map)
        std::cerr << std::hex << (int)it.first << " : " << std::dec << it.second << std::endl;

    std::cerr << "  Total identities: " << total_identities << std::endl;
    std::cerr << "  Last used hist: " << std::endl;
    std::cerr << last_used_hist << std::endl;
    std::cerr << "  Publish date hist: " << std::endl;
    std::cerr << publish_date_hist << std::endl;
#endif

    // Now draw the info int the widget's pixmap

    float size = QFontMetricsF(font()).height() ;
    float fact = size/14.0 ;

    QPixmap tmppixmap(mMaxWidth, mMaxHeight);
	tmppixmap.fill(Qt::transparent);
    setFixedHeight(mMaxHeight);

    QPainter painter(&tmppixmap);
    painter.initFrom(this);
    painter.setPen(QColor::fromRgb(0,0,0)) ;

    QFont times_f(font());//"Times") ;
    QFont monospace_f("Monospace") ;
    monospace_f.setStyleHint(QFont::TypeWriter) ;
    monospace_f.setPointSize(font().pointSize()) ;

    QFontMetricsF fm_monospace(monospace_f) ;
    QFontMetricsF fm_times(times_f) ;

    int cellx = fm_monospace.width(QString(" ")) ;
    int celly = fm_monospace.height() ;

    // Display general statistics

	int ox=5*fact,oy=15*fact ;

    painter.setFont(times_f) ;
    painter.drawText(ox,oy,tr("Total identities: ")+QString::number(total_identities)) ; oy += celly*2 ;

    uint32_t total_per_type = 0;
    for(auto it:usage_map)
        total_per_type += it.second;

    painter.setFont(times_f) ;
    painter.drawText(ox,oy,tr("Usage types") + "(" + QString::number(total_per_type) + " identities actually used): ") ; oy += 2*celly;

    for(auto it:usage_map)
    {
        painter.drawText(ox+2*cellx,oy, getUsageStatisticsName(it.first) + ": " + QString::number(it.second)) ;
        oy += celly ;
    }
    oy += celly ;

    // Display per-service statistics

    uint32_t total_per_service = 0;
    for(auto it:per_service_usage_map)
        total_per_service += it.second;

    painter.setFont(times_f) ;
    painter.drawText(ox,oy,tr("Usage per service") + "(" + QString::number(total_per_service) + " identities actually used): ") ; oy += 2*celly;

    for(auto it:per_service_usage_map)
    {
        painter.drawText(ox+2*cellx,oy, getServiceName(it.first) + ": " + QString::number(it.second)) ;
        oy += celly ;
    }
    oy += celly ;

    // Draw the creation time histogram

    painter.setFont(times_f) ;
    painter.drawText(ox,oy,tr("Identity age (in weeks):")) ; oy += celly ;

    uint32_t hist_height = 10;
    oy += hist_height*celly;

    painter.drawLine(QPoint(ox+4*cellx,oy),QPoint(ox+4*cellx+cellx*nb_weeks*2,oy));
    painter.drawLine(QPoint(ox+4*cellx,oy),QPoint(ox+4*cellx,oy-celly*hist_height));

    uint32_t max_entry=0;
    for(int i=0;i<publish_date_hist.entries().size();++i)
        max_entry = std::max(max_entry,publish_date_hist.entries()[i]);

    for(int i=0;i<publish_date_hist.entries().size();++i)
    {
        float h = floor(celly*publish_date_hist.entries()[i]/(float)max_entry*hist_height);
        int I = publish_date_hist.entries().size() - 1 - i;

        painter.fillRect(ox+4*cellx+I*2*cellx+cellx, oy-h, cellx, h,QColor::fromRgbF(0.9,0.6,0.2));
		painter.setPen(QColor::fromRgb(0,0,0));
        painter.drawRect(ox+4*cellx+I*2*cellx+cellx, oy-h, cellx, h);
    }
    for(int i=0;i<publish_date_hist.entries().size();++i)
    {
        QString txt = QString::number(i);
        painter.drawText(ox+4*cellx+i*2*cellx+cellx*1.5 - 0.5*fm_times.width(txt),oy+celly,txt);
    }

    for(int i=0;i<5;++i)
    {
        QString txt = QString::number((int)rint(max_entry*i/5.0));
        painter.drawText(ox + 4*cellx - cellx - fm_times.width(txt),oy - i*hist_height/5.0 * celly,txt );
    }

    oy += 2*celly;
    oy +=   celly;

    // Last used histogram

    painter.setFont(times_f) ;
    painter.drawText(ox,oy,tr("Last used (hours ago): ")) ; oy += celly ;

    oy += hist_height*celly;

    painter.drawLine(QPoint(ox+4*cellx,oy),QPoint(ox+4*cellx+cellx*nb_hours*2,oy));
    painter.drawLine(QPoint(ox+4*cellx,oy),QPoint(ox+4*cellx,oy-celly*hist_height));

    max_entry=0;
    for(int i=0;i<last_used_hist.entries().size();++i)
        max_entry = std::max(max_entry,last_used_hist.entries()[i]);

    for(int i=0;i<last_used_hist.entries().size();++i)
    {
        float h = floor(celly*last_used_hist.entries()[i]/(float)max_entry*hist_height);
        int I = last_used_hist.entries().size() - 1 - i;

        painter.fillRect(ox+4*cellx+I*2*cellx+cellx, oy-h, cellx, h,QColor::fromRgbF(0.6,0.9,0.4));
		painter.setPen(QColor::fromRgb(0,0,0));
        painter.drawRect(ox+4*cellx+I*2*cellx+cellx, oy-h, cellx, h);
    }
    for(int i=0;i<last_used_hist.entries().size();++i)
    {
        QString txt = QString::number(i);
        painter.drawText(ox+4*cellx+i*2*cellx+cellx*1.5 - 0.5*fm_times.width(txt),oy+celly,txt);
    }

    for(int i=0;i<5;++i)
    {
        QString txt = QString::number((int)rint(max_entry*i/5.0));
        painter.drawText(ox + 4*cellx - cellx - fm_times.width(txt),oy - i*hist_height/5.0 * celly,txt );
    }

    oy += 2*celly;


    // set the pixmap

    pixmap = tmppixmap;
    mMaxHeight = oy;
}


GxsIdStatisticsWidget::GxsIdStatisticsWidget(QWidget *parent)
	: QWidget(parent)
{
    float size = QFontMetricsF(font()).height() ;
    float fact = size/14.0 ;

    mMaxWidth = 400*fact ;
	mMaxHeight = 0 ;
    //mCurrentN = PARTIAL_VIEW_SIZE/2+1 ;
}

#ifdef TODO
void GxsIdStatisticsWidget::updateContent()
{
    // 1 - get info

    // 2 - draw histograms

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
#endif

void GxsIdStatisticsWidget::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void GxsIdStatisticsWidget::resizeEvent(QResizeEvent *event)
{
    QRect rect = geometry();

    mMaxWidth = rect.width();
    mMaxHeight = rect.height() ;

	QWidget::resizeEvent(event);
	updateContent();
}
