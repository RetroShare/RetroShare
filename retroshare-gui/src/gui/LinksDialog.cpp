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

#include <QFile>
#include <QFileInfo>
#include <QDateTime>

#include <QDesktopServices>
#include <QUrl>

#include "common/vmessagebox.h"

//#include "rshare.h"
#include "LinksDialog.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsrank.h"

#include <iostream>
#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QMessageBox>
#include <QHeaderView>


/* Images for context menu icons */
#define IMAGE_REMOVEFRIEND       ":/images/removefriend16.png"
#define IMAGE_EXPIORTFRIEND      ":/images/exportpeers_16x16.png"
#define IMAGE_CHAT               ":/images/chat.png"
/* Images for Status icons */
#define IMAGE_ONLINE             ":/images/donline.png"
#define IMAGE_OFFLINE            ":/images/dhidden.png"

/** Constructor */
LinksDialog::LinksDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.linkTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( linkTreeWidgetCostumPopupMenu( QPoint ) ) );


  /* link combos */
  connect( ui.rankComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changedSortRank( int ) ) );
  connect( ui.periodComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changedSortPeriod( int ) ) );
  connect( ui.fromComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changedSortFrom( int ) ) );
  connect( ui.topComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changedSortTop( int ) ) );

  /* add button */
  connect( ui.addButton, SIGNAL( clicked( void ) ), this, SLOT( addLinkComment( void ) ) );
  connect( ui.expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggleWindows( void ) ) );

  connect( ui.linkTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem * ) ), 
  		this, SLOT( changedItem ( QTreeWidgetItem *, QTreeWidgetItem * ) ) );
 
  connect( ui.linkTreeWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int ) ), 
  		this, SLOT( openLink ( QTreeWidgetItem *, int ) ) );




  mStart = 0;


  /* hide the Tree +/- */
//  ui.linkTreeWidget -> setRootIsDecorated( false );
  
    /* Set header resize modes and initial section sizes */
	QHeaderView * _header = ui.linkTreeWidget->header () ;
//   	_header->setResizeMode (0, QHeaderView::Custom);
   	_header->setResizeMode (0, QHeaderView::Interactive);
	_header->setResizeMode (1, QHeaderView::Interactive);
	_header->setResizeMode (2, QHeaderView::Interactive);

	_header->resizeSection ( 0, 400 );
	_header->resizeSection ( 1, 50 );
	_header->resizeSection ( 2, 150 );

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void LinksDialog::linkTreeWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      voteupAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Vote Link Up" ), this );
      connect( voteupAct , SIGNAL( triggered() ), this, SLOT( voteup() ) );

      //votedownAct = new QAction(QIcon(IMAGE_REMOVEFRIEND), tr( "Vote Down" ), this );
      //connect( votedownAct , SIGNAL( triggered() ), this, SLOT( votedown() ) );
      
      contextMnu.clear();
      contextMnu.addAction(voteupAct);
      //contextMnu.addSeparator(); 
      //contextMnu.addAction(votedownAct);
      contextMnu.exec( mevent->globalPos() );
}

void LinksDialog::changedSortRank( int index )
{
	/* update */
	if (!rsRanks)
		return;

	/* translate */
	uint32_t type = 0;
	switch (index)
	{
		case 1:
			type = RS_RANK_TIME;
			break;
		case 2:
			type = RS_RANK_SCORE;
			break;
		default:
		case 0:
			type = RS_RANK_ALG;
			break;
	}

	if (type)
	{
		rsRanks->setSortMethod(type);
	}
	updateLinks();
}

void LinksDialog::changedSortPeriod( int index )
{
	/* update */
	if (!rsRanks)
		return;

	/* translate */
	uint32_t period = 0;
	switch (index)
	{
		case 1:
			period = 60 * 60 * 24 * 7; /* WEEK */
			break;
		case 2:
			period = 60 * 60 * 24; /* DAY */
			break;
		default:
		case 0:
			period = 60 * 60 * 24 * 30; /* MONTH */
			break;
	}

	if (period)
	{
		rsRanks->setSortPeriod(period);
	}
	updateLinks();
}

void LinksDialog::changedSortFrom( int index )
{
	/* update */
	if (!rsRanks)
		return;

	std::list<std::string> peers;

	/* translate */
	switch (index)
	{
		default:
		case 0:
			break;
		case 1:
			peers.push_back(rsPeers->getOwnId());
			break;
	}

	if (peers.size() < 1)
	{
		rsRanks->clearPeerFilter();
	}
	else
	{
		rsRanks->setPeerFilter(peers);
	}
	updateLinks();
}


