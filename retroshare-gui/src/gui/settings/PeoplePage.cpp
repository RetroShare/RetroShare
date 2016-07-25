/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#include "PeoplePage.h"
#include "rsharesettings.h"
#include "retroshare/rsreputations.h"

PeoplePage::PeoplePage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);
}

PeoplePage::~PeoplePage()
{
}

/** Saves the changes on this page */
bool PeoplePage::save(QString &/*errmsg*/)
{
    if(!ui.identityBan_CB->isChecked())
        rsReputations->setNodeAutoBanThreshold(0) ;
    else
        rsReputations->setNodeAutoBanThreshold(ui.identityBanThreshold_SB->value()) ;

    if(ui.autoPositiveOpinion_CB->isChecked())
        rsReputations->setNodeAutoPositiveOpinionForContacts(true) ;
    else
        rsReputations->setNodeAutoPositiveOpinionForContacts(false) ;

    rsReputations->setNodeAutoBanIdentitiesLimit(ui.autoBanIdentitiesLimit_SB->value());

    return true;
}

/** Loads the settings for this page */
void PeoplePage::load()
{
    uint32_t ban_limit = rsReputations->nodeAutoBanThreshold() ;
    bool auto_positive_contacts = rsReputations->nodeAutoPositiveOpinionForContacts() ;
    float node_auto_ban_identities_limit = rsReputations->nodeAutoBanIdentitiesLimit();

    ui.identityBan_CB->setChecked(ban_limit > 0) ;
    ui.identityBanThreshold_SB->setValue(ban_limit) ;
    ui.autoPositiveOpinion_CB->setChecked(auto_positive_contacts);
    ui.autoBanIdentitiesLimit_SB->setValue(node_auto_ban_identities_limit);
}
