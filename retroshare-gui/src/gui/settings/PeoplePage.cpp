/*******************************************************************************
 * gui/settings/PeoplePage.cpp                                                 *
 *                                                                             *
 * Copyright 2006, Crypton         <retroshare.project@gmail.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "PeoplePage.h"
#include "util/misc.h"
#include "rsharesettings.h"
#include "retroshare/rsreputations.h"
#include "retroshare/rsidentity.h"

PeoplePage::PeoplePage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);

    connect(ui.autoPositiveOpinion_CB,SIGNAL(toggled(bool)),this,SLOT(updateAutoPositiveOpinion())) ;
    connect(ui.thresholdForPositive_SB,SIGNAL(valueChanged(int)),this,SLOT(updateThresholdForRemotelyPositiveReputation()));
    connect(ui.thresholdForNegative_SB,SIGNAL(valueChanged(int)),this,SLOT(updateThresholdForRemotelyNegativeReputation()));
    connect(ui.preventReloadingBannedIdentitiesFor_SB,SIGNAL(valueChanged(int)),this,SLOT(updateRememberDeletedNodes()));
    connect(ui.deleteBannedIdentitiesAfter_SB,SIGNAL(valueChanged(int)),this,SLOT(updateDeleteBannedNodesThreshold()));
    connect(ui.autoAddFriendIdsAsContact_CB,SIGNAL(toggled(bool)),this,SLOT(updateAutoAddFriendIdsAsContact()));
}

void PeoplePage::updateAutoPositiveOpinion() {  rsReputations->setAutoPositiveOpinionForContacts(ui.autoPositiveOpinion_CB->isChecked()) ; }

void PeoplePage::updateThresholdForRemotelyPositiveReputation() {  rsReputations->setThresholdForRemotelyPositiveReputation(ui.thresholdForPositive_SB->value()); }
void PeoplePage::updateThresholdForRemotelyNegativeReputation() {  rsReputations->setThresholdForRemotelyNegativeReputation(ui.thresholdForNegative_SB->value()); }

void PeoplePage::updateRememberDeletedNodes()       {    rsReputations->setRememberBannedIdThreshold(ui.preventReloadingBannedIdentitiesFor_SB->value()); }
void PeoplePage::updateDeleteBannedNodesThreshold() {    rsIdentity->setDeleteBannedNodesThreshold(ui.deleteBannedIdentitiesAfter_SB->value());}
void PeoplePage::updateAutoAddFriendIdsAsContact()  {    rsIdentity->setAutoAddFriendIdsAsContact(ui.autoAddFriendIdsAsContact_CB->isChecked()) ; }

PeoplePage::~PeoplePage()
{
}

/** Loads the settings for this page */
void PeoplePage::load()
{
    bool auto_positive_contacts = rsReputations->autoPositiveOpinionForContacts() ;
    uint32_t threshold_for_positive = rsReputations->thresholdForRemotelyPositiveReputation();
    uint32_t threshold_for_negative = rsReputations->thresholdForRemotelyNegativeReputation();
    bool auto_add_friend_ids_as_contact = rsIdentity->autoAddFriendIdsAsContact();

    whileBlocking(ui.autoAddFriendIdsAsContact_CB          )->setChecked(auto_add_friend_ids_as_contact);
    whileBlocking(ui.autoPositiveOpinion_CB                )->setChecked(auto_positive_contacts);
    whileBlocking(ui.thresholdForPositive_SB               )->setValue(threshold_for_positive);
    whileBlocking(ui.thresholdForNegative_SB               )->setValue(threshold_for_negative);
    whileBlocking(ui.deleteBannedIdentitiesAfter_SB        )->setValue(rsIdentity->deleteBannedNodesThreshold());
    whileBlocking(ui.preventReloadingBannedIdentitiesFor_SB)->setValue(rsReputations->rememberBannedIdThreshold());
}
