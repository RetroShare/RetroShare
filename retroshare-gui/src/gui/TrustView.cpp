#include <math.h>
#include <QWheelEvent>
#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"

#include <QTableView>
#include "TrustView.h"

using namespace std ;

TrustView::TrustView()
{
	setupUi(this) ;

	trustTableTW->setMouseTracking(true) ;
	trustInconsistencyCB->setEnabled(false) ;

	zoomHS->setValue(100) ;

	QObject::connect(zoomHS,SIGNAL(valueChanged(int)),this,SLOT(updateZoom(int))) ;
	QObject::connect(updatePB,SIGNAL(clicked()),this,SLOT(update())) ;
	QObject::connect(trustTableTW,SIGNAL(cellClicked(int,int)),this,SLOT(selectCell(int,int))) ;

	update() ;
}

void TrustView::wheelEvent(QWheelEvent *e)
{
	if(e->modifiers() & Qt::ShiftModifier)
	{
		if(e->delta() > 0)
			zoomHS->setValue(zoomHS->value()-5) ;
		else
			zoomHS->setValue(zoomHS->value()+5) ;
	}
}

void TrustView::selectCell(int row,int col)
{
	static int last_row = -1 ;
	static int last_col = -1 ;

	if(last_row > -1)
	{
		int col_s,row_s ;
		getCellSize(zoomHS->value(),col_s,row_s) ;

		trustTableTW->setColumnWidth(last_col,col_s) ;
		trustTableTW->setRowHeight(last_row,row_s) ;
	}

	if(row != last_row || col != last_col)
	{
		trustTableTW->setColumnWidth(col,_base_cell_width) ;
		trustTableTW->setRowHeight(row,_base_cell_height) ;
	}

	last_col = col ;
	last_row = row ;
}

void TrustView::getCellSize(int z,int& col_s,int& row_s) const
{
	col_s = max(10,(int)rint( z/100.0 * _base_cell_width )) ;
	row_s = max(10,(int)rint( z/100.0 * _base_cell_height)) ;
}

void TrustView::updateZoom(int z)
{
	int col_s,row_s ;
	getCellSize(z,col_s,row_s) ;

	for(int i=0;i<trustTableTW->columnCount();++i)
		trustTableTW->setColumnWidth(i,col_s) ;

	for(int i=0;i<trustTableTW->rowCount();++i)
		trustTableTW->setRowHeight(i,row_s) ;

	cout << "updated zoom" << endl;
}

int TrustView::getRowColId(const string& name)
{
	static map<string,int> nameToRow ;

	map<string,int>::const_iterator itpr(nameToRow.find( name )) ;
	int i ;

	if(itpr == nameToRow.end())
	{
		i = trustTableTW->columnCount() ;
		cout << "  -> peer not in table. Creating entry # " << i << endl ;

		trustTableTW->insertColumn(i) ;
		trustTableTW->insertRow(i) ;

		nameToRow[name] = i ;

		trustTableTW->setHorizontalHeaderItem(i,new QTableWidgetItem(QString(name.c_str()))) ;
		trustTableTW->setVerticalHeaderItem(i,new QTableWidgetItem(QString(name.c_str()))) ;

		trustTableTW->setColumnWidth(i,_base_cell_width) ;
		trustTableTW->setRowHeight(i,_base_cell_height) ;
	}
	else
		i = (*itpr).second ;

	return i ;
}

void TrustView::update()
{
	// collect info.

	std::list<std::string> neighs;

	if(!rsPeers->getOthersList(neighs))
		return ;

	neighs.push_back(rsPeers->getOwnId()) ;

	trustTableTW->setSortingEnabled(false) ;

	RsPeerDetails details ;

#ifdef A_VIRER
	// Build rows and columns
	//
	for(list<string>::const_iterator it(neighs.begin()); it != neighs.end(); it++)
	{
		cout << "Looking for peer " << *it << endl ;

		if(!rsPeers->getPeerDetails(*it,details)) 
		{
			cout << "  -> no details" << endl ;
			continue ;
		}

		cout << "  -> name = " << details.name << endl ;

		int i = getRowColId( details.name ) ;

	}
#endif

	// Fill everything
	for(list<string>::const_iterator it1(neighs.begin()); it1 != neighs.end(); ++it1)
	{
		if(!rsPeers->getPeerDetails(*it1,details)) 
			continue ;

		cout << "treating neigh = " << details.name << endl ;
		cout << "  signers = " ;
		int i = getRowColId(details.name) ;

		for(list<string>::const_iterator it2(details.signers.begin());it2!=details.signers.end();++it2) 
		{
			cout << *it2 << " " ;
			// Signers are identified by there name, so if we have twice the same signers, this gets crappy.

			int j = getRowColId(*it2) ;

			QString trr( (i==j)?"Self":"Trust") ;

			if(trustTableTW->item(i,j) == NULL)
				trustTableTW->setItem(i,j,new QTableWidgetItem(trr)) ;
			else
				trustTableTW->item(i,j)->setText(trr) ;
		}
		cout << endl ;
	}
	// assign colors
	for(int i=0;i<trustTableTW->rowCount();++i)
		for(int j=0;j<trustTableTW->columnCount();++j)
		{
			QTableWidgetItem *i_ij(trustTableTW->item(i,j)) ;
			QTableWidgetItem *i_ji(trustTableTW->item(j,i)) ;

			QColor color ;

			// check bidirectional trust
			//
			if(i_ij != NULL)
			{
				if(i_ji == NULL)
				{
					i_ij->setBackgroundColor(Qt::yellow) ;
					i_ij->setToolTip(trustTableTW->horizontalHeaderItem(i)->text() + QString(" is trusted (one way) by " )+trustTableTW->verticalHeaderItem(j)->text()) ;
				}
				else
				{
					if(i==j)
					{
						i_ij->setBackgroundColor(Qt::red) ;
						i_ij->setToolTip(trustTableTW->horizontalHeaderItem(i)->text() + QString(" trusts himself") ) ;
					}
					else
					{
						i_ij->setBackgroundColor(Qt::green) ;
						i_ij->setToolTip(trustTableTW->horizontalHeaderItem(i)->text() + " and " +trustTableTW->verticalHeaderItem(j)->text() + QString(" trust each others") ) ;
					}
				}
			}
		}
}


