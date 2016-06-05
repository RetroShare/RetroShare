#include <QObject>
#include <retroshare/rsturtle.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsgxstunnel.h>
#include "TurtleRouterDialog.h"
#include <QPainter>
#include <QStylePainter>

#include "gui/settings/rsharesettings.h"

static const uint MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;


TurtleRouterDialog::TurtleRouterDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	m_bProcessSettings = false;

	// Init the basic setup.
	//
	QStringList stl ;
	int n=0 ;

	stl.clear() ;
	stl.push_back(tr("Search requests")) ;
	top_level_s_requests = new QTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_s_requests) ;

	stl.clear() ;
	stl.push_back(tr("Tunnel requests")) ;
	top_level_t_requests = new QTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_t_requests) ;

	top_level_hashes.clear() ;

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


void TurtleRouterDialog::updateDisplay()
{
	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<TurtleRequestDisplayInfo > search_reqs_info ;
	std::vector<TurtleRequestDisplayInfo > tunnel_reqs_info ;

	rsTurtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

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
																const std::vector<TurtleRequestDisplayInfo >& search_reqs_info, 
																const std::vector<TurtleRequestDisplayInfo >& tunnel_reqs_info)
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

		QString str = tr("Tunnel id") + ": " + QString::fromUtf8(tunnels_info[i][0].c_str()) + "\t [" + QString::fromUtf8(tunnels_info[i][2].c_str()) + "] --> [" + QString::fromUtf8(tunnels_info[i][1].c_str()) + "]\t\t " + tr("last transfer") + ": " + QString::fromStdString(tunnels_info[i][4]) + "\t " + tr("Speed") + ": " + QString::fromStdString(tunnels_info[i][5]) ;
		stl.clear() ;
		stl.push_back(str) ;

		parent->addChild(new QTreeWidgetItem(stl)) ;
	}

	for(uint i=0;i<search_reqs_info.size();++i)
	{
		QString str = tr("Request id: %1\t from [%2]\t %3 secs ago").arg(search_reqs_info[i].request_id,0,16).arg(getPeerName(search_reqs_info[i].source_peer_id)).arg(search_reqs_info[i].age);

		stl.clear() ;
		stl.push_back(str) ;

		top_level_s_requests->addChild(new QTreeWidgetItem(stl)) ;
	}
	top_level_s_requests->setText(0, tr("Search requests") + "(" + QString::number(search_reqs_info.size()) + ")" ) ;

	for(uint i=0;i<tunnel_reqs_info.size();++i)
		if(i+MAX_TUNNEL_REQUESTS_DISPLAY >= tunnel_reqs_info.size() || i < MAX_TUNNEL_REQUESTS_DISPLAY)
		{
			QString str = tr("Request id: %1\t from [%2]\t %3 secs ago").arg(tunnel_reqs_info[i].request_id,0,16).arg(getPeerName(tunnel_reqs_info[i].source_peer_id)).arg(tunnel_reqs_info[i].age);

			stl.clear() ;
			stl.push_back(str) ;

			top_level_t_requests->addChild(new QTreeWidgetItem(stl)) ;
		}
		else if(i == MAX_TUNNEL_REQUESTS_DISPLAY)
		{
			stl.clear() ;
			stl.push_back(QString("...")) ;
			top_level_t_requests->addChild(new QTreeWidgetItem(stl)) ;

		} 

	top_level_t_requests->setText(0, tr("Tunnel requests") + "("+QString::number(tunnel_reqs_info.size()) + ")") ;

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
	QList<QTreeWidgetItem*> items = _f2f_TW->findItems((hash==null_hash)?tr("Unknown hashes"):QString::fromStdString(hash),Qt::MatchStartsWith) ;

	if(items.empty())
	{	
		QStringList stl ;
		stl.push_back((hash==null_hash)?tr("Unknown hashes"):QString::fromStdString(hash)) ;
		QTreeWidgetItem *item = new QTreeWidgetItem(_f2f_TW,stl) ;
		_f2f_TW->insertTopLevelItem(0,item) ;

		return item ;
	}
	else
		return items.front() ;
}
//=======================================================================================================================//


GxsTunnelsDialog::GxsTunnelsDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
//	setupUi(this) ;
	
	m_bProcessSettings = false;

        //float fontHeight = QFontMetricsF(font()).height();
        //float fact = fontHeight/14.0;
	
	maxWidth = 200 ;
	maxHeight = 200 ;

	// load settings
    processSettings(true);
}

GxsTunnelsDialog::~GxsTunnelsDialog()
{

    // save settings
    processSettings(false);
}

void GxsTunnelsDialog::processSettings(bool bLoad)
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

void GxsTunnelsDialog::updateDisplay()
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
	painter.initFrom(this);

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
		painter.drawText(ox+6*cellx,oy+celly,tr("from: %1").arg(QString::fromStdString(tunnel_infos[i].source_gxs_id.toStdString()))) ; oy += celly ;
		painter.drawText(ox+6*cellx,oy+celly,tr("to: %1").arg(QString::fromStdString(tunnel_infos[i].destination_gxs_id.toStdString()))) ; oy += celly ;
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

QString GxsTunnelsDialog::getPeerName(const RsPeerId &peer_id)
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

QString GxsTunnelsDialog::speedString(float f)
{
	if(f < 1.0f) 
		return QString("0 B/s") ;
	if(f < 1024.0f)
		return QString::number((int)f)+" B/s" ;

	return QString::number(f/1024.0,'f',2) + " KB/s";
}

void GxsTunnelsDialog::paintEvent(QPaintEvent */*event*/)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void GxsTunnelsDialog::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    
    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height() ;

    QWidget::resizeEvent(event);
    update();
}
