/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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
#include <QtGui>

#include "BlogDialog.h"

#include "rsiface/rsQblog.h"
#include "rsiface/rspeers.h" //to retrieve peer/usrId info

#include "gui/feeds/BlogMsgItem.h"


/** Constructor */
BlogDialog::BlogDialog(QWidget *parent)
: MainPage (parent)
{
  /* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

  	connect(postButton, SIGNAL(clicked()), this, SLOT(postBlog()));
	
	/* mLayout -> to add widgets to */
	mLayout = new QVBoxLayout;
	
	QWidget *middleWidget = new QWidget();
	middleWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	middleWidget->setLayout(mLayout);
	
	QScrollArea *scrollArea = new QScrollArea;
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(middleWidget);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	
	QVBoxLayout *layout2 = new QVBoxLayout;
	layout2->addWidget(scrollArea);
	
	frame->setLayout(layout2);
	
	addDummyData();

	updateBlogsStatic();

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateBlogs()));
	timer->start(15631);

}


void BlogDialog::updateBlogs(void)
{

}


void BlogDialog::updateBlogsStatic(void)
{

#if 0
	rsQblog->getFilterSwitch();
	
	std::map<std::string, std::string> UsrStatus;
	
	if(!rsQblog->getStatus(UsrStatus))
		std::cerr << "failed to get usr status" << std::endl;
	
	clear(); //create a clear screen
	 
	/* retrieve usr names and populate usr list bar */
	
	std::list<std::string> usrList; 
	QString TempVar; // to convert numerics to string note: tbd find way to avoid temporary
	
	if (!rsPeers)
	{
		/* not ready yet! */
		return;
	}
	
	if(!rsPeers->getFriendList(usrList))
		std::cerr << "failed to get friend list";
	
	
	usrList.push_back(rsPeers->getOwnId()); // add your id
	
	/* populate the blog msgs screen */
	
	std::map< std::string, std::multimap<long int, std::string> > blogs; // to store blogs
	
	if(!rsQblog->getBlogs(blogs))
		std::cerr << "failed to get blogs" << std::endl;
	
	/* print usr name and their blogs to screen */
	for(std::list<std::string>::iterator it = usrList.begin(); it !=usrList.end(); it++)
	{	


		TempVar = rsPeers->getPeerName(*it).c_str(); // store usr name in temporary
		blogText->setTextColor(QColor(255, 0, 0, 255));
		blogText->setCurrentFont(mUsrFont); // make bold for username		
		blogText->append("\n" + TempVar); // write usr name to screen
		std::cerr << "creating usr tree" << std::endl;
		
		/*print blog time-posted/msgs to screen*/
		
		std::multimap<long int, std::string>::reverse_iterator blogIt =  blogs[*it].rbegin(); 
		
		if(blogs[*it].empty())
		{
			std::cerr << "usr blog empty!"  << std::endl;
			continue; 	
		}
	
		for( ; blogIt != blogs[*it].rend(); blogIt++)
		{	
			std::cerr << "now printing blogs" << std::endl;
			time_t postedTime = blogIt->first;
			time(&postedTime); //convert to human readable time
			blogText->setTextColor(QColor(255, 0, 0, 255)); // 
			blogText->setCurrentFont(mUsrFont); // make bold for posted date
			blogText->append("\nPosted: " + QString (ctime(&postedTime))); // print time of blog to screen 
			blogText->setCurrentFont(mCurrentFont); // reset the font for blog messages
			blogText->setTextColor(QColor(0, 0, 0, 255)); // set back color to black
			blogText->append(blogIt->second.c_str()); // print blog msg to screen 
		}	
			
	}	

#endif

}
	
	
void BlogDialog::addDummyData()
{
	BlogMsgItem *bm1 = new BlogMsgItem(this, 0, "peerId", "msgId", true);
	BlogMsgItem *bm2 = new BlogMsgItem(this, 0, "peerId", "msgId", true);
	BlogMsgItem *bm3 = new BlogMsgItem(this, 0, "peerId", "msgId", true);
	BlogMsgItem *bm4 = new BlogMsgItem(this, 0, "peerId", "msgId", true);
	BlogMsgItem *bm5 = new BlogMsgItem(this, 0, "peerId", "msgId", true);

	mLayout->addWidget(bm1);
	mLayout->addWidget(bm2);
	mLayout->addWidget(bm3);
	mLayout->addWidget(bm4);
	mLayout->addWidget(bm5);
}



/* FeedHolder Functions (for FeedItem functionality) */
void BlogDialog::deleteFeedItem(QWidget *item, uint32_t type)
{
        std::cerr << "BlogDialog::deleteFeedItem()";
        std::cerr << std::endl;
}


void BlogDialog::openChat(std::string peerId)
{
        std::cerr << "BlogDialog::openChat()";
        std::cerr << std::endl;
}

void BlogDialog::postBlog()
{
	openMsg(FEEDHOLDER_MSG_BLOG, "", "");
}

void BlogDialog::openMsg(uint32_t type, std::string grpId, std::string inReplyTo)
{
        std::cerr << "BlogDialog::openMsg()";
        std::cerr << std::endl;
}



