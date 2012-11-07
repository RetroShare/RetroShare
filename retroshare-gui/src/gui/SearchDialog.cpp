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

#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include <QShortcut>

#include "rshare.h"
#include "SearchDialog.h"
#include "RetroShareLink.h"
#include "msgs/MessageComposer.h"
#include "gui/RSHumanReadableDelegate.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/common/RsCollectionFile.h"
#include "gui/common/FilesDefs.h"
#include "settings/rsharesettings.h"
#include "advsearch/advancedsearchdialog.h"
#include "common/RSTreeWidgetItem.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rsexpr.h>

/* Images for context menu icons */
#define IMAGE_START  		    ":/images/download.png"
#define IMAGE_REMOVE  		  ":/images/delete.png"
#define IMAGE_REMOVEALL   	":/images/deleteall.png"
#define IMAGE_DIRECTORY			":/images/folder16.png"

/* Key for UI Preferences */
#define UI_PREF_ADVANCED_SEARCH  "UIOptions/AdvancedSearch"

/* indicies for search summary item columns SS_ = Search Summary */
#define SS_TEXT_COL         0
#define SS_COUNT_COL        1
#define SS_SEARCH_ID_COL    2
#define SS_FILE_TYPE_COL    3
#define SS_DATA_COL         SS_TEXT_COL

#define ROLE_KEYWORDS       Qt::UserRole
#define ROLE_SORT           Qt::UserRole + 1

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
	nextSearchId(1)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_bProcessSettings = false;

	 _queueIsAlreadyTakenCareOf = false ;
    ui.lineEdit->setFocus();

    /* initialise the filetypes mapping */
    if (!SearchDialog::initialised)
    {
	initialiseFileTypeMappings();
    }

    connect(ui.toggleAdvancedSearchBtn, SIGNAL(clicked()), this, SLOT(showAdvSearchDialog()));

    connect( ui.searchResultWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( searchtableWidgetCostumPopupMenu( QPoint ) ) );

    connect( ui.searchSummaryWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( searchtableWidget2CostumPopupMenu( QPoint ) ) );

    connect( ui.lineEdit, SIGNAL( returnPressed ( void ) ), this, SLOT( searchKeywords( void ) ) );
    connect( ui.lineEdit, SIGNAL( textChanged ( const QString& ) ), this, SLOT( checkText( const QString& ) ) );
    connect( ui.pushButtonSearch, SIGNAL( released ( void ) ), this, SLOT( searchKeywords( void ) ) );
    connect( ui.pushButtonDownload, SIGNAL( released ( void ) ), this, SLOT( download( void ) ) );
    connect( ui.cloaseallsearchresultsButton, SIGNAL(clicked()), this, SLOT(searchRemoveAll()));

    connect( ui.searchResultWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int)), this, SLOT(download()));

    connect ( ui.searchSummaryWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem * ) ),
                    this, SLOT( selectSearchResults( void ) ) );

    connect(ui.FileTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFileType(int)));
    
    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems()));
    connect(ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterItems()));

    compareSummaryRole = new RSTreeWidgetItemCompareRole;
    compareSummaryRole->setRole(SS_COUNT_COL, ROLE_SORT);

    compareResultRole = new RSTreeWidgetItemCompareRole;
    compareResultRole->setRole(SR_SIZE_COL, ROLE_SORT);
    compareResultRole->setRole(SR_AGE_COL, ROLE_SORT);

    /* hide the Tree +/- */
    ui.searchResultWidget -> setRootIsDecorated( true );
    ui.searchResultWidget -> setColumnHidden( SR_UID_COL,true );
    ui.searchSummaryWidget -> setRootIsDecorated( false );

	 // We set some delegates to handle the display of size and date.
	 // To allow a proper sorting, be careful to pad at right with spaces. This
	 // is achieved by using QString("%1").arg(number,15,10).
	 //
	 ui.searchResultWidget->setItemDelegateForColumn(SR_SIZE_COL,new RSHumanReadableSizeDelegate()) ;
	 ui.searchResultWidget->setItemDelegateForColumn(SR_AGE_COL,new RSHumanReadableAgeDelegate()) ;

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

    /* Set initial size the splitter */
    QList<int> sizes;
    sizes << 250 << width(); // Qt calculates the right sizes
    ui.splitter->setSizes(sizes);

    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("File Name"), SR_NAME_COL);
