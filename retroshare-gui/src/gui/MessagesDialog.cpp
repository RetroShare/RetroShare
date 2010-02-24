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


#include "MessagesDialog.h"
#include "msgs/ChanMsgDialog.h"
#include "gui/toaster/MessageToaster.h"
#include "util/printpreview.h"
#include "util/misc.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"
#include <sstream>
#include <iostream>
#include <string>

#include <QtGui>
#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QDateTime>
#include <QHeaderView>
#include <QModelIndex>
#include <QStandardItemModel>

/* Images for context menu icons */
#define IMAGE_MESSAGE		   ":/images/folder-draft.png"
#define IMAGE_MESSAGEREPLY	   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREPLYALL      ":/images/mail_replyall.png"
#define IMAGE_MESSAGEFORWARD	   ":/images/mail_forward.png"
#define IMAGE_MESSAGEREMOVE 	   ":/images/message-mail-imapdelete.png"
#define IMAGE_DOWNLOAD    	   ":/images/start.png"
#define IMAGE_DOWNLOADALL          ":/images/startall.png"

/** Constructor */
MessagesDialog::MessagesDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
    /* Create RshareSettings object */
  _settings = new RshareSettings();

  connect( ui.messagestreeView, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( messageslistWidgetCostumPopupMenu( QPoint ) ) );
  connect( ui.msgList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( msgfilelistWidgetCostumPopupMenu( QPoint ) ) );
  connect( ui.messagestreeView, SIGNAL(clicked ( const QModelIndex &) ) , this, SLOT( updateCurrentMessage( const QModelIndex & ) ) );
  connect( ui.listWidget, SIGNAL( currentRowChanged ( int) ), this, SLOT( changeBox ( int) ) );
  
  connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(newmessage()));
  connect(ui.removemessageButton, SIGNAL(clicked()), this, SLOT(removemessage()));
  connect(ui.replymessageButton, SIGNAL(clicked()), this, SLOT(replytomessage()));
  connect(ui.replyallmessageButton, SIGNAL(clicked()), this, SLOT(replyallmessage()));
  connect(ui.forwardmessageButton, SIGNAL(clicked()), this, SLOT(forwardmessage()));

  connect(ui.actionPrint, SIGNAL(triggered()), this, SLOT(print()));
  connect(ui.actionPrintPreview, SIGNAL(triggered()), this, SLOT(printpreview()));

  connect(ui.expandFilesButton, SIGNAL(clicked()), this, SLOT(togglefileview()));
  connect(ui.downloadButton, SIGNAL(clicked()), this, SLOT(getcurrentrecommended()));
  
  connect( ui.msgText, SIGNAL( anchorClicked(const QUrl &)), SLOT(anchorClicked(const QUrl &)));
  
  connect(ui.actionTextBesideIcon, SIGNAL(triggered()), this, SLOT(buttonstextbesideicon()));
  connect(ui.actionIconOnly, SIGNAL(triggered()), this, SLOT(buttonsicononly()));
  connect(ui.actionTextUnderIcon, SIGNAL(triggered()), this, SLOT(buttonstextundericon()));
  
  connect(ui.actionSave_as, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
  
  connect( ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
  connect( ui.filterPatternLineEdit, SIGNAL( textChanged(const QString &)), this, SLOT(toggleclearButton()));
  
  connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(filterRegExpChanged()));

  connect(ui.filterColumnComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(filterColumnChanged()));



  mCurrCertId = "";
  mCurrMsgId  = "";
  
    // Set the QStandardItemModel 
    MessagesModel = new QStandardItemModel(0, 6);
    MessagesModel->setHeaderData(0, Qt::Horizontal, tr("#"));
    MessagesModel->setHeaderData(1, Qt::Horizontal, tr("Subject"));
    MessagesModel->setHeaderData(2, Qt::Horizontal, tr("From"));
    MessagesModel->setHeaderData(3, Qt::Horizontal, tr("Date"));    
    MessagesModel->setHeaderData(4, Qt::Horizontal, tr("SRCID"));
    MessagesModel->setHeaderData(5, Qt::Horizontal, tr("MSGID"));
   
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSourceModel(MessagesModel);
    ui.messagestreeView->setModel(proxyModel);
    ui.messagestreeView->setSelectionBehavior(QTreeView::SelectRows);
    
    ui.messagestreeView->setRootIsDecorated(false);
    ui.messagestreeView->setSortingEnabled(true); 
    ui.messagestreeView->sortByColumn(3, Qt::DescendingOrder);
     
  /* hide the Tree +/- */
  ui.msgList->setRootIsDecorated( false );
  ui.msgList->setSelectionMode( QAbstractItemView::ExtendedSelection );
  
  /* Set header resize modes and initial section sizes */
  QHeaderView * msgwheader = ui.messagestreeView->header () ;
  msgwheader->setResizeMode (0, QHeaderView::Custom);
  msgwheader->setResizeMode (3, QHeaderView::Interactive);
    
  msgwheader->resizeSection ( 0, 24 );
  msgwheader->resizeSection ( 1, 250 );
  msgwheader->resizeSection ( 2, 140 );
  msgwheader->resizeSection ( 3, 140 );
  
    /* Set header resize modes and initial section sizes */
	QHeaderView * msglheader = ui.msgList->header () ;
  msglheader->setResizeMode (0, QHeaderView::Interactive);
	msglheader->setResizeMode (1, QHeaderView::Interactive);
	msglheader->setResizeMode (2, QHeaderView::Interactive);
	msglheader->setResizeMode (3, QHeaderView::Interactive);
  
	msglheader->resizeSection ( 0, 200 );
	msglheader->resizeSection ( 1, 100 );
	msglheader->resizeSection ( 2, 100 );
	msglheader->resizeSection ( 3, 200 );

	ui.forwardmessageButton->setToolTip(tr("Forward selected Message"));
 	ui.replyallmessageButton->setToolTip(tr("Reply to All"));

    QMenu * printmenu = new QMenu();
    printmenu->addAction(ui.actionPrint);
    printmenu->addAction(ui.actionPrintPreview);
    ui.printbutton->setMenu(printmenu);
    
    QMenu * viewmenu = new QMenu();
    viewmenu->addAction(ui.actionTextBesideIcon);
    viewmenu->addAction(ui.actionIconOnly);
    viewmenu->addAction(ui.actionTextUnderIcon);
    ui.viewtoolButton->setMenu(viewmenu);

	 loadToolButtonsettings();
	 
	 mFont = QFont("Arial", 10, QFont::Bold);
   ui.subjectText->setFont(mFont);

	//sertting default filter by column as subject
    proxyModel->setFilterKeyColumn(ui.filterColumnComboBox->currentIndex());
    
      ui.clearButton->hide();

        
  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void MessagesDialog::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Delete)
	{
		removemessage() ;
		e->accept() ;
	}
	else
		MainPage::keyPressEvent(e) ;
}

