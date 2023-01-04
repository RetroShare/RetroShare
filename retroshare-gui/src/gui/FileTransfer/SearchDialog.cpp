/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/SearchDialog.cpp                        *
 *                                                                             *
 * Copyright (c) 2006, Crypton       <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include <QShortcut>

#include "rshare.h"
#include "SearchDialog.h"
#include "gui/FileTransfer/BannedFilesDialog.h"
#include "gui/RSHumanReadableDelegate.h"
#include "gui/RetroShareLink.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/common/RsCollection.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/RsUrlHandler.h"
#include "gui/settings/rsharesettings.h"
#include "gui/advsearch/advancedsearchdialog.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "util/QtVersion.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rsexpr.h>

/* Images for context menu icons */
#define IMAGE_START                ":/icons/png/download.png"
#define IMAGE_SEARCHAGAIN          ":/images/update.png"
#define IMAGE_REMOVE               ":/images/delete.png"
#define IMAGE_REMOVEALL            ":/images/deleteall.png"
#define IMAGE_DIRECTORY            ":/images/folder16.png"
#define IMAGE_OPENFOLDER           ":/images/folderopen.png"
#define IMAGE_LIBRARY              ":/icons/collections.png"
#define IMAGE_COLLCREATE           ":/iconss/png/add.png"
#define IMAGE_COLLMODIF            ":/icons/png/pencil-edit-button.png"
#define IMAGE_COLLVIEW             ":/images/find.png"
#define IMAGE_COLLOPEN             ":/icons/collections.png"
#define IMAGE_COPYLINK             ":/images/copyrslink.png"
#define IMAGE_BANFILE              ":/icons/biohazard_red.png"

/* Key for UI Preferences */
#define UI_PREF_ADVANCED_SEARCH  "UIOptions/AdvancedSearch"

/* indicies for search summary item columns SS_ = Search Summary */
#define SS_KEYWORDS_COL     0
#define SS_RESULTS_COL      1
#define SS_SEARCH_ID_COL    2
#define SS_FILE_TYPE_COL    3
#define SS_COL_COUNT        3 //4 ???
#define SS_DATA_COL         SS_KEYWORDS_COL