//    ui.filterLineEdit->addFilter(QIcon(), tr("File Size"), SR_SIZE_COL);
    ui.filterLineEdit->setCurrentFilter(SR_NAME_COL);

    // load settings
    processSettings(true);
  
  	ui._ownFiles_CB->setMinimumWidth(20);
  	ui._friendListsearch_SB->setMinimumWidth(20);
  	ui._anonF2Fsearch_CB->setMinimumWidth(20);
  	ui.label->setMinimumWidth(20);

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.searchSummaryWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT(searchRemove()));

    checkText(ui.lineEdit->text());

/* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

SearchDialog::~SearchDialog()
{
    // save settings
    processSettings(false);

    if (compareSummaryRole) {
        delete(compareSummaryRole);
    }
    if (compareResultRole) {
        delete(compareResultRole);
    }
}

void SearchDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    QHeaderView *pHeader = ui.searchSummaryWidget->header () ;

    Settings->beginGroup(QString("SearchDialog"));

    if (bLoad) {
        // load settings

        // state of SearchSummary tree
        pHeader->restoreState(Settings->value("SearchSummaryTree").toByteArray());

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of SearchSummary tree
        Settings->setValue("SearchSummaryTree", pHeader->saveState());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
    }

    Settings->endGroup();
    m_bProcessSettings = false;
}


void SearchDialog::checkText(const QString& txt)
{
	bool valid;
	QColor color;

	if(txt.length() < 3)
	{
		std::cout << "setting palette 1" << std::endl ;
		valid = false;
		color = QApplication::palette().color(QPalette::Disabled, QPalette::Base);
	}
	else
	{
		std::cout << "setting palette 2" << std::endl ;
		valid = true;
		color = QApplication::palette().color(QPalette::Active, QPalette::Base);
	}

	/* unpolish widget to clear the stylesheet's palette cache */
	ui.searchLineFrame->style()->unpolish(ui.searchLineFrame);

	QPalette palette = ui.lineEdit->palette();
	palette.setColor(ui.lineEdit->backgroundRole(), color);
	ui.lineEdit->setPalette(palette);

	ui.searchLineFrame->setProperty("valid", valid);
	Rshare::refreshStyleSheet(ui.searchLineFrame, false);
}

void SearchDialog::initialiseFileTypeMappings()
{
	/* edit these strings to change the range of extensions recognised by the search */
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_ANY, "");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_AUDIO,
		"aac aif flac iff m3u m4a mid midi mp3 mpa ogg ra ram wav wma");
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

void SearchDialog::searchtableWidgetCostumPopupMenu( QPoint /*point*/ )
{
    // block the popup if no results available
    if ((ui.searchResultWidget->selectedItems()).size() == 0) return;

    QMenu contextMnu(this);

    contextMnu.addAction(QIcon(IMAGE_START), tr("Download"), this, SLOT(download()));
    contextMnu.addSeparator();
    contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyResultLink()));
    contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Send RetroShare Link"), this, SLOT(sendLinkTo()));

//    contextMnu.addAction(tr("Broadcast on Channel"), this, SLOT(broadcastonchannel()));
//    contextMnu.addAction(tr("Recommend to Friends"), this, SLOT(recommendtofriends()));

    contextMnu.exec(QCursor::pos());
}

void SearchDialog::getSourceFriendsForHash(const std::string& hash,std::list<std::string>& srcIds)
{
	std::cerr << "Searching sources for file " << hash << std::endl ;
	srcIds.clear();

	FileInfo finfo ;
	rsFiles->FileDetails(hash, RS_FILE_HINTS_REMOTE,finfo) ;

	for(std::list<TransferInfo>::const_iterator it(finfo.peers.begin());it!=finfo.peers.end();++it)
	{
		std::cerr << "  adding peerid " << (*it).peerId << std::endl ;
		srcIds.push_back((*it).peerId) ;
	}
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
		 //
		 if (item->text(SR_HASH_COL).isEmpty()) // we have a folder
			 downloadDirectory(item, tr(""));
		 else 
		 {
			 std::cerr << "SearchDialog::download() Calling File Request";
			 std::cerr << std::endl;
			 std::list<std::string> srcIds;

			 std::string hash = item->text(SR_HASH_COL).toStdString();
			 getSourceFriendsForHash(hash,srcIds) ;

			 if(!rsFiles -> FileRequest((item->text(SR_NAME_COL)).toUtf8().constData(), hash, (item->text(SR_SIZE_COL)).toULongLong(), "", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds))
				 attemptDownloadLocal = true ;
			 else
			 {
				 std::cout << "isuing file request from search dialog: -" << (item->text(SR_NAME_COL)).toStdString() << "-" << hash << "-" << (item->text(SR_SIZE_COL)).toULongLong() << "-ids=" ;
				 for(std::list<std::string>::const_iterator it(srcIds.begin());it!=srcIds.end();++it)
					 std::cout << *it << "-" << std::endl ;
			 }
		 }
	 }
    if (attemptDownloadLocal)
    	QMessageBox::information(this, tr("Download Notice"), tr("Skipping Local Files"));
}

