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

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>

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
    
    ui.onlinesince->setText(QDateTime::currentDateTime().toString(DATETIME_FMT));

}

/** Destructor. */
ProfileWidget::~ProfileWidget()
{

}

void ProfileWidget::closeEvent (QCloseEvent * event)
{
    QWidget::closeEvent(event);
}

void ProfileWidget::showEvent ( QShowEvent * /*event*/ )
{

	RsPeerDetails detail;
    if (rsPeers->getPeerDetails(rsPeers->getOwnId(),detail))
    {

    ui.name->setText(QString::fromUtf8(detail.name.c_str()));
	ui.country->setText(QString::fromUtf8(detail.location.c_str()));
  
    ui.peerid->setText(QString::fromStdString(detail.id));
        
	// Dont Show a timestamp in RS calculate the day
	QDateTime date = QDateTime::fromTime_t(detail.lastConnect);
	QString stime = date.toString(Qt::LocalDate);

    /* set retroshare version */
    std::map<std::string, std::string>::iterator vit;
    std::map<std::string, std::string> versions;
	bool retv = rsDisc->getDiscVersions(versions);
	if (retv && versions.end() != (vit = versions.find(detail.id)))
	{
		ui.version->setText(QString::fromStdString(vit->second));
	}

    ui.ipAddressList->clear();
    for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
    ui.ipAddressList->addItem(QString::fromStdString(*it));


	/* set local address */
	ui.localAddress->setText(QString::fromStdString(detail.localAddr));
    ui.localPort -> setText(QString::number(detail.localPort));
	/* set the server address */
	ui.extAddress->setText(QString::fromStdString(detail.extAddr));
    ui.extPort -> setText(QString::number(detail.extPort));
    /* set DynDNS */
    ui.dyndns->setText(QString::fromStdString(detail.dyndns));
    ui.dyndns->setCursorPosition (0);
	
    std::list<std::string> ids;
    ids.clear();
    rsPeers->getGPGAcceptedList(ids);
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



