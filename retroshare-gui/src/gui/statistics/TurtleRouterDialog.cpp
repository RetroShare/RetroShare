/*******************************************************************************
 * gui/statistics/TurtleRouterDialog.cpp                                       *
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

#include <QObject>
#include <util/rsprint.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsgxstunnel.h>
#include <retroshare/rsservicecontrol.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsdistsync.h>
#include "TurtleRouterDialog.h"
#include <QPainter>
#include <QStylePainter>
#include <algorithm> // for sort
#include <time.h>

#include "util/RsQtVersion.h"
#include "gui/settings/rsharesettings.h"

#define COL_ID                  0
#define COL_FROM                1
#define COL_TIME                2
#define COL_STRING              3

#define COL_TUNNELID            0
#define COL_SPEED               1
#define COL_LASTTRANSFER        2
#define COL_TO                  3

static const uint MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;

class FTTunnelsListDelegate: public QAbstractItemDelegate
{
public:
    FTTunnelsListDelegate(QObject *parent=0);
    virtual ~FTTunnelsListDelegate();
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
};

FTTunnelsListDelegate::FTTunnelsListDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{
}

FTTunnelsListDelegate::~FTTunnelsListDelegate(void)
{
}

void FTTunnelsListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QString strNA = tr("N/A");
	QStyleOptionViewItem opt = option;

	QString temp ;
	float flValue;
	qint64 qi64Value;

	// prepare
	painter->save();
	painter->setClipRect(opt.rect);

	//set text color
	QVariant value = index.data(Qt::ForegroundRole);
	if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
		opt.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));
	}
	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if(option.state & QStyle::State_Selected){
		painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
	} else {
		painter->setPen(opt.palette.color(cg, QPalette::Text));
	}

	// draw the background color
	if(option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
		if(cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
			cg = QPalette::Inactive;
		}
		painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
	} else {
		value = index.data(Qt::BackgroundRole);
		if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
			painter->fillRect(option.rect, qvariant_cast<QColor>(value));
		}
	}

	switch(index.column()) {
	/*case COLUMN_SERVICE :
		//temp = QString::asprintf("%.3f ", index.data().toFloat());
		//temp=QString::number(index.data().toFloat());
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
		break;
	case COLUMN_GROUPID:
		//temp = QString::asprintf("%.3f ", index.data().toFloat());
		//temp=QString::number(index.data().toFloat());
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
		break;
	case COLUMN_POLICY:
		//temp=QString::number(index.data().toInt());
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
		break;
	case COLUMN_STATUS:
		/*flValue = index.data().toFloat();
		if (flValue < std::numeric_limits<float>::max()){
			temp = QString::asprintf("%.3f ", flValue);
		} else {
			temp=strNA;
		}*/
		/*painter->drawText(option.rect, Qt::AlignHCenter, index.data().toString());
		break;
	case COLUMN_LASTCONTACT:
		/*qi64Value = index.data().value<qint64>();
		if (qi64Value < std::numeric_limits<qint64>::max()){
			temp= QString::number(qi64Value);
		} else {
			temp = strNA;
		}*/
		/*painter->drawText(option.rect, Qt::AlignHCenter, index.data().toString());
		break;*/
	default:
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
	}

	// done
	painter->restore();
}

QSize FTTunnelsListDelegate::sizeHint(const QStyleOptionViewItem & option/*option*/, const QModelIndex & index) const
{
    float FS = QFontMetricsF(option.font).height();
    //float fact = FS/14.0 ;

    float w = QFontMetrics_horizontalAdvance(QFontMetricsF(option.font), index.data(Qt::DisplayRole).toString());

    return QSize(w,FS*1.2);
    //return QSize(50*fact,17*fact);
}

TurtleRouterDialog::TurtleRouterDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	m_bProcessSettings = false;

	// Init the basic setup.
	//
	QStringList stl ;
	int n=0 ;

	FTTDelegate = new FTTunnelsListDelegate();
	_f2f_TW->setItemDelegate(FTTDelegate);
	tunnels_treeWidget->setItemDelegate(FTTDelegate);

	stl.clear() ;
	stl.push_back(tr("Search requests")) ;
	top_level_s_requests = new QTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_s_requests) ;

	stl.clear() ;
	stl.push_back(tr("Tunnel requests")) ;
	top_level_t_requests = new QTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_t_requests) ;

	top_level_hashes.clear() ;

	float FS = QFontMetricsF(font()).height();
	float fact = FS/14.0 ;

	QHeaderView * _header = _f2f_TW->header () ;
	_header->resizeSection ( COL_ID, 170*fact );
	QHeaderView * _header2 = tunnels_treeWidget->header () ;
	_header2->resizeSection ( COL_TUNNELID, 270*fact );

	// load settings
    processSettings(true);
}

