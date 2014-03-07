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

#include "ServicePermissionsPage.h"

#include "rshare.h"

#include <retroshare/rsservicecontrol.h>

#include <iostream>
#include <QTimer>

ServicePermissionsPage::ServicePermissionsPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

	QObject::connect(ui.tableWidget,SIGNAL(itemChanged(QTableWidgetItem *)), 
	this, SLOT(tableItemChanged(QTableWidgetItem *))); 

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

QString ServicePermissionsPage::helpText() const
{
   return tr("<h1><img width=\"24\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Permissions</h1>                   \
<p>Permissions allow you to control which services are available to which friends </p>");
}

	/** Saves the changes on this page */
bool ServicePermissionsPage::save(QString &/*errmsg*/)
{

	return true;
}


QBrush getColor(bool defaultOn, Qt::CheckState checkState)
{
	switch(checkState)
	{
		case Qt::Unchecked:
			return QBrush(Qt::red);
			break;
		case Qt::Checked:
			return QBrush(Qt::green);
			break;
		case Qt::PartiallyChecked:
		default:
			if (defaultOn)
			{
				return QBrush(Qt::darkGreen);
			}
			else
			{
				return QBrush(Qt::darkRed);
			}
			break;
	}
	return QBrush(Qt::gray);
}

	/** Loads the settings for this page */
void ServicePermissionsPage::load()
{
	/* get all the peers */
	/* get all the services */

	std::list<std::string> peerList;
	std::list<std::string>::const_iterator pit;
	peerList.push_back("peer1");
	peerList.push_back("peer2");
	peerList.push_back("peer3");
	peerList.push_back("peer4");

	std::list<std::string> serviceList;
	std::list<std::string>::const_iterator sit;
	serviceList.push_back("service1");
	serviceList.push_back("service2");
	serviceList.push_back("service3");
	serviceList.push_back("service4");

     	ui.tableWidget->setRowCount(serviceList.size());
     	ui.tableWidget->setColumnCount(peerList.size() + 1);

	QStringList columnHeaders;
	QStringList rowHeaders;
	columnHeaders.push_back(tr("Default"));
	for(pit = peerList.begin(); pit != peerList.end(); pit++)
	{
		columnHeaders.push_back(QString::fromStdString(*pit));
	}

	int row;
	int column;
	for(row = 0, sit = serviceList.begin(); sit != serviceList.end(); sit++, row++)
	{
		rowHeaders.push_back(QString::fromStdString(*sit));
		{
     			QTableWidgetItem *item = new QTableWidgetItem(tr("Default"));
			Qt::ItemFlags flags(Qt::ItemIsUserCheckable);
			flags |= Qt::ItemIsEnabled;
			item->setFlags(flags);
			item->setCheckState(Qt::Checked);
			ui.tableWidget->setItem(row, 0, item);
		}

		for(column = 1, pit = peerList.begin(); 
				pit != peerList.end(); pit++, column++)
		{
     			QTableWidgetItem *item = new QTableWidgetItem();
			Qt::ItemFlags flags(Qt::ItemIsUserCheckable);
			flags |= Qt::ItemIsEnabled;
			flags |= Qt::ItemIsTristate;
			item->setFlags(flags);
			item->setCheckState(Qt::PartiallyChecked);
			ui.tableWidget->setItem(row, column, item);
		}
	}

	ui.tableWidget->setHorizontalHeaderLabels(columnHeaders);
	ui.tableWidget->setVerticalHeaderLabels(rowHeaders);
}

void ServicePermissionsPage::tableItemChanged ( QTableWidgetItem * item ) 
{
	std::cerr << "ServicePermissionsPage::tableItemChanged()";
	std::cerr << std::endl;
	std::cerr << "\t Location: Row: " << item->row() << " Column: " << item->column();
	std::cerr << std::endl;
	std::cerr << "\t IsChecked: " << (Qt::Checked == item->checkState());
	std::cerr << std::endl;

	if (item->column() == 0)
	{
		/* update the row */
		bool defaultOn = (Qt::Checked == item->checkState());
		item->setBackground(getColor(defaultOn, Qt::PartiallyChecked));
		for(int column = 1; column < ui.tableWidget->columnCount(); column++)
		{
			QTableWidgetItem *rowitem = ui.tableWidget->item (item->row(), column);
			if (rowitem)
			{
				rowitem->setBackground(getColor(defaultOn, rowitem->checkState()));
			}
		}
	}
	else
	{
		QTableWidgetItem *defitem = ui.tableWidget->item (item->row(), 0);
		if (defitem)
		{
			bool defaultOn = (Qt::Checked == defitem->checkState());
			item->setBackground(getColor(defaultOn, item->checkState()));
		}
	}
}



	