void MessagesDialog::messageslistWidgetCostumPopupMenu( QPoint point )
{
      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      contextMnu.clear();

      newmsgAct = new QAction(QIcon(IMAGE_MESSAGE), tr( "New Message" ), this );
      connect( newmsgAct , SIGNAL( triggered() ), this, SLOT( newmessage() ) );
      
	//To check if the selection has more than one row.
		QList<QModelIndex> selectedIndexList = ui.messagestreeView->selectionModel() -> selectedIndexes ();
		QList<int> rowList;
		for(QList<QModelIndex>::iterator it = selectedIndexList.begin(); it != selectedIndexList.end(); it++)
		{
			
			int row = it->row();
			if (rowList.contains(row) == false)
			{
				rowList.append(row);
			}
		}
		
		int nn = rowList.size();
		if(nn > 1)
		{
			removemsgAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Remove Messages" ), this );
			connect( removemsgAct , SIGNAL( triggered() ), this, SLOT( removemessage() ) );
			contextMnu.addAction( removemsgAct);
		}
		else if(nn == 1)
		{
			replytomsgAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply to Message" ), this );
			connect( replytomsgAct , SIGNAL( triggered() ), this, SLOT( replytomessage() ) );
			contextMnu.addAction( replytomsgAct);

			replyallmsgAct = new QAction(QIcon(IMAGE_MESSAGEREPLYALL), tr( "Reply to All" ), this );
			connect( replyallmsgAct , SIGNAL( triggered() ), this, SLOT( replyallmessage() ) );
			contextMnu.addAction( replyallmsgAct);

			forwardmsgAct = new QAction(QIcon(IMAGE_MESSAGEFORWARD), tr( "Forward Message" ), this );
			connect( forwardmsgAct , SIGNAL( triggered() ), this, SLOT( forwardmessage() ) );
			contextMnu.addAction( forwardmsgAct);

			contextMnu.addSeparator(); 

			removemsgAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Remove Message" ), this );
			connect( removemsgAct , SIGNAL( triggered() ), this, SLOT( removemessage() ) );
			contextMnu.addAction( removemsgAct);
		}
      contextMnu.addAction( ui.actionSave_as);
		  contextMnu.addAction( ui.actionPrintPreview);
      contextMnu.addAction( ui.actionPrint);
      contextMnu.addSeparator(); 
      contextMnu.addAction( newmsgAct);
      contextMnu.exec( mevent->globalPos() );
}


void MessagesDialog::msgfilelistWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      getRecAct = new QAction(QIcon(IMAGE_DOWNLOAD), tr( "Download" ), this );
      connect( getRecAct , SIGNAL( triggered() ), this, SLOT( getcurrentrecommended() ) );
      
//     getAllRecAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "Download" ), this );
//     connect( getAllRecAct , SIGNAL( triggered() ), this, SLOT( getallrecommended() ) );


      contextMnu.clear();
      contextMnu.addAction( getRecAct);
//      contextMnu.addAction( getAllRecAct);
      contextMnu.exec( mevent->globalPos() );
}

