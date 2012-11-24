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

#include "util/misc.h"
#include "CreateForum.h"
#include "gui/common/PeerDefs.h"

#include <algorithm>

#include <retroshare/rsforums.h>
#include <retroshare/rspeers.h>

/** Constructor */
CreateForum::CreateForum()
: QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	ui.headerFrame->setHeaderImage(QPixmap(":/images/konversation64.png"));
	ui.headerFrame->setHeaderText(tr("New Forum"));

	// connect up the buttons.
	connect( ui.buttonBox, SIGNAL(accepted()), this, SLOT(createForum()));
	connect( ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
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

	newForum();
}

void CreateForum::newForum()
{
	/* enforce Public for the moment */
	ui.typePublic->setChecked(true);

	ui.typePrivate->setEnabled(false);
	ui.typeEncrypted->setEnabled(true);

#ifdef RS_RELEASE_VERSION
	ui.typePrivate->setVisible(false);
	ui.typeEncrypted->setVisible(true);
#endif

	ui.msgAnon->setChecked(true);
	//ui.msgAuth->setEnabled(false);

	ui.forumName->clear();
	ui.forumDesc->clear();

	ui.forumName->setFocus();
}

void CreateForum::createForum()
{
	QString name = misc::removeNewLine(ui.forumName->text());
	QString desc = ui.forumDesc->toPlainText(); //toHtml();
	uint32_t flags = 0;

	if(name.isEmpty()) {
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Please add a Name"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty name!!
	}

	if (ui.typePublic->isChecked()) {
		flags |= RS_DISTRIB_PUBLIC;
	} else if (ui.typePrivate->isChecked()) {
		flags |= RS_DISTRIB_PRIVATE;
	} else if (ui.typeEncrypted->isChecked()) {
		flags |= RS_DISTRIB_ENCRYPTED;
	}

	if (ui.msgAuth->isChecked()) {
		flags |= RS_DISTRIB_AUTHEN_REQ;
	} else if (ui.msgAnon->isChecked()) {
		flags |= RS_DISTRIB_AUTHEN_ANON;
	}

	if (rsForums) {
		std::string forumId = rsForums->createForum(name.toStdWString(), desc.toStdWString(), flags);

		if (ui.pubKeyShare_cb->isChecked()) {
			std::list<std::string> shareList;
			ui.keyShareList->selectedSslIds(shareList, false);
			rsForums->forumShareKeys(forumId, shareList);
		}
	}

	close();
}

void CreateForum::setShareList()
{
	if (ui.pubKeyShare_cb->isChecked()){
		this->resize(this->size().width() + ui.contactsdockWidget->size().width(), this->size().height());
		ui.contactsdockWidget->show();
	} else {  // hide share widget
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}
}