void LinksDialog::changedSortTop( int index )
{
	/* update */
	if (!rsRanks)
		return;

	std::list<std::string> peers;

	/* translate */
	switch (index)
	{
		default:
		case 0:
			mStart = 0;
			break;
		case 2:
			mStart = 100;
			break;
		case 3:
			mStart = 200;
			break;
		case 4:
			mStart = 300;
			break;
		case 5:
			mStart = 400;
			break;
		case 6:
			mStart = -1;
			break;
	}
	updateLinks();
}


/* get the list of Links from the RsRanks.  */
void  LinksDialog::updateLinks()
{

	std::list<std::string> rids;
	std::list<std::string>::iterator rit;
	std::list<RsRankComment>::iterator cit;

	/* Work out the number/entries to show */
	uint32_t count = rsRanks->getRankingsCount();
	uint32_t start;

	uint32_t entries = 100;
	if (count < entries)
	{
		entries = count;
	}

	if (mStart == -1)
	{
		/* backwards */
		start = count-entries;
	}
	else
	{
		start = mStart;
		if (start + entries > count)
		{
			start = count - entries;
		}
	}

        /* get a link to the table */
        QTreeWidget *linkWidget = ui.linkTreeWidget;
        QList<QTreeWidgetItem *> items;

	rsRanks->getRankings(start, entries, rids);
	float maxRank = rsRanks->getMaxRank();

	for(rit = rids.begin(); rit != rids.end(); rit++)
	{
		RsRankDetails detail;
		if (!rsRanks->getRankDetails(*rit, detail))
		{
			continue;
		}

		/* create items */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* (0) Title */		
		{
			item -> setText(0, QString::fromStdWString(detail.title));
			/* Bold and bigger */
			QFont font = item->font(0);
			font.setBold(true);
			font.setPointSize(font.pointSize() + 2);
			item->setFont(0, font);
		}

		/* (1) Rank */
		{
			std::ostringstream out;
			out << 100 * (detail.rank / (maxRank + 0.01));
			item -> setText(1, QString::fromStdString(out.str()));
			/* Bold and bigger */
			QFont font = item->font(1);
			//font.setBold(true);
			font.setPointSize(font.pointSize() + 2);
			item->setFont(1, font);
		}

		/* (2) Link */		
		{
			item -> setText(2, QString::fromStdWString(detail.link));
			/* Bold and bigger */
			QFont font = item->font(2);
			font.setBold(true);
			font.setPointSize(font.pointSize() + 2);
			item->setFont(2, font);
		}

		/* (4) rid */		
		item -> setText(4, QString::fromStdString(detail.rid));


		/* add children */
		int i = 0;
		for(cit = detail.comments.begin();
			cit != detail.comments.end(); cit++, i++)
		{
			/* create items */
           		QTreeWidgetItem *child = new QTreeWidgetItem((QTreeWidget*)0);

			/* (0) Comment */		
			if (cit->comment != L"")
			{
				child -> setText(0, QString::fromStdWString(cit->comment));
			}
			else
			{
				child -> setText(0, "No Comment");
			}

			/* (2) Peer / Date */
		        {
				QDateTime qtime;
				qtime.setTime_t(cit->timestamp);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");

				QString peerLabel = QString::fromStdString(rsPeers->getPeerName(cit->id));
				peerLabel += "  ";
				peerLabel += timestamp;
				child -> setText(2, peerLabel);

			}
			
			/* (4) Id */		
			child -> setText(4, QString::fromStdString(cit->id));

			if (i % 2 == 1)
			{
				/* set to light gray background */
				child->setBackground(0,QBrush(Qt::lightGray));
				child->setBackground(1,QBrush(Qt::lightGray));
				child->setBackground(2,QBrush(Qt::lightGray));
			}

			/* push to items */
			item->addChild(child);
		}

		/* add to the list */
		items.append(item);
	}

        /* remove old items */
	linkWidget->clear();
	linkWidget->setColumnCount(3);

	/* add the items in! */
	linkWidget->insertTopLevelItems(0, items);

	linkWidget->update(); /* update display */


}

