/****************************************************************
 *  RShare is distributed under the following license:
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

#include <set>

#include "rshare.h"
#include "SearchDialog.h"
#include "RetroShareLink.h"
#include "msgs/ChanMsgDialog.h"

#include "rsiface/rsiface.h"
#include "rsiface/rsexpr.h"
#include "rsiface/rsfiles.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsturtle.h"
#include "util/misc.h"

#include <iostream>
#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>

/* Images for context menu icons */
#define IMAGE_START  		    ":/images/download.png"
#define IMAGE_REMOVE  		  ":/images/delete.png"
#define IMAGE_REMOVEALL   	":/images/deleteall.png"
#define IMAGE_DIRECTORY			":/images/folder16.png"

/* Key for UI Preferences */
#define UI_PREF_ADVANCED_SEARCH  "UIOptions/AdvancedSearch"

/* indicies for search results item columns SR_ = Search Result */
/* indicies for search results item columns SR_ = Search Result */
#define SR_NAME_COL         0
#define SR_SIZE_COL         1
#define SR_ID_COL           2
#define SR_TYPE_COL         3
#define SR_AGE_COL          4
#define SR_HASH_COL         5

#define SR_SEARCH_ID_COL    6

#define SR_UID_COL          7
#define SR_REALSIZE_COL     8

/* indicies for search summary item columns SS_ = Search Summary */
#define SS_TEXT_COL         0
#define SS_COUNT_COL        1
#define SS_SEARCH_ID_COL    2

#define IMAGE_COPYLINK             ":/images/copyrslink.png"

/* static members */
/* These indices MUST be identical to their equivalent indices in the combobox */
const int SearchDialog::FILETYPE_IDX_ANY = 0;
const int SearchDialog::FILETYPE_IDX_ARCHIVE = 1;
const int SearchDialog::FILETYPE_IDX_AUDIO = 2;
const int SearchDialog::FILETYPE_IDX_CDIMAGE = 3;
const int SearchDialog::FILETYPE_IDX_DOCUMENT = 4;
const int SearchDialog::FILETYPE_IDX_PICTURE = 5;
const int SearchDialog::FILETYPE_IDX_PROGRAM = 6;
const int SearchDialog::FILETYPE_IDX_VIDEO = 7;
const int SearchDialog::FILETYPE_IDX_DIRECTORY = 8;
QMap<int, QString> * SearchDialog::FileTypeExtensionMap = new QMap<int, QString>();
bool SearchDialog::initialised = false;

/** Constructor */
SearchDialog::SearchDialog(QWidget *parent)
: MainPage(parent),
	advSearchDialog(NULL),
	contextMnu(NULL), contextMnu2(NULL),
	nextSearchId(1)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    ui.lineEdit->setFocus();
    ui.lineEdit->setToolTip(tr("Enter a keyword here (at least 3 char long)"));

    /* initialise the filetypes mapping */
    if (!SearchDialog::initialised)
    {
	initialiseFileTypeMappings();
    }

    RshareSettings rsharesettings;

    connect(ui.toggleAdvancedSearchBtn, SIGNAL(clicked()), this, SLOT(showAdvSearchDialog()));

    connect( ui.searchResultWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( searchtableWidgetCostumPopupMenu( QPoint ) ) );

    connect( ui.searchSummaryWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( searchtableWidget2CostumPopupMenu( QPoint ) ) );

    connect( ui.lineEdit, SIGNAL( returnPressed ( void ) ), this, SLOT( searchKeywords( void ) ) );
    connect( ui.lineEdit, SIGNAL( textChanged ( const QString& ) ), this, SLOT( checkText( const QString& ) ) );
    connect( ui.pushButtonsearch, SIGNAL( released ( void ) ), this, SLOT( searchKeywords( void ) ) );
    connect( ui.pushButtonDownload, SIGNAL( released ( void ) ), this, SLOT( download( void ) ) );
    connect( ui.cloaseallsearchresultsButton, SIGNAL(clicked()), this, SLOT(searchRemoveAll()));
    connect( ui.resetButton, SIGNAL(clicked()), this, SLOT(clearKeyword()));
    connect( ui.lineEdit, SIGNAL( textChanged(const QString &)), this, SLOT(togglereset()));

    //connect( ui.searchSummaryWidget, SIGNAL( itemSelectionChanged ( void ) ), this, SLOT( selectSearchResults( void ) ) );

    connect( ui.searchResultWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int)), this, SLOT(download()));

    connect ( ui.searchSummaryWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem * ) ),
                    this, SLOT( selectSearchResults( void ) ) );

    //connect(ui.FileTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboIndexChanged(int)));
    connect(ui.FileTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectSearchResults(int)));

    /* hide the Tree +/- */
    ui.searchResultWidget -> setRootIsDecorated( true );
    ui.searchResultWidget -> setColumnHidden( SR_UID_COL,true );
    ui.searchResultWidget -> setColumnHidden( SR_REALSIZE_COL,true );
    ui.searchSummaryWidget -> setRootIsDecorated( false );

    /* make it extended selection */
    ui.searchResultWidget -> setSelectionMode(QAbstractItemView::ExtendedSelection);


    /* Set header resize modes and initial section sizes */
    ui.searchSummaryWidget->setColumnCount(3);
    ui.searchSummaryWidget->setColumnHidden ( 2, true);

    QHeaderView * _smheader = ui.searchSummaryWidget->header () ;
    _smheader->setResizeMode (0, QHeaderView::Interactive);
    _smheader->setResizeMode (1, QHeaderView::Interactive);

    _smheader->resizeSection ( 0, 160 );
    _smheader->resizeSection ( 1, 50 );

    ui.searchResultWidget->setColumnCount(6);
    _smheader = ui.searchResultWidget->header () ;
    _smheader->setResizeMode (0, QHeaderView::Interactive);
    _smheader->setResizeMode (1, QHeaderView::Interactive);
    _smheader->setResizeMode (2, QHeaderView::Interactive);

    _smheader->resizeSection ( 0, 240 );
    _smheader->resizeSection ( 1, 75 );
    _smheader->resizeSection ( 2, 75 );
    _smheader->resizeSection ( 3, 75 );
    _smheader->resizeSection ( 4, 90 );
    _smheader->resizeSection ( 5, 240 );


    // set header text aligment
    QTreeWidgetItem * headerItem = ui.searchResultWidget->headerItem();
    headerItem->setTextAlignment(1, Qt::AlignRight   | Qt::AlignRight);
    headerItem->setTextAlignment(2, Qt::AlignRight | Qt::AlignRight);

    ui.searchResultWidget->sortItems(SR_NAME_COL, Qt::AscendingOrder);

    ui.resetButton->hide();
  
  	ui._ownFiles_CB->setMinimumWidth(20);
  	ui._friendListsearch_SB->setMinimumWidth(20);
  	ui._anonF2Fsearch_CB->setMinimumWidth(20);
  	ui.label->setMinimumWidth(20);

/* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void SearchDialog::checkText(const QString& txt)
{
	if(txt.length() < 3)
	{
		std::cout << "setting palette 1" << std::endl ;
		ui.frame_2->setStyleSheet("QFrame{ border: 2px solid #079E00; background-color: #DBDBDB; }");
	}
	else
	{
		ui.frame_2->setStyleSheet("QFrame { border: 2px solid #079E00; background-color: white; }");
		std::cout << "setting palette 2" << std::endl ;
	}
}

void SearchDialog::initialiseFileTypeMappings()
{
	/* edit these strings to change the range of extensions recognised by the search */
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_ANY, "");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_AUDIO,
		"aac aif iff m3u mid midi mp3 mpa ogg ra ram wav wma");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_ARCHIVE,
		"7z bz2 gz pkg rar sea sit sitx tar zip");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_CDIMAGE,
		"iso nrg mdf");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_DOCUMENT,
		"doc odt ott rtf pdf ps txt log msg wpd wps" );
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_PICTURE,
		"3dm 3dmf ai bmp drw dxf eps gif ico indd jpe jpeg jpg mng pcx pcc pct pgm "
		"pix png psd psp qxd qxprgb sgi svg tga tif tiff xbm xcf");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_PROGRAM,
		"app bat cgi com bin exe js pif py pl sh vb ws ");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_VIDEO,
		"3gp asf asx avi mov mp4 mkv flv mpeg mpg qt rm swf vob wmv");
	SearchDialog::initialised = true;
}

void SearchDialog::searchtableWidgetCostumPopupMenu( QPoint point )
{
      // block the popup if no results available
      if ((ui.searchResultWidget->selectedItems()).size() == 0) return;

      // create the menu as required
      if (contextMnu == 0)
      {
        contextMnu = new QMenu(this);

        downloadAct = new QAction(QIcon(IMAGE_START), tr( "Download" ), this );
        connect( downloadAct , SIGNAL( triggered() ), this, SLOT( download() ) );

        copysearchlinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy retroshare Link" ), this );
        connect( copysearchlinkAct , SIGNAL( triggered() ), this, SLOT( copysearchLink() ) );
        
        sendrslinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Send retroshare Link" ), this );
	      connect( sendrslinkAct , SIGNAL( triggered() ), this, SLOT( sendLinkTo( ) ) );

        broadcastonchannelAct = new QAction( tr( "Broadcast on Channel" ), this );
        connect( broadcastonchannelAct , SIGNAL( triggered() ), this, SLOT( broadcastonchannel() ) );

        recommendtofriendsAct = new QAction( tr( "Recommend to Friends" ), this );
        connect( recommendtofriendsAct , SIGNAL( triggered() ), this, SLOT( recommendtofriends() ) );


        contextMnu->clear();
        contextMnu->addAction( downloadAct);
        contextMnu->addSeparator();

		  if ((ui.searchResultWidget->selectedItems()).size() == 1) 
		  {
			  contextMnu->addAction( copysearchlinkAct);
			  contextMnu->addAction( sendrslinkAct);
		  }
      }

      QMouseEvent *mevent = new QMouseEvent(QEvent::MouseButtonPress,point,Qt::RightButton, Qt::RightButton,Qt::NoModifier);
      contextMnu->exec( mevent->globalPos() );
}

void SearchDialog::download()
{
    /* should also be able to handle multi-selection */
    QList<QTreeWidgetItem*> itemsForDownload = ui.searchResultWidget->selectedItems();
    int numdls = itemsForDownload.size();
    QTreeWidgetItem * item;
    bool attemptDownloadLocal = false;

    for (int i = 0; i < numdls; ++i) 
	 {
		 item = itemsForDownload.at(i);
		 // call the download
                 if (!item->text(SR_HASH_COL).isEmpty() || !item->childCount())
		 {
			 std::cerr << "SearchDialog::download() Calling File Request";
			 std::cerr << std::endl;
			 std::list<std::string> srcIds;
#ifdef SUSPENDED
			 // I suspend this. For turtle F2F download, we dont' need sources: 
			 // 	- if we put sources, they make double with some tunnels.
			 // 	- they won't transfer because ASSUME_AVAILABILITY can't be used,
			 // 		and no chunk maps are transfered except in tunnels.
			 srcIds.push_back(item->text(SR_UID_COL).toStdString()) ;
#endif

			 if(!rsFiles -> FileRequest((item->text(SR_NAME_COL)).toStdString(),
						 (item->text(SR_HASH_COL)).toStdString(),
						 (item->text(SR_REALSIZE_COL)).toInt(),
						 "", RS_FILE_HINTS_NETWORK_WIDE, srcIds))
				 attemptDownloadLocal = true ;
			 else
			 {
				 std::cout << "isuing file request from search dialog: -" << (item->text(SR_NAME_COL)).toStdString() << "-" << (item->text(SR_HASH_COL)).toStdString() << "-" << (item->text(SR_REALSIZE_COL)).toInt() << "-ids=" ;
				 for(std::list<std::string>::const_iterator it(srcIds.begin());it!=srcIds.end();++it)
					 std::cout << *it << "-" << std::endl ;
			 }
		 }
		 else // we have a folder
			 downloadDirectory(item, tr(""));
	 }
    if (attemptDownloadLocal)
    	QMessageBox::information(0, tr("Download Notice"), tr("Skipping Local Files"));
}

