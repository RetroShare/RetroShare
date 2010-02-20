#include <iostream>
#include <QTimer>
#include <QMenu>
#include <QMouseEvent>
#include <rsiface/rsturtle.h>
#include "TurtleRouterDialog.h"

TurtleRouterDialog::TurtleRouterDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;

	// Init the basic setup.
	//
	QStringList stl ;
	int n=0 ;

	stl.clear() ;
	stl.push_back(QString("Search requests")) ;
	top_level_s_requests = new QTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_s_requests) ;

	stl.clear() ;
	stl.push_back(QString("Tunnel requests")) ;
	top_level_t_requests = new QTreeWidgetItem(_f2f_TW,stl) ;
	_f2f_TW->insertTopLevelItem(n++,top_level_t_requests) ;

	top_level_hashes.clear() ;
}

void TurtleRouterDialog::updateDisplay()
{
	std::cout << "updatign turtle router console."<< std::endl ;

	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<std::vector<std::string> > search_reqs_info ;
	std::vector<std::vector<std::string> > tunnel_reqs_info ;

	rsTurtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

	// now display this in the QTableWidgets

	QStringList stl ;

	// remove all children of top level objects
	for(int i=0;i<_f2f_TW->topLevelItemCount();++i)
		while(_f2f_TW->topLevelItem(i)->takeChild(0) != NULL) ;

	// check that an entry exist for all hashes
	for(uint i=0;i<tunnels_info.size();++i)
	{
		const std::string& hash(tunnels_info[i][3]) ;

		QTreeWidgetItem *parent = findParentHashItem(hash) ;

		QString str = QString::fromStdString("Tunnel id: "+tunnels_info[i][0] + "\t from [" + tunnels_info[i][2] + "] to [" + tunnels_info[i][1] + "]\t\t last transfer: " + tunnels_info[i][4]) ;
		stl.clear() ;
		stl.push_back(str) ;

		new QTreeWidgetItem(parent,stl) ;
	}

	for(uint i=0;i<search_reqs_info.size();++i)
	{
		QString str = QString::fromStdString("Request id: "+search_reqs_info[i][0] + "\t from [" + search_reqs_info[i][1] + "]\t " + search_reqs_info[i][2]) ;

		stl.clear() ;
		stl.push_back(str) ;

		new QTreeWidgetItem(top_level_s_requests,stl) ;
	}

	for(uint i=0;i<tunnel_reqs_info.size();++i)
	{
		QString str = QString::fromStdString("Request id: "+tunnel_reqs_info[i][0] + "\t from [" + tunnel_reqs_info[i][1] + "]\t " + tunnel_reqs_info[i][2]) ;

		stl.clear() ;
		stl.push_back(str) ;

		new QTreeWidgetItem(top_level_t_requests,stl) ;
	}

	for(int i=2;i<_f2f_TW->topLevelItemCount();)
		if(_f2f_TW->topLevelItem(i)->childCount() == 0) 
			_f2f_TW->takeTopLevelItem(i) ;
		else
			++i ;
}
	
QTreeWidgetItem *TurtleRouterDialog::findParentHashItem(const std::string& hash)
{
	// look for the hash, and insert a new element if necessary.
	//
	QList<QTreeWidgetItem*> items = _f2f_TW->findItems((hash=="")?QString("Unknown hashes"):QString::fromStdString(hash),Qt::MatchExactly) ;

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