void MessagesDialog::newmessage()
{
    ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

    /* fill it in */
    //std::cerr << "MessagesDialog::newmessage()" << std::endl;
    nMsgDialog->newMsg();
    nMsgDialog->show();
    nMsgDialog->activateWindow();


    /* window will destroy itself! */
}

void MessagesDialog::replytomessage()
{
	/* put msg on msgBoard, and switch to it. */

	std::string cid;
	std::string mid;

	if(!getCurrentMsg(cid, mid))
		return ;

	mCurrCertId = cid;
	mCurrMsgId  = mid;

	MessageInfo msgInfo;
	if (!rsMsgs -> getMessage(mid, msgInfo))
		return ;

	ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);
	/* fill it in */
	//std::cerr << "MessagesDialog::newmessage()" << std::endl;
	nMsgDialog->newMsg();
	
	QString text = QString::fromStdWString(msgInfo.title);
			
	if (text.startsWith("Re:", Qt::CaseInsensitive))
	{
	nMsgDialog->insertTitleText( QString::fromStdWString(msgInfo.title).toStdString()) ;
	}
	else
	{
	nMsgDialog->insertTitleText( (QString("Re: ") + QString::fromStdWString(msgInfo.title)).toStdString()) ;
	}
	
	nMsgDialog->setWindowTitle( tr ("Compose: ") + tr("Re: ") + QString::fromStdWString(msgInfo.title) ) ;


	QTextDocument doc ;
	doc.setHtml(QString::fromStdWString(msgInfo.msg)) ;
	std::string cited_text(doc.toPlainText().toStdString()) ;

	nMsgDialog->insertPastedText(cited_text) ;
	nMsgDialog->addRecipient( msgInfo.srcId ) ;
	nMsgDialog->show();
	nMsgDialog->activateWindow();
}

void MessagesDialog::replyallmessage()
{
	/* put msg on msgBoard, and switch to it. */

	std::string cid;
	std::string mid;

	if(!getCurrentMsg(cid, mid))
		return ;

	mCurrCertId = cid;
	mCurrMsgId  = mid;

	MessageInfo msgInfo;
	if (!rsMsgs -> getMessage(mid, msgInfo))
		return ;

	ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);
	/* fill it in */
	//std::cerr << "MessagesDialog::newmessage()" << std::endl;
	nMsgDialog->newMsg();
	
	QString text = QString::fromStdWString(msgInfo.title);
			
	if (text.startsWith("Re:", Qt::CaseInsensitive))
	{
	nMsgDialog->insertTitleText( QString::fromStdWString(msgInfo.title).toStdString()) ;
	}
	else
	{
	nMsgDialog->insertTitleText( (QString("Re: ") + QString::fromStdWString(msgInfo.title)).toStdString()) ;
	}
	nMsgDialog->setWindowTitle( tr ("Compose: ") + tr("Re: ") + QString::fromStdWString(msgInfo.title) ) ;


	QTextDocument doc ;
	doc.setHtml(QString::fromStdWString(msgInfo.msg)) ;
	std::string cited_text(doc.toPlainText().toStdString()) ;

	nMsgDialog->insertPastedText(cited_text) ;
	nMsgDialog->addRecipient( msgInfo.srcId ) ;
    
    std::list<std::string> tl ( msgInfo.msgto );

    for( std::list<std::string>::iterator tli = tl.begin();
         tli!= tl.end() ;
         tli++ )
    {
        nMsgDialog->addRecipient( *tli ) ;
    }    
    
	nMsgDialog->show();
	nMsgDialog->activateWindow();
}

void MessagesDialog::forwardmessage()
{
	/* put msg on msgBoard, and switch to it. */

	std::string cid;
	std::string mid;

	if(!getCurrentMsg(cid, mid))
		return ;

	mCurrCertId = cid;
	mCurrMsgId  = mid;

	MessageInfo msgInfo;
	if (!rsMsgs -> getMessage(mid, msgInfo))
		return ;

	ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);
	/* fill it in */
	//std::cerr << "MessagesDialog::newmessage()" << std::endl;
	nMsgDialog->newMsg();
	
	QString text = QString::fromStdWString(msgInfo.title);
			
	if (text.startsWith("Fwd:", Qt::CaseInsensitive))
	{
	nMsgDialog->insertTitleText( QString::fromStdWString(msgInfo.title).toStdString()) ;
	}
	else
	{
	nMsgDialog->insertTitleText( (QString("Fwd: ") + QString::fromStdWString(msgInfo.title)).toStdString()) ;
	}
	
	nMsgDialog->setWindowTitle( tr ("Compose: ") + tr("Fwd: ") + QString::fromStdWString(msgInfo.title) ) ;


	QTextDocument doc ;
	doc.setHtml(QString::fromStdWString(msgInfo.msg)) ;
	std::string cited_text(doc.toPlainText().toStdString()) ;

	nMsgDialog->insertForwardPastedText(cited_text) ;
	//nMsgDialog->addRecipient( msgInfo.srcId ) ;
	nMsgDialog->show();
	nMsgDialog->activateWindow();
}

