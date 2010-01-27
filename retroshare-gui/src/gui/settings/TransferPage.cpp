/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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

#include "TransferPage.h"
#include <gui/TurtleRouterDialog.h>

#include "rshare.h"

#include <iostream>
#include <sstream>

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"

#include <QTimer>

TransferPage::TransferPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui._showTurtleDialogPB,SIGNAL(clicked()),this,SLOT( showTurtleRouterDialog() )) ;

	  ui._enableTurtleCB->setChecked(true) ;
	  ui._enableTurtleCB->setEnabled(false) ;

   QTimer *timer = new QTimer(this);
   timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
   timer->start(1000);

   updateStatus();


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void TransferPage::showTurtleRouterDialog()
{
	TurtleRouterDialog::showUp() ;
}

void
TransferPage::closeEvent (QCloseEvent * event)
{
    QWidget::closeEvent(event);
}


/** Saves the changes on this page */
bool
TransferPage::save(QString &errmsg)
{

  /* save the server address */
  /* save local address */
  /* save the url for DNS access */

  /* restart server */

  /* save all? */
   //saveAddresses();
  return true;
}

/** Loads the settings for this page */
void TransferPage::load()
{

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		return;
	}


}

/** Loads the settings for this page */
void TransferPage::updateStatus()
{

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		return;
	}


}



