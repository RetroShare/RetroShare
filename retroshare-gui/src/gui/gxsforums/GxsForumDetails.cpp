/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 RetroShare Team
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
#include "GxsForumDetails.h"

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsgxsforums.h>

#include <QTime>
#include <QDateTime>

#include <list>
#include <iostream>
#include <string>


/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

/** Default constructor */
GxsForumDetails::GxsForumDetails(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeinfodlg()));

  ui.applyButton->setToolTip(tr("Apply and Close"));
  
  ui.nameline ->setReadOnly(true);
  ui.popline ->setReadOnly(true);
  ui.postline ->setReadOnly(true);
  ui.IDline ->setReadOnly(true);
  ui.DescriptiontextEdit ->setReadOnly(true);
  
  ui.radioButton_authd->setEnabled(false);
  ui.radioButton_anonymous->setEnabled(false);
}


/**
 Overloads the default show() slot so we can set opacity*/

void
GxsForumDetails::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QDialog::show();

  }
}

void GxsForumDetails::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void GxsForumDetails::closeinfodlg()
{
	close();
}

void GxsForumDetails::showDetails(std::string mCurrForumId)
{
	fId = mCurrForumId;
	loadDialog();
}

void GxsForumDetails::loadDialog()
{
	if (!rsGxsForums)
	{
		return;
	}

#warning "GxsForumDetails Incomplete"
#if 0
	ForumInfo fi;
	rsGxsForums->getForumInfo(fId, fi);

	// Set Forum Name
	ui.nameline->setText(QString::fromStdWString(fi.forumName));

	// Set Popularity
	ui.popline->setText(QString::number(fi.pop));

	// Set Last Post Date
	if (fi.lastPost) {
		QDateTime qtime;
		qtime.setTime_t(fi.lastPost);
		QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
		ui.postline->setText(timestamp);
	}

	// Set Forum ID
	ui.IDline->setText(QString::fromStdString(fi.forumId));

	// Set Forum Description
	ui.DescriptiontextEdit->setText(QString::fromStdWString(fi.forumDesc));

	if (fi.forumFlags & RS_DISTRIB_AUTHEN_REQ)
	{
		ui.radioButton_authd->setChecked(true);
		ui.radioButton_anonymous->setChecked(false);
	}
	if (fi.forumFlags & RS_DISTRIB_AUTHEN_ANON)
	{
		ui.radioButton_authd->setChecked(false);
		ui.radioButton_anonymous->setChecked(true);
	}
#endif
	
}

void GxsForumDetails::applyDialog()
{

	/* reload now */
	loadDialog();

	/* close the Dialog after the Changes applied */
	closeinfodlg();

}