void MessagesDialog::togglefileview()
{
	/* if msg header visible -> change icon and tooltip 
	 * three widgets...
	 */

	if (ui.expandFilesButton->isChecked())
	{
	  ui.expandFilesButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
	  ui.expandFilesButton->setToolTip("Hide");
	}
	else
	{
    ui.expandFilesButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
	  ui.expandFilesButton->setToolTip("Expand");
	}	

}


/* download the recommendations... */
void MessagesDialog::getcurrentrecommended()
{
	MessageInfo msgInfo;
	if (!rsMsgs -> getMessage(mCurrMsgId, msgInfo))
		return;

	std::list<std::string> srcIds;
	srcIds.push_back(msgInfo.srcId);

	QModelIndexList list	= ui.msgList->selectionModel()->selectedIndexes();

	std::map<int,FileInfo> files ;

	for(QModelIndexList::const_iterator it(list.begin());it!=list.end();++it)
	{
		FileInfo& f(files[it->row()]) ;

		switch(it->column())
		{
			case 0: f.fname = it->data().toString().toStdString() ;
					  break ;
			case 1: f.size = it->data().toULongLong() ;
					  break ;
			case 3: f.hash = it->data().toString().toStdString() ;
					  break ;
			default: ;
		}
	}

	for(std::map<int,FileInfo>::const_iterator it(files.begin());it!=files.end();++it)
	{
		const FileInfo& f(it->second) ;
		std::cout << "Requesting file " << f.fname << ", size=" << f.size << ", hash=" << f.hash << std::endl ;
		rsFiles -> FileRequest(it->second.fname,it->second.hash,it->second.size, "", 0, srcIds);
	}
}

#if 0
void MessagesDialog::getallrecommended()
{
	/* get Message */
	MessageInfo msgInfo;
	if (!rsMsgs -> getMessage(mCurrMsgId, msgInfo))
	{
		return;
	}

        const std::list<FileInfo> &recList = msgInfo.files;
	std::list<FileInfo>::const_iterator it;

	std::list<std::string> fnames;
	std::list<std::string> hashes;
	std::list<int>         sizes;

	for(it = recList.begin(); it != recList.end(); it++)
	{
		fnames.push_back(it->fname);
		hashes.push_back(it->hash);
		sizes.push_back(it->size);
	}

	/* now do requests */
	std::list<std::string>::const_iterator fit;
	std::list<std::string>::const_iterator hit;
	std::list<int>::const_iterator sit;

	for(fit = fnames.begin(), hit = hashes.begin(), sit = sizes.begin(); fit != fnames.end(); fit++, hit++, sit++)
	{
		std::cerr << "MessagesDialog::getallrecommended() Calling File Request";
		std::cerr << std::endl;
		std::list<std::string> srcIds;
		srcIds.push_back(msgInfo.srcId);
        	rsFiles -> FileRequest(*fit, *hit, *sit, "", 0, srcIds);
	}
}
#endif

void MessagesDialog::changeBox(int)
{
	insertMessages();
	insertMsgTxtAndFiles();
}