void SearchDialog::downloadDirectory(const QTreeWidgetItem *item, const QString &base)
{
	if (!item->childCount()) 
	{
		std::list<std::string> srcIds;

		QString path = QString::fromStdString(rsFiles->getDownloadDirectory())
                                                + "/" + base + "/";
		QString cleanPath = QDir::cleanPath(path);

		std::string hash = item->text(SR_HASH_COL).toStdString();

		getSourceFriendsForHash(hash,srcIds) ;

		rsFiles->FileRequest(item->text(SR_NAME_COL).toUtf8().constData(),
				hash,
				item->text(SR_SIZE_COL).toULongLong(),
				cleanPath.toUtf8().constData(),RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds);

		std::cout << "SearchDialog::downloadDirectory(): "\
				"issuing file request from search dialog: -"
			<< (item->text(SR_NAME_COL)).toStdString()
			<< "-" << hash
			<< "-" << (item->text(SR_SIZE_COL)).toULongLong()
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
                        path = base + "/" + item->text(SR_NAME_COL);
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
void SearchDialog::searchtableWidget2CostumPopupMenu( QPoint /*point*/ )
{
    // block the popup if no results available
    if ((ui.searchSummaryWidget->selectedItems()).size() == 0) return;

    QMenu contextMnu(this);

    QTreeWidgetItem* ci = ui.searchSummaryWidget->currentItem();
    QAction* action = contextMnu.addAction(tr("Search again"), this, SLOT(searchAgain()));
    if (!ci || ci->data(SS_DATA_COL, ROLE_KEYWORDS).toString().isEmpty()) {
        action->setDisabled(true);
    }
    contextMnu.addAction(QIcon(IMAGE_REMOVE), tr("Remove"), this, SLOT(searchRemove()));
    contextMnu.addAction(QIcon(IMAGE_REMOVE), tr("Remove All"), this, SLOT(searchRemoveAll()));
    contextMnu.addSeparator();
    action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copySearchLink()));
    if (!ci || ci->data(SS_DATA_COL, ROLE_KEYWORDS).toString().isEmpty()) {
        action->setDisabled(true);
    }

    contextMnu.exec(QCursor::pos());
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
	ui.FileTypeComboBox->setCurrentIndex(0);
	nextSearchId = 1;
}