#define ROLE_KEYWORDS       Qt::UserRole
#define ROLE_SORT           Qt::UserRole + 1

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

    _queueIsAlreadyTakenCareOf = false;
    ui.lineEdit->setFocus();

    collCreateAct= new QAction(QIcon(IMAGE_COLLCREATE), tr("Create Collection..."), this);
    connect(collCreateAct,SIGNAL(triggered()),this,SLOT(collCreate()));
    collModifAct= new QAction(QIcon(IMAGE_COLLMODIF), tr("Modify Collection..."), this);
    connect(collModifAct,SIGNAL(triggered()),this,SLOT(collModif()));
    collViewAct= new QAction(QIcon(IMAGE_COLLVIEW), tr("View Collection..."), this);
    connect(collViewAct,SIGNAL(triggered()),this,SLOT(collView()));
    collOpenAct = new QAction(QIcon(IMAGE_COLLOPEN), tr( "Download from collection file..." ), this );
    connect(collOpenAct, SIGNAL(triggered()), this, SLOT(collOpen()));

    /* initialise the filetypes mapping */
    if (!SearchDialog::initialised)
    {
        initialiseFileTypeMappings() ;
    }

    connect(ui.toggleAdvancedSearchBtn, SIGNAL(clicked()), this, SLOT(showAdvSearchDialog()));

    connect( ui.searchResultWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( searchResultWidgetCustomPopupMenu( QPoint ) ) );

    connect( ui.searchSummaryWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( searchSummaryWidgetCustomPopupMenu( QPoint ) ) );
    connect( ui.showBannedFiles_TB, SIGNAL( clicked() ), this, SLOT( openBannedFiles() ) );

    connect( ui.lineEdit, SIGNAL( returnPressed () ), this, SLOT( searchKeywords() ) );
    connect( ui.lineEdit, SIGNAL( textChanged ( const QString& ) ), this, SLOT( checkText( const QString& ) ) );
    connect( ui.searchButton, SIGNAL( released () ), this, SLOT( searchKeywords() ) );
    connect( ui.pushButtonDownload, SIGNAL( released () ), this, SLOT( download() ) );
    connect( ui.cloaseallsearchresultsButton, SIGNAL(clicked()), this, SLOT(searchRemoveAll()));

    connect( ui.searchResultWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int)), this, SLOT(download()));

    connect ( ui.searchSummaryWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem * ) ),
                    this, SLOT( selectSearchResults( void ) ) );

    connect(ui.FileTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFileType(int)));
    
    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems()));
    connect(ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterItems()));

    compareSummaryRole = new RSTreeWidgetItemCompareRole;
    compareSummaryRole->setRole(SS_RESULTS_COL, ROLE_SORT);

    compareResultRole = new RSTreeWidgetItemCompareRole;
    compareResultRole->setRole(SR_SIZE_COL, ROLE_SORT);
    compareResultRole->setRole(SR_AGE_COL, ROLE_SORT);
    compareResultRole->setRole(SR_SOURCES_COL, ROLE_SORT);

    /* hide the Tree +/- */
    ui.searchResultWidget -> setRootIsDecorated( true );
    ui.searchResultWidget -> setColumnHidden( SR_UID_COL,true );
    ui.searchSummaryWidget -> setRootIsDecorated( false );

	 // We set some delegates to handle the display of size and date.
    //  To allow a proper sorting, be careful to pad at right with spaces. This
    //  is achieved by using QString("%1").arg(number,15,10).
    //
    ui.searchResultWidget->setItemDelegateForColumn(SR_SIZE_COL, mSizeColumnDelegate=new RSHumanReadableSizeDelegate()) ;
    ui.searchResultWidget->setItemDelegateForColumn(SR_AGE_COL, mAgeColumnDelegate=new RSHumanReadableAgeDelegate()) ;

    /* make it extended selection */
    ui.searchResultWidget -> setSelectionMode(QAbstractItemView::ExtendedSelection);

    /* Set header resize modes and initial section sizes */
    ui.searchSummaryWidget->setColumnCount(SS_COL_COUNT);
    ui.searchSummaryWidget->setColumnHidden(SS_SEARCH_ID_COL, true);

    QHeaderView * _smheader = ui.searchSummaryWidget->header () ;
    QHeaderView_setSectionResizeModeColumn(_smheader, SS_KEYWORDS_COL, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_smheader, SS_RESULTS_COL, QHeaderView::Interactive);

    float f = QFontMetricsF(font()).height()/14.0 ;

    _smheader->resizeSection ( SS_KEYWORDS_COL, 160*f );
    _smheader->resizeSection ( SS_RESULTS_COL, 50*f );

    ui.searchResultWidget->setColumnCount(SR_COL_COUNT);
    _smheader = ui.searchResultWidget->header () ;
    QHeaderView_setSectionResizeModeColumn(_smheader, SR_NAME_COL, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_smheader, SR_SIZE_COL, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(_smheader, SR_SOURCES_COL, QHeaderView::Interactive);

    _smheader->resizeSection ( SR_NAME_COL, 240*f );
    _smheader->resizeSection ( SR_SIZE_COL, 75*f );
    _smheader->resizeSection ( SR_SOURCES_COL, 75*f );
    _smheader->resizeSection ( SR_TYPE_COL, 75*f );
    _smheader->resizeSection ( SR_AGE_COL, 90*f );
    _smheader->resizeSection ( SR_HASH_COL, 240*f );

    ui.searchResultWidget->sortItems(SR_NAME_COL, Qt::AscendingOrder);

    QFontMetricsF fontMetrics(ui.searchResultWidget->font());
    int iconHeight = fontMetrics.height() * 1.4;
    ui.searchResultWidget->setIconSize(QSize(iconHeight, iconHeight));

    /* Set initial size the splitter */
    QList<int> sizes;
    sizes << 250 << width(); // Qt calculates the right sizes
    ui.splitter->setSizes(sizes);

    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("File Name"), SR_NAME_COL);
    //ui.filterLineEdit->addFilter(QIcon(), tr("File Size"), SR_SIZE_COL);
    ui.filterLineEdit->setCurrentFilter(SR_NAME_COL);

    // load settings
    processSettings(true);
  
  	ui._ownFiles_CB->setMinimumWidth(20*f);
  	ui._friendListsearch_SB->setMinimumWidth(20*f);
    ui._anonF2Fsearch_CB->setMinimumWidth(20*f);
    ui.label->setMinimumWidth(20*f);

    // workaround for Qt bug, be solved in next Qt release 4.7.0
    // https://bugreports.qt-project.org/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.searchSummaryWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT(searchRemove()));

    checkText(ui.lineEdit->text());

}

SearchDialog::~SearchDialog()
{
    // save settings
    processSettings(false);

    if (compareSummaryRole)
        delete(compareSummaryRole);

    if (compareResultRole)
        delete(compareResultRole);

    delete mSizeColumnDelegate;
    delete mAgeColumnDelegate;

    ui.searchResultWidget->setItemDelegateForColumn(SR_SIZE_COL, nullptr);
    ui.searchResultWidget->setItemDelegateForColumn(SR_AGE_COL, nullptr);
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
        
        ui._max_results_SB->setValue(Settings->value("MaxResults").toInt());
        
    } else {
        // save settings

        // state of SearchSummary tree
        Settings->setValue("SearchSummaryTree", pHeader->saveState());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
        
        Settings->setValue("MaxResults",  ui._max_results_SB->value());
    }

    Settings->endGroup();
    m_bProcessSettings = false;
}


void SearchDialog::checkText(const QString& txt)
{
	ui.searchButton->setDisabled(txt.length() < 3);
	ui.searchLineFrame->setProperty("valid", (txt.length() >= 3));
	ui.searchLineFrame->style()->unpolish(ui.searchLineFrame);
	Rshare::refreshStyleSheet(ui.searchLineFrame, false);
}