void MessagesDialog::insertMessages()
{
	std::cerr <<"MessagesDialog::insertMessages called";
	fflush(0);
    int c;
    c = MessagesModel->rowCount();
    MessagesModel->removeRows(0,c);
    //ui.messagestreeView->showColumn(4);
    //ui.messagestreeView->showColumn(5);
	std::list<MsgInfoSummary> msgList;
	std::list<MsgInfoSummary>::const_iterator it;

	rsMsgs -> getMessageSummaries(msgList);

	/* get the MsgId of the current one ... */

	std::string cid;
	std::string mid;

	bool oldSelected = getCurrentMsg(cid, mid);
	QStandardItem *newSelected = NULL;

	/* remove old items ??? */

	int listrow = ui.listWidget -> currentRow();

	std::cerr << "MessagesDialog::insertMessages()" << std::endl;
	std::cerr << "Current Row: " << listrow << std::endl;
	fflush(0);
	/* check the mode we are in */
	unsigned int msgbox = 0;
	switch(listrow)
	{
		case 3:
			msgbox = RS_MSG_SENTBOX;
			break;
		case 2:
			msgbox = RS_MSG_DRAFTBOX;
			break;
		case 1:
			msgbox = RS_MSG_OUTBOX;
			break;
		case 0:
		default:
			msgbox = RS_MSG_INBOX;

			break;
	}

	//std::list<std::string>::iterator it;
	for(it = msgList.begin(); it != msgList.end(); it++)
	{
		/* check the message flags, to decide which
		 * group it should go in...
		 *
		 * InBox 
		 * OutBox 
		 * Drafts 
		 * Sent 
		 *
		 * FLAGS = OUTGOING.
		 * 	-> Outbox/Drafts/Sent
		 * 	  + SENT -> Sent
		 *	  + IN_PROGRESS -> Draft.
		 *	  + nuffing -> Outbox.
		 * FLAGS = INCOMING = (!OUTGOING)
		 * 	-> + NEW -> Bold.
		 *
		 */

		if ((it -> msgflags & RS_MSG_BOXMASK) != msgbox)
		{
			//std::cerr << "Msg from other box: " << it->msgflags;
			//std::cerr << std::endl;
			continue;
		}

		/* make a widget per friend */
    
    QStandardItem *item0 = new QStandardItem();
    QStandardItem *item1 = new QStandardItem();
    QStandardItem *item2 = new QStandardItem();
    QStandardItem *item3 = new QStandardItem();
    QStandardItem *item4 = new QStandardItem();
    QStandardItem *item5 = new QStandardItem();
    
    //set this false if you want to expand on double click
    item0->setEditable(false);
    item1->setEditable(false);
    item2->setEditable(false);
    item3->setEditable(false);
    item4->setEditable(false);
    

		/* So Text should be:
		 * (1) Msg / Broadcast
		 * (1b) Person / Channel Name
		 * (2) Rank
		 * (3) Date
		 * (4) Title
		 * (5) Msg
		 * (6) File Count
		 * (7) File Total
		 */

		// Date First.... (for sorting)
		{
			QDateTime qtime;
			qtime.setTime_t(it->ts);
			QString timestamp = qtime.toString("dd.MM.yyyy hh:mm:ss");
			item3 -> setText(timestamp);
		}

		//  From ....
		{
			item2 -> setText(QString::fromStdString(rsPeers->getPeerName(it->srcId)));
		}

		// Subject
      item1 -> setText(QString::fromStdWString(it->title));
		
		if ((it -> msgflags & RS_MSG_NEW) == RS_MSG_NEW)
		{
			QFont qf = item1->font();
			qf.setBold(true);
			item1->setFont(qf);
			
		}
		
		// Change Message icon when Subject is Re: or Fwd:
		QString text = QString::fromStdWString(it->title);
			
    if (text.startsWith("Re:", Qt::CaseInsensitive))
    {
      item1 -> setIcon(QIcon(":/images/message-mail-replied-read.png"));
    }
    else if (text.startsWith("Fwd:", Qt::CaseInsensitive))
    {
      item1 -> setIcon(QIcon(":/images/message-mail-forwarded-read.png"));
    }
    else
		{
      item1 -> setIcon(QIcon(":/images/message-mail-read.png"));
		}
		
		if (it -> msgflags & RS_MSG_NEW)
		{
			for(int i = 0; i < 10; i++)
			{
				/*QFont qf = item->font(i);
				qf.setBold(true);
				item->setFont(i, qf);*/
			}
			QString text = QString::fromStdWString(it->title);
			
      			if (text.startsWith("Re:", Qt::CaseInsensitive))
      			{
       				item1 -> setIcon(QIcon(":/images/message-mail-replied.png"));
      			}
      				else if (text.startsWith("Fwd:", Qt::CaseInsensitive))
      			{
        			item1 -> setIcon(QIcon(":/images/message-mail-forwarded.png"));
      			}
      			else
      			{
        			item1 -> setIcon(QIcon(":/images/message-mail.png"));
      			}
		}

		// No of Files.
		{
			std::ostringstream out;
			out << it -> count;
			item0 -> setText(QString::fromStdString(out.str()));
			//item -> setTextAlignment( 0, Qt::AlignCenter );
		}
	
		item4 -> setText(QString::fromStdString(it->srcId));
		item5 -> setText(QString::fromStdString(it->msgId));
		
		if ((oldSelected) && (mid == it->msgId))
		{
			//newSelected = item;
		}

		/* add to the list */
		QList<QStandardItem *> itemList;
		itemList.append(item0);
		itemList.append(item1);
		itemList.append(item2);
		itemList.append(item3);
		itemList.append(item4);
		itemList.append(item5);
		MessagesModel->appendRow(itemList);
	}
	
	updateMessageSummaryList();
   	ui.messagestreeView->hideColumn(4);
   	ui.messagestreeView->hideColumn(5);
}

void MessagesDialog::updateCurrentMessage(const QModelIndex &index )
{
	insertMsgTxtAndFiles(index);
	setMsgAsRead(index);
	updateMessageSummaryList();
}

void MessagesDialog::setMsgAsRead(const QModelIndex &index)
{
	QString text;
	QModelIndex currentIndex = proxyModel->mapToSource(index);
	if (currentIndex.isValid() == false)
	{
		return;
	}
	else
	{
		for(int i = 0; i < 6; i++)
		{

			QStandardItem* item;
			item = MessagesModel->item(currentIndex.row(),i);
			QFont qf = item->font();
			qf.setBold(false);
			item->setFont(qf);

			//change the icon to read. this need to be done when user clicks to the new message
			if(i == 1)
			{
			  text = item->text();
    			  if (text.startsWith("Re:", Qt::CaseInsensitive))
    			  {
      			    item -> setIcon(QIcon(":/images/message-mail-replied-read.png"));
    			  }
    			    else if (text.startsWith("Fwd:", Qt::CaseInsensitive))
    			  {
			    item -> setIcon(QIcon(":/images/message-mail-forwarded-read.png"));
    			  }
    			  else
			  {
			    item -> setIcon(QIcon(":/images/message-mail-read.png"));
			  }

			}
			if(i == 5)
			{
				std::string mid(item->text().toStdString());
				rsMsgs->MessageRead(mid);
			}
		}
	}
}