void SearchDialog::downloadDirectory(const QTreeWidgetItem *item, const QString &base)
{
	if (!item->childCount()) {
		std::list<std::string> srcIds;
#ifdef SUSPENDED
		srcIds.push_back(item->text(SR_UID_COL).toStdString());
#endif

		QString path = QString::fromStdString(rsFiles->getDownloadDirectory())
						+ tr("/") + base + tr("/");
		QString cleanPath = QDir::cleanPath(path);

		rsFiles->FileRequest(item->text(SR_NAME_COL).toStdString(),
				item->text(SR_HASH_COL).toStdString(),
				item->text(SR_REALSIZE_COL).toInt(),
				cleanPath.toStdString(),RS_FILE_HINTS_NETWORK_WIDE, srcIds);

		std::cout << "SearchDialog::downloadDirectory(): "\
				"issuing file request from search dialog: -"
			<< (item->text(SR_NAME_COL)).toStdString()
			<< "-" << (item->text(SR_HASH_COL)).toStdString()
			<< "-" << (item->text(SR_REALSIZE_COL)).toInt()
			<< "-ids=" ;
		for(std::list<std::string>::const_iterator it(srcIds.begin());
				it!=srcIds.end();++it)
			std::cout << *it << "-" << std::endl ;
	} else {
		QDir dwlDir(QString::fromStdString(rsFiles->getDownloadDirectory()));
		QString path;
		if (base == tr(""))
			path = item->text(SR_NAME_COL);
		else
			path = base + tr("/") + item->text(SR_NAME_COL);
		QString cleanPath = QDir::cleanPath(path);

		// create this folder in download path
		if (!dwlDir.mkpath(cleanPath)) {
			std::cerr << "SearchDialog::downloadDirectory() - can't create "
				<< cleanPath.toStdString() << " directory" << std::endl;
			return;
		}

		// recursive call for every child - file or folder
		for (int i = 0, cnt = item->childCount(); i < cnt; i++) {
			QTreeWidgetItem *child = item->child(i);
			downloadDirectory(child, path);
		}
	}
}

void SearchDialog::broadcastonchannel()
{

    QMessageBox::warning(0, tr("Sorry"), tr("This function is not yet implemented."));
}


void SearchDialog::recommendtofriends()
{
   QMessageBox::warning(0, tr("Sorry"), tr("This function is not yet implemented."));

}


/** context menu searchTablewidget2 **/
void SearchDialog::searchtableWidget2CostumPopupMenu( QPoint point )
{

    // block the popup if no results available
    if ((ui.searchSummaryWidget->selectedItems()).size() == 0) return;

    // create the menu as required
    if (contextMnu2 == 0)
    {
        contextMnu2 = new QMenu( this );

        searchRemoveAct = new QAction(QIcon(IMAGE_REMOVE), tr( "Remove" ), this );
        connect( searchRemoveAct , SIGNAL( triggered() ), this, SLOT( searchRemove() ) );

        searchRemoveAllAct = new QAction(QIcon(IMAGE_REMOVEALL), tr( "Remove All" ), this );
        connect( searchRemoveAllAct , SIGNAL( triggered() ), this, SLOT( searchRemoveAll() ) );

        contextMnu2->clear();
        contextMnu2->addAction( searchRemoveAct);
        contextMnu2->addAction( searchRemoveAllAct);
    }

    QMouseEvent *mevent2 = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );
    contextMnu2->exec( mevent2->globalPos() );
}

/** remove selected search result **/
void SearchDialog::searchRemove()
{
	/* get the current search id from the summary window */
        QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
	if (!ci)
		return;

        /* get the searchId text */
        QString searchId = ci->text(SS_SEARCH_ID_COL);

        std::cerr << "SearchDialog::searchRemove(): searchId: " << searchId.toStdString();
        std::cerr << std::endl;

        /* show only matching searchIds in main window */
        int items = ui.searchResultWidget->topLevelItemCount();
        for(int i = 0; i < items;)
        {
                /* get item */
                QTreeWidgetItem *ti = ui.searchResultWidget->topLevelItem(i);
                if (ti->text(SR_SEARCH_ID_COL) == searchId)
                {
			/* remove */
                        delete (ui.searchResultWidget->takeTopLevelItem(i));
			items--;
                }
                else
                {
			/* step to the next */
			i++;
                }
        }
	int sii = ui.searchSummaryWidget->indexOfTopLevelItem(ci);
	if (sii != -1)
	{
        	delete (ui.searchSummaryWidget->takeTopLevelItem(sii));
	}

        ui.searchResultWidget->update();
        ui.searchSummaryWidget->update();
}