void SearchDialog::initialiseFileTypeMappings()
{
	/* edit these strings to change the range of extensions recognised by the search */
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_ANY, "");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_AUDIO,
		"aac aif flac iff m3u m4a mid midi mp3 mpa ogg ra ram wav wma weba");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_ARCHIVE,
		"7z bz2 gz pkg rar sea sit sitx tar zip tgz");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_CDIMAGE,
		"iso nrg mdf bin");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_DOCUMENT,
		"doc odt ott rtf pdf ps txt log msg wpd wps ods xls epub" );
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_PICTURE,
		"3dm 3dmf ai bmp drw dxf eps gif ico indd jpe jpeg jpg mng pcx pcc pct pgm "
		"pix png psd psp qxd qxprgb sgi svg tga tif tiff xbm xcf webp");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_PROGRAM,
		"app bat cgi com bin exe js pif py pl sh vb ws bash");
	SearchDialog::FileTypeExtensionMap->insert(FILETYPE_IDX_VIDEO,
		"3gp asf asx avi mov mp4 mkv flv mpeg mpg qt rm swf vob wmv webm");
	SearchDialog::initialised = true;
}

void SearchDialog::searchResultWidgetCustomPopupMenu( QPoint /*point*/ )
{
	// Block the popup if no results available
	if ((ui.searchResultWidget->selectedItems()).isEmpty()) return ;

	bool add_CollActions = false ;

	QMenu contextMnu(this) ;

	contextMnu.addAction(QIcon(IMAGE_START), tr("Download"), this, SLOT(download())) ;
	contextMnu.addSeparator();//--------------------------------------

	contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyResultLink())) ;
	contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Send RetroShare Link"), this, SLOT(sendLinkTo())) ;
	contextMnu.addSeparator();//--------------------------------------

    contextMnu.addAction(QIcon(IMAGE_BANFILE), tr("Mark as bad"), this, SLOT(ban())) ;
    contextMnu.addSeparator();//--------------------------------------

	QMenu collectionMenu(tr("Collection"), this);
	collectionMenu.setIcon(QIcon(IMAGE_LIBRARY));
	collectionMenu.addAction(collCreateAct);
	collectionMenu.addAction(collModifAct);
	collectionMenu.addAction(collViewAct);
	collectionMenu.addAction(collOpenAct);

	//contextMnu.addAction(tr("Broadcast on Channel"), this, SLOT(broadcastonchannel()));
	//contextMnu.addAction(tr("Recommend to Friends"), this, SLOT(recommendtofriends()));

	if (ui.searchResultWidget->selectedItems().size() == 1) {
		QList<QTreeWidgetItem*> item =ui.searchResultWidget->selectedItems() ;
		if (item.at(0)->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool()) {
			contextMnu.addAction(QIcon(IMAGE_OPENFOLDER), tr("Open Folder"), this, SLOT(openFolderSearch())) ;
			if (item.at(0)->text(SR_NAME_COL).endsWith(RsCollection::ExtensionString)) {
				add_CollActions = true ;
			}//if (item.at(0)->text(SR_NAME_COL).endsWith(RsCollectionFile::ExtensionString))
		}//if (item.at(0)->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool())
	}//if (ui.searchResultWidget->selectedItems().size() == 1)

	collCreateAct->setEnabled(true) ;
	collModifAct->setEnabled(add_CollActions) ;
	collViewAct->setEnabled(add_CollActions) ;
	collOpenAct->setEnabled(true) ;
	contextMnu.addMenu(&collectionMenu) ;

	contextMnu.exec(QCursor::pos()) ;
}

void SearchDialog::getSourceFriendsForHash(const RsFileHash& hash,std::list<RsPeerId>& srcIds)
{
	std::cerr << "Searching sources for file " << hash << std::endl ;
	srcIds.clear();

	FileInfo finfo ;
	rsFiles->FileDetails(hash, RS_FILE_HINTS_REMOTE,finfo) ;

	for(std::vector<TransferInfo>::const_iterator it(finfo.peers.begin());it!=finfo.peers.end();++it)
	{
		std::cerr << "  adding peerid " << (*it).peerId << std::endl ;
		srcIds.push_back((*it).peerId) ;
	}
}

void SearchDialog::download()
{
	/* should also be able to handle multi-selection  */
	QList<QTreeWidgetItem*> itemsForDownload = ui.searchResultWidget->selectedItems() ;
	int numdls = itemsForDownload.size() ;
	bool attemptDownloadLocal = false ;

	for (int i = 0; i < numdls; ++i) {
		QTreeWidgetItem *item = itemsForDownload.at(i) ;
		 //  call the download
		// *
		if (item->text(SR_HASH_COL).isEmpty()) { // we have a folder
			downloadDirectory(item, tr("")) ;
		} else {
			std::cerr << "SearchDialog::download() Calling File Request" << std::endl ;
			std::list<RsPeerId> srcIds ;

			RsFileHash hash( item->text(SR_HASH_COL).toStdString()) ;
			getSourceFriendsForHash( hash, srcIds) ;

			if(!rsFiles -> FileRequest( (item->text(SR_NAME_COL)).toUtf8().constData()
			                            , hash, (item->text(SR_SIZE_COL)).toULongLong()
			                            , "", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds)) {
				attemptDownloadLocal = true;
			 } else {
				std::cout << "isuing file request from search dialog: -"
				          << (item->text(SR_NAME_COL)).toStdString()
				          << "-" << hash << "-" << (item->text(SR_SIZE_COL)).toULongLong() << "-ids=" ;
				for(std::list<RsPeerId>::const_iterator it(srcIds.begin()); it!=srcIds.end(); ++it)
					std::cout << *it << "-" << std::endl;

				QColor foreground = textColorDownloading();
				for (int j = 0; j < item->columnCount(); ++j)
					item->setData(j, Qt::ForegroundRole, foreground );
			}
		}
	}
	if (attemptDownloadLocal)
		QMessageBox::information(this, tr("Download Notice"), tr("Skipping Local Files")) ;
}