TurtleRouterDialog::~TurtleRouterDialog()
{
    // save settings
    processSettings(false);
}

void TurtleRouterDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("TurtleRouterDialog"));

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

bool sr_Compare( TurtleSearchRequestDisplayInfo m1,  TurtleSearchRequestDisplayInfo m2)  { return m1.age < m2.age; }

void TurtleRouterDialog::updateDisplay()
{
	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<TurtleSearchRequestDisplayInfo > search_reqs_info ;
	std::vector<TurtleTunnelRequestDisplayInfo > tunnel_reqs_info ;

	rsTurtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;
	
	std::sort(search_reqs_info.begin(),search_reqs_info.end(),sr_Compare) ;
	
	updateTunnelRequests(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

}

QString TurtleRouterDialog::getPeerName(const RsPeerId& peer_id)
{
    static std::map<RsPeerId, QString> names ;

    std::map<RsPeerId,QString>::const_iterator it = names.find(peer_id) ;

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

void TurtleRouterDialog::updateTunnelRequests(	const std::vector<std::vector<std::string> >& hashes_info, 
																const std::vector<std::vector<std::string> >& tunnels_info, 
																const std::vector<TurtleSearchRequestDisplayInfo >& search_reqs_info,
																const std::vector<TurtleTunnelRequestDisplayInfo >& tunnel_reqs_info)
{
	// now display this in the QTableWidgets

	QStringList stl ;

	// remove all children of top level objects
	for(int i=0;i<_f2f_TW->topLevelItemCount();++i)
	{
		QTreeWidgetItem *taken ;
		while( (taken = _f2f_TW->topLevelItem(i)->takeChild(0)) != NULL) 
			delete taken ;
	}
	
	// remove all children of top level objects
	for(int i=0;i<tunnels_treeWidget->topLevelItemCount();++i)
	{
		QTreeWidgetItem *taken ;
		while( (taken = tunnels_treeWidget->topLevelItem(i)->takeChild(0)) != NULL) 
			delete taken ;
	}

    _f2f_TW->setSelectionMode(QAbstractItemView::SingleSelection);
    tunnels_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	for(uint i=0;i<hashes_info.size();++i)
		findParentHashItem(hashes_info[i][0]) ;

	bool unknown_hash_found = false ;

	// check that an entry exist for all hashes
	for(uint i=0;i<tunnels_info.size();++i)
	{
		const std::string& hash(tunnels_info[i][3]) ;

		QTreeWidgetItem *parent = findParentHashItem(hash) ;

		if(parent->text(0).left(14) == tr("Unknown hashes"))
			unknown_hash_found = true ;
		
		float num = strtof(tunnels_info[i][5].c_str(), NULL);  // printFloatNumber
		char tmp[100] ;
		std::string units[4] = { "B/s","KB/s","MB/s","GB/s" } ;
		int k=0 ;
		while(num >= 800.0f && k<4)
			num /= 1024.0f,++k;
		sprintf(tmp,"%3.2f %s",num,units[k].c_str()) ;

		QTreeWidgetItem *tunnel_item = NULL;
		tunnel_item = new QTreeWidgetItem();

		tunnel_item->setData(COL_TUNNELID,       Qt::DisplayRole, QString::fromUtf8(tunnels_info[i][0].c_str()));
		tunnel_item->setData(COL_SPEED,          Qt::DisplayRole, QString::fromStdString(tmp)) ;
		tunnel_item->setData(COL_LASTTRANSFER,   Qt::DisplayRole, QString::fromStdString(tunnels_info[i][4]));
		tunnel_item->setData(COL_TO,             Qt::DisplayRole, QString::fromUtf8(tunnels_info[i][2].c_str()) + " -> " + QString::fromUtf8(tunnels_info[i][1].c_str()));

		tunnel_item->setTextAlignment(COL_SPEED, Qt::AlignRight);
		tunnel_item->setTextAlignment(COL_LASTTRANSFER, Qt::AlignCenter);

		parent->addChild(tunnel_item) ;

		QFont font = tunnel_item->font(0);
		if(strtol(tunnels_info[i][4].c_str(), NULL, 0)>10) // stuck
		{
			font.setItalic(true);
			tunnel_item->setFont(0,font);
		}
		if(strtof(tunnels_info[i][5].c_str(), NULL)>1000) // fast
		{
			font.setBold(true);
			tunnel_item->setFont(0,font);
		}
	}

	for(uint i=0;i<search_reqs_info.size();++i)
	{

		QTreeWidgetItem *sr_item = NULL;
		sr_item = new QTreeWidgetItem();
		top_level_s_requests->addChild(sr_item) ;

		sr_item->setData(COL_ID,      Qt::DisplayRole, QString::number(search_reqs_info[i].request_id));
		sr_item->setData(COL_FROM,    Qt::DisplayRole, getPeerName(search_reqs_info[i].source_peer_id) ) ;
		sr_item->setData(COL_TIME,    Qt::DisplayRole, QString::number(search_reqs_info[i].age) + " secs ago");
		sr_item->setData(COL_STRING,  Qt::DisplayRole, QString::fromUtf8(search_reqs_info[i].keywords.c_str()) + QString::number(search_reqs_info[i].keywords.length()) + " (" + QString::number(search_reqs_info[i].hits) + " hits)");

	}
	top_level_s_requests->setText(0, tr("Search requests") + " (" + QString::number(search_reqs_info.size()) + ")" ) ;

	for(uint i=0;i<tunnel_reqs_info.size();++i)
		if(i+MAX_TUNNEL_REQUESTS_DISPLAY >= tunnel_reqs_info.size() || i < MAX_TUNNEL_REQUESTS_DISPLAY)
		{
			/* find the entry */
			QTreeWidgetItem *tunnelr_item = NULL;
			tunnelr_item = new QTreeWidgetItem();
			top_level_t_requests->addChild(tunnelr_item) ;

			tunnelr_item->setData(COL_ID,    Qt::DisplayRole, QString::number(tunnel_reqs_info[i].request_id));
			tunnelr_item->setData(COL_FROM,  Qt::DisplayRole, getPeerName(tunnel_reqs_info[i].source_peer_id) ) ;
			tunnelr_item->setData(COL_TIME,  Qt::DisplayRole, QString::number(tunnel_reqs_info[i].age) + " secs ago");
		}
		else if(i == MAX_TUNNEL_REQUESTS_DISPLAY)
		{
			stl.clear() ;
			stl.push_back(QString("...")) ;
			top_level_t_requests->addChild(new QTreeWidgetItem(stl)) ;

		} 

	top_level_t_requests->setText(0, tr("Tunnel requests") + " ("+QString::number(tunnel_reqs_info.size()) + ")") ;

	QTreeWidgetItem *unknown_hashs_item = findParentHashItem(RsFileHash().toStdString()) ;
	unknown_hashs_item->setText(0,tr("Unknown hashes") + " (" + QString::number(unknown_hashs_item->childCount())+QString(")")) ;

	// Ok, this is a N2 search, but there are very few elements in the list.
	for(int i=2;i<_f2f_TW->topLevelItemCount();)
	{
		bool found = false ;

		if(_f2f_TW->topLevelItem(i)->text(0).left(14) == tr("Unknown hashes") && unknown_hash_found)
			found = true ;

		if(_f2f_TW->topLevelItem(i)->childCount() > 0)	// this saves uploading hashes
			found = true ;

		for(uint j=0;j<hashes_info.size() && !found;++j)
			if(_f2f_TW->topLevelItem(i)->text(0).toStdString() == hashes_info[j][0]) 
				found=true ;

		if(!found)
			delete _f2f_TW->takeTopLevelItem(i) ;
		else
			++i ;
	}
}
	
QTreeWidgetItem *TurtleRouterDialog::findParentHashItem(const std::string& hash)
{
	static const std::string null_hash = RsFileHash().toStdString() ;

	// look for the hash, and insert a new element if necessary.
	//
	QList<QTreeWidgetItem*> items =tunnels_treeWidget->findItems((hash==null_hash)?tr("Unknown hashes"):QString::fromStdString(hash),Qt::MatchStartsWith) ;

	if(items.empty())
	{	
		QStringList stl ;
		stl.push_back((hash==null_hash)?tr("Unknown hashes"):QString::fromStdString(hash)) ;
		QTreeWidgetItem *item = new QTreeWidgetItem(tunnels_treeWidget,stl) ;
		tunnels_treeWidget->insertTopLevelItem(0,item) ;

		return item ;
	}
	else
		return items.front() ;
}

//=======================================================================================================================//

TunnelStatisticsDialog::TunnelStatisticsDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	m_bProcessSettings = false;

	maxWidth = 200 ;
	maxHeight = 200 ;

	// load settings
    processSettings(true);
}

TunnelStatisticsDialog::~TunnelStatisticsDialog()
{
    // save settings
    processSettings(false);
}

void TunnelStatisticsDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("TurtleRouterStatistics"));

    if (bLoad) {
        // load settings
    } else {
        // save settings
    }

    Settings->endGroup();
    
    m_bProcessSettings = false;
}

