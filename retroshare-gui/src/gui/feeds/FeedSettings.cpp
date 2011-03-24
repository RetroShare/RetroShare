/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 RetroShare Team
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
#include "FeedSettings.h"

#include <rshare.h>

#include <retroshare/rsnotify.h>
#include "gui/settings/rsharesettings.h"

/** Default constructor */
FeedSettings::FeedSettings(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeinfodlg()));

  ui.applyButton->setToolTip(tr("Apply and Close"));
  
  load();
  
  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif

}

void FeedSettings::show()
{
  if(!this->isVisible()) {
    QDialog::show();

  }
}

void FeedSettings::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

/** Saves the changes on this page */
bool FeedSettings::save()
{
    /* extract from rsNotify the flags */

    uint newsflags   = 0;

    if (ui.notify_Peers->isChecked())
        newsflags |= RS_FEED_TYPE_PEER;
    if (ui.notify_Channels->isChecked())
        newsflags |= RS_FEED_TYPE_CHAN;
    if (ui.notify_Forums->isChecked())
        newsflags |= RS_FEED_TYPE_FORUM;
    if (ui.notify_Blogs->isChecked())
        newsflags |= RS_FEED_TYPE_BLOG;
    if (ui.notify_Chat->isChecked())
        newsflags |= RS_FEED_TYPE_CHAT;
    if (ui.notify_Messages->isChecked())
        newsflags |= RS_FEED_TYPE_MSG;
    if (ui.notify_Chat->isChecked())
        newsflags |= RS_FEED_TYPE_CHAT;

    Settings->setNewsFeedFlags(newsflags);
    
    Settings->setAddFeedsAtEnd(ui.addFeedsAtEnd->isChecked());

    load();
    return true;
}

/** Loads the news feed settings */
void FeedSettings::load()
{
    /* extract from rsNotify the flags */
    uint newsflags = Settings->getNewsFeedFlags();

    ui.notify_Peers->setChecked(newsflags & RS_FEED_TYPE_PEER);
    ui.notify_Channels->setChecked(newsflags & RS_FEED_TYPE_CHAN);
    ui.notify_Forums->setChecked(newsflags & RS_FEED_TYPE_FORUM);
    ui.notify_Blogs->setChecked(newsflags & RS_FEED_TYPE_BLOG);
    ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);
    ui.notify_Messages->setChecked(newsflags & RS_FEED_TYPE_MSG);
    ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);

    ui.addFeedsAtEnd->setChecked(Settings->getAddFeedsAtEnd());

}

void FeedSettings::closeinfodlg()
{
	close();
}

void FeedSettings::applyDialog()
{

	/* reload now */
	save();

	/* close the Dialog after the Changes applied */
	closeinfodlg();

}