void MessagesDialog::insertMsgTxtAndFiles(QModelIndex Index)
{
	std::cerr << "MessagesDialog::insertMsgTxtAndFiles()" << std::endl;

	/* get its Ids */
	std::string cid;
	std::string mid;

	QModelIndex currentIndex = proxyModel->mapToSource(Index);
	if (currentIndex.isValid() == false)
	{
		/* blank it */
		ui.dateText-> setText("");
		ui.toText->setText("");
		ui.fromText->setText("");
		ui.filesText->setText("");

		ui.subjectText->setText("");
		ui.msgList->clear();
		ui.msgText->clear();

		return;
	}
	else
	{
		QStandardItem * item;
		item = MessagesModel->item(currentIndex.row(),4);
		cid = item->text().toStdString();
		fflush(0);
		
		item = MessagesModel->item(currentIndex.row(),5);
		mid = item->text().toStdString();
	}


	/* Save the Data.... for later */

	mCurrCertId = cid;
	mCurrMsgId  = mid;

	MessageInfo msgInfo;
	if (!rsMsgs -> getMessage(mid, msgInfo))
	{
		std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
		return;
	}

	const std::list<FileInfo> &recList = msgInfo.files;
	std::list<FileInfo>::const_iterator it;

	/* get a link to the table */
	QTreeWidget *tree = ui.msgList;

	/* get the MessageInfo */

	tree->clear(); 
	tree->setColumnCount(5); 

	QList<QTreeWidgetItem *> items;
	for(it = recList.begin(); it != recList.end(); it++)
	{
		/* make a widget per person */
		QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);
		/* (0) Filename */
		item -> setText(0, QString::fromStdString(it->fname));
		//std::cerr << "Msg FileItem(" << it->fname.length() << ") :" << it->fname << std::endl;

		item -> setText(1, QString::number(it->size)); /* (1) Size */
		item -> setText(2, QString::number(0)); 			/* (2) Rank */ // what is this ???
		item -> setText(3, QString::fromStdString(it->hash)); 

		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	tree->insertTopLevelItems(0, items);

	/* iterate through the sources */
	std::list<std::string>::const_iterator pit;

	QString msgTxt;
	for(pit = msgInfo.msgto.begin(); pit != msgInfo.msgto.end(); pit++)
	{
		msgTxt += QString::fromStdString(*pit);
		msgTxt += " <";
		msgTxt += QString::fromStdString(rsPeers->getPeerName(*pit));
		msgTxt += ">, ";
	}

	if (msgInfo.msgcc.size() > 0)
		msgTxt += "\nCc: ";
	for(pit = msgInfo.msgcc.begin(); pit != msgInfo.msgcc.end(); pit++)
	{
		msgTxt += QString::fromStdString(*pit);
		msgTxt += " <";
		msgTxt += QString::fromStdString(rsPeers->getPeerName(*pit));
		msgTxt += ">, ";
	}

	if (msgInfo.msgbcc.size() > 0)
		msgTxt += "\nBcc: ";
	for(pit = msgInfo.msgbcc.begin(); pit != msgInfo.msgbcc.end(); pit++)
	{
		msgTxt += QString::fromStdString(*pit);
		msgTxt += " <";
		msgTxt += QString::fromStdString(rsPeers->getPeerName(*pit));
		msgTxt += ">, ";
	}

	{
		QDateTime qtime;
		qtime.setTime_t(msgInfo.ts);
		QString timestamp = qtime.toString("dd.MM.yyyy hh:mm:ss");
		ui.dateText-> setText(timestamp);
	}
	ui.toText->setText(msgTxt);
	ui.fromText->setText("<a style='color: blue;' href='" + QString::fromStdString(rsPeers->getPeerName(msgInfo.srcId)) + "'> " + QString::fromStdString(rsPeers->getPeerName(msgInfo.srcId)) +"</a>");

	ui.subjectText->setText(QString::fromStdWString(msgInfo.title));
	ui.msgText->setHtml(QString::fromStdWString(msgInfo.msg));

	{
		std::ostringstream out;
		out << "(" << msgInfo.count << " Files)";
		ui.filesText->setText(QString::fromStdString(out.str()));
	}

	std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Msg Displayed OK!" << std::endl;
}