void SearchDialog::copySearchLink()
{
    /* get the current search id from the summary window */
    QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
    if (!ci)
        return;

    /* get the keywords */
    QString keywords = ci->text(SS_TEXT_COL);

    std::cerr << "SearchDialog::copySearchLink(): keywords: " << keywords.toStdString();
    std::cerr << std::endl;

    RetroShareLink link;
    if (link.createSearch(keywords)) {
        QList<RetroShareLink> urls;
        urls.push_back(link);
        RSLinkClipboard::copyLinks(urls);
    }
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
    QString key (UI_PREF_ADVANCED_SEARCH);
    Settings->setValue(key, QVariant(toggled));

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
void SearchDialog::initSearchResult(const QString& txt, qulonglong searchId, int fileType, bool advanced)
{
	QString sid_hexa = QString::number(searchId,16) ;

	QTreeWidgetItem *item2 = new RSTreeWidgetItem(compareSummaryRole);
	if (fileType == FILETYPE_IDX_ANY) {
		item2->setText(SS_TEXT_COL, txt);
	} else {
		item2->setText(SS_TEXT_COL, txt + " (" + ui.FileTypeComboBox->itemText(fileType) + ")");
	}
	item2->setText(SS_COUNT_COL, QString::number(0));
	item2->setData(SS_COUNT_COL, ROLE_SORT, 0);
	item2->setText(SS_SEARCH_ID_COL, sid_hexa);
	item2->setText(SS_FILE_TYPE_COL, QString::number(fileType));

	if (!advanced)
		item2->setData(SS_DATA_COL, ROLE_KEYWORDS, txt);

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
	initSearchResult("bool exp",req_id, ui.FileTypeComboBox->currentIndex(), true) ;

	rsFiles -> SearchBoolExp(expression, results, RS_FILE_HINTS_REMOTE);// | DIR_FLAGS_NETWORK_WIDE | DIR_FLAGS_BROWSABLE);

	/* abstraction to allow reusee of tree rendering code */
	resultsToTree(advSearchDialog->getSearchAsString(),req_id, results);

//	// debug stuff
//	Expression *expression2 = LinearizedExpression::toExpr(e) ;
//	results.clear() ;
//	rsFiles -> SearchBoolExp(expression2, results, DIR_FLAGS_REMOTE | DIR_FLAGS_NETWORK_WIDE | DIR_FLAGS_BROWSABLE);
//	resultsToTree((advSearchDialog->getSearchAsString()).toStdString(),req_id+1, results);
}

void SearchDialog::searchKeywords()
{
	searchKeywords(ui.lineEdit->text());
}

void SearchDialog::searchAgain()
{
	/* get the current search text from the summary window */
	QTreeWidgetItem* ci = ui.searchSummaryWidget->currentItem();
	if (!ci || ci->data(SS_DATA_COL, ROLE_KEYWORDS).toString().isEmpty())
		return;

	/* get the search text */
	QString txt = ci->data(SS_DATA_COL, ROLE_KEYWORDS).toString();
	int fileType = ci->text(SS_FILE_TYPE_COL).toInt();

	/* remove the old search */
	searchRemove();

	/* search for the same keywords and filetype again */
	ui.FileTypeComboBox->setCurrentIndex(fileType);
	searchKeywords(txt);
}

void SearchDialog::searchKeywords(const QString& keywords)
{
	if (keywords.length() < 3)
		return ;

	QStringList qWords = keywords.split(" ", QString::SkipEmptyParts);
	std::list<std::string> words;
	QStringListIterator qWordsIter(qWords);
	while (qWordsIter.hasNext())
		words.push_back(qWordsIter.next().toUtf8().constData());

	int n = words.size() ;

	if (n < 1)
		return;

	NameExpression exprs(ContainsAllStrings,words,true) ;
	LinearizedExpression lin_exp ;
	exprs.linearize(lin_exp) ;

	TurtleRequestId req_id ;

	if(ui._anonF2Fsearch_CB->isChecked())
	{
		if(n==1)
			req_id = rsTurtle->turtleSearch(words.front()) ;
		else
			req_id = rsTurtle->turtleSearch(lin_exp) ;
	}
	else
		req_id = ((((uint32_t)rand()) << 16)^0x1e2fd5e4) + (((uint32_t)rand())^0x1b19acfe) ; // generate a random 32 bits request id

	initSearchResult(keywords,req_id, ui.FileTypeComboBox->currentIndex(), false) ;	// this will act before turtle results come to the interface, thanks to the signals scheduling policy.

	if(ui._friendListsearch_SB->isChecked() || ui._ownFiles_CB->isChecked())
	{
		/* extract keywords from lineEdit */
		// make a compound expression with an AND
		//

		std::list<DirDetails> finalResults ;

		if(ui._friendListsearch_SB->isChecked())
		{
			std::list<DirDetails> initialResults;

			rsFiles->SearchBoolExp(&exprs, initialResults, RS_FILE_HINTS_REMOTE) ;

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

			rsFiles->SearchBoolExp(&exprs, initialResults, RS_FILE_HINTS_LOCAL);// | DIR_FLAGS_NETWORK_WIDE | DIR_FLAGS_BROWSABLE) ;

			/* which extensions do we use? */
			DirDetails dd;

			for(std::list<DirDetails>::iterator resultsIter = initialResults.begin(); resultsIter != initialResults.end(); resultsIter ++)
			{
				dd = *resultsIter;
				finalResults.push_back(dd);
			}
		}

		/* abstraction to allow reusee of tree rendering code */
		resultsToTree(keywords,req_id, finalResults);
		ui.lineEdit->clear() ;
	}
}

void SearchDialog::updateFiles(qulonglong search_id,FileDetail file)
{
	searchResultsQueue.push_back(std::pair<qulonglong,FileDetail>(search_id,file)) ;

	if(!_queueIsAlreadyTakenCareOf)
	{
		QTimer::singleShot(100,this,SLOT(processResultQueue())) ; 
		_queueIsAlreadyTakenCareOf = true ;
	}
}

void SearchDialog::processResultQueue()
{
	// This avoids a deadlock when gpg callback asks a passwd.
	// Send again in 10 secs.
	//
	if(RsAutoUpdatePage::eventsLocked())
	{
		QTimer::singleShot(10000,this,SLOT(processResultQueue())) ; 
		return ;
	}

	int nb_treated_elements = 0 ;

	ui.searchResultWidget->setSortingEnabled(false);
	while(!searchResultsQueue.empty() && nb_treated_elements++ < 250)
	{
		qulonglong search_id = searchResultsQueue.back().first ;
		FileDetail file = searchResultsQueue.back().second ;

		searchResultsQueue.pop_back() ;

#ifdef DEBUG
		std::cout << "Updating file detail:" << std::endl ;
		std::cout << "  size = " << file.size << std::endl ;
		std::cout << "  name = " << file.name << std::endl ;
		std::cout << "  s_id = " << search_id << std::endl ;
#endif

		insertFile(search_id,file);
	}
	ui.searchResultWidget->setSortingEnabled(true);
	if(!searchResultsQueue.empty())
		QTimer::singleShot(500,this,SLOT(processResultQueue())) ;
	else
		_queueIsAlreadyTakenCareOf = false ;
}

void SearchDialog::insertDirectory(const QString &txt, qulonglong searchId, const DirDetails &dir, QTreeWidgetItem *item)
{
	QString sid_hexa = QString::number(searchId,16) ;

	if (dir.type == DIR_TYPE_FILE)
	{
		QTreeWidgetItem *child = new RSTreeWidgetItem(compareResultRole);

		/* translate search result for a file */
		
		child->setText(SR_NAME_COL, QString::fromUtf8(dir.name.c_str()));
		child->setText(SR_HASH_COL, QString::fromStdString(dir.hash));
		child->setText(SR_SIZE_COL, QString::number(dir.count));
		child->setData(SR_SIZE_COL, ROLE_SORT, (qulonglong) dir.count);
		child->setText(SR_AGE_COL, QString::number(dir.age));
		child->setData(SR_AGE_COL, ROLE_SORT, dir.age);
		child->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );
		
		child->setText(SR_ID_COL, QString::number(1));
		child->setTextAlignment( SR_ID_COL, Qt::AlignRight );

		child->setText(SR_SEARCH_ID_COL, sid_hexa);
		setIconAndType(child, QString::fromUtf8(dir.name.c_str()));

		if (item == NULL) {
			ui.searchResultWidget->addTopLevelItem(child);
		} else {
			item->addChild(child);
		}
	}
	else
	{ /* it is a directory */
		QTreeWidgetItem *child = new RSTreeWidgetItem(compareResultRole);

		child->setIcon(SR_NAME_COL, QIcon(IMAGE_DIRECTORY));
		child->setText(SR_NAME_COL, QString::fromUtf8(dir.name.c_str()));
		child->setText(SR_HASH_COL, QString::fromStdString(dir.hash));
		child->setText(SR_SIZE_COL, QString::number(dir.count));
		child->setData(SR_SIZE_COL, ROLE_SORT, (qulonglong) dir.count);
		child->setText(SR_AGE_COL, QString::number(dir.age));
		child->setData(SR_AGE_COL, ROLE_SORT, dir.age);
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
					ui.searchSummaryWidget->topLevelItem(i)->setData(SS_COUNT_COL, ROLE_SORT, s+1);
					found = true ;
				}
			}
			if(!found)
			{
				QTreeWidgetItem *item2 = new RSTreeWidgetItem(compareSummaryRole);
				item2->setText(SS_TEXT_COL, txt);
				item2->setText(SS_COUNT_COL, QString::number(1));
				item2->setData(SS_COUNT_COL, ROLE_SORT, 1);
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
			rsFiles->RequestDirDetails(it->ref, details, FileSearchFlags(0u));
			insertDirectory(txt, searchId, details, child);
		}
	}
}