void SearchDialog::ban()
{
	/* should also be able to handle multi-selection  */

	QList<QTreeWidgetItem*> itemsForDownload = ui.searchResultWidget->selectedItems() ;
	int numdls = itemsForDownload.size() ;
	QTreeWidgetItem * item ;

	for (int i = 0; i < numdls; ++i)
    {
		item = itemsForDownload.at(i) ;
		 //  call the download
		// *
		if(!item->text(SR_HASH_COL).isEmpty())
		{
			std::cerr << "SearchDialog::download() Calling File Ban" << std::endl ;

			RsFileHash hash( item->text(SR_HASH_COL).toStdString()) ;

			rsFiles -> banFile( hash, (item->text(SR_NAME_COL)).toUtf8().constData() , (item->text(SR_SIZE_COL)).toULongLong());

            while(item->parent() != NULL)
                item = item->parent();

            ui.searchResultWidget->takeTopLevelItem(ui.searchResultWidget->indexOfTopLevelItem(item)) ;
		}
	}
}

void SearchDialog::openBannedFiles()
{
    BannedFilesDialog d ;
    d.exec();
}

void SearchDialog::collCreate()
{
	std::vector <DirDetails> dirVec;

	QList<QTreeWidgetItem*> selectedItems = ui.searchResultWidget->selectedItems() ;
	int selectedCount = selectedItems.size() ;
	QTreeWidgetItem * item ;

	for (int i = 0; i < selectedCount; ++i) {
		item = selectedItems.at(i) ;

		if (!item->text(SR_HASH_COL).isEmpty()) {
			std::string name = item->text(SR_NAME_COL).toUtf8().constData();
			RsFileHash hash( item->text(SR_HASH_COL).toStdString() );
			uint64_t count = item->text(SR_SIZE_COL).toULongLong();

			DirDetails details;
			details.name = name;
			details.hash = hash;
            details.size = count;
			details.type = DIR_TYPE_FILE;

			dirVec.push_back(details);
		}
	}

	RsCollection(dirVec,RS_FILE_HINTS_LOCAL).openNewColl(this);
}

void SearchDialog::collModif()
{
	FileInfo info;

	QList<QTreeWidgetItem*> selectedItems = ui.searchResultWidget->selectedItems() ;
	if (selectedItems.size() != 1) return;
	QTreeWidgetItem* item ;

	item = selectedItems.at(0) ;
	if (!item->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool()) return;

	RsFileHash hash( item->text(SR_HASH_COL).toStdString() );

	if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL
	                                | RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE
	                                | RS_FILE_HINTS_SPEC_ONLY, info)) return;

	/* make path for downloaded files */
	std::string path;
	path = info.path;

	/* open file with a suitable application */
	QFileInfo qinfo;
	qinfo.setFile(QString::fromUtf8(path.c_str()));
	if (qinfo.exists()) {
		if (qinfo.absoluteFilePath().endsWith(RsCollection::ExtensionString)) {
			RsCollection collection;
			collection.openColl(qinfo.absoluteFilePath());
		}//if (qinfo.absoluteFilePath().endsWith(RsCollectionFile::ExtensionString))
	}//if (qinfo.exists())
}

void SearchDialog::collView()
{
	FileInfo info;

	QList<QTreeWidgetItem*> selectedItems = ui.searchResultWidget->selectedItems() ;
	if (selectedItems.size() != 1) return;
	QTreeWidgetItem* item ;

	item = selectedItems.at(0) ;
	if (!item->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool()) return;

	RsFileHash hash( item->text(SR_HASH_COL).toStdString() );

	if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL
	                                | RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE
	                                | RS_FILE_HINTS_SPEC_ONLY, info)) return;

	/* make path for downloaded files */
	std::string path;
	path = info.path;

	/* open file with a suitable application */
	QFileInfo qinfo;
	qinfo.setFile(QString::fromUtf8(path.c_str()));
	if (qinfo.exists()) {
		if (qinfo.absoluteFilePath().endsWith(RsCollection::ExtensionString)) {
			RsCollection collection;
			collection.openColl(qinfo.absoluteFilePath(), true);
		}//if (qinfo.absoluteFilePath().endsWith(RsCollectionFile::ExtensionString))
	}//if (qinfo.exists())
}