bool MessagesDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
	QModelIndex currentIndex = ui.messagestreeView->currentIndex();
	currentIndex = proxyModel->mapToSource(currentIndex);
	int rowSelected = -1;

	/* get its Ids */
	if (currentIndex.isValid() == false)
	{
		//If no message is selected. assume first message is selected.
		if(MessagesModel->rowCount() == 0)
		{
			return false;
		}
		else
		{
			rowSelected = 0;
		}
	}
	else
	{
		rowSelected = currentIndex.row();
	}

	QStandardItem *item;
	item = MessagesModel->item(rowSelected,4);
	cid = item->text().toStdString();
	
	item = MessagesModel->item(rowSelected,5);
	mid = item->text().toStdString();
	return true;
}

void MessagesDialog::removemessage()
{
	QList<QModelIndex> selectedIndexList= ui.messagestreeView->selectionModel() -> selectedIndexes ();
	QList<int> rowList;
	for(QList<QModelIndex>::iterator it = selectedIndexList.begin(); it != selectedIndexList.end(); it++)
	{
		int row = it->row();
		if (rowList.contains(row) == false)
		{
			rowList.append(row);
		}
	}

	for(QList<int>::const_iterator it1(rowList.begin());it1!=rowList.end();++it1)
	{
		rsMsgs->MessageDelete(MessagesModel->item((*it1),5)->text().toStdString());
	}

	insertMessages();
	return;
}

void MessagesDialog::print()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (ui.msgText->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted) {
        ui.msgText->print(&printer);
    }
    delete dlg;
#endif
}

void MessagesDialog::printpreview()
{
    PrintPreview *preview = new PrintPreview(ui.msgText->document(), this);
    preview->setWindowModality(Qt::WindowModal);
    preview->setAttribute(Qt::WA_DeleteOnClose);
    preview->show();
}

void MessagesDialog::anchorClicked (const QUrl& link ) 
{
    #ifdef FORUM_DEBUG
		    std::cerr << "MessagesDialog::anchorClicked link.scheme() : " << link.scheme().toStdString() << std::endl;
    #endif
    
	if (link.scheme() == "retroshare")
	{
		QStringList L = link.toString().split("|") ;

		std::string fileName = L.at(1).toStdString() ;
		uint64_t fileSize = L.at(2).toULongLong();
		std::string fileHash = L.at(3).toStdString() ;

#ifdef FORUM_DEBUG
		std::cerr << "MessagesDialog::anchorClicked FileRequest : fileName : " << fileName << ". fileHash : " << fileHash << ". fileSize : " << fileSize << std::endl;
#endif

		if (fileName != "" && fileHash != "")
		{
			std::list<std::string> srcIds;

			if(rsFiles->FileRequest(fileName, fileHash, fileSize, "", RS_FILE_HINTS_NETWORK_WIDE, srcIds))
			{
				QMessageBox mb(tr("File Request Confirmation"), tr("The file has been added to your download list."),QMessageBox::Information,QMessageBox::Ok,0,0);
				mb.setButtonText( QMessageBox::Ok, "OK" );
				mb.exec();
			}
			else
			{
				QMessageBox mb(tr("File Request canceled"), tr("The file has not been added to your download list, because you already have it."),QMessageBox::Information,QMessageBox::Ok,0,0);
				mb.setButtonText( QMessageBox::Ok, "OK" );
				mb.exec();
			}
		} 
		else 
		{
			QMessageBox mb(tr("File Request Error"), tr("The file link is malformed."),QMessageBox::Information,QMessageBox::Ok,0,0);
			mb.setButtonText( QMessageBox::Ok, "OK" );
			mb.exec();
		}
	} 
	else if (link.scheme() == "http") 
	{
		QDesktopServices::openUrl(link);
	} 
	else if (link.scheme() == "") 
	{
		//it's probably a web adress, let's add http:// at the beginning of the link
		QString newAddress = link.toString();
		newAddress.prepend("http://");
		QDesktopServices::openUrl(QUrl(newAddress));
	}
}

bool MessagesDialog::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.msgText->document()->toHtml("UTF-8");
    ui.msgText->document()->setModified(false);
    return true;
}

bool MessagesDialog::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();
}

void MessagesDialog::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgText->document()->setModified(false);

    setWindowModified(false);
}

