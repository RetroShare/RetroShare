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

#include "GxsGroupShareKey.h"

#include <QMessageBox>
#include <algorithm>

#include <retroshare/rspeers.h>
#include <retroshare/rsgxschannels.h>
#include <retroshare/rsgxsforums.h>
#include <retroshare/rsposted.h>

#include "gui/common/PeerDefs.h"

GroupShareKey::GroupShareKey(QWidget *parent, const RsGxsGroupId &grpId, int grpType) :
	QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mGrpId(grpId), mGrpType(grpType)
{
	ui = new Ui::ShareKey();
	ui->setupUi(this);

	ui->headerFrame->setHeaderImage(QPixmap(":/images/user/agt_forum64.png"));
	ui->headerFrame->setHeaderText(tr("Share Channel"));

	connect( ui->buttonBox, SIGNAL(accepted()), this, SLOT(shareKey()));
	connect( ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));

	/* initialize key share list */
	ui->keyShareList->setHeaderText(tr("Contacts:"));
	ui->keyShareList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui->keyShareList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL);
	ui->keyShareList->start();
}

GroupShareKey::~GroupShareKey()
{
	delete ui;
}

void GroupShareKey::changeEvent(QEvent *e)
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

void GroupShareKey::shareKey()
{
	std::list<RsPeerId> shareList;
	ui->keyShareList->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(shareList, false);

	if (shareList.empty()) {
		QMessageBox::warning(this, "RetroShare", tr("Please select at least one peer"), QMessageBox::Ok, QMessageBox::Ok);

		return;
	}

    if (mGrpType == CHANNEL_KEY_SHARE)
    {
        if (!rsGxsChannels)
            return;

        if (!rsGxsChannels->groupShareKeys(mGrpId, shareList)) {
            std::cerr << "Failed to share keys!" << std::endl;
            return;
        }
    }
    else if(mGrpType == FORUM_KEY_SHARE)
    {
        QMessageBox::warning(NULL,"Not implemented.","Not implemented") ;

        //	if (!rsForums->forumShareKeys(mGrpId, shareList)) {
        //	std::cerr << "Failed to share keys!" << std::endl;
        //return;
        //}
    }
    else if (mGrpType == POSTED_KEY_SHARE)
    {
        if (!rsPosted)
            return;

        if (!rsPosted->groupShareKeys(mGrpId, shareList)) {
            std::cerr << "Failed to share keys!" << std::endl;
            return;
        }
    }
    else
    {
		// incorrect type
		return;
	}

	close();
}
