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

#include <algorithm>

#include "CreateChannel.h"
#include "gui/common/PeerDefs.h"
#include "util/misc.h"

#include <retroshare/rschannels.h>
#include <retroshare/rspeers.h>

/** Constructor */
CreateChannel::CreateChannel()
: QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	ui.headerFrame->setHeaderImage(QPixmap(":/images/add_channel64.png"));
	ui.headerFrame->setHeaderText(tr("New Channel"));

	picture = NULL;

	// connect up the buttons.
	connect( ui.buttonBox, SIGNAL(accepted()), this, SLOT(createChannel()));
	connect( ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

	connect( ui.logoButton, SIGNAL(clicked() ), this , SLOT(addChannelLogo()));
	connect( ui.pubKeyShare_cb, SIGNAL( clicked() ), this, SLOT( setShareList( ) ));

	if (!ui.pubKeyShare_cb->isChecked()) {
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}

	/* initialize key share list */
	ui.keyShareList->setHeaderText(tr("Contacts:"));
	ui.keyShareList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui.keyShareList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL);
	ui.keyShareList->start();

	newChannel();
}

void CreateChannel::setShareList(){

	if (ui.pubKeyShare_cb->isChecked()){
		this->resize(this->size().width() + ui.contactsdockWidget->size().width(), this->size().height());
		ui.contactsdockWidget->show();
	} else {
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}
}

void CreateChannel::newChannel()
{
	/* enforce Private for the moment */
	ui.typePrivate->setChecked(true);

	ui.typeEncrypted->setEnabled(true);

	ui.msgAnon->setChecked(true);
	ui.msgAuth->setEnabled(false);
	ui.msgGroupBox->hide();

	ui.channelName->setFocus();
}

void CreateChannel::createChannel()
{
	QString name = misc::removeNewLine(ui.channelName->text());
	QString desc = ui.channelDesc->toPlainText();
	uint32_t flags = 0;

	if (name.isEmpty()) {
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Please add a Name"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty name!!
	}

	if (ui.typePrivate->isChecked()) {
		flags |= RS_DISTRIB_PRIVATE;
	} else if (ui.typeEncrypted->isChecked()) {
		flags |= RS_DISTRIB_ENCRYPTED;
	}

	if (ui.msgAuth->isChecked()) {
		flags |= RS_DISTRIB_AUTHEN_REQ;
	} else if (ui.msgAnon->isChecked()) {
		flags |= RS_DISTRIB_AUTHEN_ANON;
	}
	QByteArray ba;
	QBuffer buffer(&ba);

	if (!picture.isNull()) {
		// send chan image

		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format
	}

	if (rsChannels) {
		std::string chId = rsChannels->createChannel(name.toStdWString(), desc.toStdWString(), flags, (unsigned char*)ba.data(), ba.size());

		if (ui.pubKeyShare_cb->isChecked()) {
			std::list<std::string> shareList;
			ui.keyShareList->selectedSslIds(shareList, false);
			rsChannels->channelShareKeys(chId, shareList);
		}
	}

	close();
}

void CreateChannel::addChannelLogo() // the same function as in EditChanDetails
{
	QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load channel logo"), 64, 64);

	if (img.isNull())
		return;

	picture = img;

	// to show the selected
	ui.logoLabel->setPixmap(picture);
}
