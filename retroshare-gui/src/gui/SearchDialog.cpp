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


#include "rshare.h"
#include "SearchDialog.h"
#include "rsiface/rsiface.h"
#include "rsiface/rsexpr.h"

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
#define IMAGE_REMOVE  		    ":/images/delete.png"
#define IMAGE_REMOVEALL   		":/images/deleteall.png"

/* Key for UI Preferences */
#define UI_PREF_ADVANCED_SEARCH  "UIOptions/AdvancedSearch"

/* indicies for search results item columns SR_ = Search Result */
#define SR_NAME_COL         0
#define SR_SIZE_COL         1
#define SR_HASH_COL         2
#define SR_ID_COL           3
#define SR_SEARCH_ID_COL    4

/* indicies for search summary item columns SS_ = Search Summary */
#define SS_TEXT_COL         0
#define SS_COUNT_COL        1
#define SS_SEARCH_ID_COL    2

/* static members */
/* These indices MUST be identical to their equivalent indices in the combobox */
const int SearchDialog::FILETYPE_IDX_ANY = 0;
const int SearchDialog::FILETYPE_IDX_AUDIO = 1;
const int SearchDialog::FILETYPE_IDX_VIDEO = 2;
const int SearchDialog::FILETYPE_IDX_PICTURE = 3;
const int SearchDialog::FILETYPE_IDX_PROGRAM = 4;
const int SearchDialog::FILETYPE_IDX_ARCHIVE = 5;
const int SearchDialog::FILETYPE_IDX_DOCUMENT = 6;
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

    /* initialise the filetypes mapping */
    if (!SearchDialog::initialised)
    {
	initialiseFileTypeMappings();
    }
   
    /* Advanced search panel specifica */
    RshareSettings rsharesettings;
    QString key (UI_PREF_ADVANCED_SEARCH);
    bool useAdvanced = rsharesettings.value(key, QVariant(false)).toBool();
    if (useAdvanced)
    {
        ui.toggleAdvancedSearchBtn->setChecked(true);
        ui.SimpleSearchPanel->hide();
    } else {
        ui.AdvancedSearchPanel->hide();
    }
    
    connect(ui.toggleAdvancedSearchBtn, SIGNAL(toggled(bool)), this, SLOT(toggleAdvancedSearchDialog(bool)));
    connect(ui.focusAdvSearchDialogBtn, SIGNAL(clicked()), this, SLOT(showAdvSearchDialog())); 
    
    /* End Advanced Search Panel specifics */


    connect( ui.searchResultWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( searchtableWidgetCostumPopupMenu( QPoint ) ) );
    
    connect( ui.searchSummaryWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( searchtableWidget2CostumPopupMenu( QPoint ) ) );
    
    connect( ui.lineEdit, SIGNAL( returnPressed ( void ) ), this, SLOT( searchKeywords( void ) ) );
    connect( ui.pushButtonsearch, SIGNAL( released ( void ) ), this, SLOT( searchKeywords( void ) ) );
    connect( ui.pushButtonDownload, SIGNAL( released ( void ) ), this, SLOT( download( void ) ) );
    //connect( ui.searchSummaryWidget, SIGNAL( itemSelectionChanged ( void ) ), this, SLOT( selectSearchResults( void ) ) );
    
    connect ( ui.searchSummaryWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem * ) ),
                    this, SLOT( selectSearchResults( void ) ) );
   

    /* hide the Tree +/- */
    ui.searchResultWidget -> setRootIsDecorated( false );
    ui.searchSummaryWidget -> setRootIsDecorated( false );

    /* make it extended selection */
    ui.searchResultWidget -> setSelectionMode(QAbstractItemView::ExtendedSelection);
    

    /* Set header resize modes and initial section sizes */
    ui.searchSummaryWidget->setColumnCount(3);
     
    QHeaderView * _smheader = ui.searchSummaryWidget->header () ;   
    _smheader->setResizeMode (0, QHeaderView::Interactive);
    _smheader->setResizeMode (1, QHeaderView::Interactive);
    _smheader->setResizeMode (2, QHeaderView::Interactive);
    
    _smheader->resizeSection ( 0, 80 );
    _smheader->resizeSection ( 1, 75 );
    _smheader->resizeSection ( 2, 75 );

    ui.searchResultWidget->setColumnCount(4);
    _smheader = ui.searchResultWidget->header () ;   
    _smheader->setResizeMode (0, QHeaderView::Interactive);
    _smheader->setResizeMode (1, QHeaderView::Interactive);
    _smheader->setResizeMode (2, QHeaderView::Interactive);
    _smheader->setResizeMode (3, QHeaderView::Interactive);
    
    _smheader->resizeSection ( 0, 200 );
    _smheader->resizeSection ( 1, 75 );
    _smheader->resizeSection ( 2, 75 );
    _smheader->resizeSection ( 3, 75 );


/* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void SearchDialog::initialiseFileTypeMappings()
{
	/* edit these strings to change the range of extensions recognised by the search */
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_ANY, "");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_AUDIO, 
		"aac aif iff m3u mid midi mp3 mpa ogg ra ram wav wma");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_VIDEO, 
		"3gp asf asx avi mov mp4 mpeg mpg qt rm swf vob wmv");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_PICTURE, 
		"3dm 3dmf ai bmp drw dxf eps gif ico indd jpe jpeg jpg mng pcx pcc pct pgm "
		"pix png psd psp qxd qxprgb sgi svg tga tif tiff xbm xcf");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_PROGRAM, 
		"app bat cgi com bin exe js pif py pl sh vb ws ");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_ARCHIVE, 
		"7z bz2 gz pkg rar sea sit sitx tar zip");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_DOCUMENT, 
		"doc odt ott rtf pdf ps txt log msg wpd wps" );	
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
        
        broadcastonchannelAct = new QAction( tr( "Broadcast on Channel" ), this );
        connect( broadcastonchannelAct , SIGNAL( triggered() ), this, SLOT( broadcastonchannel() ) );
        
        recommendtofriendsAct = new QAction( tr( "Recommend to Friends" ), this );
        connect( recommendtofriendsAct , SIGNAL( triggered() ), this, SLOT( recommendtofriends() ) );
    
    
        contextMnu->clear();
        contextMnu->addAction( downloadAct);
        contextMnu->addSeparator();
        contextMnu->addAction( broadcastonchannelAct);
        contextMnu->addAction( recommendtofriendsAct);
      }

      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, 
                                             Qt::RightButton, Qt::RightButton, Qt::NoModifier );
      contextMnu->exec( mevent->globalPos() );
}


