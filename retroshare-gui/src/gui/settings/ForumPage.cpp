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

#include <QPushButton>
#include <QMessageBox>
#include "retroshare/rsgxsforums.h"
#include "ForumPage.h"
#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "util/rsthreads.h"
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
	connect(ui.minimumFontSize    , SIGNAL(valueChanged(int)), this, SLOT(updateFonts()));
	connect(ui.minimumContrast    , SIGNAL(valueChanged(int)), this, SLOT(updateFonts()));

    ui.groupFrameSettingsWidget->setType(GroupFrameSettings::Forum) ;

#ifdef RS_DEEP_FORUMS_INDEX
    mReindexButton = new QPushButton(tr("Rebuild the search index"), this);
    mReindexButton->setToolTip(
                tr("Rebuild the index used to search the content of the "
                   "forum posts. This may take a few minutes.") );
    connect(mReindexButton, SIGNAL(clicked()), this, SLOT(reindexAll()));
    layout()->addWidget(mReindexButton);
#endif
}

void ForumPage::reindexAll()
{
#ifdef RS_DEEP_FORUMS_INDEX
    if( QMessageBox::question(
                this, tr("Rebuild the search index"),
                tr("Rebuild the index used to search the content of the forum "
                   "posts?\nIt may take a few minutes on a big profile, the "
                   "search keeps working meanwhile."),
                QMessageBox::Yes | QMessageBox::No ) != QMessageBox::Yes )
        return;

    mReindexButton->setEnabled(false);
    mReindexButton->setText(tr("Rebuilding the search index..."));

    // Blocking and long: off the GUI thread
    RsThread::async([this]()
    {
        const bool done = rsGxsForums->reindexAll();

        RsQThreadUtils::postToObject([this, done]()
        {
            mReindexButton->setEnabled(true);
            mReindexButton->setText(tr("Rebuild the search index"));
            if(done)
                QMessageBox::information(
                            this, tr("Rebuild the search index"),
                            tr("The search index has been rebuilt.") );
            else
                QMessageBox::warning(
                            this, tr("Rebuild the search index"),
                            tr("The search index could not be fully rebuilt, "
                               "or a rebuild was already running. See the "
                               "log for details.") );
        }, this);
    });
#endif
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
	Settings->beginGroup(QString("Forum"));
	whileBlocking(ui.setMsgToReadOnActivate)->setChecked(Settings->getForumMsgSetToReadOnActivate());
	whileBlocking(ui.expandNewMessages)->setChecked(Settings->getForumExpandNewMessages());
	whileBlocking(ui.loadEmbeddedImages)->setChecked(Settings->getForumLoadEmbeddedImages());
	whileBlocking(ui.loadEmoticons)->setChecked(Settings->getForumLoadEmoticons());
	whileBlocking(ui.minimumFontSize)->setValue(Settings->value("MinimumFontSize", 10).toInt());
	whileBlocking(ui.minimumContrast)->setValue(Settings->value("MinimumContrast", 4.5).toDouble());
	Settings->endGroup();

	ui.groupFrameSettingsWidget->loadSettings(GroupFrameSettings::Forum);
}

void ForumPage::updateFonts()
{
	Settings->beginGroup(QString("Forum"));
	Settings->setValue("MinimumFontSize", ui.minimumFontSize->value());
	Settings->setValue("MinimumContrast", ui.minimumContrast->value());
	Settings->endGroup();
}
