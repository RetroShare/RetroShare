/*******************************************************************************
 * gui/settings/ForumPage.cpp                                                  *
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

#include "ForumPage.h"
#include "util/misc.h"
#include "rsharesettings.h"

ForumPage::ForumPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);

	/* Initialize GroupFrameSettingsWidget */
	ui.groupFrameSettingsWidget->setOpenAllInNewTabText(tr("Open each forum in a new tab"));

    connect(ui.setMsgToReadOnActivate,SIGNAL(toggled(bool)),this,SLOT(updateMsgReadOnActivate())) ;
	connect(ui.expandNewMessages  , SIGNAL(toggled(bool)), this, SLOT( updateExpandNewMessages()));
	connect(ui.loadEmbeddedImages , SIGNAL(toggled(bool)), this, SLOT(updateLoadEmbeddedImages() ));
	connect(ui.loadEmoticons      , SIGNAL(toggled(bool)), this, SLOT(   updateLoadEmoticons()	 ));

    ui.groupFrameSettingsWidget->setType(GroupFrameSettings::Forum) ;
}

ForumPage::~ForumPage()
{
}

void ForumPage::updateMsgReadOnActivate()   { Settings->setForumMsgSetToReadOnActivate(ui.setMsgToReadOnActivate->isChecked()); }
void ForumPage::updateExpandNewMessages()	{ Settings->setForumExpandNewMessages(     ui.expandNewMessages     ->isChecked());}
void ForumPage::updateLoadEmbeddedImages()	{ Settings->setForumLoadEmbeddedImages(    ui.loadEmbeddedImages    ->isChecked());}
void ForumPage::updateLoadEmoticons()		{ Settings->setForumLoadEmoticons(         ui.loadEmoticons         ->isChecked()); }

/** Loads the settings for this page */
void ForumPage::load()
{
	whileBlocking(ui.setMsgToReadOnActivate)->setChecked(Settings->getForumMsgSetToReadOnActivate());
	whileBlocking(ui.expandNewMessages)->setChecked(Settings->getForumExpandNewMessages());
	whileBlocking(ui.loadEmbeddedImages)->setChecked(Settings->getForumLoadEmbeddedImages());
	whileBlocking(ui.loadEmoticons)->setChecked(Settings->getForumLoadEmoticons());

	ui.groupFrameSettingsWidget->loadSettings(GroupFrameSettings::Forum);
}