void SearchDialog::download()
{
    /* should also be able to handle multi-selection */
    QList<QTreeWidgetItem*> itemsForDownload = ui.searchResultWidget->selectedItems();
    int numdls = itemsForDownload.size();
    QTreeWidgetItem * item;
    bool attemptDownloadLocal = false;
    
    for (int i = 0; i < numdls; ++i) {
        item = itemsForDownload.at(i);
        // call the download
	if (item->text(SR_ID_COL) != "Local")
	{
        	rsicontrol -> FileRequest((item->text(SR_NAME_COL)).toStdString(), 
                                  (item->text(SR_HASH_COL)).toStdString(), 
                                  (item->text(SR_SIZE_COL)).toInt(), 
                                  "");
	}
	else
	{
		attemptDownloadLocal = true;
	}
    }
    if (attemptDownloadLocal)
    {
    	QMessageBox::information(0, tr("Download Notice"), tr("Skipping Local Files"));
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

void SearchDialog::advancedSearch(Expression* expression)
{
        advSearchDialog->hide();

	/* call to core */
	std::list<FileDetail> results;
	rsicontrol -> SearchBoolExp(expression, results);

        /* abstraction to allow reusee of tree rendering code */
        resultsToTree((advSearchDialog->getSearchAsString()).toStdString(), results);
}



void SearchDialog::searchKeywords()
{	
	QString qTxt = ui.lineEdit->text();
	std::string txt = qTxt.toStdString();

	std::cerr << "SearchDialog::searchKeywords() : " << txt << std::endl;

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

	/* call to core */
	std::list<FileDetail> initialResults;
	std::list<FileDetail> * finalResults = 0;
	
	rsicontrol -> SearchKeywords(words, initialResults);
	/* which extensions do we use? */
	QString qExt, qName;
	int extIndex;
	bool matched =false;
	FileDetail fd;

	if (ui.FileTypeComboBox->currentIndex() == FILETYPE_IDX_ANY) 
	{
		finalResults = &initialResults;
	} else {
		finalResults = new std::list<FileDetail>;
		// amend the text description of the search
		txt += " (" + ui.FileTypeComboBox->currentText().toStdString() + ")";
		// collect the extensions to use
		QString extStr = SearchDialog::FileTypeExtensionMap->value(ui.FileTypeComboBox->currentIndex());	
		QStringList extList = extStr.split(" ");
		
		// now iterate through the results ignoring those with wrong extensions
		std::list<FileDetail>::iterator resultsIter;
		for (resultsIter = initialResults.begin(); resultsIter != initialResults.end(); resultsIter++)
		{
			fd = *resultsIter;
			// get this file's extension
			qName = QString::fromStdString(fd.name);
			extIndex = qName.lastIndexOf(".");
			if (extIndex >= 0) {
				qExt = qName.mid(extIndex+1);
				if (qExt != "" )
				{
					// does it match?
					matched = false;
					/* iterate through the requested extensions */
					for (int i = 0; i < extList.size(); ++i)
					{
						if (qExt.toUpper() == extList.at(i).toUpper())
						{					
							finalResults->push_back(fd);	
							matched = true;
						}
					}
				}
			}
		}
	}

        /* abstraction to allow reusee of tree rendering code */
        resultsToTree(txt, *finalResults);
}
 
void SearchDialog::resultsToTree(std::string txt, std::list<FileDetail> results)
{
	/* translate search results */
	int searchId = nextSearchId++;
	std::ostringstream out;
	out << searchId;

	std::list<FileDetail>::iterator it;
	for(it = results.begin(); it != results.end(); it++)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(SR_NAME_COL, QString::fromStdString(it->name));
		item->setText(SR_HASH_COL, QString::fromStdString(it->hash));
		item->setText(SR_ID_COL, QString::fromStdString(it->id));
		item->setText(SR_SEARCH_ID_COL, QString::fromStdString(out.str()));
		/*
		 * to facilitate downlaods we need to save the file size too
		 */
                QVariant * variant = new QVariant(it->size);
                item->setText(SR_SIZE_COL, QString(variant->toString()));

		if (it->id == "Local")
		{
			/* colour green? */
			item->setBackground(3, QBrush(Qt::green));
		}
		else
		{
			/* colour blue? */
			item->setBackground(3, QBrush(Qt::blue));
		}
		ui.searchResultWidget->addTopLevelItem(item);
	}

	/* add to the summary as well */

	QTreeWidgetItem *item = new QTreeWidgetItem();
	item->setText(SS_TEXT_COL, QString::fromStdString(txt));
	std::ostringstream out2;
	out2 << results.size();

	item->setText(SS_COUNT_COL, QString::fromStdString(out2.str()));
	item->setText(SS_SEARCH_ID_COL, QString::fromStdString(out.str()));

	ui.searchSummaryWidget->addTopLevelItem(item);
	ui.searchSummaryWidget->setCurrentItem(item);

	/* select this search result */
	selectSearchResults();
}


//void QTreeWidget::currentItemChanged ( QTreeWidgetItem * current, QTreeWidgetItem * previous )  [signal]


void SearchDialog::selectSearchResults()
{
	/* highlight this search in summary window */
	QTreeWidgetItem *ci = ui.searchSummaryWidget->currentItem();
	if (!ci)
		return;

	/* get the searchId text */
	QString searchId = ci->text(SS_SEARCH_ID_COL);

	std::cerr << "SearchDialog::selectSearchResults(): searchId: " << searchId.toStdString();
	std::cerr << std::endl;

	/* show only matching searchIds in main window */
	int items = ui.searchResultWidget->topLevelItemCount();
	for(int i = 0; i < items; i++)
	{
		/* get item */
		QTreeWidgetItem *ti = ui.searchResultWidget->topLevelItem(i);
		if (ti->text(SR_SEARCH_ID_COL) == searchId)
		{
			ti->setHidden(false);
		}
		else
		{
			ti->setHidden(true);
		}
	}
	ui.searchResultWidget->update();
}