void SearchDialog::insertDirectory(const QString &txt, qulonglong searchId, const DirDetails &dir)
{
	return ; // Remove this statement to allow adding directories to the search results.

    QString sid_hexa = QString::number(searchId,16) ;
    QTreeWidgetItem *child = new RSTreeWidgetItem(compareResultRole);

    child->setIcon(SR_NAME_COL, QIcon(IMAGE_DIRECTORY));
    child->setText(SR_NAME_COL, QString::fromUtf8(dir.name.c_str()));
    child->setText(SR_HASH_COL, QString::fromStdString(dir.hash));
    child->setText(SR_SIZE_COL, QString::number(dir.count));
    child->setData(SR_SIZE_COL, ROLE_SORT, (qulonglong) dir.count);
    child->setText(SR_AGE_COL, QString::number(dir.min_age));
    child->setData(SR_AGE_COL, ROLE_SORT, dir.min_age);
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
                    ui.searchSummaryWidget->topLevelItem(i)->setData(SS_COUNT_COL, ROLE_SORT, s+1);
                    found = true ;
            }
    }
    if(!found)
    {
            RSTreeWidgetItem *item2 = new RSTreeWidgetItem(compareSummaryRole);
            item2->setText(SS_TEXT_COL, txt);
            item2->setText(SS_COUNT_COL, QString::number(1));
            item2->setData(SS_COUNT_COL, ROLE_SORT, 1);
            item2->setTextAlignment( SS_COUNT_COL, Qt::AlignRight );
            item2->setText(SS_SEARCH_ID_COL, sid_hexa);

            ui.searchSummaryWidget->addTopLevelItem(item2);
            ui.searchSummaryWidget->setCurrentItem(item2);
    }

    selectSearchResults();