void MessagesDialog::buttonsicononly()
{
    _settings->beginGroup("MessageDialog");
    
    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    
    _settings->setValue("ToolButon_Stlye1",ui.newmessageButton->toolButtonStyle());
    _settings->setValue("ToolButon_Stlye2",ui.removemessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye3",ui.replymessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye4",ui.replyallmessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye5",ui.forwardmessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye6",ui.printbutton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye7",ui.viewtoolButton->toolButtonStyle());   

    _settings->endGroup();
}

void MessagesDialog::buttonstextbesideicon()
{
    _settings->beginGroup("MessageDialog");
	
    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    _settings->setValue("ToolButon_Stlye1",ui.newmessageButton->toolButtonStyle());
    _settings->setValue("ToolButon_Stlye2",ui.removemessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye3",ui.replymessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye4",ui.replyallmessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye5",ui.forwardmessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye6",ui.printbutton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye7",ui.viewtoolButton->toolButtonStyle());   

    _settings->endGroup();

}

void MessagesDialog::buttonstextundericon()
{
    _settings->beginGroup("MessageDialog");

    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    _settings->setValue("ToolButon_Stlye1",ui.newmessageButton->toolButtonStyle());
    _settings->setValue("ToolButon_Stlye2",ui.removemessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye3",ui.replymessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye4",ui.replyallmessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye5",ui.forwardmessageButton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye6",ui.printbutton->toolButtonStyle());  
    _settings->setValue("ToolButon_Stlye7",ui.viewtoolButton->toolButtonStyle());   

    _settings->endGroup();
}

void MessagesDialog::loadToolButtonsettings()
{
    _settings->beginGroup("MessageDialog");

    if(_settings->value("ToolButon_Stlye1","0").toInt() == 0)
    {
    qDebug() << "ToolButon IconOnly";
    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    else if (_settings->value("ToolButon_Stlye1","2").toInt() ==2)
    {
    qDebug() << "ToolButon TextBesideIcon";
    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }

    else if(_settings->value("ToolButon_Stlye1","3").toInt() ==3)
    {
    qDebug() << "ToolButton TextUnderIcon";
    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    }

    _settings->endGroup();
}

void MessagesDialog::filterRegExpChanged()
{
    QRegExp regExp(ui.filterPatternLineEdit->text(),  Qt::CaseInsensitive , QRegExp::FixedString);
    proxyModel->setFilterRegExp(regExp);
}

void MessagesDialog::filterColumnChanged()
{
    proxyModel->setFilterKeyColumn(ui.filterColumnComboBox->currentIndex());
}

void MessagesDialog::updateMessageSummaryList()
{
	std::list<MsgInfoSummary> msgList;
	std::list<MsgInfoSummary>::const_iterator it;

	rsMsgs -> getMessageSummaries(msgList);
	int newInboxCount = 0;
	int newOutboxCount = 0;
	int newDraftCount = 0;

	/*calculating the new messages*/
	for(it = msgList.begin(); it != msgList.end(); it++)
	{
		if ((it -> msgflags & RS_MSG_BOXMASK) == RS_MSG_INBOX && ((it -> msgflags & RS_MSG_NEW) == RS_MSG_NEW))
		{
			newInboxCount ++;
		}
		if ((it -> msgflags & RS_MSG_BOXMASK) == RS_MSG_OUTBOX && ((it -> msgflags & RS_MSG_NEW) == RS_MSG_NEW))
		{
			newOutboxCount ++;
		}
		if ((it -> msgflags & RS_MSG_BOXMASK) == RS_MSG_DRAFTBOX )
		{
			newDraftCount ++;
		}
	}	

		QString textItem;
	/*updating the labels in leftcolumn*/
	if(newInboxCount != 0)
	{
		//QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
		QListWidgetItem* item = ui.listWidget->item(0);
		textItem = tr("Inbox") + "(" + QString::number(newInboxCount)+")";
		item->setText(textItem);
		QFont qf = item->font();
		qf.setBold(true);
		item->setFont(qf);
		item->setIcon(QIcon(":/images/folder-inbox-new.png"));
	}
	else
	{
		QListWidgetItem* item = ui.listWidget->item(0);

		textItem = tr("Inbox");
		item->setText(textItem);
		QFont qf = item->font();
		qf.setBold(false);
		item->setFont(qf);
   		item->setIcon(QIcon(":/images/folder-inbox.png"));	
	}
	
	if(newOutboxCount != 0)
	{
		//QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
		QListWidgetItem* item = ui.listWidget->item(1);

		textItem = tr("Outbox") + "(" + QString::number(newOutboxCount)+")";
		item->setText(textItem);
		QFont qf = item->font();
		qf.setBold(true);
		item->setFont(qf);
	}
	else
	{
		QListWidgetItem* item = ui.listWidget->item(1);

		textItem = tr("Outbox");
		item->setText(textItem);
		QFont qf = item->font();
		qf.setBold(false);
		item->setFont(qf);
		
	}	
	
	if(newDraftCount != 0)
	{
		//QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
		QListWidgetItem* item = ui.listWidget->item(2);

		textItem = tr("Draft") + "(" + QString::number(newDraftCount)+")";
		item->setText(textItem);
		QFont qf = item->font();
		qf.setBold(true);
		item->setFont(qf);
	}
	else
	{
		QListWidgetItem* item = ui.listWidget->item(2);

		textItem = tr("Draft");
		item->setText(textItem);
		QFont qf = item->font();
		qf.setBold(false);
		item->setFont(qf);
		
	}	
}

/** clear Filter **/
void MessagesDialog::clearFilter()
{
 	ui.filterPatternLineEdit->clear();
 	ui.filterPatternLineEdit->setFocus();
}

/* toggle clearButton */
void MessagesDialog::toggleclearButton()
{
    QString text = ui.filterPatternLineEdit->text();
    
    if (text.isEmpty())
    {
      ui.clearButton->hide();
    }
    else
    {
      ui.clearButton->show();
    }
    
}
