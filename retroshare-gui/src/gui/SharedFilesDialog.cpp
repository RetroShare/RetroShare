/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#include "rshare.h"
#include "SharedFilesDialog.h"
#include "rsiface/rsiface.h"
#include "rsiface/RemoteDirModel.h"
#include "util/RsAction.h"
#include "msgs/ChanMsgDialog.h"

#include <iostream>
#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>

/* Images for context menu icons */
#define IMAGE_DOWNLOAD       ":/images/start.png"

/** Constructor */
SharedFilesDialog::SharedFilesDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.localDirTreeView, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( shareddirtreeWidgetCostumPopupMenu( QPoint ) ) );

  connect( ui.remoteDirTreeView, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( shareddirtreeviewCostumPopupMenu( QPoint ) ) );

/*
  connect( ui.remoteDirTreeView, SIGNAL( itemExpanded( QTreeWidgetItem * ) ), 
	this, SLOT( checkForLocalDirRequest( QTreeWidgetItem * ) ) );

  connect( ui.localDirTreeWidget, SIGNAL( itemExpanded( QTreeWidgetItem * ) ), 
	this, SLOT( checkForRemoteDirRequest( QTreeWidgetItem * ) ) );
*/


  model = new RemoteDirModel(true);
  localModel = new RemoteDirModel(false);
  ui.remoteDirTreeView->setModel(model);
  ui.localDirTreeView->setModel(localModel);

  connect( ui.remoteDirTreeView, SIGNAL( collapsed(const QModelIndex & ) ),
  	model, SLOT(  collapsed(const QModelIndex & ) ) );
  connect( ui.remoteDirTreeView, SIGNAL( expanded(const QModelIndex & ) ),
  	model, SLOT(  expanded(const QModelIndex & ) ) );

  connect( ui.localDirTreeView, SIGNAL( collapsed(const QModelIndex & ) ),
  	localModel, SLOT(  collapsed(const QModelIndex & ) ) );
  connect( ui.localDirTreeView, SIGNAL( expanded(const QModelIndex & ) ),
  	localModel, SLOT(  expanded(const QModelIndex & ) ) );

  
  /* Set header resize modes and initial section sizes  */
	QHeaderView * l_header = ui.localDirTreeView->header () ;   
	l_header->setResizeMode (0, QHeaderView::Interactive);
	l_header->setResizeMode (1, QHeaderView::Interactive);
	l_header->setResizeMode (2, QHeaderView::Interactive);
	l_header->setResizeMode (3, QHeaderView::Interactive);
   
	l_header->resizeSection ( 0, 210 );
	l_header->resizeSection ( 1, 100 );
	l_header->resizeSection ( 2, 100 );
	l_header->resizeSection ( 3, 100 );
	
	/* Set header resize modes and initial section sizes */
	QHeaderView * r_header = ui.remoteDirTreeView->header () ;   
	r_header->setResizeMode (0, QHeaderView::Interactive);
	r_header->setResizeMode (1, QHeaderView::Interactive);
	r_header->setResizeMode (2, QHeaderView::Interactive);
	r_header->setResizeMode (3, QHeaderView::Interactive);
   
	r_header->resizeSection ( 0, 210 );
	r_header->resizeSection ( 1, 100 );
	r_header->resizeSection ( 2, 100 );
	r_header->resizeSection ( 3, 100 );

  /* Set Multi Selection */
  ui.remoteDirTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui.localDirTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void SharedFilesDialog::shareddirtreeviewCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      downloadAct = new QAction(QIcon(IMAGE_DOWNLOAD), tr( "Download" ), this );
      connect( downloadAct , SIGNAL( triggered() ), this, SLOT( downloadRemoteSelected() ) );
      
    //  addMsgAct = new QAction( tr( "Add to Message" ), this );
    //  connect( addMsgAct , SIGNAL( triggered() ), this, SLOT( addMsgRemoteSelected() ) );
      

      contextMnu.clear();
      contextMnu.addAction( downloadAct);
   //   contextMnu.addAction( addMsgAct);
      contextMnu.exec( mevent->globalPos() );
}


void SharedFilesDialog::downloadRemoteSelected()
{
  /* call back to the model (which does all the interfacing? */

  std::cerr << "Downloading Files";
  std::cerr << std::endl;

  QItemSelectionModel *qism = ui.remoteDirTreeView->selectionModel();
  model -> downloadSelected(qism->selectedIndexes());

}


void SharedFilesDialog::addMsgRemoteSelected()
{
  /* call back to the model (which does all the interfacing? */

  std::cerr << "Recommending Files";
  std::cerr << std::endl;

  QItemSelectionModel *qism = ui.remoteDirTreeView->selectionModel();
  model -> recommendSelected(qism->selectedIndexes());


}