void SearchDialog::collOpen()
{
	FileInfo info;

	QList<QTreeWidgetItem*> selectedItems = ui.searchResultWidget->selectedItems() ;
	if (selectedItems.size() == 1) {
		QTreeWidgetItem* item ;

		item = selectedItems.at(0) ;
		if (item->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool()) {

			RsFileHash hash( item->text(SR_HASH_COL).toStdString() );

			if (rsFiles->FileDetails(hash, RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL
			                               | RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE
                                     | RS_FILE_HINTS_SPEC_ONLY, info)) {

				/* make path for downloaded files */
				std::string path;
				path = info.path;

				/* open file with a suitable application */
				QFileInfo qinfo;
				qinfo.setFile(QString::fromUtf8(path.c_str()));
				if (qinfo.exists()) {
					if (qinfo.absoluteFilePath().endsWith(RsCollection::ExtensionString)) {
						RsCollection collection;
						if (collection.load(qinfo.absoluteFilePath())) {
							collection.downloadFiles();
							return;
						}
					}
				}
			}
		}
	}

	RsCollection collection;
	if (collection.load(this)) {
		collection.downloadFiles();
	}//if (collection.load(this))
}

void SearchDialog::downloadDirectory(const QTreeWidgetItem *item, const QString &base)
{
	if (!item->childCount()) 
	{
        std::list<RsPeerId> srcIds;

		QString path = QString::fromStdString(rsFiles->getDownloadDirectory())
                                                + "/" + base + "/";
		QString cleanPath = QDir::cleanPath(path);

        RsFileHash hash ( item->text(SR_HASH_COL).toStdString());

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
        for(std::list<RsPeerId>::const_iterator it(srcIds.begin());
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
		for (int i = 0, cnt = item->childCount(); i < cnt; ++i) {
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
void SearchDialog::searchSummaryWidgetCustomPopupMenu( QPoint /*point*/ )
{
    // block the popup if no results available
    if ((ui.searchSummaryWidget->selectedItems()).isEmpty()) return;

    QMenu contextMnu(this);

    QTreeWidgetItem* ci = ui.searchSummaryWidget->currentItem();
    QAction* action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_SEARCHAGAIN),tr("Search again"), this, SLOT(searchAgain()));
    if (!ci || ci->data(SS_DATA_COL, ROLE_KEYWORDS).toString().isEmpty()) {
        action->setDisabled(true);
    }
    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_REMOVE), tr("Remove"), this, SLOT(searchRemove()));
    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_REMOVEALL), tr("Remove All"), this, SLOT(searchRemoveAll()));
    contextMnu.addSeparator();
    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copySearchLink()));
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
			++i;
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
    QString keywords = ci->text(SS_KEYWORDS_COL);

    std::cerr << "SearchDialog::copySearchLink(): keywords: " << keywords.toStdString();
    std::cerr << std::endl;

    RetroShareLink link = RetroShareLink::createSearch(keywords);
    if (link.valid()) {
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
        connect(advSearchDialog, SIGNAL(search(RsRegularExpression::Expression*)),
                this, SLOT(advancedSearch(RsRegularExpression::Expression*)));
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
		item2->setText(SS_KEYWORDS_COL, txt);
	} else {
		item2->setText(SS_KEYWORDS_COL, txt + " (" + ui.FileTypeComboBox->itemText(fileType) + ")");
	}
	item2->setText(SS_RESULTS_COL, QString::number(0));
	item2->setData(SS_RESULTS_COL, ROLE_SORT, 0);
	item2->setText(SS_SEARCH_ID_COL, sid_hexa);
	item2->setText(SS_FILE_TYPE_COL, QString::number(fileType));

	if (!advanced)
		item2->setData(SS_DATA_COL, ROLE_KEYWORDS, txt);

	ui.searchSummaryWidget->addTopLevelItem(item2);
	ui.searchSummaryWidget->setCurrentItem(item2);
}

void SearchDialog::advancedSearch(RsRegularExpression::Expression* expression)
{
	advSearchDialog->hide();

	/* call to core */
	std::list<DirDetails> results;

	// send a turtle search request
    RsRegularExpression::LinearizedExpression e ;
	expression->linearize(e) ;

	TurtleRequestId req_id = rsFiles->turtleSearch(e) ;

	// This will act before turtle results come to the interface, thanks to the signals scheduling policy.
	initSearchResult(QString::fromStdString(e.GetStrings()),req_id, ui.FileTypeComboBox->currentIndex(), true) ;

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

    RsRegularExpression::NameExpression exprs(RsRegularExpression::ContainsAllStrings,words,true) ;
    RsRegularExpression::LinearizedExpression lin_exp ;
	exprs.linearize(lin_exp) ;

	TurtleRequestId req_id ;

	if(ui._anonF2Fsearch_CB->isChecked())
	{
		if(n==1)
			req_id = rsFiles->turtleSearch(words.front()) ;
		else
			req_id = rsFiles->turtleSearch(lin_exp) ;
	}
	else
		req_id = RSRandom::random_u32() ; // generate a random 32 bits request id

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

			for(std::list<DirDetails>::iterator resultsIter = initialResults.begin(); resultsIter != initialResults.end(); ++resultsIter)
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

			for(std::list<DirDetails>::iterator resultsIter = initialResults.begin(); resultsIter != initialResults.end(); ++resultsIter)
			{
				dd = *resultsIter;
				finalResults.push_back(dd);
			}
		}

		/* abstraction to allow reusee of tree rendering code */
		resultsToTree(keywords,req_id, finalResults);
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
		FileDetail& file = searchResultsQueue.back().second ;

