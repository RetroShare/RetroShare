#include <iostream>
#include <QTimer>
#include <retroshare/rsturtle.h>
#include "TurtleRouterDialog.h"

static const int MAX_TUNNEL_REQUESTS_DISPLAY = 10 ;

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
}

void TurtleRouterDialog::updateDisplay()
{
	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<std::vector<std::string> > search_reqs_info ;
	std::vector<std::vector<std::string> > tunnel_reqs_info ;

	rsTurtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

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
		QString str = QString::fromStdString( "Request id: " + search_reqs_info[i][0] + "\t from [" + search_reqs_info[i][1] + "]\t " + search_reqs_info[i][2]) ;

		stl.clear() ;
		stl.push_back(str) ;

		top_level_s_requests->addChild(new QTreeWidgetItem(stl)) ;
	}
	top_level_s_requests->setText(0, tr("Search requests") + "(" + QString::number(search_reqs_info.size()) + ")" ) ;

	int reqs_size = tunnel_reqs_info.size() ;

	for(uint i=0;i<reqs_size;++i)
		if(i+MAX_TUNNEL_REQUESTS_DISPLAY >= reqs_size || i < MAX_TUNNEL_REQUESTS_DISPLAY)
		{
			QString str = QString::fromStdString( "Request id: " + tunnel_reqs_info[i][0] + "\t from [" + tunnel_reqs_info[i][1] + "]\t " + tunnel_reqs_info[i][2]) ;

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


