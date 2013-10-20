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

#include <QMenu>
#include <QTimer>
#include <QMessageBox>
#include <QDateTime>
#include <QDesktopServices>
#include "LinksDialog.h"
#include <gui/RetroShareLink.h>
#include "AddLinksDialog.h"
#include "rsrank.h"
#include "util/QtVersion.h"

#include <sstream>


/* Images for context menu icons */
#define IMAGE_EXPORTFRIEND      ":/images/exportpeers_16x16.png"
#define IMAGE_GREAT			    ":/images/filerating5.png"
#define IMAGE_GOOD			    ":/images/filerating4.png"
#define IMAGE_OK			    ":/images/filerating3.png"
#define IMAGE_SUX			    ":/images/filerating2.png"
#define IMAGE_BADLINK			   ":/images/filerating1.png"
#define IMAGE_NOCOMMENTRATING			":/images/filerating0.png"
#define IMAGE_DOWNLOAD       		":/images/download16.png"

/******
 * #define LINKS_DEBUG 1
 *****/

/** Constructor */
LinksDialog::LinksDialog(RsPeers *peers, RsFiles *files, QWidget *parent)
: MainPage(parent), mPeers(peers), mFiles(files)
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
  
  connect( ui.addToolButton, SIGNAL( clicked( ) ), this, SLOT( addNewLink( ) ) );

  connect( ui.linkTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem * ) ),
  		this, SLOT( changedItem ( QTreeWidgetItem *, QTreeWidgetItem * ) ) );

  connect( ui.linkTreeWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int ) ),
  		this, SLOT( openLink ( QTreeWidgetItem *, int ) ) );

  connect( ui.anonBox, SIGNAL( stateChanged ( int ) ), this, SLOT( checkAnon ( void  ) ) );

  mStart = 0;


    /* Set header resize modes and initial section sizes */
	QHeaderView * _header = ui.linkTreeWidget->header () ;
	QHeaderView_setSectionResizeMode(_header, 0, QHeaderView::Interactive);
	QHeaderView_setSectionResizeMode(_header, 1, QHeaderView::Interactive);
	QHeaderView_setSectionResizeMode(_header, 2, QHeaderView::Interactive);

	_header->resizeSection ( 0, 400 );
	_header->resizeSection ( 1, 60 );
	_header->resizeSection ( 2, 150 );

	ui.linkTreeWidget->setSortingEnabled(true);
	
	ui.linklabel->setMinimumWidth(20);


	/* Set a GUI update timer - much cleaner than
	 * doing everything through the notify agent
	 */

  QTimer *timer = new QTimer(this);
  timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
  timer->start(1000);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif

}

void LinksDialog::checkUpdate()
{
	/* update */
	if (!rsRanks)
	{
		std::cerr << "  rsRanks = 0 !!!!" << std::endl;
		return;
	}

	if (rsRanks->updated())
	{
#ifdef LINKS_DEBUG
		std::cerr << "  rsRanks was updated -> redraw()" << std::endl;
#endif
		updateLinks();
	}

	return;
}