// TODO: check for duplicate directories
}

void SearchDialog::insertFile(qulonglong searchId, const FileDetail& file, int searchType)
{
	// algo:
	//
	// 1 - look in result window whether the file already exist.
	// 	1.1 - If yes, just increment the source number.
	// 	2.2 - Otherwise, add an entry.
	// 2 - look in the summary whether there exist the same request id.
	// 	1.1 - If yes, just increment the result number.
	// 	2.2 - Otherwise, ignore this file
	//
	//

	QString sid_hexa = QString::number(searchId,16) ;

	//check if search ID is still in the summary list, if not it was already closed by
	// the user, so nothing has to be done here
	int summaryItemCount = ui.searchSummaryWidget->topLevelItemCount();
	int summaryItemIndex = -1 ;
	for (int i = 0; i < summaryItemCount; i++) {
		if(ui.searchSummaryWidget->topLevelItem(i)->text(SS_SEARCH_ID_COL) == sid_hexa) {
			summaryItemIndex = i ;
			break ;
		}
	}
	if (summaryItemIndex == -1)
		return;

	/* which extensions do we use? */
	int fileTypeIndex = ui.searchSummaryWidget->topLevelItem(summaryItemIndex)->text(SS_FILE_TYPE_COL).toInt();
	if (fileTypeIndex != FILETYPE_IDX_ANY) {
		// collect the extensions to use
		QStringList extList = FileTypeExtensionMap->value(fileTypeIndex).split(" ");

		// check this file's extension
		QString qName = QString::fromUtf8(file.name.c_str());
		if (!extList.contains(QFileInfo(qName).suffix(), Qt::CaseInsensitive)) {
			return;
		}
	}

	static std::map<qulonglong,int> nb_results ;

	if(nb_results.find(searchId) == nb_results.end())
		nb_results[searchId] = 0 ;

	if (nb_results[searchId] >= ui._max_results_SB->value())
		return ;

	// 1 - look in result window whether the file already exists.
	//
	bool found = false ;
	int sources;
	int friendSource = 0;
	int anonymousSource = 0;
	QString modifiedResult;

	QList<QTreeWidgetItem*> itms = ui.searchResultWidget->findItems(QString::fromStdString(file.hash),Qt::MatchExactly,SR_HASH_COL) ;

	for(QList<QTreeWidgetItem*>::const_iterator it(itms.begin());it!=itms.end();++it)
		if((*it)->text(SR_SEARCH_ID_COL) == sid_hexa)
		{
			QString resultCount = (*it)->text(SR_ID_COL);
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
			modifiedResult = QString::number(friendSource) + "/" + QString::number(anonymousSource);
			(*it)->setText(SR_ID_COL,modifiedResult);
			QTreeWidgetItem *item = (*it);
			found = true ;

			if (!item->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool()) {
				QColor foreground;

				int sources = friendSource + anonymousSource ;
				if (sources < 1)
				{
					foreground = QColor(0, 0, 19);
				}
				else if (sources < 2)
				{
					foreground = QColor(0, 0, 38);
				}
				else if (sources < 3)
				{
					foreground = QColor(0, 0, 57);
				}
				else if (sources < 4)
				{
					foreground = QColor(0, 0, 76);
				}
				else if (sources < 5)
				{
					foreground = QColor(0, 0, 96);
				}
				else if (sources < 6)
				{
					foreground = QColor(0, 0, 114);
				}
				else if (sources < 7)
				{
					foreground = QColor(0, 0, 133);
				}
				else if (sources < 8)
				{
					foreground = QColor(0, 0, 152);
				}
				else if (sources < 9)
				{
					foreground = QColor(0, 0, 171);
				}
				else if (sources < 10)
				{
					foreground = QColor(0, 0, 190);
				}
				else if (sources < 11)
				{
					foreground = QColor(0, 0, 209);
				}
				else
				{
					foreground = QColor(0, 0, 228);
				}

				QBrush brush(foreground);
				for (int i = 0; i < item->columnCount(); i++)
				{
					item->setForeground(i, brush);
				}
			}
			break ;
		}

	if(!found)
	{
		++nb_results[searchId] ;

		/* translate search results */
		
		QTreeWidgetItem *item = new RSTreeWidgetItem(compareResultRole);
		item->setText(SR_NAME_COL, QString::fromUtf8(file.name.c_str()));
		item->setText(SR_HASH_COL, QString::fromStdString(file.hash));

		setIconAndType(item, QString::fromUtf8(file.name.c_str()));

		/*
		 * to facilitate downloads we need to save the file size too
		 */

		item->setText(SR_SIZE_COL, QString::number(file.size));
		item->setData(SR_SIZE_COL, ROLE_SORT, (qulonglong) file.size);
		item->setText(SR_AGE_COL, QString::number(file.age));
		item->setData(SR_AGE_COL, ROLE_SORT, file.age);
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

		modifiedResult =QString::number(friendSource) + "/" + QString::number(anonymousSource);
		item->setText(SR_ID_COL,modifiedResult);
		item->setTextAlignment( SR_ID_COL, Qt::AlignRight );
		item->setText(SR_SEARCH_ID_COL, sid_hexa);

		QColor foreground;
		bool setForeground = false;

		FileInfo fi;
		if (rsFiles->FileDetails(file.hash, RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_SPEC_ONLY, fi)) {
			item->setData(SR_DATA_COL, SR_ROLE_LOCAL, true);
			foreground = Qt::red;
			setForeground = true;
		} else {
			item->setData(SR_DATA_COL, SR_ROLE_LOCAL, false);

			sources = item->text(SR_ID_COL).toInt();
			if (sources == 1)
			{
				foreground = QColor(0, 0, 0);
				setForeground = true;
			}
		}

		if (setForeground) {
			QBrush brush(foreground);
			for (int i = 0; i < item->columnCount(); i++)
			{
				item->setForeground(i, brush);
			}
		}

		ui.searchResultWidget->addTopLevelItem(item);

		/* hide/show this search result */
		hideOrShowSearchResult(item);
	}

	/* update the summary as well */
	if(!found)		// only increment result when it's a new item.
	{
		int s = ui.searchSummaryWidget->topLevelItem(summaryItemIndex)->text(SS_COUNT_COL).toInt() ;
		ui.searchSummaryWidget->topLevelItem(summaryItemIndex)->setText(SS_COUNT_COL,QString::number(s+1));
		ui.searchSummaryWidget->topLevelItem(summaryItemIndex)->setData(SS_COUNT_COL, ROLE_SORT, s+1);
	}
}

