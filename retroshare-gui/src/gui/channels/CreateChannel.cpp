/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include <QMessageBox>
#include <QBuffer>
#include <QFileDialog>

#include <algorithm>

#include "CreateChannel.h"

#include <retroshare/rschannels.h>
#include <retroshare/rspeers.h>

/** Constructor */
CreateChannel::CreateChannel(QWidget *parent)
: QDialog(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
  
    picture = NULL;

    // connect up the buttons.
    connect( ui.cancelButton, SIGNAL( clicked ( bool ) ), this, SLOT( cancelChannel( ) ) );
    connect( ui.createButton, SIGNAL( clicked ( bool ) ), this, SLOT( createChannel( ) ) );
  
    connect( ui.LogoButton, SIGNAL(clicked() ), this , SLOT(addChannelLogo()));	
    connect( ui.ChannelLogoButton, SIGNAL(clicked() ), this , SLOT(addChannelLogo()));
	connect( ui.pubKeyShare_cb, SIGNAL( clicked() ), this, SLOT( setShareList( ) ));
	connect( ui.keyShareList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
	        this, SLOT(togglePersonItem( QTreeWidgetItem *, int ) ));

	if(!ui.pubKeyShare_cb->isChecked()){

		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(),
				this->size().height());
	}

    newChannel();

}

void CreateChannel::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QWidget::show();

  }
}

void CreateChannel::setShareList(){

	if(ui.pubKeyShare_cb->isChecked()){
		this->resize(this->size().width() + ui.contactsdockWidget->size().width(),
				this->size().height());
		ui.contactsdockWidget->show();


		if (!rsPeers)
		{
			/* not ready yet! */
			return;
		}

		std::list<std::string> peers;
		std::list<std::string>::iterator it;

		rsPeers->getFriendList(peers);

	    /* get a link to the table */
	    QTreeWidget *shareWidget = ui.keyShareList;

	    QList<QTreeWidgetItem *> items;

		for(it = peers.begin(); it != peers.end(); it++)
		{

			RsPeerDetails detail;
			if (!rsPeers->getPeerDetails(*it, detail))
			{
				continue; /* BAD */
			}

			/* make a widget per friend */
	        QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

			item -> setText(0, QString::fromStdString(detail.name) + " - " + QString::fromStdString(detail.location));
			if (detail.state & RS_PEER_STATE_CONNECTED) {
				item -> setTextColor(0,(Qt::darkBlue));
			}
            item -> setSizeHint(0,  QSize( 17,17 ) );

			item -> setText(1, QString::fromStdString(detail.id));

			item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
			item -> setCheckState(0, Qt::Unchecked);


			/* add to the list */
			items.append(item);
		}

	    /* remove old items */
		shareWidget->clear();
		shareWidget->setColumnCount(1);

		/* add the items in! */
		shareWidget->insertTopLevelItems(0, items);

		shareWidget->update(); /* update display */

	}else{
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(),
				this->size().height());
		mShareList.clear();
	}

}

void  CreateChannel::newChannel()
{
	/* enforce Private for the moment */
	ui.typePrivate->setChecked(true);

    ui.typeEncrypted->setEnabled(true);

	ui.msgAnon->setChecked(true);
	ui.msgAuth->setEnabled(false);
	ui.msgGroupBox->hide();
}

void  CreateChannel::createChannel()
{
	QString name = ui.channelName->text();
	QString desc = ui.channelDesc->toPlainText();
	uint32_t flags = 0;

	if(name.isEmpty())
	{	/* error message */
		QMessageBox::warning(this, tr("RetroShare"),tr("Please add a Name"),
		QMessageBox::Ok, QMessageBox::Ok);

		return; //Don't add  a empty name!!
	}
	else

	if (ui.typePrivate->isChecked())
	{
		flags |= RS_DISTRIB_PRIVATE;
	}
	else if (ui.typeEncrypted->isChecked())
	{
		flags |= RS_DISTRIB_ENCRYPTED;
	}

	if (ui.msgAuth->isChecked())
	{
		flags |= RS_DISTRIB_AUTHEN_REQ;
	}
	else if (ui.msgAnon->isChecked())
	{
		flags |= RS_DISTRIB_AUTHEN_ANON;
	}
	QByteArray ba;
	QBuffer buffer(&ba);

	if(!picture.isNull()){
		// send chan image

		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format
	}

	if (rsChannels)
	{
		std::string chId = rsChannels->createChannel(name.toStdWString(), desc.toStdWString(), flags, (unsigned char*)ba.data(), ba.size());

		if(ui.pubKeyShare_cb->isChecked())
			rsChannels->channelShareKeys(chId, mShareList);
	}


	close();
	return;
}


void CreateChannel::togglePersonItem( QTreeWidgetItem *item, int col )
{

        /* extract id */
        std::string id = (item -> text(1)).toStdString();

        /* get state */
        bool checked = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */
        std::list<std::string>::iterator lit = std::find(mShareList.begin(), mShareList.end(), id);

        if(checked && (lit == mShareList.end())){

        	// make sure ids not added already
        	mShareList.push_back(id);

        }else
			if(lit != mShareList.end()){

        	mShareList.erase(lit);

        }

        return;
}

void  CreateChannel::cancelChannel()
{
	close();
	return;
}

void CreateChannel::addChannelLogo()
{
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), QDir::homePath(), tr("Pictures (*.png *.xpm *.jpg)"));
	if(!fileName.isEmpty())
	{
            picture = QPixmap(fileName).scaled(64,64, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

            // to show the selected
            ui.ChannelLogoButton->setIcon(picture);
	}
}