void LinksDialog::openLink ( QTreeWidgetItem * item, int column )
{
	std::cerr << "LinksDialog::openLink()" << std::endl;

	/* work out the ids */
	if (!item)
	{
		std::cerr << "LinksDialog::openLink() Failed Item" << std::endl;
		return;
	}

	std::string rid;	
	std::string pid;	

	QTreeWidgetItem *parent = item->parent();
	if (parent)
	{
		/* a child comment -> ignore double click */
		std::cerr << "LinksDialog::openLink() Failed Child" << std::endl;
		return;
	}

	std::cerr << "LinksDialog::openLink() " << (item->text(2)).toStdString() << std::endl;
	/* open a browser */
	QUrl url(item->text(2));
	QDesktopServices::openUrl ( url );

	/* close expansion */
	bool state = item->isExpanded();
	item->setExpanded(!state);
}

void  LinksDialog::changedItem(QTreeWidgetItem *curr, QTreeWidgetItem *prev)
{
	/* work out the ids */
	if (!curr)
	{
		updateComments("", "");
		return;
	}

	std::string rid;	
	std::string pid;	

	QTreeWidgetItem *parent = curr->parent();
	if (parent)
	{
		rid = (parent->text(4)).toStdString();
		pid = (curr->text(4)).toStdString();

		std::cerr << "LinksDialog::changedItem() Rid: " << rid << " Pid: " << pid;
		std::cerr << std::endl;

		updateComments(rid, pid);
	}
	else
	{
		rid = (curr->text(4)).toStdString();

		std::cerr << "LinksDialog::changedItem() Rid: " << rid << " Pid: NULL";
		std::cerr << std::endl;

		updateComments(rid, "");
	}
}



/* get the list of Links from the RsRanks.  */
void  LinksDialog::updateComments(std::string rid, std::string pid)
{
	std::list<RsRankComment>::iterator cit;

	if (rid == "")
	{
		/* clear it up */
		ui.titleLineEdit->setText("");
		ui.linkLineEdit->setText("");
		ui.linkTextEdit->setText("");
		mLinkId = rid;
		return;
	}

	RsRankDetails detail;
	if (!rsRanks->getRankDetails(rid, detail))
	{
		/* clear it up */
		ui.titleLineEdit->setText("");
		ui.linkLineEdit->setText("");
		ui.linkTextEdit->setText("");
		mLinkId = "";
		return;
	}

	/* set Link details */
	ui.titleLineEdit->setText(QString::fromStdWString(detail.title));
	ui.linkLineEdit->setText(QString::fromStdWString(detail.link));

	if (mLinkId == rid)
	{
		/* leave comments */
		//ui.linkTextEdit->setText("");
		return;
	}

	mLinkId = rid;

	/* Add your text to the comment */
	std::string ownId = rsPeers->getOwnId();

	for(cit = detail.comments.begin(); cit != detail.comments.end(); cit++)
	{
		if (cit->id == ownId)
			break;
	}

	if (cit != detail.comments.end())
	{
		QString comment = QString::fromStdWString(cit->comment);
		ui.linkTextEdit->setText(comment);
	}
	else
	{
		ui.linkTextEdit->setText("");

	}

	return;
		

#if 0

	for(cit = detail.comments.begin(); cit != detail.comments.end(); cit++)
	{
		if (cit->id == pid)
			break;
	}

	if (cit != detail.comments.end())
	{
		/* one comment selected */
		QString comment;

		QDateTime qtime;
		qtime.setTime_t(cit->timestamp);
		QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");

		comment += "Submitter: ";
		comment += QString::fromStdString(rsPeers->getPeerName(cit->id));
		comment += "  Date: ";
		comment += timestamp;
		comment += "\n";
		comment += QString::fromStdWString(cit->comment);

		ui.linkTextEdit->setText(comment);
		return;
	}


	QString comment;
	for(cit = detail.comments.begin(); cit != detail.comments.end(); cit++)
	{
		/* add all comments */
		QDateTime qtime;
		qtime.setTime_t(cit->timestamp);
		QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");

		comment += "Submitter: ";
		comment += QString::fromStdString(rsPeers->getPeerName(cit->id));
		comment += "  Date: ";
		comment += timestamp;
		comment += "\n";
		comment += QString::fromStdWString(cit->comment);
		comment += "\n====================================================";
		comment += "====================================================\n";
	}

	ui.linkTextEdit->setText(comment);

	/* .... 
	 * */
	/**
	mLinkId = rid;
	mPeerId = pid;
	mCommentFilled = true;
	**/
#endif

}

