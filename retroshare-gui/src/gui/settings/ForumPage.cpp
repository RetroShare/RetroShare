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

#include "ForumPage.h"
#include "rshare.h"
#include "rsharesettings.h"

#include "../MainWindow.h"
#include "../ForumsDialog.h"

ForumPage::ForumPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);
}

ForumPage::~ForumPage()
{
}

/** Saves the changes on this page */
bool
ForumPage::save(QString &/*errmsg*/)
{
    Settings->setForumMsgSetToReadOnActivate(ui.setMsgToReadOnActivate->isChecked());
    Settings->setForumExpandNewMessages(ui.expandNewMessages->isChecked());
    Settings->setForumOpenAllInNewTab(ui.openAllInNewTabCheckBox->isChecked());
    Settings->setForumLoadEmbeddedImages(ui.loadEmbeddedImages->isChecked());

    return true;
}

/** Loads the settings for this page */
void
ForumPage::load()
{
    ui.setMsgToReadOnActivate->setChecked(Settings->getForumMsgSetToReadOnActivate());
    ui.expandNewMessages->setChecked(Settings->getForumExpandNewMessages());
    ui.openAllInNewTabCheckBox->setChecked(Settings->getForumOpenAllInNewTab());
    ui.loadEmbeddedImages->setChecked(Settings->getForumLoadEmbeddedImages());
}
