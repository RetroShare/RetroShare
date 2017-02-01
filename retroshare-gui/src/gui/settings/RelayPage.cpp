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

#include "RelayPage.h"

#include "rshare.h"

#include <iostream>

#include <retroshare/rsiface.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdht.h>

#include <QTimer>

RelayPage::RelayPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

	QObject::connect(ui.noFriendSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.noFOFSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.noGeneralSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.bandFriendSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.bandFOFSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.bandGeneralSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));

	QObject::connect(ui.addPushButton,SIGNAL(clicked()),this,SLOT(addServer()));
	QObject::connect(ui.removePushButton,SIGNAL(clicked()),this,SLOT(removeServer()));
	QObject::connect(ui.DhtLineEdit,SIGNAL(textChanged(const QString &)),this,SLOT(checkKey()));

	QObject::connect(ui.enableCheckBox,SIGNAL(stateChanged(int)),this,SLOT(updateEnabled()));
	QObject::connect(ui.serverCheckBox,SIGNAL(stateChanged(int)),this,SLOT(updateEnabled()));

	QObject::connect(ui.noFriendSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.bandFriendSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.noFOFSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.bandFOFSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.noGeneralSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.bandGeneralSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));

	QObject::connect(ui.enableCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateRelayMode()));
	QObject::connect(ui.serverCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateRelayMode()));
}

QString RelayPage::helpText() const
{
   return tr("<h1><img width=\"24\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Relays</h1>                   \
              <p>By activating relays, you allow your Retroshare node to act as a bridge between Retroshare  \
				  users who cannot connect directly, e.g. because they're firewalled.</p>                        \
				  <p>You may choose to act as a relay by checking <i>enable relay connections</i>, or simply     \
				  benefit from other peers acting as relay, by checking <i>use relay servers</i>. For the former,\
				  you may specify the bandwidth allocated when acting as a relay for friends of you, for friends \
				  of your friends, or anyone in the Retroshare network.</p>                                      \
				  <p>In any case, a Retroshare node acting as a relay cannot see the relayed traffic, since it   \
				  is encrypted and authenticated by the two relayed nodes.</p>") ;
}

void RelayPage::updateTotals()
{
	int nFriends = ui.noFriendSpinBox->value();
	int friendBandwidth = ui.bandFriendSpinBox->value();

	int nFOF = ui.noFOFSpinBox->value();
	int fofBandwidth = ui.bandFOFSpinBox->value();

	int nGeneral = ui.noGeneralSpinBox->value();
	int genBandwidth = ui.bandGeneralSpinBox->value();

	int total = nFriends + nFOF + nGeneral;

	rsDht->setRelayAllowance(RSDHT_RELAY_CLASS_ALL, total, 0);
	rsDht->setRelayAllowance(RSDHT_RELAY_CLASS_FRIENDS, nFriends, 1024 * friendBandwidth);
	rsDht->setRelayAllowance(RSDHT_RELAY_CLASS_FOF, nFOF, 1024 * fofBandwidth);
	rsDht->setRelayAllowance(RSDHT_RELAY_CLASS_GENERAL, nGeneral, 1024 * genBandwidth);
}

/** Saves the changes on this page */

void RelayPage::updateRelayMode()
{
	uint32_t relayMode = 0;
	if (ui.enableCheckBox->isChecked())
	{
		relayMode |= RSDHT_RELAY_ENABLED;

		if (ui.serverCheckBox->isChecked())
		{
			relayMode |= RSDHT_RELAY_MODE_ON;
		}
		else
		{
			relayMode |= RSDHT_RELAY_MODE_OFF;
		}
	}
	else
	{
		relayMode |= RSDHT_RELAY_MODE_OFF;
	}

	rsDht->setRelayMode(relayMode);
}

	/** Loads the settings for this page */
