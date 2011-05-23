#include <iostream>
#include <QTimer>
#include <QObject>
#include <retroshare/rsturtle.h>
#include <retroshare/rspeers.h>
#include "TurtleRouterDialog.h"
#include <QPainter>
#include <QStylePainter>

static const int MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;

class TRHistogram
{
	public:
		TRHistogram(const std::vector<TurtleRequestDisplayInfo >& info) :_infos(info) {}

		QColor colorScale(float f)
		{
			return QColor::fromHsv((int)((1.0-f)*280),200,255) ;
		}

		virtual void draw(QPainter *painter,int& ox,int& oy,const QString& title) 
		{
			static const int MaxTime = 61 ;
			static const int cellx = 6 ;
			static const int celly = 10 ;

			int save_ox = ox ;
			painter->setPen(QColor::fromRgb(0,0,0)) ;
			painter->drawText(2+ox,celly+oy,title) ;
			oy+=2+2*celly ;

			if(_infos.empty())
				return ;

			ox += 10 ;
			std::map<std::string,std::vector<int> > hits ;
			std::map<std::string,std::vector<int> >::iterator it ;

			int max_hits = 1;
			for(uint32_t i=0;i<_infos.size();++i)
			{
				std::vector<int>& h(hits[_infos[i].source_peer_id]) ;

				if(h.size() <= _infos[i].age)
					h.resize(MaxTime,0) ;

				if(_infos[i].age < h.size())
				{
					h[_infos[i].age]++ ;
					if(h[_infos[i].age] > max_hits)
						max_hits = h[_infos[i].age] ;
				}
			}

			int p=0 ;

			for(it=hits.begin();it!=hits.end();++it,++p)
			{
				int total = 0 ;

				for(int i=0;i<MaxTime;++i)
				{
					painter->fillRect(ox+i*cellx,oy+p*celly,cellx,celly,colorScale(it->second[i]/(float)max_hits)) ;
					total += it->second[i] ;
				}

				painter->setPen(QColor::fromRgb(0,0,0)) ;
				painter->drawText(ox+(MaxTime+1)*cellx,oy+(p+1)*celly,TurtleRouterDialog::getPeerName(it->first)) ;
				painter->drawText(ox+(MaxTime+1)*cellx+80,oy+(p+1)*celly,"("+QString::number(total)+")") ;
			}

			painter->drawRect(ox,oy,MaxTime*cellx,p*celly) ;

			for(int i=0;i<MaxTime;i+=5)
				painter->drawText(ox+i*cellx,oy+(p+1)*celly+4,QString::number(i)) ;

			oy += (p+1)*celly+4 ;

			painter->drawText(ox,oy+celly,QObject::tr("(Age in seconds)"));
			oy += 3*celly ;

			// now, draw a scale

			for(int i=0;i<=10;++i)
			{
				painter->fillRect(ox+i*(cellx+40),oy,cellx,celly,colorScale(i/10.0f)) ;
				painter->setPen(QColor::fromRgb(0,0,0)) ;
				painter->drawRect(ox+i*(cellx+40),oy,cellx,celly) ;
				painter->drawText(ox+i*(cellx+40)+cellx+4,oy+celly,QString::number((int)(max_hits*i/10.0))) ;
			}

			oy += celly*2 ;

			ox = save_ox ;
		}

	private:
		const std::vector<TurtleRequestDisplayInfo>& _infos ;
};

TurtleRouterDialog::TurtleRouterDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;

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

	QVBoxLayout v(_tunnel_statistics_F) ;
	v.addWidget( _tst_CW = new TurtleRouterStatisticsWidget() ) ; 
}