void LinksDialog::linkTreeWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );

      QAction *voteupAct = new QAction(QIcon(IMAGE_EXPORTFRIEND), tr( "Share Link Anonymously" ), &contextMnu );
      connect( voteupAct , SIGNAL( triggered() ), this, SLOT( voteup_anon() ) );


      	QMenu *voteMenu = new QMenu( tr("Vote on Link"), &contextMnu );
      	voteMenu->setIcon(QIcon(IMAGE_EXPORTFRIEND));

        QAction *vote_p2 = new QAction( QIcon(IMAGE_GREAT), tr("+2 Great!"), &contextMnu );
        connect( vote_p2 , SIGNAL( triggered() ), this, SLOT( voteup_p2() ) );
	voteMenu->addAction(vote_p2);
		QAction *vote_p1 = new QAction( QIcon(IMAGE_GOOD), tr("+1 Good"), &contextMnu );
        connect( vote_p1 , SIGNAL( triggered() ), this, SLOT( voteup_p1() ) );
	voteMenu->addAction(vote_p1);
		QAction *vote_p0 = new QAction( QIcon(IMAGE_OK), tr("0 Okay"), &contextMnu );
        connect( vote_p0 , SIGNAL( triggered() ), this, SLOT( voteup_p0() ) );
	voteMenu->addAction(vote_p0);
		QAction *vote_m1 = new QAction( QIcon(IMAGE_SUX), tr("-1 Sux"), &contextMnu );
        connect( vote_m1 , SIGNAL( triggered() ), this, SLOT( voteup_m1() ) );
	voteMenu->addAction(vote_m1);
		QAction *vote_m2 = new QAction( QIcon(IMAGE_BADLINK), tr("-2 Bad Link"), &contextMnu );
        connect( vote_m2 , SIGNAL( triggered() ), this, SLOT( voteup_m2() ) );
	voteMenu->addAction(vote_m2);

	QAction *downloadAct = new QAction(QIcon(IMAGE_DOWNLOAD), tr("Download"), &contextMnu);
	connect(downloadAct, SIGNAL(triggered()), this, SLOT(downloadSelected()));

      contextMnu.addAction(voteupAct);
      contextMnu.addSeparator();
      contextMnu.addMenu(voteMenu);
      contextMnu.addSeparator();
      contextMnu.addAction(downloadAct);

      contextMnu.exec(ui.linkTreeWidget->viewport()->mapToGlobal(point));
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
                        peers.push_back(mPeers->getOwnId());
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

#define ENTRIES_PER_BLOCK 100

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
		case 1:
			mStart = 1 * ENTRIES_PER_BLOCK;
			break;
		case 2:
			mStart = 2 * ENTRIES_PER_BLOCK;
			break;
		case 3:
			mStart = 3 * ENTRIES_PER_BLOCK;
			break;
		case 4:
			mStart = 4 * ENTRIES_PER_BLOCK;
			break;
		case 5:
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

#ifdef LINKS_DEBUG
	std::cerr << "LinksDialog::updateLinks()" << std::endl;