/** remove all search results **/
void SearchDialog::searchRemoveAll()
{
	ui.searchResultWidget->clear();
	ui.searchSummaryWidget->clear();
	nextSearchId = 1;
}

/** clear keywords and ComboBox **/
void SearchDialog::clearKeyword()
{
 	ui.lineEdit->clear();
 	ui.FileTypeComboBox->setCurrentIndex(0);
 	ui.lineEdit->setFocus();
}


/* *****************************************************************
        Advanced search implementation
*******************************************************************/
// Event handlers for hide and show events
void SearchDialog::hideEvent(QHideEvent * event)
{
    showAdvSearchDialog(false);
    MainPage::hideEvent(event);
}

void SearchDialog::toggleAdvancedSearchDialog(bool toggled)
{
    // record the users preference for future reference
    RshareSettings rsharesettings;
    QString key (UI_PREF_ADVANCED_SEARCH);
    rsharesettings.setValue(key, QVariant(toggled));

    showAdvSearchDialog(toggled);
}

void SearchDialog::showAdvSearchDialog(bool show)
{
    // instantiate if about to show for the first time
    if (advSearchDialog == 0 && show)
    {
        advSearchDialog = new AdvancedSearchDialog();
        connect(advSearchDialog, SIGNAL(search(Expression*)),
                this, SLOT(advancedSearch(Expression*)));
    }
    if (show) {
        advSearchDialog->show();
        advSearchDialog->raise();
        advSearchDialog->setFocus();
    } else if (advSearchDialog != 0){
        advSearchDialog->hide();
    }
}

// Creates a new entry in the search summary, not to leave it blank whatever happens.
//
void SearchDialog::initSearchResult(const std::string& txt,qulonglong searchId)
{
	QString sid_hexa = QString::number(searchId,16) ;

	QTreeWidgetItem *item2 = new QTreeWidgetItem();
	item2->setText(SS_TEXT_COL, QString::fromStdString(txt));
	item2->setText(SS_COUNT_COL, QString::number(0));
	item2->setText(SS_SEARCH_ID_COL, sid_hexa);

	ui.searchSummaryWidget->addTopLevelItem(item2);
	ui.searchSummaryWidget->setCurrentItem(item2);
}

void SearchDialog::advancedSearch(Expression* expression)
{
        advSearchDialog->hide();

	/* call to core */
	std::list<DirDetails> results;

	// send a turtle search request
	LinearizedExpression e ;
	expression->linearize(e) ;

	TurtleRequestId req_id = rsTurtle->turtleSearch(e) ;

	// This will act before turtle results come to the interface, thanks to the signals scheduling policy.
	// The text "bool exp" should be replaced by an appropriate text describing the actual search.
	initSearchResult(std::string("bool exp"),req_id) ;

	rsFiles -> SearchBoolExp(expression, results, DIR_FLAGS_REMOTE | DIR_FLAGS_NETWORK_WIDE | DIR_FLAGS_BROWSABLE);

	/* abstraction to allow reusee of tree rendering code */
	resultsToTree((advSearchDialog->getSearchAsString()).toStdString(),req_id, results);

//	// debug stuff
//	Expression *expression2 = LinearizedExpression::toExpr(e) ;
//	results.clear() ;
//	rsFiles -> SearchBoolExp(expression2, results, DIR_FLAGS_REMOTE | DIR_FLAGS_NETWORK_WIDE | DIR_FLAGS_BROWSABLE);
//	resultsToTree((advSearchDialog->getSearchAsString()).toStdString(),req_id+1, results);
}



void SearchDialog::searchKeywords()
{
	QString qTxt = ui.lineEdit->text();
	std::string txt = qTxt.toStdString();

	if(txt.length() < 3)
		return ;

	std::cerr << "SearchDialog::searchKeywords() : " << txt << std::endl;

	TurtleRequestId req_id ;

	if(ui._anonF2Fsearch_CB->isChecked())
		req_id = rsTurtle->turtleSearch(txt) ;
	else
		req_id = (((uint32_t)rand()) << 16)^0x1e2fd5e4 + ((uint32_t)rand())^0x1b19acfe ; // generate a random 32 bits request id

	initSearchResult(txt,req_id) ;	// this will act before turtle results come to the interface, thanks to the signals scheduling policy.

	if(ui._friendListsearch_SB->isChecked() || ui._ownFiles_CB->isChecked())
	{
		/* extract keywords from lineEdit */
		QStringList qWords = qTxt.split(" ", QString::SkipEmptyParts);
		std::list<std::string> words;
		QStringListIterator qWordsIter(qWords);
		while (qWordsIter.hasNext())
		{
			words.push_back(qWordsIter.next().toStdString());
		}

		if (words.size() < 1)
		{
			/* ignore */
			return;
		}

		std::list<DirDetails> finalResults ;

		if(ui._friendListsearch_SB->isChecked())
		{
			std::list<DirDetails> initialResults;

			rsFiles -> SearchKeywords(words, initialResults, DIR_FLAGS_REMOTE) ;

			/* which extensions do we use? */
			DirDetails dd;

			for(std::list<DirDetails>::iterator resultsIter = initialResults.begin(); resultsIter != initialResults.end(); resultsIter ++)
			{
				dd = *resultsIter;
				finalResults.push_back(dd);
			}
		}

		if(ui._ownFiles_CB->isChecked())
		{
			std::list<DirDetails> initialResults;

			rsFiles -> SearchKeywords(words, initialResults, DIR_FLAGS_LOCAL | DIR_FLAGS_NETWORK_WIDE | DIR_FLAGS_BROWSABLE) ;

			/* which extensions do we use? */
			DirDetails dd;

			for(std::list<DirDetails>::iterator resultsIter = initialResults.begin(); resultsIter != initialResults.end(); resultsIter ++)
			{
				dd = *resultsIter;
				finalResults.push_back(dd);
			}
		}

		/* abstraction to allow reusee of tree rendering code */
		resultsToTree(txt,req_id, finalResults);
		ui.lineEdit->clear() ;
	}
}

