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
#include "BlogDetails.h"

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsblogs.h>

#include <QTime>
#include <QDateTime>

#include <list>
#include <iostream>
#include <string>


/** Default constructor */
BlogDetails::BlogDetails(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  setAttribute ( Qt::WA_DeleteOnClose, true );

  connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeinfodlg()));

  ui.applyButton->setToolTip(tr("Close"));
  
  ui.nameline ->setReadOnly(true);
  ui.popline ->setReadOnly(true);
  ui.postline ->setReadOnly(true);
  ui.IDline ->setReadOnly(true);
  ui.DescriptiontextEdit ->setReadOnly(true);
  

}

/** Overloads the default show() */
void
BlogDetails::show()
{
  if(!this->isVisible()) {
    QDialog::show();

  }
}

void BlogDetails::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void BlogDetails::closeinfodlg()
{
	close();
}

void BlogDetails::showDetails(std::string mBlogId)
{
	bId = mBlogId;
	loadBlog();
}

void BlogDetails::loadBlog()
{

	if (!rsBlogs)
	{
		return;
	}	

	std::list<BlogInfo> channelList;
	std::list<BlogInfo>::iterator it;
	
	BlogInfo bi;
	rsBlogs->getBlogInfo(bId, bi);

	rsBlogs->getBlogList(channelList);
	
	
	for(it = channelList.begin(); it != channelList.end(); it++)
	{
	
    // Set Blog Name
    ui.nameline->setText(QString::fromStdWString(bi.blogName));

    // Set Blog Popularity
    {
      std::ostringstream out;
      out << it->pop;
      ui.popline -> setText(QString::fromStdString(out.str()));
    }
	
    // Set Last Blog Post Date 
    {
      QDateTime qtime;
      qtime.setTime_t(it->lastPost);
      QString timestamp = qtime.toString(Qt::DefaultLocaleShortDate);
      ui.postline -> setText(timestamp);
    }

    // Set Blog ID
    ui.IDline->setText(QString::fromStdString(bi.blogId));
	
    // Set Blog Description
    ui.DescriptiontextEdit->setText(QString::fromStdWString(bi.blogDesc));
        

	
	}

}

void BlogDetails::applyDialog()
{

	/* reload now */
	loadBlog();

	/* close the Dialog after the Changes applied */
	closeinfodlg();

}