void SharedFilesDialog::recommendfile()
{
  /* call back to the model (which does all the interfacing? */

  std::cerr << "Recommending Files";
  std::cerr << std::endl;

  QItemSelectionModel *qism = ui.localDirTreeView->selectionModel();
  localModel -> recommendSelected(qism->selectedIndexes());
}




void SharedFilesDialog::recommendFileSetOnly()
{
  /* call back to the model (which does all the interfacing? */

  std::cerr << "Recommending File Set (clearing old selection)";
  std::cerr << std::endl;

  /* clear current recommend Selection done by model */

  QItemSelectionModel *qism = ui.localDirTreeView->selectionModel();
  localModel -> recommendSelectedOnly(qism->selectedIndexes());
}


void SharedFilesDialog::recommendFilesTo( std::string rsid )
{
  recommendFileSetOnly();
  rsicontrol -> ClearInMsg();
  rsicontrol -> SetInMsg(rsid, true);
 
  /* create a message */
  ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

  /* fill it in 
   * files are receommended already
   * just need to set peers
   */
  std::cerr << "SharedFilesDialog::recommendFilesTo()" << std::endl;
  nMsgDialog->newMsg();
  nMsgDialog->insertTitleText("Recommendation(s)");
  nMsgDialog->insertMsgText("Recommendation(s)");

  nMsgDialog->sendMessage();
  nMsgDialog->close();
}



void SharedFilesDialog::recommendFilesToMsg( std::string rsid )
{
  recommendFileSetOnly();

  rsicontrol -> ClearInMsg();
  rsicontrol -> SetInMsg(rsid, true);

  /* create a message */
  ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

  /* fill it in 
   * files are receommended already
   * just need to set peers
   */
  std::cerr << "SharedFilesDialog::recommendFilesToMsg()" << std::endl;
  nMsgDialog->newMsg();
  nMsgDialog->insertTitleText("Recommendation(s)");
  nMsgDialog->insertMsgText("Recommendation(s)");

  nMsgDialog->show();
}



void SharedFilesDialog::openfile()
{
  /* call back to the model (which does all the interfacing? */

  std::cerr << "Opening File";
  std::cerr << std::endl;

  QItemSelectionModel *qism = ui.localDirTreeView->selectionModel();
  model -> openSelected(qism->selectedIndexes());

}


void SharedFilesDialog::openfolder()
{

}


void  SharedFilesDialog::preModDirectories(bool update_local)
{
	if (update_local)
	{
		localModel->preMods();
	}
	else
	{
		model->preMods();
	}
}


void  SharedFilesDialog::ModDirectories(bool update_local)
{
	if (update_local)
	{
		localModel->postMods();
	}
	else
	{
		model->postMods();
	}
}


void SharedFilesDialog::shareddirtreeWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu2( this );
      QMouseEvent *mevent2 = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      openfileAct = new QAction( tr( "Add to Recommend List" ), this );
      connect( openfileAct , SIGNAL( triggered() ), this, SLOT( recommendfile() ) );
      
     // openfolderAct = new QAction( tr( "Play File" ), this );
     // connect( openfolderAct , SIGNAL( triggered() ), this, SLOT( openfile() ) );
     //
	/* now we're going to ask who to recommend it to...
	 * First Level.
	 *
	 * Add to Recommend List.
	 * Recommend to >
	 * 	all.
	 * 	list of <people>
	 * Recommend with msg >
	 *	all.
	 * 	list of <people>
	 *
	 */

      	QMenu *recMenu = new QMenu( tr("Recommend To "), this );
      	QMenu *msgMenu = new QMenu( tr("Message Friend "), &contextMnu2 );


        rsiface->lockData(); /* Lock Interface */

        std::map<RsCertId,NeighbourInfo>::const_iterator it;
        const std::map<RsCertId,NeighbourInfo> &friends =
                                rsiface->getFriendMap();

	for(it = friends.begin(); it != friends.end(); it++)
	{
		/* parents are
		 * 	recMenu
		 * 	msgMenu
		 */
		std::string rsid;
		{
			std::ostringstream out;
		        out << it -> second.id;
			rsid = out.str();
		}

      		RsAction *qaf1 = new RsAction( QString::fromStdString( it->second.name ), recMenu, rsid );
      		connect( qaf1 , SIGNAL( triggeredId( std::string ) ), this, SLOT( recommendFilesTo( std::string ) ) );
        	recMenu->addAction(qaf1);

      		RsAction *qaf2 = new RsAction( QString::fromStdString( it->second.name ), msgMenu, rsid );
      		connect( qaf2 , SIGNAL( triggeredId( std::string ) ), this, SLOT( recommendFilesToMsg( std::string ) ) );
        	msgMenu->addAction(qaf2);

		/* create list of ids */

	}

        rsiface->unlockData(); /* UnLock Interface */


        contextMnu2.addAction( openfileAct);
        contextMnu2.addMenu( recMenu);
        contextMnu2.addMenu( msgMenu);


        contextMnu2.exec( mevent2->globalPos() );


}