void SearchDialog::updateFiles(qulonglong search_id,FileDetail file)
{
	/* which extensions do we use? */
	std::string txt = ui.lineEdit->text().toStdString();
#ifdef DEBUG
	std::cout << "Updating file detail:" << std::endl ;
	std::cout << "  size = " << file.size << std::endl ;
	std::cout << "  name = " << file.name << std::endl ;
	std::cout << "  s_id = " << search_id << std::endl ;
#endif

	if (ui.FileTypeComboBox->currentIndex() == FILETYPE_IDX_ANY)
		insertFile(txt,search_id,file);
	else
	{
		// amend the text description of the search
		txt += " (" + ui.FileTypeComboBox->currentText().toStdString() + ")";
		// collect the extensions to use
		QString extStr = SearchDialog::FileTypeExtensionMap->value(ui.FileTypeComboBox->currentIndex());
		QStringList extList = extStr.split(" ");

		// get this file's extension
		QString qName = QString::fromUtf8(file.name.c_str());
		int extIndex = qName.lastIndexOf(".");

		if (extIndex >= 0)
		{
			QString qExt = qName.mid(extIndex+1);

			if (qExt != "" )
				for (int i = 0; i < extList.size(); ++i)
					if (qExt.toUpper() == extList.at(i).toUpper())
						insertFile(txt,search_id,file);
		}
	}
}

void SearchDialog::insertDirectory(const std::string &txt, qulonglong searchId, const DirDetails &dir, QTreeWidgetItem *item)
{
	QString sid_hexa = QString::number(searchId,16) ;

	if (dir.type == DIR_TYPE_FILE)
	{
		QTreeWidgetItem *child;
		if (item == NULL) {
			child = new QTreeWidgetItem(ui.searchResultWidget);
		} else {
			child = new QTreeWidgetItem(item);
		}

		/* translate search result for a file */
		
		child->setText(SR_NAME_COL, QString::fromUtf8(dir.name.c_str()));
		child->setText(SR_HASH_COL, QString::fromStdString(dir.hash));
		QString ext = QFileInfo(QString::fromStdString(dir.name)).suffix();
		child->setText(SR_SIZE_COL, misc::friendlyUnit(dir.count));
		child->setText(SR_AGE_COL, misc::userFriendlyDuration(dir.age));
		child->setText(SR_REALSIZE_COL, QString::number(dir.count));
		child->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );
		
		child->setText(SR_ID_COL, QString::number(1));
		child->setTextAlignment( SR_ID_COL, Qt::AlignRight );

		child->setText(SR_SEARCH_ID_COL, sid_hexa);
		setIconAndType(child, ext);

		if (item == NULL) {
			ui.searchResultWidget->addTopLevelItem(child);
		} else {
			item->addChild(child);
		}
	}
	else
	{ /* it is a directory */
		QTreeWidgetItem *child;
		if (item == NULL) {
			child = new QTreeWidgetItem(ui.searchResultWidget);
		} else {
			child = new QTreeWidgetItem(item);
		}
    
		child->setIcon(SR_NAME_COL, QIcon(IMAGE_DIRECTORY));
		child->setText(SR_NAME_COL, QString::fromUtf8(dir.name.c_str()));
		child->setText(SR_HASH_COL, QString::fromStdString(dir.hash));
		child->setText(SR_SIZE_COL, misc::friendlyUnit(dir.count));
		child->setText(SR_AGE_COL, misc::userFriendlyDuration(dir.age));
		child->setText(SR_REALSIZE_COL, QString::number(dir.count));
		child->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );
		child->setText(SR_ID_COL, QString::number(1));
		child->setTextAlignment( SR_ID_COL, Qt::AlignRight );
		child->setText(SR_SEARCH_ID_COL, sid_hexa);
		child->setText(SR_TYPE_COL, tr("Folder"));


		if (item == NULL) {
			ui.searchResultWidget->addTopLevelItem(child);

			/* add to the summary as well */

			int items = ui.searchSummaryWidget->topLevelItemCount();
			bool found = false ;

			for(int i = 0; i < items; i++)
			{
				if(ui.searchSummaryWidget->topLevelItem(i)->text(SS_SEARCH_ID_COL) == sid_hexa)
				{
					// increment result since every item is new
					int s = ui.searchSummaryWidget->topLevelItem(i)->text(SS_COUNT_COL).toInt() ;
					ui.searchSummaryWidget->topLevelItem(i)->setText(SS_COUNT_COL,QString::number(s+1));
					found = true ;
				}
			}
			if(!found)
			{
				QTreeWidgetItem *item2 = new QTreeWidgetItem();
				item2->setText(SS_TEXT_COL, QString::fromStdString(txt));
				item2->setText(SS_COUNT_COL, QString::number(1));
				item2->setTextAlignment( SS_COUNT_COL, Qt::AlignRight );
				item2->setText(SS_SEARCH_ID_COL, sid_hexa);

				ui.searchSummaryWidget->addTopLevelItem(item2);
				ui.searchSummaryWidget->setCurrentItem(item2);
			}

			/* select this search result */
			selectSearchResults();
		} else {
			item->addChild(child);
		}

		/* go through all children directories/files for a recursive call */
		for (std::list<DirStub>::const_iterator it(dir.children.begin()); it != dir.children.end(); it ++) {
			DirDetails details;
			rsFiles->RequestDirDetails(it->ref, details, 0);
			insertDirectory(txt, searchId, details, child);
		}
	}
}

