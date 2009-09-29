/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009,  RetroShre Team
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
#include "ProfileWidget.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsdisc.h"

#include "StatusMessage.h"

#include <QTime>

#include <sstream>
#include <iomanip>

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

/** Default constructor */
ProfileWidget::ProfileWidget(QWidget *parent, Qt::WFlags flags)
  : QWidget(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  connect(ui.editstatuspushButton,SIGNAL(clicked()), this, SLOT(statusmessagedlg()));

  loadDialog();
}

void ProfileWidget::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void ProfileWidget::loadDialog()
{

	RsPeerDetails detail;
  if (rsPeers->getPeerDetails(rsPeers->getOwnId(),detail)) 	
  {	

	ui.name->setText(QString::fromStdString(detail.name));
	ui.orgloc->setText(QString::fromStdString(detail.org));
	ui.country->setText(QString::fromStdString(detail.location));
  
  ui.peerid->setText(QString::fromStdString(detail.id));
        
	// Dont Show a timestamp in RS calculate the day
	QDateTime date = QDateTime::fromTime_t(detail.lastConnect);
	QString stime = date.toString(Qt::LocalDate);
  ui.lastcontact-> setText(stime);

    /* set retroshare version */
    std::map<std::string, std::string>::iterator vit;
    std::map<std::string, std::string> versions;
	bool retv = rsDisc->getDiscVersions(versions);
	if (retv && versions.end() != (vit = versions.find(detail.id)))
	{
		ui.version->setText(QString::fromStdString(vit->second));
	}

	/* set local address */
	ui.localAddress->setText(QString::fromStdString(detail.localAddr));
	ui.localPort -> setValue(detail.localPort);
	/* set the server address */
	ui.extAddress->setText(QString::fromStdString(detail.extAddr));
	ui.extPort -> setValue(detail.extPort);
	
	  std::list<std::string> ids;
	  ids.clear();
    rsPeers->getFriendList(ids);
    int friends = ids.size();
    
    std::ostringstream out;
    out << friends << "";
    ui.friendsEdit->setText(QString::fromStdString(out.str()));
	
  }

}

void ProfileWidget::statusmessagedlg()
{
    static StatusMessage *statusmsgdialog = new StatusMessage();
    statusmsgdialog->show();
}