#endif

	/* Work out the number/entries to show */
	uint32_t count = rsRanks->getRankingsCount();
	uint32_t start;

	uint32_t entries = ENTRIES_PER_BLOCK;
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
			item -> setSizeHint(0,  QSize( 20,20 ) ); 

			/* Bold and bigger */
			/*QFont font = item->font(0);
			font.setBold(true);
			font.setPointSize(font.pointSize() + 2);
			item->setFont(0, font);*/
		}

		/* (1) Rank */
		{
			std::ostringstream out;
			out << 100 * (detail.rank / (maxRank + 0.01));
			item -> setText(1, QString::fromStdString(out.str()));
			item -> setSizeHint(1,  QSize( 20,20 ) ); 
			
			/* Bold and bigger */
			/*QFont font = item->font(1);
			font.setBold(true);
			font.setPointSize(font.pointSize() + 2);
			item->setFont(1, font);*/
		}

		/* (2) Link */
		{
			item -> setText(2, QString::fromStdWString(detail.link));
			item -> setSizeHint(2,  QSize( 20,20 ) ); 

			/* Bold and bigger */
			/*QFont font = item->font(2);
			font.setBold(true);
			font.setPointSize(font.pointSize() + 2);
			item->setFont(2, font);*/
		}
		
		/* (3) Date */
		/*{
				QDateTime qtime;
				qtime.setTime_t(it->lastPost);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
				item -> setText(3, timestamp);
	 }*/

		
		/* (4) rid */
		item -> setText(4, QString::fromStdString(detail.rid));


		/* add children */
		int i = 0;
		for(cit = detail.comments.begin();
			cit != detail.comments.end(); cit++, i++)
		{
			/* create items */
           		QTreeWidgetItem *child = new QTreeWidgetItem((QTreeWidget*)0);

			QString commentText;
			QString peerScore;
			if (cit->score > 1)
			{
				peerScore = "[+2] ";
				child -> setIcon(0,(QIcon(IMAGE_GREAT)));
				item -> setIcon(0,(QIcon(IMAGE_GREAT)));
				//peerScore = "[+2 Great Link] ";
			}
			else if (cit->score == 1)
			{
				peerScore = "[+1] ";
				child -> setIcon(0,(QIcon(IMAGE_GOOD)));
				item -> setIcon(0,(QIcon(IMAGE_GOOD)));
				//peerScore = "[+1 Good] ";
			}
			else if (cit->score == 0)
			{
				peerScore = "[+0] ";
				child -> setIcon(0,(QIcon(IMAGE_OK)));
				item -> setIcon(0,(QIcon(IMAGE_OK)));
				//peerScore = "[+0 Okay] ";
			}
			else if (cit->score == -1)
			{
				peerScore = "[-1] ";
				child -> setIcon(0,(QIcon(IMAGE_SUX)));
				item -> setIcon(0,(QIcon(IMAGE_SUX)));
				//peerScore = "[-1 Not Worth It] ";
			}
			else //if (cit->score < -1)
			{
				peerScore = "[-2 BAD] ";
				child -> setIcon(0,(QIcon(IMAGE_BADLINK)));
				item -> setIcon(0,(QIcon(IMAGE_BADLINK)));
				//peerScore = "[-2 BAD Link] ";
			}

			/* (0) Comment */
			if (cit->comment != L"")
			{
				commentText = peerScore + QString::fromStdWString(cit->comment);
			}
			else
			{
				commentText = peerScore + "No Comment";
			}
			child -> setText(0, commentText);

			/* (2) Peer / Date */
		        {
				QDateTime qtime;
				qtime.setTime_t(cit->timestamp);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");

                                QString peerLabel = QString::fromStdString(mPeers->getPeerName(cit->id));
				if (peerLabel == "")
				{
					peerLabel = "<";
					peerLabel += QString::fromStdString(cit->id);
					peerLabel += ">";
				}
				peerLabel += " ";

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

void LinksDialog::openLink ( QTreeWidgetItem * item, int )
{
#ifdef LINKS_DEBUG
	std::cerr << "LinksDialog::openLink()" << std::endl;
#endif

	/* work out the ids */
	if (!item)
	{

#ifdef LINKS_DEBUG
		std::cerr << "LinksDialog::openLink() Failed Item" << std::endl;
#endif
		return;
	}

	std::string rid;
	std::string pid;

	QTreeWidgetItem *parent = item->parent();
	if (parent)
	{
		/* a child comment -> ignore double click */
#ifdef LINKS_DEBUG
		std::cerr << "LinksDialog::openLink() Failed Child" << std::endl;
#endif
		return;
	}

#ifdef LINKS_DEBUG
	std::cerr << "LinksDialog::openLink() " << (item->text(2)).toStdString() << std::endl;
#endif
	/* open a browser */
	QUrl url(item->text(2));
	QDesktopServices::openUrl ( url );

	/* close expansion */
	bool state = item->isExpanded();
	item->setExpanded(!state);
}

void  LinksDialog::changedItem(QTreeWidgetItem *curr, QTreeWidgetItem *)
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

#ifdef LINKS_DEBUG
		std::cerr << "LinksDialog::changedItem() Rid: " << rid << " Pid: " << pid;
		std::cerr << std::endl;
#endif

		updateComments(rid, pid);
	}
	else
	{
		rid = (curr->text(4)).toStdString();

#ifdef LINKS_DEBUG
		std::cerr << "LinksDialog::changedItem() Rid: " << rid << " Pid: NULL";
		std::cerr << std::endl;
#endif

		updateComments(rid, "");
	}
}

void LinksDialog::checkAnon()
{
	changedItem(ui.linkTreeWidget->currentItem(), NULL);
}


int IndexToScore(int index)
{
	if ((index == -1) || (index > 4))
		return 0;
	int score = 2 - index;
	return score;
}

int ScoreToIndex(int score)
{
	if ((score < -2) || (score > 2))
		return 2;
	int index = 2 - score;
	return index;
}