void SearchDialog::insertDirectory(const std::string &txt, qulonglong searchId, const DirDetails &dir)
{
    QString sid_hexa = QString::number(searchId,16) ;
    QTreeWidgetItem *child = new QTreeWidgetItem(ui.searchResultWidget);

    child->setIcon(SR_NAME_COL, QIcon(IMAGE_DIRECTORY));
    child->setText(SR_NAME_COL, QString::fromUtf8(dir.name.c_str()));
    child->setText(SR_HASH_COL, QString::fromStdString(dir.hash));
    child->setText(SR_SIZE_COL, misc::friendlyUnit(dir.count));
    child->setText(SR_AGE_COL, misc::userFriendlyDuration(dir.age));
    child->setText(SR_REALSIZE_COL, QString::number(dir.count));
    child->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );
    child->setText(SR_ID_COL, QString::number(1));
    child->setTextAlignment( SR_ID_COL, Qt::AlignRight );
    child->setText(SR_SEARCH_ID_COL, sid_hexa);
    child->setText(SR_TYPE_COL, tr("Folder"));

    ui.searchResultWidget->addTopLevelItem(child);

    /* add to the summary as well */

    int items = ui.searchSummaryWidget->topLevelItemCount();
    bool found = false ;

    for(int i = 0; i < items; i++)
    {
            if(ui.searchSummaryWidget->topLevelItem(i)->text(SS_SEARCH_ID_COL) == sid_hexa)
            {
                    // increment result since every item is new
                    int s = ui.searchSummaryWidget->topLevelItem(i)->text(SS_COUNT_COL).toInt() ;
                    ui.searchSummaryWidget->topLevelItem(i)->setText(SS_COUNT_COL,QString::number(s+1));
                    found = true ;
            }
    }
    if(!found)
    {
            QTreeWidgetItem *item2 = new QTreeWidgetItem();
            item2->setText(SS_TEXT_COL, QString::fromStdString(txt));
            item2->setText(SS_COUNT_COL, QString::number(1));
            item2->setTextAlignment( SS_COUNT_COL, Qt::AlignRight );
            item2->setText(SS_SEARCH_ID_COL, sid_hexa);

            ui.searchSummaryWidget->addTopLevelItem(item2);
            ui.searchSummaryWidget->setCurrentItem(item2);
    }

    selectSearchResults();
// TODO: check for duplicate directories
}

void SearchDialog::insertFile(const std::string& txt,qulonglong searchId, const FileDetail& file, int searchType)
{
	// algo:
	//
	// 1 - look in result window whether the file already exist.
	// 	1.1 - If yes, just increment the source number.
	// 	2.2 - Otherwize, add an entry.
	// 2 - look in the summary whether there exist the same request id.
	// 	1.1 - If yes, just increment the result number.
	// 	2.2 - Otherwize, add an entry.
	//
	//
	static std::map<qulonglong,uint32_t> nb_results ;

	if(nb_results.find(searchId) == nb_results.end())
		nb_results[searchId] = 0 ;

	if(nb_results[searchId] >= ui._max_results_SB->value())
		return ;
	else
		++nb_results[searchId] ;


	// 1 - look in result window whether the file already exists.
	//
	int items = ui.searchResultWidget->topLevelItemCount();
	bool found = false ;
	int sources;
	int friendSource = 0;
	int anonymousSource = 0;
	QString modifiedResult;

	QString sid_hexa = QString::number(searchId,16) ;

	for(int i = 0; i < items; i++)
		if(ui.searchResultWidget->topLevelItem(i)->text(SR_HASH_COL) == QString::fromStdString(file.hash)
				&& ui.searchResultWidget->topLevelItem(i)->text(SR_SEARCH_ID_COL) == sid_hexa)
		{
			QString resultCount = ui.searchResultWidget->topLevelItem(i)->text(SR_ID_COL);
			QStringList modifiedResultCount = resultCount.split("/", QString::SkipEmptyParts); 
			if(searchType == FRIEND_SEARCH)
			{
				friendSource = modifiedResultCount.at(0).toInt() + 1;
				anonymousSource = modifiedResultCount.at(1).toInt();
			}
			else
			{
				friendSource = modifiedResultCount.at(0).toInt();
				anonymousSource = modifiedResultCount.at(1).toInt() + 1;
			}
			anonymousSource = anonymousSource + friendSource;
			modifiedResult = QString::number(friendSource) + tr("/") + QString::number(anonymousSource);
			ui.searchResultWidget->topLevelItem(i)->setText(SR_ID_COL,modifiedResult);
			QTreeWidgetItem *item = ui.searchResultWidget->topLevelItem(i);
			found = true ;
			int sources = friendSource + anonymousSource ;
			if ( sources < 1)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 19)));
				}
			}
			else if ( sources < 2)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 38)));
				}
			}
			else if ( sources < 3)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 57)));
				}
			}	
			else if ( sources < 4)
			{
				for(int i = 0; i < 7; i++)
				{	
					item->setForeground(i,QBrush( QColor(0, 0, 76)));
				}
			}
			else if ( sources < 5)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 96)));
				}
			}
			else if ( sources < 6)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 114)));
				}
			}
			else if ( sources < 7)
			{	
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 133)));
				}
			}
			else if ( sources < 8)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 152)));
				}	
			}
			else if ( sources < 9)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 171)));
				}
			}
			else if ( sources < 10)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 190)));
				}
			}
			else if ( sources < 11)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 209)));
				}
			}
			else if ( sources < 12)
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 228)));
				}
			}
			else
			{
				for(int i = 0; i < 7; i++)
				{
					item->setForeground(i,QBrush( QColor(0, 0, 228)));
				}
			}
			break ;
		}

	if(!found)
	{
		/* translate search results */
		
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(SR_NAME_COL, QString::fromUtf8(file.name.c_str()));
		item->setText(SR_HASH_COL, QString::fromStdString(file.hash));

		QString ext = QFileInfo(QString::fromStdString(file.name)).suffix();
		setIconAndType(item, ext);

		/*
		 * to facilitate downlaods we need to save the file size too
		 */

		item->setText(SR_SIZE_COL, misc::friendlyUnit(file.size));
		item->setText(SR_REALSIZE_COL, QString::number(file.size));
		item->setText(SR_AGE_COL, misc::userFriendlyDuration(file.age));
		item->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );
		if(searchType == FRIEND_SEARCH)
		{
			friendSource = 1;
			anonymousSource = 0;
		}
		else
		{
			friendSource = 0;
			anonymousSource = 1;
		}

		anonymousSource = anonymousSource + friendSource;
		modifiedResult =QString::number(friendSource) + tr("/") + QString::number(anonymousSource);
		item->setText(SR_ID_COL,modifiedResult);
		item->setTextAlignment( SR_ID_COL, Qt::AlignRight );
		item->setText(SR_SEARCH_ID_COL, sid_hexa);
	
			
		sources = item->text(SR_ID_COL).toInt(); 
		if ( sources == 1)
		{
			for(int i = 0; i < 7; i++)
			{
				item->setForeground(i,QBrush( QColor(0, 0, 0)));
			}
		}

		ui.searchResultWidget->addTopLevelItem(item);
	}

	/* add to the summary as well */

	int items2 = ui.searchSummaryWidget->topLevelItemCount();
	bool found2 = false ;

	for(int i = 0; i < items2; i++)
		if(ui.searchSummaryWidget->topLevelItem(i)->text(SS_SEARCH_ID_COL) == sid_hexa)
		{
			if(!found)									// only increment result when it's a new item.
			{
				int s = ui.searchSummaryWidget->topLevelItem(i)->text(SS_COUNT_COL).toInt() ;
				ui.searchSummaryWidget->topLevelItem(i)->setText(SS_COUNT_COL,QString::number(s+1));
			}
			found2 = true ;
			break ;
		}

	if(!found2)
	{
		QTreeWidgetItem *item2 = new QTreeWidgetItem();
		item2->setText(SS_TEXT_COL, QString::fromStdString(txt));
		item2->setText(SS_COUNT_COL, QString::number(1));
		item2->setTextAlignment( SS_COUNT_COL, Qt::AlignRight );
		item2->setText(SS_SEARCH_ID_COL, sid_hexa);

		ui.searchSummaryWidget->addTopLevelItem(item2);
		ui.searchSummaryWidget->setCurrentItem(item2);
	}

	/* select this search result */
	selectSearchResults();
}