void SearchDialog::resultsToTree(const QString& txt,qulonglong searchId, const std::list<DirDetails>& results)
{
	ui.searchResultWidget->setSortingEnabled(false);

	/* translate search results */

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

			insertFile(searchId,fd, FRIEND_SEARCH);
		} else if (it->type == DIR_TYPE_DIR) {
//			insertDirectory(txt, searchId, *it, NULL);
			insertDirectory(txt, searchId, *it);
		}

	ui.searchResultWidget->setSortingEnabled(true);
}

void SearchDialog::selectSearchResults(int index)
{
	/* highlight this search in summary window */
	QString searchId;
	QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
	if (ci) {
		/* get the searchId text */
		searchId = ci->text(SS_SEARCH_ID_COL);
		if (ui.FileTypeComboBox->currentIndex() != FILETYPE_IDX_ANY)
			ui.FileTypeComboBox->setCurrentIndex(ci->text(SS_FILE_TYPE_COL).toInt());
	}
#ifdef DEBUG
	std::cerr << "SearchDialog::selectSearchResults(): searchId: " << searchId.toStdString();
	std::cerr << std::endl;
#endif

	/* show only matching searchIds in main window */
	int items = ui.searchResultWidget->topLevelItemCount();
	for(int i = 0; i < items; i++)
	{
		hideOrShowSearchResult(ui.searchResultWidget->topLevelItem(i), searchId, index);
	}
	ui.searchResultWidget->update();
	ui.filterLineEdit->clear();
}

