/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2010 Christopher Evi-Parker
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

#include "ShareKey.h"

#include <QMessageBox>
#include <algorithm>

#include <retroshare/rschannels.h>
#include <retroshare/rsforums.h>
#include <retroshare/rspeers.h>

#include "gui/common/PeerDefs.h"

ShareKey::ShareKey(QWidget *parent, Qt::WFlags flags, std::string grpId,
		int grpType) :
        QDialog(parent, flags), mGrpId(grpId), mGrpType(grpType)
{
    ui = new Ui::ShareKey();
    ui->setupUi(this);


    connect( ui->shareButton, SIGNAL( clicked ( bool ) ), this, SLOT( shareKey( ) ) );
    connect( ui->cancelButton, SIGNAL( clicked ( bool ) ), this, SLOT( cancel( ) ) );

    connect(ui->keyShareList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
            this, SLOT(togglePersonItem( QTreeWidgetItem *, int ) ));

    setShareList();

}


ShareKey::~ShareKey()
{
    delete ui;
}

void ShareKey::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void ShareKey::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ShareKey::shareKey()
{



    if(mShareList.empty())
    {
        QMessageBox::warning(this, tr("RetroShare"),tr("Please select at least one peer"),
        QMessageBox::Ok, QMessageBox::Ok);

        return;
    }

	if(mGrpType & CHANNEL_KEY_SHARE)
	{
		if(!rsChannels)
			return;

	    if(!rsChannels->channelShareKeys(mGrpId, mShareList)){

	        std::cerr << "Failed to share keys!" << std::endl;

	        return;
	    }

	}else if(mGrpType & FORUM_KEY_SHARE)
	{
		if(!rsForums)
			return;

	    if(!rsForums->forumShareKeys(mGrpId, mShareList)){

	        std::cerr << "Failed to share keys!" << std::endl;

	        return;
	    }

	}else{

		// incorrect type
		return;
	}



    close();
    return;
}

void ShareKey::cancel()
{
      close();
      return;
}

void ShareKey::setShareList(){

    if (!rsPeers)
    {
            /* not ready yet! */
            return;
    }

    std::list<std::string> peers;
    std::list<std::string>::iterator it;

    rsPeers->getFriendList(peers);

    /* get a link to the table */
    QTreeWidget *shareWidget = ui->keyShareList;

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

}

void ShareKey::togglePersonItem( QTreeWidgetItem *item, int /*col*/ )
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