void RelayPage::load()
{
	uint32_t count;
	uint32_t bandwidth;
	rsDht->getRelayAllowance(RSDHT_RELAY_CLASS_FRIENDS, count, bandwidth);
	ui.noFriendSpinBox->setValue(count);
	ui.bandFriendSpinBox->setValue(bandwidth / 1000);

	rsDht->getRelayAllowance(RSDHT_RELAY_CLASS_FOF, count, bandwidth);
	ui.noFOFSpinBox->setValue(count);
	ui.bandFOFSpinBox->setValue(bandwidth / 1000);

	rsDht->getRelayAllowance(RSDHT_RELAY_CLASS_GENERAL, count, bandwidth);
	ui.noGeneralSpinBox->setValue(count);
	ui.bandGeneralSpinBox->setValue(bandwidth / 1000);


	uint32_t relayMode = rsDht->getRelayMode();
	if (relayMode & RSDHT_RELAY_ENABLED)
	{
		ui.enableCheckBox->setCheckState(Qt::Checked);
		if ((relayMode & RSDHT_RELAY_MODE_MASK) == RSDHT_RELAY_MODE_OFF)
		{
			ui.serverCheckBox->setCheckState(Qt::Unchecked);
		}
		else
		{
			ui.serverCheckBox->setCheckState(Qt::Checked);
		}
	}
	else
	{
		ui.enableCheckBox->setCheckState(Qt::Unchecked);
		ui.serverCheckBox->setCheckState(Qt::Unchecked);
	}

	loadServers();
	updateRelayOptions();
	updateEnabled();
	checkKey();
}

void RelayPage::loadServers()
{
	std::list<std::string> servers;
	std::list<std::string>::iterator it;

	rsDht->getRelayServerList(servers);

	ui.serverTreeWidget->clear();
	for(it = servers.begin(); it != servers.end(); ++it)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setData(0, Qt::DisplayRole, QString::fromStdString(*it));	
		ui.serverTreeWidget->addTopLevelItem(item);
	}
}



void RelayPage::updateRelayOptions()
{
	int nFriends = ui.noFriendSpinBox->value();
	int friendBandwidth = ui.bandFriendSpinBox->value();

	int nFOF = ui.noFOFSpinBox->value();
	int fofBandwidth = ui.bandFOFSpinBox->value();

	int nGeneral = ui.noGeneralSpinBox->value();
	int genBandwidth = ui.bandGeneralSpinBox->value();

	ui.totalFriendLineEdit->setText(QString::number(nFriends * friendBandwidth * 2));
	ui.totalFOFLineEdit->setText(QString::number(nFOF * fofBandwidth * 2));
	ui.totalGeneralLineEdit->setText(QString::number(nGeneral * genBandwidth * 2));
	ui.totalBandwidthLineEdit->setText(QString::number((nFriends * friendBandwidth + nFOF * fofBandwidth + nGeneral * genBandwidth) * 2));
	ui.noTotalLineEdit->setText(QString::number(nFriends + nFOF + nGeneral));
}

void RelayPage::updateEnabled()
{
	std::cerr << "RelayPage::updateEnabled()" << std::endl;

	if (ui.enableCheckBox->isChecked())
	{
		ui.groupBox->setEnabled(true);
		if (ui.serverCheckBox->isChecked())
		{
			std::cerr << "RelayPage::updateEnabled() Both Enabled" << std::endl;
			ui.serverGroupBox->setEnabled(true);
		}
		else
		{
			std::cerr << "RelayPage::updateEnabled() Options Only Enabled" << std::endl;
			ui.serverGroupBox->setEnabled(false);
		}
	}
	else
	{
		std::cerr << "RelayPage::updateEnabled() Both Disabled" << std::endl;
		ui.groupBox->setEnabled(false);
		ui.serverGroupBox->setEnabled(false);
	}

}


void RelayPage::checkKey()
{

	std::string server = ui.DhtLineEdit->text().toStdString();
	std::cerr << "RelayPage::checkKey() length: " << server.length();
	std::cerr << std::endl;
	if (server.length() == 40)
	{
		ui.keyOkBox->setChecked(Qt::Checked);
	}
	else
	{
		ui.keyOkBox->setChecked(Qt::Unchecked);
	}
}


void RelayPage::addServer()
{
	std::cerr << "RelayPage::addServer()";
	std::cerr << std::endl;

	if (!ui.keyOkBox->isChecked())
	{
		return;
	}

	std::string server = ui.DhtLineEdit->text().toStdString();

	bool ok = rsDht->addRelayServer(server);
	if (ok)
	{
		ui.DhtLineEdit->setText(QString(""));
	}	
	loadServers();
}

void RelayPage::removeServer()
{
	QTreeWidgetItem *item = ui.serverTreeWidget->currentItem();
	if (item)
	{
		std::string server = item->data(0, Qt::DisplayRole).toString().toStdString();
		rsDht->removeRelayServer(server);
	}

	loadServers();
}