void LinksDialog::addLinkComment( void ) 
{
	/* get the title / link / comment */
	QString title = ui.titleLineEdit->text();
	QString link = ui.linkLineEdit->text();
	QString comment = ui.linkTextEdit->toPlainText();

	if (mLinkId == "") 
	{
		if ((link == "") || (title == ""))
		{
                	QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
		                                "Add Link Failure",
						"Missing Link and/or Title",
						QMessageBox::Ok);
			/* can't do anything */
			return;
		}
		rsRanks->newRankMsg(
			link.toStdWString(),
			title.toStdWString(),
			comment.toStdWString());
	
		updateLinks();
		return;
	}

	/* get existing details */

	RsRankDetails detail;
	if (!rsRanks->getRankDetails(mLinkId, detail))
	{
		/* strange error! */
                QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
		                        "Add Link Failure",
					"Missing Link Data",
					QMessageBox::Ok);
		return;
	}

	if (link.toStdWString() == detail.link) /* same link! - we can add a comment */
	{
		if (comment == "") /* no comment! */
		{
                	QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
		                        "Add Link Failure",
					"Missing Comment",
					QMessageBox::Ok);
			return;
		}

		rsRanks->updateComment(mLinkId, 
			comment.toStdWString());
	}
	else
	{
        	QMessageBox::StandardButton sb = QMessageBox::Yes;

		if ((title.toStdWString() == detail.title) /* same title! - wrong */
		     || (title == ""))
		{
        		sb = QMessageBox::question ( NULL, "Link Title Not Changed", 
				"Do you want to continue?",
			        (QMessageBox::Yes | QMessageBox::No));
		}

		/* add Link! */
		if (sb == QMessageBox::Yes)
		{
			rsRanks->newRankMsg(
				link.toStdWString(),
				title.toStdWString(),
				comment.toStdWString());
		}
	}
	updateLinks();
	return;
}

void LinksDialog::toggleWindows( void )
{
	/* if msg header visible -> hide by changing splitter 
	 */

	QList<int> sizeList = ui.msgSplitter->sizes();
	QList<int>::iterator it;

	int listSize = 0;
	int msgSize = 0;
	int i = 0;

	for(it = sizeList.begin(); it != sizeList.end(); it++, i++)
	{
		if (i == 0)
		{
			listSize = (*it);
		}
		else if (i == 1)
		{
			msgSize = (*it);
		}
	}

	int totalSize = listSize + msgSize;

	bool toShrink = true;
	if (msgSize < (int) totalSize / 10)
	{
		toShrink = false;
	}

	QList<int> newSizeList;
	if (toShrink)
	{
		newSizeList.push_back(totalSize);
		newSizeList.push_back(0);
	}
	else
	{
		newSizeList.push_back(totalSize * 3/4);
		newSizeList.push_back(totalSize * 1/4);
	}

	ui.msgSplitter->setSizes(newSizeList);
	return;
}


QTreeWidgetItem *LinksDialog::getCurrentLine()
{
	/* get the current, and extract the Id */

	/* get a link to the table */
        QTreeWidget *peerWidget = ui.linkTreeWidget;
        QTreeWidgetItem *item = peerWidget -> currentItem();
        if (!item)
        {
		std::cerr << "Invalid Current Item" << std::endl;
		return NULL;
	}

	/* Display the columns of this item. */
	std::ostringstream out;
        out << "CurrentPeerItem: " << std::endl;

	for(int i = 1; i < 6; i++)
	{
		QString txt = item -> text(i);
		out << "\t" << i << ":" << txt.toStdString() << std::endl;
	}
	std::cerr << out.str();
	return item;
}

void LinksDialog::voteup()
{
	//QTreeWidgetItem *c = getCurrentLine();

	if (mLinkId == "")
	{
		return;
	}

	RsRankDetails detail;
	if (!rsRanks->getRankDetails(mLinkId, detail))
	{
		/* not there! */
		return;
	}

	QString link = QString::fromStdWString(detail.link);
	std::cerr << "LinksDialog::voteup() : " << link.toStdString() << std::endl;
}

void LinksDialog::votedown()
{
	QTreeWidgetItem *c = getCurrentLine();
	std::cerr << "LinksDialog::votedown()" << std::endl;

	/* need to get the input address / port */
	/*
 	std::string addr;
	unsigned short port;
	rsServer->FriendSetAddress(getPeerRsCertId(c), addr, port);
	*/
}