#ifdef DEBUG
		std::cout << "Updating file detail:" << std::endl ;
		std::cout << "  size = " << file.size << std::endl ;
		std::cout << "  name = " << file.name << std::endl ;
		std::cout << "  s_id = " << search_id << std::endl ;
#endif

		insertFile(search_id,file);

		searchResultsQueue.pop_back() ;
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
        child->setText(SR_HASH_COL, QString::fromStdString(dir.hash.toStdString()));
        child->setText(SR_SIZE_COL, QString::number(dir.size));
        child->setData(SR_SIZE_COL, ROLE_SORT, (qulonglong) dir.size);
		child->setText(SR_AGE_COL, QString::number(dir.mtime));
		child->setData(SR_AGE_COL, ROLE_SORT, dir.mtime);
		child->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );

		child->setText(SR_SOURCES_COL, QString::number(1));
		child->setData(SR_SOURCES_COL, ROLE_SORT, 1);
		child->setTextAlignment( SR_SOURCES_COL, Qt::AlignRight );

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
        child->setText(SR_HASH_COL, QString::fromStdString(dir.hash.toStdString()));
        child->setText(SR_SIZE_COL, QString::number(dir.size));
        child->setData(SR_SIZE_COL, ROLE_SORT, (qulonglong) dir.size);
		child->setText(SR_AGE_COL, QString::number(dir.mtime));
		child->setData(SR_AGE_COL, ROLE_SORT, dir.mtime);
		child->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );
		child->setText(SR_SOURCES_COL, QString::number(1));
		child->setData(SR_SOURCES_COL, ROLE_SORT, 1);
		child->setTextAlignment( SR_SOURCES_COL, Qt::AlignRight );
		child->setText(SR_SEARCH_ID_COL, sid_hexa);
		child->setText(SR_TYPE_COL, tr("Folder"));

		if (item == NULL) {
			ui.searchResultWidget->addTopLevelItem(child);

			/* add to the summary as well */

			int items = ui.searchSummaryWidget->topLevelItemCount();
			bool found = false ;

			for(int i = 0; i < items; ++i)
			{
				if(ui.searchSummaryWidget->topLevelItem(i)->text(SS_SEARCH_ID_COL) == sid_hexa)
				{
					// increment result since every item is new
					int s = ui.searchSummaryWidget->topLevelItem(i)->text(SS_RESULTS_COL).toInt() ;
					ui.searchSummaryWidget->topLevelItem(i)->setText(SS_RESULTS_COL, QString::number(s+1));
					ui.searchSummaryWidget->topLevelItem(i)->setData(SS_RESULTS_COL, ROLE_SORT, s+1);
					found = true ;
				}
			}
			if(!found)
			{
				QTreeWidgetItem *item2 = new RSTreeWidgetItem(compareSummaryRole);
				item2->setText(SS_KEYWORDS_COL, txt);
				item2->setText(SS_RESULTS_COL, QString::number(1));
				item2->setData(SS_RESULTS_COL, ROLE_SORT, 1);
				item2->setTextAlignment( SS_RESULTS_COL, Qt::AlignRight );
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
        for (uint32_t i=0;i<dir.children.size();++i)
        {
			DirDetails details;
            rsFiles->RequestDirDetails(dir.children[i].ref, details, FileSearchFlags(0u));
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
    child->setText(SR_HASH_COL, QString::fromStdString(dir.hash.toStdString()));
    child->setText(SR_SIZE_COL, QString::number(dir.size));
    child->setData(SR_SIZE_COL, ROLE_SORT, (qulonglong) dir.size);
    child->setText(SR_AGE_COL, QString::number(dir.max_mtime));
    child->setData(SR_AGE_COL, ROLE_SORT, dir.max_mtime);
    child->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );
    child->setText(SR_SOURCES_COL, QString::number(1));
    child->setData(SR_SOURCES_COL, ROLE_SORT, 1);
    child->setTextAlignment( SR_SOURCES_COL, Qt::AlignRight );
    child->setText(SR_SEARCH_ID_COL, sid_hexa);
    child->setText(SR_TYPE_COL, tr("Folder"));

    ui.searchResultWidget->addTopLevelItem(child);

    /* add to the summary as well */

    int items = ui.searchSummaryWidget->topLevelItemCount();
    bool found = false ;

    for(int i = 0; i < items; ++i)
    {
            if(ui.searchSummaryWidget->topLevelItem(i)->text(SS_SEARCH_ID_COL) == sid_hexa)
            {
                    // increment result since every item is new
                    int s = ui.searchSummaryWidget->topLevelItem(i)->text(SS_RESULTS_COL).toInt() ;
                    ui.searchSummaryWidget->topLevelItem(i)->setText(SS_RESULTS_COL, QString::number(s+1));
                    ui.searchSummaryWidget->topLevelItem(i)->setData(SS_RESULTS_COL, ROLE_SORT, s+1);
                    found = true ;
            }
    }
    if(!found)
    {
            RSTreeWidgetItem *item2 = new RSTreeWidgetItem(compareSummaryRole);
            item2->setText(SS_KEYWORDS_COL, txt);
            item2->setText(SS_RESULTS_COL, QString::number(1));
            item2->setData(SS_RESULTS_COL, ROLE_SORT, 1);
            item2->setTextAlignment( SS_RESULTS_COL, Qt::AlignRight );
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
	for (int i = 0; i < summaryItemCount; ++i) {
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
	bool altname = false ;
	QString modifiedResult;

	QList<QTreeWidgetItem*> itms = ui.searchResultWidget->findItems(QString::fromStdString(file.hash.toStdString()),Qt::MatchExactly,SR_HASH_COL) ;

	for(auto &it : itms)
		if(it->text(SR_SEARCH_ID_COL) == sid_hexa)
		{
			int friendSource = 0;
			int anonymousSource = 0;
			QString resultCount = it->text(SR_SOURCES_COL);
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
			float fltRes = friendSource + (float)anonymousSource/1000;
			it->setText(SR_SOURCES_COL,modifiedResult);
			it->setData(SR_SOURCES_COL, ROLE_SORT, fltRes);
			QTreeWidgetItem *item = it;
			
			found = true ;
			
			if(QString::compare(it->text(SR_NAME_COL), QString::fromUtf8(file.name.c_str()), Qt::CaseSensitive)!=0)
				altname = true;

			if (!item->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool()) {
			
				FileInfo fi;
				if (rsFiles->FileDetails(file.hash, RS_FILE_HINTS_DOWNLOAD, fi))
					break;
				
				QColor foreground;

				int sources = friendSource + anonymousSource ;
				if (sources <= 0)
				{
					foreground = textColorNoSources();
				}
				else if (sources <= 10)
				{
					QColor colorLow = textColorLowSources();
					QColor colorHigh = textColorHighSources();

					float percent = (float) (sources - 1) / 10; // 100% not used here, see next else

					int red = (int) (colorLow.red() + (float) (colorHigh.red() - colorLow.red()) * percent);
					int green = (int) (colorLow.green() + (float)(colorHigh.green() - colorLow.green()) * percent);
					int blue = (int) (colorLow.blue() + (float)(colorHigh.blue() - colorLow.blue()) * percent);

					foreground = QColor(red, green, blue);
				}
				else
				{
					// > 10
					foreground = textColorHighSources();
				}

				for (int i = 0; i < item->columnCount(); ++i)
				{
					item->setData(i, Qt::ForegroundRole, foreground);
				}
			}

			if(altname)
			{
				QTreeWidgetItem *altItem = new RSTreeWidgetItem(compareResultRole);
				altItem->setText(SR_NAME_COL, QString::fromUtf8(file.name.c_str()));
				altItem->setText(SR_HASH_COL, QString::fromStdString(file.hash.toStdString()));
				setIconAndType(altItem, QString::fromUtf8(file.name.c_str()));
				altItem->setText(SR_SIZE_COL, QString::number(file.size));
				setIconAndType(altItem, QString::fromUtf8(file.name.c_str()));
				it->addChild(altItem);
			}
		}
	
	if(!found)
	{
		++nb_results[searchId] ;

		/* translate search results */
		
		QTreeWidgetItem *item = new RSTreeWidgetItem(compareResultRole);
		item->setText(SR_NAME_COL, QString::fromUtf8(file.name.c_str()));
		item->setText(SR_HASH_COL, QString::fromStdString(file.hash.toStdString()));

		setIconAndType(item, QString::fromUtf8(file.name.c_str()));

		/*
		 * to facilitate downloads we need to save the file size too
		 */

		item->setText(SR_SIZE_COL, QString::number(file.size));
		item->setData(SR_SIZE_COL, ROLE_SORT, (qulonglong) file.size);
		item->setText(SR_AGE_COL, QString::number(file.age));
		item->setData(SR_AGE_COL, ROLE_SORT, file.age);
		item->setTextAlignment( SR_SIZE_COL, Qt::AlignRight );
		int friendSource = 0;
		int anonymousSource = 0;
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
		float fltRes = friendSource + (float)anonymousSource/1000;
		item->setText(SR_SOURCES_COL,modifiedResult);
		item->setToolTip(SR_SOURCES_COL, tr("Obtained via ")+QString::fromStdString(rsPeers->getPeerName(file.id)) );
		item->setData(SR_SOURCES_COL, ROLE_SORT, fltRes);
		item->setTextAlignment( SR_SOURCES_COL, Qt::AlignRight );
		item->setText(SR_SEARCH_ID_COL, sid_hexa);

		QColor foreground;
		bool setForeground = false;

		FileInfo fi;
		if (rsFiles->FileDetails(file.hash, RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_SPEC_ONLY, fi)) {
			item->setData(SR_DATA_COL, SR_ROLE_LOCAL, true);
			foreground = textColorLocal();
			setForeground = true;
		} else {
			item->setData(SR_DATA_COL, SR_ROLE_LOCAL, false);

			int sources = item->text(SR_SOURCES_COL).toInt();
			if (sources == 1)
			{
				foreground = ui.searchResultWidget->palette().color(QPalette::Text);
				setForeground = true;
			}
		}
		if (rsFiles->FileDetails(file.hash, RS_FILE_HINTS_DOWNLOAD, fi))
		{
			//foreground = QColor(0, 128, 0); // green
			foreground = textColorDownloading();
			setForeground = true;
		}

		if (setForeground) {
			for (int i = 0; i < item->columnCount(); ++i)
			{
				item->setData(i, Qt::ForegroundRole, foreground);
			}
		}

		ui.searchResultWidget->addTopLevelItem(item);

		/* hide/show this search result */
		hideOrShowSearchResult(item);

		// only increment result when it's a new item.
		int s = ui.searchSummaryWidget->topLevelItem(summaryItemIndex)->text(SS_RESULTS_COL).toInt() ;
		ui.searchSummaryWidget->topLevelItem(summaryItemIndex)->setText(SS_RESULTS_COL, QString::number(s+1));
		ui.searchSummaryWidget->topLevelItem(summaryItemIndex)->setData(SS_RESULTS_COL, ROLE_SORT, s+1);
	}
}

void SearchDialog::resultsToTree(const QString& txt,qulonglong searchId, const std::list<DirDetails>& results)
{
	ui.searchResultWidget->setSortingEnabled(false);

	/* translate search results */

	std::list<DirDetails>::const_iterator it;
	for(it = results.begin(); it != results.end(); ++it)
		if (it->type == DIR_TYPE_FILE) {
			FileDetail fd;
			fd.id	= it->id;
			fd.name = it->name;
			fd.hash = it->hash;
			fd.path = it->path;
            fd.size = it->size;
			fd.age 	= it->mtime;
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
	for(int i = 0; i < items; ++i)
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
	item->setIcon(SR_NAME_COL, FilesDefs::getIconFromFileType(filename));
	item->setText(SR_TYPE_COL, FilesDefs::getNameFromFilename(filename));
}

void SearchDialog::copyResultLink()
{
    /* should also be able to handle multi-selection */
    QList<QTreeWidgetItem*> itemsForCopy = ui.searchResultWidget->selectedItems();
    QTreeWidgetItem * item;

    std::map<RsFileHash,RetroShareLink> url_map;

    for (auto item:itemsForCopy)
    {
        // call copy

        QString fhash = item->text(SR_HASH_COL);
        RsFileHash hash(fhash.toStdString());

        if(!hash.isNull())
        {
            std::cerr << "SearchDialog::copyResultLink() Calling set retroshare link";
            std::cerr << std::endl;

            qulonglong fsize = item->text(SR_SIZE_COL).toULongLong();
            QString fname = item->text(SR_NAME_COL);

            RetroShareLink link = RetroShareLink::createFile(fname, fsize, fhash);

            if (link.valid())
                url_map[hash] = link;
        }
    }
    QList<RetroShareLink> urls ;

    for(auto link:url_map)
    {
        std::cerr << "new link added to clipboard: " << link.second.toString().toStdString() << std::endl ;
        urls.push_back(link.second);
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
	for(int i = 0; i < items; ++i)
	{
		hideOrShowSearchResult(ui.searchResultWidget->topLevelItem(i), searchId, index);
	}
}

void SearchDialog::filterItems()
{
    int count = ui.searchResultWidget->topLevelItemCount ();
    for (int index = 0; index < count; ++index) {
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
    for (int index = 0; index < count; ++index) {
        if (filterItem(item->child(index), text, filterColumn)) {
            ++visibleChildCount;
        }
    }

    if (visible || visibleChildCount) {
        item->setHidden(false);
    } else {
        item->setHidden(true);
    }

    return (visible || visibleChildCount);
}
void SearchDialog::openFolderSearch()
{
    FileInfo info;

    QList<QTreeWidgetItem*> item =ui.searchResultWidget->selectedItems();


    if (item.at(0)->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool()){
        QString strFileID = item.at(0)->text(SR_HASH_COL);
        if (rsFiles->FileDetails(RsFileHash(strFileID.toStdString()), RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_SPEC_ONLY, info)){
            /* make path for downloaded or downloading files */
            QFileInfo qinfo;
            std::string path;
            path = info.path.substr(0,info.path.length()-info.fname.length());

            /* open folder with a suitable application */
            qinfo.setFile(QString::fromUtf8(path.c_str()));
            if (qinfo.exists() && qinfo.isDir()) {
                if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
                    std::cerr << "openFolderSearch(): can't open folder " << path << std::endl;
                }
            }
        }
    }
}