void TurtleRouterDialog::updateDisplay()
{
	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<TurtleRequestDisplayInfo > search_reqs_info ;
	std::vector<TurtleRequestDisplayInfo > tunnel_reqs_info ;

	rsTurtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

	updateTunnelRequests(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;
	_tst_CW->updateTunnelStatistics(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;
}

QString TurtleRouterDialog::getPeerName(const std::string& peer_id)
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

		return (names[peer_id] = QString::fromStdString(detail.name)) ;
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

		if(parent->text(0).left(14) == QString("Unknown hashes"))
			unknown_hash_found = true ;

		QString str = QString::fromStdString( "Tunnel id: " + tunnels_info[i][0] + "\t [" + tunnels_info[i][2] + "] --> [" + tunnels_info[i][1] + "]\t\t last transfer: " + tunnels_info[i][4] + "\t Speed: " + tunnels_info[i][5] ) ;
		stl.clear() ;
		stl.push_back(str) ;

		parent->addChild(new QTreeWidgetItem(stl)) ;
	}

	for(uint i=0;i<search_reqs_info.size();++i)
	{
		QString str = "Request id: " + QString::number(search_reqs_info[i].request_id,16) + "\t from [" + getPeerName(search_reqs_info[i].source_peer_id) + "]\t " + QString::number(search_reqs_info[i].age)+" secs ago" ;

		stl.clear() ;
		stl.push_back(str) ;

		top_level_s_requests->addChild(new QTreeWidgetItem(stl)) ;
	}
	top_level_s_requests->setText(0, tr("Search requests") + "(" + QString::number(search_reqs_info.size()) + ")" ) ;

	for(uint i=0;i<tunnel_reqs_info.size();++i)
		if(i+MAX_TUNNEL_REQUESTS_DISPLAY >= tunnel_reqs_info.size() || i < MAX_TUNNEL_REQUESTS_DISPLAY)
		{
			QString str = "Request id: " + QString::number(tunnel_reqs_info[i].request_id,16) + "\t from [" + getPeerName(tunnel_reqs_info[i].source_peer_id) + "]\t " + QString::number(tunnel_reqs_info[i].age)+" secs ago" ;

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

	QTreeWidgetItem *unknown_hashs_item = findParentHashItem("") ;
	unknown_hashs_item->setText(0,QString("Unknown hashes (") + QString::number(unknown_hashs_item->childCount())+QString(")")) ;

	// Ok, this is a N2 search, but there are very few elements in the list.
	for(int i=2;i<_f2f_TW->topLevelItemCount();)
	{
		bool found = false ;

		if(_f2f_TW->topLevelItem(i)->text(0).left(14) == "Unknown hashes" && unknown_hash_found)
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
	// look for the hash, and insert a new element if necessary.
	//
	QList<QTreeWidgetItem*> items = _f2f_TW->findItems((hash=="")?QString("Unknown hashes"):QString::fromStdString(hash),Qt::MatchStartsWith) ;

	if(items.empty())
	{	
		QStringList stl ;
		stl.push_back((hash=="")?QString("Unknown hashes"):QString::fromStdString(hash)) ;
		QTreeWidgetItem *item = new QTreeWidgetItem(_f2f_TW,stl) ;
		_f2f_TW->insertTopLevelItem(0,item) ;

		return item ;
	}
	else
		return items.front() ;
}

TurtleRouterStatisticsWidget::TurtleRouterStatisticsWidget(QWidget *parent)
	: QWidget(parent)
{
	maxWidth = 200 ;
	maxHeight = 100 ;
}

void TurtleRouterStatisticsWidget::updateTunnelStatistics(const std::vector<std::vector<std::string> >& hashes_info, 
																const std::vector<std::vector<std::string> >& tunnels_info, 
																const std::vector<TurtleRequestDisplayInfo >& search_reqs_info, 
																const std::vector<TurtleRequestDisplayInfo >& tunnel_reqs_info)

{
	QPixmap tmppixmap(maxWidth, maxHeight);
	tmppixmap.fill(this, 0, 0);

	QPainter painter(&tmppixmap);
	painter.initFrom(this);

	// std::cerr << "Drawing into pixmap of size " << maxWidth << "x" << maxHeight << std::endl;
	// draw...
	int ox=5,oy=5 ;
	TRHistogram(search_reqs_info).draw(&painter,ox,oy,QObject::tr("Evolution of search requests:")) ;
	TRHistogram(tunnel_reqs_info).draw(&painter,ox,oy,QObject::tr("Evolution of tunnel requests:")) ;

	// update the pixmap
	pixmap = tmppixmap;
}

void TurtleRouterStatisticsWidget::paintEvent(QPaintEvent *event)
{
    QStylePainter(this).drawPixmap(0, 0, pixmap);
}

void TurtleRouterStatisticsWidget::resizeEvent(QResizeEvent *event)
{
    QRect TaskGraphRect = geometry();
    maxWidth = TaskGraphRect.width();
    maxHeight = TaskGraphRect.height();

	 QWidget::resizeEvent(event);
}


