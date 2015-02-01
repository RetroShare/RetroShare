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

#define PermissionStateUserRole		(Qt::UserRole)
#define ServiceIdUserRole		(Qt::UserRole + 1)
#define PeerIdUserRole			(Qt::UserRole + 2)

ServicePermissionsPage::ServicePermissionsPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

    //QObject::connect(ui.tableWidget,SIGNAL(itemChanged(QTableWidgetItem *)),  this, SLOT(tableItemChanged(QTableWidgetItem *)));

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
#ifdef SUSPENDED
	std::cerr << "ServicePermissionsPage::save()";
	std::cerr << std::endl;
	size_t row, column;
	for(row = 0; row < mStdRowCount; ++row)
	{
		bool requiresSaving = false;
		bool defaultOn = true;
		uint32_t serviceId;

		{
			QTableWidgetItem *item = ui.tableWidget->item (row, 0);
			QVariant qvDefault   = item->data(PermissionStateUserRole);
			QVariant qvServiceId = item->data(ServiceIdUserRole);
			serviceId = qvServiceId.toUInt();
			Qt::CheckState currentState = item->checkState();
			Qt::CheckState origState = (Qt::CheckState) qvDefault.toUInt();

			if (currentState != origState)
			{
				requiresSaving = true;
			}
			defaultOn = (currentState == Qt::Checked);
		}

		if (!requiresSaving)
		{
			for(column = 1; column < mStdColumnCount; ++column)
			{
				QTableWidgetItem *item = ui.tableWidget->item (row, column);
				Qt::CheckState currentState = item->checkState();
				QVariant qvOrigState = item->data(PermissionStateUserRole);
				Qt::CheckState origState = (Qt::CheckState) qvOrigState.toUInt();
				
				if (currentState != origState)
				{
					requiresSaving = true;
					break;
				}
			}
		}

		if (requiresSaving)
		{
			RsServicePermissions permissions;
			permissions.mDefaultAllowed = defaultOn;
			permissions.mServiceId = serviceId;

			std::cerr << "ServicePermissionsPage::save() saving row: " << row;
			std::cerr << " serviceId: " << serviceId;
			std::cerr << " defaultAllowed: " << defaultOn;
			std::cerr << std::endl;

			for(column = 1; column < mStdColumnCount; ++column)
			{
				QTableWidgetItem *item = ui.tableWidget->item (row, column);
				Qt::CheckState currentState = item->checkState();
			
				QTableWidgetItem *iditem = ui.tableWidget->item (0, column);
				RsPeerId peerId(iditem->data(PeerIdUserRole).toString().toStdString());

				switch(currentState)
				{
					case Qt::Checked:
						std::cerr << "ServicePermissionsPage::save() peer: " << peerId.toStdString();
						std::cerr << " Allowed";
						std::cerr << std::endl;
						permissions.mPeersAllowed.insert(peerId);
						break;
					case Qt::Unchecked:
						std::cerr << "ServicePermissionsPage::save() peer: " << peerId.toStdString();
						std::cerr << " Allowed";
						std::cerr << std::endl;
						permissions.mPeersDenied.insert(peerId);
						break;
					case Qt::PartiallyChecked:
						std::cerr << "ServicePermissionsPage::save() peer: " << peerId.toStdString();
						std::cerr << " Default";
						std::cerr << std::endl;
						/* default */	
						break;
				}
			}
			rsServiceControl->updateServicePermissions(serviceId, permissions);
		}
    }
#endif
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

#ifdef SUSPENDED
	mStdRowCount = ownServices.mServiceList.size();
	mStdColumnCount = peerList.size() + 1;
     	ui.tableWidget->setRowCount(mStdRowCount);
     	ui.tableWidget->setColumnCount(mStdColumnCount);

	QStringList columnHeaders;
	QStringList rowHeaders;
	columnHeaders.push_back(tr("Default"));
	for(pit = peerList.begin(); pit != peerList.end(); ++pit)
	{
		columnHeaders.push_back(QString::fromUtf8(rsPeers->getPeerName(*pit).c_str()));
	}

	// Fill in CheckBoxes.
	size_t row;
	size_t column;
	for(row = 0, sit = ownServices.mServiceList.begin(); sit != ownServices.mServiceList.end(); ++sit, ++row)
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
				item->setData(PermissionStateUserRole, QVariant((int) (Qt::Checked)));
			}
			else
			{
				item->setCheckState(Qt::Unchecked);
				item->setData(PermissionStateUserRole, QVariant((int) (Qt::Unchecked)));
			}
			item->setData(ServiceIdUserRole, QVariant((uint) sit->first));
			ui.tableWidget->setItem(row, 0, item);
		}

		for(column = 1, pit = peerList.begin(); 
				pit != peerList.end(); ++pit, ++column)
		{
     			QTableWidgetItem *item = new QTableWidgetItem();
			Qt::ItemFlags flags(Qt::ItemIsUserCheckable);
			flags |= Qt::ItemIsEnabled;
			flags |= Qt::ItemIsTristate;
			item->setFlags(flags);
			if (permissions.mPeersAllowed.end() != permissions.mPeersAllowed.find(*pit))
			{
				item->setCheckState(Qt::Checked);
				item->setData(PermissionStateUserRole, QVariant((int) (Qt::Checked)));

			}
			else if (permissions.mPeersDenied.end() != permissions.mPeersDenied.find(*pit))
			{
				item->setCheckState(Qt::Unchecked);
				item->setData(PermissionStateUserRole, QVariant((int) (Qt::Unchecked)));
			}
			else
			{
				item->setCheckState(Qt::PartiallyChecked);
				item->setData(PermissionStateUserRole, QVariant((int) (Qt::PartiallyChecked)));
			}
			ui.tableWidget->setItem(row, column, item);

			if (row == 0)
			{
				item->setData(PeerIdUserRole, QVariant(QString::fromStdString(pit->toStdString())));
			}
		}
	}

	// Now Get a List of Services Provided by Peers - and add text.
	int maxRowCount = mStdRowCount;
	for(column = 1, pit = peerList.begin(); 
			pit != peerList.end(); ++pit, ++column)
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
			int extraRowIndex = mStdRowCount;

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
			}
		}
		else
		{
			for(row = 0; row < mStdRowCount; ++row)
			{
				QTableWidgetItem *item = ui.tableWidget->item(row, column);
				item->setText(tr("N/A"));
			}
		}
	}

	ui.tableWidget->setHorizontalHeaderLabels(columnHeaders);
    ui.tableWidget->setVerticalHeaderLabels(rowHeaders);
#endif
}

void ServicePermissionsPage::tableItemChanged ( int row, int col )
{
#ifdef SUSPENDED
	std::cerr << "ServicePermissionsPage::tableItemChanged()";
	std::cerr << std::endl;
	std::cerr << "\t Node: Row: " << item->row() << " Column: " << item->column();
	std::cerr << std::endl;
	std::cerr << "\t IsChecked: " << (Qt::Checked == item->checkState());
	std::cerr << std::endl;

	if (item->column() == 0)
	{
		/* update the row */
		bool defaultOn = (Qt::Checked == item->checkState());
		item->setBackground(getColor(defaultOn, Qt::PartiallyChecked));
		for(int column = 1; column < ui.tableWidget->columnCount(); ++column)
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
#endif
}