/* get the list of Links from the RsRanks.  */
void  LinksDialog::updateComments(std::string rid, std::string )
{
	std::list<RsRankComment>::iterator cit;


	if (ui.anonBox->isChecked())
	{
		/* empty everything */
		ui.titleLineEdit->setText("");
		ui.linkLineEdit->setText("");
		ui.linkTextEdit->setText("");
		ui.scoreBox->setCurrentIndex(ScoreToIndex(0));
		mLinkId = rid; /* must be set for Context Menu */

		/* disable comment + score */
		ui.scoreBox->setEnabled(false);
		ui.linkTextEdit->setEnabled(false);

		/* done! */
		return;
	}
	else
	{
		/* enable comment + score */
		ui.scoreBox->setEnabled(true);
		ui.linkTextEdit->setEnabled(true);
	}


	RsRankDetails detail;
	if ((rid == "") || (!rsRanks->getRankDetails(rid, detail)))
	{
		/* clear it up */
		ui.titleLineEdit->setText("");
		ui.linkLineEdit->setText("");
		ui.linkTextEdit->setText("");
		ui.scoreBox->setCurrentIndex(ScoreToIndex(0));
		mLinkId = rid;
		return;
	}


	/* set Link details */
	ui.titleLineEdit->setText(QString::fromStdWString(detail.title));
	ui.linkLineEdit->setText(QString::fromStdWString(detail.link));
  ui.linklabel->setText("<a href='" + QString::fromStdWString(detail.link) + "'> " + QString::fromStdWString(detail.link) +"</a>");


	if (mLinkId == rid)
	{
		/* leave comments */
		//ui.linkTextEdit->setText("");
		return;
	}

	mLinkId = rid;

	/* Add your text to the comment */
        std::string ownId = mPeers->getOwnId();

	for(cit = detail.comments.begin(); cit != detail.comments.end(); cit++)
	{
		if (cit->id == ownId)
			break;
	}

	if (cit != detail.comments.end())
	{
		QString comment = QString::fromStdWString(cit->comment);
		ui.linkTextEdit->setText(comment);
		ui.scoreBox->setCurrentIndex(ScoreToIndex(cit->score));
	}
	else
	{
		ui.linkTextEdit->setText("");
		ui.scoreBox->setCurrentIndex(ScoreToIndex(0));

	}

	return;
}