void SearchDialog::hideOrShowSearchResult(QTreeWidgetItem* resultItem, QString currentSearchId, int fileTypeIndex)
{
	if (currentSearchId.isEmpty()) {
		QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
		if (ci)
			/* get the searchId text */
			currentSearchId = ci->text(SS_SEARCH_ID_COL);
	}

	if (resultItem->text(SR_SEARCH_ID_COL) != currentSearchId) {
		resultItem->setHidden(true);
		return;
	}

	// check if file type matches
	if (fileTypeIndex == -1)
		fileTypeIndex = ui.FileTypeComboBox->currentIndex();

	if (fileTypeIndex != FILETYPE_IDX_ANY) {
		if (!(fileTypeIndex == FILETYPE_IDX_DIRECTORY && resultItem->text(SR_HASH_COL).isEmpty())) {
			QStringList extList = FileTypeExtensionMap->value(fileTypeIndex).split(" ");

			if (!extList.contains(QFileInfo(resultItem->text(SR_NAME_COL)).suffix(), Qt::CaseInsensitive)) {
				resultItem->setHidden(true);
				return;
			}
		}
	}

	// file type matches, now filter text
	if (ui.filterLineEdit->text().isEmpty()) {
		resultItem->setHidden(false);
	} else {
		int filterColumn = ui.filterLineEdit->currentFilter();
		filterItem(resultItem, ui.filterLineEdit->text(), filterColumn);
	}
}

void SearchDialog::setIconAndType(QTreeWidgetItem *item, const QString& filename)
{
	item->setIcon(SR_NAME_COL, FilesDefs::getIconFromFilename(filename));
	item->setText(SR_TYPE_COL, FilesDefs::getNameFromFilename(filename));
}

void SearchDialog::copyResultLink()
{
    /* should also be able to handle multi-selection */
    QList<QTreeWidgetItem*> itemsForCopy = ui.searchResultWidget->selectedItems();
    int numdls = itemsForCopy.size();
    QTreeWidgetItem * item;

	QList<RetroShareLink> urls ;

    for (int i = 0; i < numdls; ++i) 
	 {
		 item = itemsForCopy.at(i);
		 // call copy

		 if (!item->childCount()) 
		 {
			 std::cerr << "SearchDialog::copyResultLink() Calling set retroshare link";
			 std::cerr << std::endl;

			 QString fhash = item->text(SR_HASH_COL);
			 qulonglong fsize = item->text(SR_SIZE_COL).toULongLong();
			 QString fname = item->text(SR_NAME_COL);

			 RetroShareLink link;
			 if (link.createFile(fname, fsize, fhash)) {
				 std::cerr << "new link added to clipboard: " << link.toString().toStdString() << std::endl ;
				 urls.push_back(link) ;
			 }
		 } 
	 }
	 RSLinkClipboard::copyLinks(urls) ;
}

void SearchDialog::sendLinkTo( )
{
    copyResultLink();

    /* create a message */
    MessageComposer *nMsgDialog = MessageComposer::newMsg();
    if (nMsgDialog == NULL) {
        return;
    }

    nMsgDialog->setTitleText(tr("New RetroShare Link(s)"));
    nMsgDialog->setMsgText(RSLinkClipboard::toHtml(), true) ;

    nMsgDialog->show();

    /* window will destroy itself! */
}

void SearchDialog::selectFileType(int index)
{
    if (!FileTypeExtensionMap->contains(index) && index != FILETYPE_IDX_DIRECTORY)
        return;

	QString searchId;
	QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
	if (ci) {
		/* get the searchId text */
		searchId = ci->text(SS_SEARCH_ID_COL);
	}

	/* show only matching file types in main window */
	int items = ui.searchResultWidget->topLevelItemCount();
	for(int i = 0; i < items; i++)
	{
		hideOrShowSearchResult(ui.searchResultWidget->topLevelItem(i), searchId, index);
	}
}

void SearchDialog::filterItems()
{
    int count = ui.searchResultWidget->topLevelItemCount ();
    for (int index = 0; index < count; index++) {
        hideOrShowSearchResult(ui.searchResultWidget->topLevelItem(index));
    }
}

bool SearchDialog::filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn)
{
    bool visible = true;

    if (text.isEmpty() == false) {
        if (item->text(filterColumn).contains(text, Qt::CaseInsensitive) == false) {
            visible = false;
        }
    }

    int visibleChildCount = 0;
    int count = item->childCount();
    for (int index = 0; index < count; index++) {
        if (filterItem(item->child(index), text, filterColumn)) {
            visibleChildCount++;
        }
    }

    if (visible || visibleChildCount) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    return (visible || visibleChildCount);
}