QString TunnelStatisticsDialog::getPeerName(const RsPeerId &peer_id)
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

QString TunnelStatisticsDialog::getPeerName(const RsGxsId& gxs_id)
{
    static std::map<RsGxsId, QString> names ;

    std::map<RsGxsId,QString>::const_iterator it = names.find(gxs_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsIdentityDetails detail ;

		if(!rsIdentity->getIdDetails(gxs_id,detail))
			return tr("Unknown Peer");

		return (names[gxs_id] = QString::fromUtf8(detail.mNickname.c_str())) ;
	}
}


QString TunnelStatisticsDialog::speedString(float f)
{
	if(f < 1.0f)
		return QString("0 B/s") ;
	if(f < 1024.0f)
		return QString::number((int)f)+" B/s" ;

	return QString::number(f/1024.0,'f',2) + " KB/s";
}

void TunnelStatisticsDialog::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void TunnelStatisticsDialog::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();

    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height() ;

    QWidget::resizeEvent(event);
    update();
}
//=======================================================================================================================//

GxsAuthenticatedTunnelsDialog::GxsAuthenticatedTunnelsDialog(QWidget *parent)
    : TunnelStatisticsDialog(parent)
{
}

void GxsAuthenticatedTunnelsDialog::updateDisplay()
{
    // Request info about ongoing tunnels
    
    std::vector<RsGxsTunnelService::GxsTunnelInfo> tunnel_infos ;
    
    rsGxsTunnel->getTunnelsInfo(tunnel_infos) ;
    
 	//    // Tunnel information
 	//           
	//    GxsTunnelId  tunnel_id ;        // GXS Id we're talking to
	//    RsGxsId  destination_gxs_id ;   // GXS Id we're talking to
	//    RsGxsId  source_gxs_id ;	          // GXS Id we're using to talk
	//    uint32_t tunnel_status ;	          // active, requested, DH pending, etc.
	//    uint32_t total_size_sent ;	          // total bytes sent through that tunnel since openned (including management). 
	//    uint32_t total_size_received ;	  // total bytes received through that tunnel since openned (including management). 

	//    // Data packets

	//    uint32_t pending_data_packets;         // number of packets not acknowledged by other side, still on their way. Should be 0 unless something bad happens.
	//    uint32_t total_data_packets_sent ;     // total number of data packets sent (does not include tunnel management)
	//    uint32_t total_data_packets_received ; // total number of data packets received (does not include tunnel management)
        
    // now draw the shit
	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(Qt::transparent);
	//setFixedHeight(maxHeight);

	QPainter painter(&tmppixmap);
	painter.begin(this);

        // extracts the height of the fonts in pixels. This is used to calibrate the size of the objects to draw.

        float fontHeight = QFontMetricsF(font()).height();
        float fact = fontHeight/14.0;
	//maxHeight = 500*fact ;

	int cellx = 6*fact ;
	int celly = (10+4)*fact ;
    	int ox=5*fact,oy=5*fact ;

	painter.setPen(QColor::fromRgb(0,0,0)) ;
	painter.drawText(ox+2*cellx,oy+celly,tr("Authenticated tunnels:")) ; oy += celly ;
        
    	for(uint32_t i=0;i<tunnel_infos.size();++i)
	{
		// std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
		// draw...

		painter.drawText(ox+4*cellx,oy+celly,tr("Tunnel ID: %1").arg(QString::fromStdString(tunnel_infos[i].tunnel_id.toStdString()))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("from: %1 (%2)").arg(QString::fromStdString(tunnel_infos[i].source_gxs_id.toStdString())).arg(getPeerName(tunnel_infos[i].source_gxs_id))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("to: %1 (%2)").arg(QString::fromStdString(tunnel_infos[i].destination_gxs_id.toStdString())).arg(getPeerName(tunnel_infos[i].destination_gxs_id))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("status: %1").arg(QString::number(tunnel_infos[i].tunnel_status))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("total sent: %1 bytes").arg(QString::number(tunnel_infos[i].total_size_sent))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("total recv: %1 bytes").arg(QString::number(tunnel_infos[i].total_size_received))) ; oy += celly ;
        
		//	painter.drawLine(0,oy,maxWidth,oy) ;
		//	oy += celly ;
	}

	// update the pixmap
	//
	pixmap = tmppixmap;
	maxHeight = std::max(oy,10*celly);
}