void LinksDialog::addLinkComment( void )
{
	/* get the title / link / comment */
	QString title = ui.titleLineEdit->text();
	QString link = ui.linkLineEdit->text();
	QString comment = ui.linkTextEdit->toPlainText();
	int32_t   score = IndexToScore(ui.scoreBox->currentIndex());

	if ((mLinkId == "") || (ui.anonBox->isChecked()))
	{
		if ((link == "") || (title == ""))
		{
			QMessageBox::warning ( NULL, tr("Add Link Failure"), tr("Missing Link and/or Title"), QMessageBox::Ok);
			/* can't do anything */
			return;
		}

		/* add it either way */
		if (ui.anonBox->isChecked())
		{
			rsRanks->anonRankMsg("", link.toStdWString(), title.toStdWString());
		}
		else
		{
			rsRanks->newRankMsg(
				link.toStdWString(),
				title.toStdWString(),
				comment.toStdWString(), score);
		}

		updateLinks();
		return;
	}

	/* get existing details */

	RsRankDetails detail;
	if (!rsRanks->getRankDetails(mLinkId, detail))
	{
		/* strange error! */
		QMessageBox::warning ( NULL, tr("Add Link Failure"), tr("Missing Link Data"), QMessageBox::Ok);
		return;
	}

	if (link.toStdWString() == detail.link) /* same link! - we can add a comment */
	{
		if (comment == "") /* no comment! */
		{
			QMessageBox::warning ( NULL, tr("Add Link Failure"), tr("Missing Comment"), QMessageBox::Ok);
			return;
		}

		rsRanks->updateComment(mLinkId,
			comment.toStdWString(),
			score);
	}
	else
	{
		QMessageBox::StandardButton sb = QMessageBox::Yes;

		if ((title.toStdWString() == detail.title) /* same title! - wrong */
		     || (title == ""))
		{
			sb = QMessageBox::question ( NULL, tr("Link Title Not Changed"), tr("Do you want to continue?"), (QMessageBox::Yes | QMessageBox::No));
		}

		/* add Link! */
		if (sb == QMessageBox::Yes)
		{
			rsRanks->newRankMsg(
				link.toStdWString(),
				title.toStdWString(),
				comment.toStdWString(),
				score);
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
		ui.expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
            ui.expandButton->setToolTip(tr("Expand"));
	}
	else
	{
		newSizeList.push_back(totalSize * 3/4);
		newSizeList.push_back(totalSize * 1/4);
	    ui.expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
            ui.expandButton->setToolTip(tr("Hide"));
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
#ifdef LINKS_DEBUG
		std::cerr << "Invalid Current Item" << std::endl;
#endif
		return NULL;
	}

#ifdef LINKS_DEBUG
	/* Display the columns of this item. */
	std::ostringstream out;
        out << "CurrentPeerItem: " << std::endl;

	for(int i = 1; i < 6; i++)
	{
		QString txt = item -> text(i);
		out << "\t" << i << ":" << txt.toStdString() << std::endl;
	}
	std::cerr << out.str();
#endif

	return item;
}

void LinksDialog::voteup_anon()
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
#ifdef LINKS_DEBUG
	std::cerr << "LinksDialog::voteup_anon() : " << link.toStdString() << std::endl;
#endif
	// need a proper anon sharing option.
	rsRanks->anonRankMsg(mLinkId, detail.link, detail.title);
}




void LinksDialog::voteup_score(int score)
{
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
	std::wstring comment;
#ifdef LINKS_DEBUG
	std::cerr << "LinksDialog::voteup_score() : " << link.toStdString() << std::endl;
#endif


	std::list<RsRankComment>::iterator cit;
	/* Add your text to the comment */
        std::string ownId = mPeers->getOwnId();

	for(cit = detail.comments.begin(); cit != detail.comments.end(); cit++)
	{
		if (cit->id == ownId)
			break;
	}

	if (cit != detail.comments.end())
	{
		comment = cit->comment;
	}

	rsRanks->updateComment(mLinkId, comment, score);
}


void LinksDialog::voteup_p2()
{
	voteup_score(2);
}

void LinksDialog::voteup_p1()
{
	voteup_score(1);
}

void LinksDialog::voteup_p0()
{
	voteup_score(0);
}

void LinksDialog::voteup_m1()
{
	voteup_score(-1);
}

void LinksDialog::voteup_m2()
{
	voteup_score(-2);
}

void LinksDialog::downloadSelected()
{
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
	std::wstring comment;
#ifdef LINKS_DEBUG
	std::cerr << "LinksDialog::downloadSelected() : " << link.toStdString() << std::endl;
#endif

        //RetroShareLink rslink(QString::fromStdWString(detail.link));

        //if(!rslink.valid() || rslink.type() != RetroShareLink::TYPE_FILE)
        //{
//		QMessageBox::critical(NULL,"Badly formed link","This link is badly formed. Can't parse/use it. This is a bug. Please contact the developers.") ;
//		return ;
//	}

	/* retrieve all peers id for this file */
//	FileInfo info;
//	rsFiles->FileDetails(rslink.hash().toStdString(), 0, info);

//	std::list<std::string> srcIds;
//	std::list<TransferInfo>::iterator pit;
//	for (pit = info.peers.begin(); pit != info.peers.end(); pit ++)
//		srcIds.push_back(pit->peerId);

//	rsFiles->FileRequest(rslink.name().toStdString(), rslink.hash().toStdString(), rslink.size(), "", 0, srcIds);
}

void LinksDialog::addNewLink()
{

	AddLinksDialog *nAddLinksDialog = new AddLinksDialog("");

	nAddLinksDialog->show();

	/* window will destroy itself! */
}
