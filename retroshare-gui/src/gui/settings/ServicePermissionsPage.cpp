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
#include <retroshare/rspeers.h>
#include <retroshare/rsservicecontrol.h>

//#include <retroshare/rsservicecontrol.h>

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
   return tr("<h1><img width=\"24\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Permissions</h1>		   \
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
	std::list<RsPeerId> peerList;
	std::list<RsPeerId>::const_iterator pit;
	rsPeers->getFriendList(peerList);

	RsPeerServiceInfo ownServices;
	std::map<uint32_t, RsServiceInfo>::const_iterator sit;
	rsServiceControl->getOwnServices(ownServices);

     	ui.tableWidget->setRowCount(ownServices.mServiceList.size());
     	ui.tableWidget->setColumnCount(peerList.size() + 1);

	QStringList columnHeaders;
	QStringList rowHeaders;
	columnHeaders.push_back(tr("Default"));
	for(pit = peerList.begin(); pit != peerList.end(); pit++)
	{
		columnHeaders.push_back(QString::fromStdString( rsPeers->getPeerName(*pit)));
	}

	// Fill in CheckBoxes.
	int row;
	int column;
	for(row = 0, sit = ownServices.mServiceList.begin(); sit != ownServices.mServiceList.end(); sit++, row++)
	{
		rowHeaders.push_back(QString::fromStdString(sit->second.mServiceName));
		RsServicePermissions permissions;
		if (!rsServiceControl->getServicePermissions(sit->first, permissions))
		{
			continue;
		}

		{
     			QTableWidgetItem *item = new QTableWidgetItem(tr("Default"));
			Qt::ItemFlags flags(Qt::ItemIsUserCheckable);
			flags |= Qt::ItemIsEnabled;
			item->setFlags(flags);

			if (permissions.mDefaultAllowed)
			{
				item->setCheckState(Qt::Checked);
			}
			else
			{
				item->setCheckState(Qt::Unchecked);
			}
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
			if (permissions.mPeersAllowed.end() != permissions.mPeersAllowed.find(*pit))
			{
				item->setCheckState(Qt::Checked);

			}
			else if (permissions.mPeersDenied.end() != permissions.mPeersDenied.find(*pit))
			{
				item->setCheckState(Qt::Unchecked);
			}
			else
			{
				item->setCheckState(Qt::PartiallyChecked);
			}
			ui.tableWidget->setItem(row, column, item);
		}
	}

	// Now Get a List of Services Provided by Peers - and add text.
	int stdRowCount = ownServices.mServiceList.size();
	int maxRowCount = stdRowCount;
	for(column = 1, pit = peerList.begin(); 
			pit != peerList.end(); pit++, column++)
	{
		RsPeerServiceInfo peerInfo;
		if (rsServiceControl->getServicesProvided(*pit, peerInfo))
		{
			std::map<uint32_t, RsServiceInfo>::const_iterator eit, sit2, eit2;
			sit = ownServices.mServiceList.begin();
			eit = ownServices.mServiceList.end();
			sit2 = peerInfo.mServiceList.begin();
			eit2 = peerInfo.mServiceList.end();
			row = 0;
			int extraRowIndex = stdRowCount;

			while((sit != eit) && (sit2 != eit2))
			{
				if (sit->first < sit2->first)
				{
					// A Service they don't have.
					QTableWidgetItem *item = ui.tableWidget->item(row, column);
					item->setText(tr("No"));

					++sit;
					++row;
				}
				else if (sit2->first < sit->first)
				{
					// They have a service we dont!
					// Add extra item on the end. (TODO)
					if (extraRowIndex + 1 > maxRowCount)
					{
						maxRowCount = extraRowIndex + 1;
     						ui.tableWidget->setRowCount(maxRowCount);
						rowHeaders.push_back(tr("Other Service"));
					}

     					QTableWidgetItem *item = new QTableWidgetItem();
					item->setText(QString::fromStdString(sit2->second.mServiceName));
					ui.tableWidget->setItem(extraRowIndex, column, item);
					++extraRowIndex;
					++sit2;
				}
				else
				{
					QTableWidgetItem *item = ui.tableWidget->item(row, column);
                        		if (ServiceInfoCompatible(sit->second, sit2->second))
					{
						item->setText(tr("Yes"));
					}
					else
					{
						item->setText(tr("Incompatible"));
					}

					// Matching service.
					++row;
					++sit;
					++sit2;
				}
			}

			// More Services they don't have.
			for(; sit != eit; ++sit)
			{
				// A Service they don't have.
				QTableWidgetItem *item = ui.tableWidget->item(row, column);
				item->setText(tr("No"));

				++sit;
				++row;
			}

			//  Services we don't have.
			for(; sit2 != eit2; ++sit2)
			{
				// A Service they don't have.
				// Add extra item on the end. (TODO)
				if (extraRowIndex + 1 > maxRowCount)
				{
					maxRowCount = extraRowIndex + 1;
     					ui.tableWidget->setRowCount(maxRowCount);
					rowHeaders.push_back(tr("Other Service"));
				}

     				QTableWidgetItem *item = new QTableWidgetItem();
				item->setText(QString::fromStdString(sit2->second.mServiceName));
				ui.tableWidget->setItem(extraRowIndex, column, item);
				++extraRowIndex;
				++sit2;
			}
		}
		else
		{
			for(row = 0; row < stdRowCount; row++)
			{
				QTableWidgetItem *item = ui.tableWidget->item(row, column);
				item->setText(tr("N/A"));
			}
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



	