void SearchDialog::resultsToTree(std::string txt,qulonglong searchId, const std::list<DirDetails>& results)
{
	ui.searchResultWidget->setSortingEnabled(false);

	/* translate search results */
	std::ostringstream out;
	out << searchId;

	std::list<DirDetails>::const_iterator it;
	for(it = results.begin(); it != results.end(); it++)
		if (it->type == DIR_TYPE_FILE) {
			FileDetail fd;
			fd.id	= it->id;
			fd.name = it->name;
			fd.hash = it->hash;
			fd.path = it->path;
			fd.size = it->count;
			fd.age 	= it->age;
			fd.rank = 0;

			insertFile(txt,searchId,fd, FRIEND_SEARCH);
		} else if (it->type == DIR_TYPE_DIR) {
//                        insertDirectory(txt, searchId, *it, NULL);
                        insertDirectory(txt, searchId, *it);
		}

	ui.searchResultWidget->setSortingEnabled(true);
}

void SearchDialog::selectSearchResults(int index)
{
    int cindex = ui.FileTypeComboBox->currentIndex();
    index = (index == -1) ? (cindex == -1 ? 0 : cindex):index;
    QString alltypes = FileTypeExtensionMap->value(index);
    QStringList types = alltypes.split(" ");


	/* highlight this search in summary window */
	QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
	if (!ci)
		return;

	/* get the searchId text */
	QString searchId = ci->text(SS_SEARCH_ID_COL);
#ifdef DEBUG
	std::cerr << "SearchDialog::selectSearchResults(): searchId: " << searchId.toStdString();
	std::cerr << std::endl;
#endif

	/* show only matching searchIds in main window */
	int items = ui.searchResultWidget->topLevelItemCount();
	for(int i = 0; i < items; i++)
	{
		/* get item */
		QTreeWidgetItem *ti = ui.searchResultWidget->topLevelItem(i);
		if (ti->text(SR_SEARCH_ID_COL) == searchId)
                {
                    if (index == FILETYPE_IDX_ANY)
                        ti->setHidden(false);
                    else if (index == FILETYPE_IDX_DIRECTORY && ti->text(SR_HASH_COL).isEmpty())
                        ti->setHidden(false);
                    else if (types.contains(QFileInfo(ti->text(SR_NAME_COL)).suffix(), Qt::CaseInsensitive))
                        ti->setHidden(false);
                    else
                        ti->setHidden(true);
		}
		else
		{
			ti->setHidden(true);
		}
	}
	ui.searchResultWidget->update();
}

