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
CreateForum::CreateForum(QWidget *parent)
: QDialog(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  // connect up the buttons.
  connect( ui.cancelButton, SIGNAL( clicked ( bool ) ), this, SLOT( cancelForum( ) ) );
  connect( ui.createButton, SIGNAL( clicked ( bool ) ), this, SLOT( createForum( ) ) );
  connect( ui.pubKeyShare_cb, SIGNAL( clicked() ), this, SLOT( setShareList( ) ));
  connect( ui.keyShareList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
  	        this, SLOT(togglePersonItem( QTreeWidgetItem *, int ) ));

	if(!ui.pubKeyShare_cb->isChecked()){

		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(),
				this->size().height());
	}

  newForum();
}

void  CreateForum::newForum()
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
}

void CreateForum::togglePersonItem( QTreeWidgetItem *item, int /*col*/ )
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

void  CreateForum::createForum()
{
	QString name = misc::removeNewLine(ui.forumName->text());
	QString desc = ui.forumDesc->toPlainText(); //toHtml();
	uint32_t flags = 0;

	if(name.isEmpty())
	{	/* error message */
		QMessageBox::warning(this, "RetroShare",
							 tr("Please add a Name"),
							 QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty name!!
	}
	else

	if (ui.typePublic->isChecked())
	{
		flags |= RS_DISTRIB_PUBLIC;
	}
	else if (ui.typePrivate->isChecked())
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

	if (rsForums)
	{
		std::string forumId = rsForums->createForum(name.toStdWString(),
				desc.toStdWString(), flags);

		if(ui.pubKeyShare_cb->isChecked())
			rsForums->forumShareKeys(forumId, mShareList);
	}

	close();
}

void CreateForum::setShareList(){

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

			item -> setText(0, PeerDefs::nameWithLocation(detail));
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

	}else{  // hide share widget
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(),
				this->size().height());
		mShareList.clear();
	}

}


void  CreateForum::cancelForum()
{
	close();
}
