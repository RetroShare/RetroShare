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
}

void PeoplePage::updateAutoPositiveOpinion() {  rsReputations->setNodeAutoPositiveOpinionForContacts(ui.autoPositiveOpinion_CB->isChecked()) ; }

void PeoplePage::updateThresholdForRemotelyPositiveReputation() {  rsReputations->setThresholdForRemotelyPositiveReputation(ui.thresholdForPositive_SB->value()); }
void PeoplePage::updateThresholdForRemotelyNegativeReputation() {  rsReputations->setThresholdForRemotelyNegativeReputation(ui.thresholdForNegative_SB->value()); }

void PeoplePage::updateRememberDeletedNodes()       {    rsReputations->setRememberDeletedNodesThreshold(ui.preventReloadingBannedIdentitiesFor_SB->value()); }
void PeoplePage::updateDeleteBannedNodesThreshold() {    rsIdentity->setDeleteBannedNodesThreshold(ui.deleteBannedIdentitiesAfter_SB->value());}

PeoplePage::~PeoplePage()
{
}

/** Loads the settings for this page */
void PeoplePage::load()
{
    bool auto_positive_contacts = rsReputations->nodeAutoPositiveOpinionForContacts() ;
    uint32_t threshold_for_positive = rsReputations->thresholdForRemotelyPositiveReputation();
    uint32_t threshold_for_negative = rsReputations->thresholdForRemotelyNegativeReputation();

    ui.autoPositiveOpinion_CB->setChecked(auto_positive_contacts);
    ui.thresholdForPositive_SB->setValue(threshold_for_positive);
    ui.thresholdForNegative_SB->setValue(threshold_for_negative);
    ui.deleteBannedIdentitiesAfter_SB->setValue(rsIdentity->deleteBannedNodesThreshold());
    ui.preventReloadingBannedIdentitiesFor_SB->setValue(rsReputations->rememberDeletedNodesThreshold());
}