void SearchDialog::setIconAndType(QTreeWidgetItem *item, QString &ext)
{
	if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp" || ext == "ico" 
	|| ext == "svg" || ext == "tif" || ext == "tiff" || ext == "JPG")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypePicture.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Picture"));
	}
	else if (ext == "avi" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "mkv" || ext == "mp4" 
	|| ext == "flv" || ext == "mov" || ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp" 
	|| ext == "dvx" || ext == "divx")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeVideo.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Video"));
	}
	else if (ext == "ogg" || ext == "mp3" || ext == "MP3"  || ext == "mp1" || ext == "mp2"  || ext == "wav" || ext == "wma")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeAudio.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Audio"));
	}
	else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "tgz" || ext == "gz" || ext == "rar"
	 || ext == "rpm" || ext == "7z" || ext == "ace" || ext == "jar" || ext == "cab" || ext == "deb")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeArchive.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Archive"));
	}
	else if (ext == "app" || ext == "bat" || ext == "cgi" || ext == "com" || ext == "bin" || ext == "exe" || ext == "js" 
	|| ext == "msi" ||ext == "pif" || ext == "py" || ext == "pl" || ext == "sh" || ext == "vb" || ext == "ws")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeProgram.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Program"));
	}
	else if (ext == "iso" || ext == "nrg" || ext == "mdf" || ext == "img" || ext == "dmg" || ext == "bin" || ext == "uif" )
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeCDImage.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("CD-Image"));
	}
	else if (ext == "txt" || ext == "cpp" || ext == "c" || ext == "h")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeDocument.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Document"));
	}
	else if (ext == "pdf" )
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/mimetypes/pdf.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Document"));
	}
  else if (ext == "doc" || ext == "rtf" || ext == "sxw" || ext == "xls" || ext == "pps" || ext == "xml" || ext == "nfo" 
  || ext == "reg" || ext == "sxc" || ext == "odt" || ext == "ods" || ext == "dot" || ext == "ppt" || ext == "css" || ext == "crt" )
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeDocument.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Document"));
	}
	else if (ext == "html" || ext == "htm" || ext == "php")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeDocument.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Document"));
	}
	else if (ext == "sub" || ext == "srt")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeAny.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Subtitles"));
	}
	else if (ext == "nds")
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeAny.png"));
		item->setText(SR_TYPE_COL, QString::fromUtf8("Nintendo DS Rom"));
	}
	else
	{
		item->setIcon(SR_NAME_COL, QIcon(":/images/FileTypeAny.png"));
	}
}

void SearchDialog::copysearchLink()
{
    /* should also be able to handle multi-selection */
    QList<QTreeWidgetItem*> itemsForCopy = ui.searchResultWidget->selectedItems();
    int numdls = itemsForCopy.size();
    QTreeWidgetItem * item;

    for (int i = 0; i < numdls; ++i) 
    {
        item = itemsForCopy.at(i);
        // call copy

      if (!item->childCount()) 
		{
			std::cerr << "SearchDialog::copysearchLink() Calling set retroshare link";
			std::cerr << std::endl;

			QString fhash = item->text(SR_HASH_COL);
			qulonglong fsize = item->text(SR_REALSIZE_COL).toULongLong();
			QString fname = item->text(SR_NAME_COL);

			RetroShareLink link(fname, fsize, fhash);

			QApplication::clipboard()->setText(link.toString());
			break ;
		} 
    }
}

void SearchDialog::sendLinkTo( )
{
    copysearchLink();

    /* create a message */
    ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

    nMsgDialog->newMsg();
    nMsgDialog->insertTitleText("New RetroShare Link(s)");
    nMsgDialog->insertHtmlText(QApplication::clipboard()->text().toStdString());

    nMsgDialog->show();
}

void SearchDialog::togglereset()
{
    QString text = ui.lineEdit->text();
    
    if (text.isEmpty())
    {
      ui.resetButton->hide();
    }
    else
    {
      ui.resetButton->show();
    }
    
}

// not in use for the moment
void SearchDialog::onComboIndexChanged(int index)
{
    if (!FileTypeExtensionMap->contains(index) && index != FILETYPE_IDX_DIRECTORY)
        return;
    QString alltypes = FileTypeExtensionMap->value(index);
    QStringList types = alltypes.split(" ");
    int items = ui.searchResultWidget->topLevelItemCount();
    for (int i = 0; i < items; i++) {
        QTreeWidgetItem *ti = ui.searchResultWidget->topLevelItem(i);
        QString name = ti->text(SR_NAME_COL);

        if (index == FILETYPE_IDX_ANY) {
            if (ti->isHidden()) {
                QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
                if (!ci) {
                    ti->setHidden(false);
                    continue;
                }
                if (ti->text(SR_SEARCH_ID_COL) == ci->text(SS_SEARCH_ID_COL)) {
                    ti->setHidden(false);
                }
            }
        } else if (index == FILETYPE_IDX_DIRECTORY) {
            if (ti->text(SR_HASH_COL).isEmpty()) {
                if (ti->isHidden()) {
                    QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
                    if (!ci) {
                        ti->setHidden(false);
                        continue;
                    }
                    if (ti->text(SR_SEARCH_ID_COL) == ci->text(SS_SEARCH_ID_COL)) {
                        ti->setHidden(false);
                    }
                }
            } else {
                ti->setHidden(true);
            }
        } else {
            if (name.lastIndexOf(".") >= 0) {
                QString ext = name.mid(name.lastIndexOf(".") + 1);
                if (!ext.isEmpty() && types.contains(ext, Qt::CaseInsensitive)) {
                    if (ti->isHidden()) {
                        QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
                        if (!ci) {
                            ti->setHidden(false);
                            continue;
                        }
                        if (ti->text(SR_SEARCH_ID_COL) == ci->text(SS_SEARCH_ID_COL)) {
                            ti->setHidden(false);
                        }
                    }
                } else {
                    ti->setHidden(true);
                }
            } else {
                ti->setHidden(true);
            }
        }
    }
}
