#include <math.h>
#include <QTimer>
#include <QWheelEvent>
#include <QHeaderView>
#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>

#include <QTableView>
#include "TrustView.h"

#include <list>

using namespace std ;

TrustView::TrustView()
	: RsAutoUpdatePage(10000)
{
	setupUi(this) ;

	trustTableTW->setMouseTracking(true) ;
//	trustInconsistencyCB->setEnabled(false) ;

	zoomHS->setValue(100) ;

	QObject::connect(zoomHS,SIGNAL(valueChanged(int)),this,SLOT(updateZoom(int))) ;
	QObject::connect(updatePB,SIGNAL(clicked()),this,SLOT(update())) ;
	QObject::connect(trustTableTW,SIGNAL(cellClicked(int,int)),this,SLOT(selectCell(int,int))) ;
	QObject::connect(trustTableTW->verticalHeader(),SIGNAL(sectionClicked(int)),this,SLOT(hideShowPeers(int))) ;
	QObject::connect(trustTableTW->horizontalHeader(),SIGNAL(sectionClicked(int)),this,SLOT(hideShowPeers(int))) ;

	updatePB->setToolTip(tr("This table normally auto-updates every 10 seconds.")) ;
}

void TrustView::showEvent(QShowEvent *e)
{
	QWidget::showEvent(e) ;
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
#ifdef DEBUG_TRUSTVIEW
	cout << "row = " << row << ", column = " << col << endl;
	if(row == 0 || col == 0)
		cout << "***********************************************" << endl ;
#endif
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

		last_col = col ;
		last_row = row ;
	}
	else
	{
		last_col = -1 ;
		last_row = -1 ;
	}
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

//	cout << "updated zoom" << endl;
}

int TrustView::getRowColId(const string& peerid)
{
	static map<string,int> peeridToRow ;

	map<string,int>::const_iterator itpr(peeridToRow.find( peerid )) ;
	int i ;

	if(itpr == peeridToRow.end())
	{
		i = trustTableTW->columnCount() ;

		trustTableTW->insertColumn(i) ;
		trustTableTW->insertRow(i) ;

		peeridToRow[peerid] = i ;

		QString name = QString::fromUtf8(rsPeers->getPeerName(peerid).c_str()) ;

		trustTableTW->setHorizontalHeaderItem(i,new QTableWidgetItem(name)) ;
		trustTableTW->setVerticalHeaderItem(i,new QTableWidgetItem(name)) ;

		trustTableTW->setColumnWidth(i,_base_cell_width) ;
		trustTableTW->setRowHeight(i,_base_cell_height) ;
	}
	else
		i = (*itpr).second ;

	return i ;
}

void TrustView::updateDisplay()
{
	update() ;
}
void TrustView::update()
{
	std::list<std::string> neighs;

	if(!rsPeers->getGPGAllList(neighs))
		return ;

	neighs.push_back(rsPeers->getGPGOwnId()) ;

	trustTableTW->setSortingEnabled(false) ;

	RsPeerDetails details ;

	// Fill everything
	for(list<string>::const_iterator it1(neighs.begin()); it1 != neighs.end(); ++it1)
	{
		std::list<std::string> friends_ids ;

		if(!rsPeers->getPeerDetails(*it1,details)) 
			continue ;

		int i = getRowColId(details.id) ;
		std::string issuer(details.issuer) ;	// the one we check for trust.

		for(list<string>::const_iterator it2(details.gpgSigners.begin());it2!=details.gpgSigners.end();++it2)
		{
#ifdef DEBUG_TRUSTVIEW
			cout << *it2 << " " ;
#endif

			int j = getRowColId(*it2) ;

			QString trr( (i==j)?tr("Self"):tr("Trust")) ;

			if(trustTableTW->item(i,j) == NULL)
				trustTableTW->setItem(i,j,new QTableWidgetItem(trr)) ;
			else
				trustTableTW->item(i,j)->setText(trr) ;
		}
		//		cout << endl ;
	}
	// assign colors
	vector<int> ni(trustTableTW->rowCount(),0) ;
	vector<int> nj(trustTableTW->columnCount(),0) ;

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
				++ni[i] ;
				++nj[j] ;

				if(i_ji == NULL)
				{
					i_ij->setBackgroundColor(Qt::yellow) ;
					i_ij->setToolTip(trustTableTW->horizontalHeaderItem(i)->text() + tr(" is authenticated (one way) by " )+trustTableTW->verticalHeaderItem(j)->text()) ;
					i_ij->setText(tr("Half")) ;
				}
				else
				{
					if(i==j)
					{
						i_ij->setBackgroundColor(Qt::red) ;
						i_ij->setToolTip(trustTableTW->horizontalHeaderItem(i)->text() + tr(" authenticated himself") ) ;
					}
					else
					{
						i_ij->setBackgroundColor(Qt::green) ;
						i_ij->setToolTip(trustTableTW->horizontalHeaderItem(i)->text() + " and " +trustTableTW->verticalHeaderItem(j)->text() + tr(" authenticated each other") ) ;
						i_ij->setText(tr("Full")) ;
					}
				}
			}
		}
	for(int i=0;i<trustTableTW->rowCount();++i)
		trustTableTW->verticalHeaderItem(i)->setToolTip(trustTableTW->verticalHeaderItem(i)->text()+ tr(" is authenticated by ") + QString::number(ni[i]) + tr(" peers, including him(her)self.")) ;

	for(int j=0;j<trustTableTW->columnCount();++j)
		trustTableTW->horizontalHeaderItem(j)->setToolTip(trustTableTW->horizontalHeaderItem(j)->text()+ tr(" authenticated ") + QString::number(nj[j]) + tr(" peers, including him(her)self.")) ;

}

void TrustView::hideShowPeers(int col)
{
	// Choose what to show/hide
	//

	static int last_col = -1 ;
	
	if(col == last_col)
	{
		for(int i=0;i<trustTableTW->rowCount();++i)
		{
			trustTableTW->setColumnHidden(i,false) ;
			trustTableTW->setRowHidden(i,false) ;
		}
		last_col = -1 ;

		showingLabel->setText(tr("Showing: whole network")) ;
	}
	else
	{
		for(int i=0;i<trustTableTW->rowCount();++i)
			if(trustTableTW->item(i,col) == NULL && trustTableTW->item(col,i) == NULL) 
			{
				trustTableTW->setColumnHidden(i,true) ;
				trustTableTW->setRowHidden(i,true) ;
			}
			else
			{
				trustTableTW->setColumnHidden(i,false) ;
				trustTableTW->setRowHidden(i,false) ;
			}
		last_col = col ;
		showingLabel->setText(tr("Showing: peers connected to ")+trustTableTW->verticalHeaderItem(col)->text()) ;
	}
}


