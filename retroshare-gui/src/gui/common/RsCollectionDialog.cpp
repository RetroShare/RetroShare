/*******************************************************************************
 * gui/common/RsCollectionDialog.cpp                                           *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "gui/common/FilesDefs.h"
#include "RsCollectionDialog.h"

#include "RsCollection.h"
#include "util/misc.h"
#include "util/rsdir.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMenu>
#include <QTextEdit>
#include <QTreeView>

#define COLUMN_FILE     0
#define COLUMN_FILEPATH 1
#define COLUMN_SIZE     2
#define COLUMN_HASH     3
#define COLUMN_FILEC    4
#define COLUMN_COUNT    5
// In COLUMN_HASH (COLUMN_FILE reserved for CheckState)
#define ROLE_NAME Qt::UserRole + 1
#define ROLE_PATH Qt::UserRole + 2
#define ROLE_TYPE Qt::UserRole + 3
// In COLUMN_SIZE (don't start from 1 to read)
#define ROLE_SIZE Qt::UserRole + 4
#define ROLE_SELSIZE Qt::UserRole + 5
// In COLUMN_FILEC (don't start from 1 to read)
#define ROLE_FILEC Qt::UserRole + 6
#define ROLE_SELFILEC Qt::UserRole + 7

#define MAX_FILE_ADDED_BEFORE_ASK  500 //Number of file added in Recursive mode before asking to continue

#define IMAGE_SEARCH               ":/icons/svg/magnifying-glass.svg"

/**
 * @brief The FSMSortFilterProxyModel class sort directory before file.
 */
class FSMSortFilterProxyModel : public QSortFilterProxyModel
{
public:
	FSMSortFilterProxyModel( QObject *parent) : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(false);	// essential to avoid random crashes
    }

protected:
	virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const
	{
		QFileSystemModel *fsm = qobject_cast<QFileSystemModel*>(sourceModel());
		bool asc = (sortOrder() == Qt::AscendingOrder ? true : false) ;

		QFileInfo leftFileInfo  = fsm->fileInfo(left);
		QFileInfo rightFileInfo = fsm->fileInfo(right);

		// If Dot move in the beginning
		if (sourceModel()->data(left).toString() == ".")
			return asc;
		if (sourceModel()->data(right).toString() == ".")
			return !asc;

		// If DotAndDot move in the beginning
		if (sourceModel()->data(left).toString() == "..")
			return asc;
		if (sourceModel()->data(right).toString() == "..")
			return !asc;

		// Move dirs upper
		if (!leftFileInfo.isDir() && rightFileInfo.isDir())
			return !asc;
		if (leftFileInfo.isDir() && !rightFileInfo.isDir())
			return asc;


		/*If sorting by Size (Take real size, not Display one 10<2)*/
		if ((sortColumn()==1) && (!leftFileInfo.isDir() && !rightFileInfo.isDir())) {
			if (leftFileInfo.size() < rightFileInfo.size())
				return true;
			if (leftFileInfo.size() > rightFileInfo.size())
				return false;
		}
		/*If sorting by Date Modified (Take real date, not Display one 01-10-2014<02-01-1980)*/
		if (sortColumn()==3) {
			if (leftFileInfo.lastModified() < rightFileInfo.lastModified())
				return true;
			if (leftFileInfo.lastModified() > rightFileInfo.lastModified())
				return false;
		}
		//Columns found here:https://qt.gitorious.org/qt/qt/source/9e8abb63ba4609887d988ee15ba6daee0b01380e:src/gui/dialogs/qfilesystemmodel.cpp

		return QSortFilterProxyModel::lessThan(left, right);
	}

};

/**
 * @brief RsCollectionDialog::RsCollectionDialog
 * @param collectionFileName: Filename of RSCollection saved
 * @param creation: Open dialog as RsColl Creation or RsColl DownLoad
 * @param readOnly: Open dialog for RsColl as ReadOnly
 */
RsCollectionDialog::RsCollectionDialog(const QString& collectionFileName, RsCollectionDialogMode mode)
  : _mode(mode)
{
    RsCollection::RsCollectionErrorCode err_code;
    mCollection = new RsCollection(collectionFileName,err_code);

    if(err_code != RsCollection::RsCollectionErrorCode::NO_ERROR)
    {
        QMessageBox::information(nullptr,tr("Could not load collection file"),tr("Could not load collection file"));
        close();
    }

	ui.setupUi(this) ;
    ui._filename_TL->setText(collectionFileName);

//	uint32_t size = colFileInfos.size();
//	for(uint32_t i=0;i<size;++i)
//	{
//		const ColFileInfo &colFileInfo = colFileInfos[i];
//		_newColFileInfos.push_back(colFileInfo);
//	}

	setWindowFlags(Qt::Window); // for maximize button
	setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);

    setWindowTitle(QString("%1 - %2").arg(windowTitle()).arg(QFileInfo(collectionFileName).completeBaseName()));
	
	
    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/icons/collections.png"));

    if(_mode == DOWNLOAD)
	{
		ui.headerFrame->setHeaderText(tr("Download files"));
		ui.downloadFolder_LE->show();
		ui.downloadFolder_LB->show();
        ui.label_filename->hide();
		ui._filename_TL->hide();

		ui.downloadFolder_LE->setText(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str())) ;

		QObject::connect(ui.downloadFolder_LE,SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(openDestinationDirectoryMenu()));
		QObject::connect(ui.destinationDir_TB,SIGNAL(pressed()), this, SLOT(openDestinationDirectoryMenu()));
    }
    else
    {
        ui.headerFrame->setHeaderText(tr("Collection Editor"));
        ui.downloadFolder_LE->hide();
        ui.downloadFolder_LB->hide();
        ui.destinationDir_TB->hide();
        ui.label_filename->show();
        ui._filename_TL->show();
    }


	// 1 - add all elements to the list.

    mCollectionModel = new RsCollectionModel(*mCollection);
    ui._fileEntriesTW->setModel(mCollectionModel);

    connect(mCollectionModel,SIGNAL(sizesChanged()),this,SLOT(updateSizes()));
    updateSizes(); // forced because it's only called when the collection is changed, or when the model is created.

	bool wrong_chars = !updateList();

	// 2 - connect necessary signals/slots

	connect(ui._changeFile, SIGNAL(clicked()), this, SLOT(changeFileName()));
    connect(ui._add_PB, SIGNAL(clicked()), this, SLOT(addSelection()));
    connect(ui._addRecur_PB, SIGNAL(clicked()), this, SLOT(addSelectionRecursive()));
	connect(ui._remove_PB, SIGNAL(clicked()), this, SLOT(remove()));
	connect(ui._makeDir_PB, SIGNAL(clicked()), this, SLOT(makeDir()));
	connect(ui._removeDuplicate_CB, SIGNAL(clicked(bool)), this, SLOT(updateRemoveDuplicate(bool)));
	connect(ui._cancel_PB, SIGNAL(clicked()), this, SLOT(cancel()));
	connect(ui._save_PB, SIGNAL(clicked()), this, SLOT(save()));
	connect(ui._download_PB, SIGNAL(clicked()), this, SLOT(download()));
	connect(ui._hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));
#ifdef TO_REMOVE
    connect(ui._fileEntriesTW, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(itemChanged(QTreeWidgetItem*,int)));
#endif

	// 3 Initialize List
	_dirModel = new QFileSystemModel(this);
	_dirModel->setRootPath("/");
	_dirModel->setFilter(QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	_dirLoaded = false;
	connect(_dirModel, SIGNAL(directoryLoaded(QString)), this, SLOT(directoryLoaded(QString)));

	_tree_proxyModel = new FSMSortFilterProxyModel(this);
	_tree_proxyModel->setSourceModel(_dirModel);
	_tree_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	_tree_proxyModel->setSortRole(Qt::DisplayRole);

	ui._systemFileTW->setModel(_tree_proxyModel);
	//Selection Setup
	_selectionProxy = ui._systemFileTW->selectionModel();

	// 4 Restore Configuration
	// load settings
	processSettings(true);

	// 5 Activate button follow creationMode
    ui._changeFile->setVisible(_mode == EDIT);
    ui._makeDir_PB->setVisible(_mode == EDIT);
    ui._removeDuplicate_CB->setVisible(_mode == EDIT);
    ui._save_PB->setVisible(_mode == EDIT);
    ui._treeViewFrame->setVisible(_mode == EDIT);
    ui._download_PB->setVisible(_mode == DOWNLOAD);

#ifdef TO_REMOVE
    ui._fileEntriesTW->installEventFilter(this);
#endif
	ui._systemFileTW->installEventFilter(this);

	// 6 Add HashBox
	setAcceptDrops(true);
	ui._hashBox->setDropWidget(this);
	ui._hashBox->setAutoHide(true);
	ui._hashBox->setDefaultTransferRequestFlags(RS_FILE_REQ_ANONYMOUS_ROUTING) ;

	if(wrong_chars)
		QMessageBox::warning(NULL,tr("Bad filenames have been cleaned"),tr("Some filenames or directory names contained forbidden characters.\nCharacters <b>\",|,/,\\,&lt;,&gt;,*,?</b> will be replaced by '_'.\n Concerned files are listed in red.")) ;
}

void RsCollectionDialog::openDestinationDirectoryMenu()
{
	QMenu contextMnu( this );

	// pop a menu with existing entries and also a custom entry
	// Now get the list of existing  directories.

	std::list< SharedDirInfo> dirs ;
	rsFiles->getSharedDirectories( dirs) ;

	for (std::list<SharedDirInfo>::const_iterator it(dirs.begin());it!=dirs.end();++it){
		// Check for existence of directory name
		QFile directory( QString::fromUtf8((*it).filename.c_str())) ;

		if (!directory.exists()) continue ;
		if (!(directory.permissions() & QFile::WriteOwner)) continue ;

		contextMnu.addAction(QString::fromUtf8((*it).filename.c_str()), this, SLOT(setDestinationDirectory()))->setData(QString::fromUtf8( (*it).filename.c_str() ) ) ;
	}

    contextMnu.addAction( FilesDefs::getIconFromQtResourcePath(IMAGE_SEARCH),tr("Specify..."),this,SLOT(chooseDestinationDirectory()));

	contextMnu.exec(QCursor::pos()) ;
}

void RsCollectionDialog::setDestinationDirectory()
{
	QString dest_dir(qobject_cast<QAction*>(sender())->data().toString()) ;
	ui.downloadFolder_LE->setText(dest_dir) ;
}

void RsCollectionDialog::chooseDestinationDirectory()
{
	QString dest_dir = QFileDialog::getExistingDirectory(this,tr("Choose directory")) ;

	if(dest_dir.isNull())
		return ;

	ui.downloadFolder_LE->setText(dest_dir) ;
}
/**
 * @brief RsCollectionDialog::~RsCollectionDialog
 */
RsCollectionDialog::~RsCollectionDialog()
{
	// save settings
	processSettings(false);
}

/**
 * @brief RsCollectionDialog::eventFilter: Proccess event in object
 * @param obj: object where event occured
 * @param event: event occured
 * @return If we don't have to process event in parent.
 */
#ifdef TODO_COLLECTION
bool RsCollectionDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui._fileEntriesTW) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent && (keyEvent->key() == Qt::Key_Space)) {
				// Space pressed

				// get state of current item
				QTreeWidgetItem *item = ui._fileEntriesTW->currentItem();
				if (item) {
					Qt::CheckState checkState = (item->checkState(COLUMN_FILE) == Qt::Checked) ? Qt::Unchecked : Qt::Checked;

					// set state of all selected items
					QList<QTreeWidgetItem*> selectedItems = ui._fileEntriesTW->selectedItems();
					QList<QTreeWidgetItem*>::iterator it;
					for (it = selectedItems.begin(); it != selectedItems.end(); ++it) {
						if ((*it)->checkState(COLUMN_FILE) != checkState)
							(*it)->setCheckState(COLUMN_FILE, checkState);
					}
				}

				return true; // eat event
			}

			if (keyEvent && (keyEvent->key() == Qt::Key_Delete)) {
				// Delete pressed
				remove();
				return true; // eat event
			}

			if (keyEvent && (keyEvent->key() == Qt::Key_Plus)) {
				// Plus pressed
				makeDir();
				return true; // eat event
			}

		}
	}

	if (obj == ui._systemFileTW) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent && ((keyEvent->key() == Qt::Key_Enter)
			                 || keyEvent->key() == Qt::Key_Return)) {
				// Enter pressed
				if (keyEvent->modifiers() == Qt::ShiftModifier)
					addRecursive();
				else if(keyEvent->modifiers() == Qt::NoModifier) {
					add();
				}

				return true; // eat event
			}
		}
	}

	// pass the event on to the parent class
	return QDialog::eventFilter(obj, event);
}
#endif

/**
 * @brief RsCollectionDialog::processSettings
 * @param bLoad: Load or Save dialog's settings
 */
void RsCollectionDialog::processSettings(bool bLoad)
{
    Settings->beginGroup("RsCollectionDialogV2");

	if (bLoad) {
		// load settings

        if(_mode == EDIT){
			// Load windows geometrie
			restoreGeometry(Settings->value("WindowGeometrie_CM").toByteArray());
			// Load splitters state
			ui._mainSplitter->restoreState(Settings->value("MainSplitterState_CM").toByteArray());
			ui._listSplitter->restoreState(Settings->value("ListSplitterState_CM").toByteArray());
			// Load system file header configuration
			ui._systemFileTW->header()->restoreState(Settings->value("SystemFileHeader_CM").toByteArray());
			// Load file entries header configuration
			ui._fileEntriesTW->header()->restoreState(Settings->value("FileEntriesHeader_CM").toByteArray());
		} else {
			// Load windows geometrie
			restoreGeometry(Settings->value("WindowGeometrie").toByteArray());
			// Load splitters state
			ui._mainSplitter->restoreState(Settings->value("MainSplitterState").toByteArray());
			ui._listSplitter->restoreState(Settings->value("ListSplitterState").toByteArray());
			// Load system file header configuration
			ui._systemFileTW->header()->restoreState(Settings->value("SystemFileHeader").toByteArray());
			// Load file entries header configuration
			ui._fileEntriesTW->header()->restoreState(Settings->value("FileEntriesHeader").toByteArray());
		}
	} else {
        if(_mode == EDIT){
			// Save windows geometrie
			Settings->setValue("WindowGeometrie_CM",saveGeometry());
			// Save splitters state
			Settings->setValue("MainSplitterState_CM", ui._mainSplitter->saveState());
			Settings->setValue("ListSplitterState_CM", ui._listSplitter->saveState());
			// Save system file header configuration
			Settings->setValue("SystemFileHeader_CM", ui._systemFileTW->header()->saveState());
			// Save file entries header configuration
			Settings->setValue("FileEntriesHeader_CM", ui._fileEntriesTW->header()->saveState());
		} else {
			// Save windows geometrie
			Settings->setValue("WindowGeometrie",saveGeometry());
			// Save splitter state
			Settings->setValue("MainSplitterState", ui._mainSplitter->saveState());
			Settings->setValue("ListSplitterState", ui._listSplitter->saveState());
			// Save system file header configuration
			Settings->setValue("SystemFileHeader", ui._systemFileTW->header()->saveState());
			// Save file entries header configuration
			Settings->setValue("FileEntriesHeader", ui._fileEntriesTW->header()->saveState());
		}
	}

	Settings->endGroup();
}

#ifdef TO_REMOVE
/**
 * @brief RsCollectionDialog::getRootItem: Create the root Item if not existing
 * @return: the root item
 */
QTreeWidgetItem* RsCollectionDialog::getRootItem()
{
	return ui._fileEntriesTW->invisibleRootItem();
}
#endif

/**
 * @brief RsCollectionDialog::updateList: Update list of item in RsCollection
 * @return If at least one item have a Wrong Char
 */
bool RsCollectionDialog::updateList()
{
#ifdef TODO_COLLECTION
	bool wrong_chars = false ;
	wrong_chars = addChild(getRootItem(), _newColFileInfos);

	_newColFileInfos.clear();

	ui._filename_TL->setText(_fileName) ;
	for (int column = 0; column < ui._fileEntriesTW->columnCount(); ++column) {
		ui._fileEntriesTW->resizeColumnToContents(column);
	}

	updateSizes() ;

	return !wrong_chars;
#endif
    return true;
}

#ifdef TO_REMOVE
/**
 * @brief RsCollectionDialog::addChild: Add Child Item in list
 * @param parent: Parent Item
 * @param child: Child ColFileInfo item to add
 * @param total_size: Saved total size of children to save in parent
 * @param total_files: Saved total file of children to save in parent
 * @return If at least one item have a Wrong Char
 */
bool RsCollectionDialog::addChild(QTreeWidgetItem* parent, const std::vector<ColFileInfo>& child)
{
	bool wrong_chars = false ;

	uint32_t childCount = child.size();
	for(uint32_t i=0; i<childCount; ++i)
	{
		const ColFileInfo &colFileInfo = child[i];

		QList<QTreeWidgetItem*> founds;
		QList<QTreeWidgetItem*> parentsFounds;
		parentsFounds = ui._fileEntriesTW->findItems(colFileInfo.path , Qt::MatchExactly | Qt::MatchRecursive, COLUMN_FILEPATH);
		if (colFileInfo.type == DIR_TYPE_DIR){
			founds = ui._fileEntriesTW->findItems(colFileInfo.path + "/" +colFileInfo.name, Qt::MatchExactly | Qt::MatchRecursive, COLUMN_FILEPATH);
		} else {
			founds = ui._fileEntriesTW->findItems(colFileInfo.path + "/" +colFileInfo.name, Qt::MatchExactly | Qt::MatchRecursive, COLUMN_FILEPATH);
			if (ui._removeDuplicate_CB->isChecked()) {
				founds << ui._fileEntriesTW->findItems(colFileInfo.hash, Qt::MatchExactly | Qt::MatchRecursive, COLUMN_HASH);
			}
		}
		if (founds.empty()) {
			QTreeWidgetItem *item = new QTreeWidgetItem;

			//item->setFlags(Qt::ItemIsUserCheckable | item->flags());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsTristate);
			item->setCheckState(COLUMN_FILE, Qt::Checked);
			item->setText(COLUMN_FILE, colFileInfo.name);
			item->setText(COLUMN_FILEPATH, colFileInfo.path + "/" + colFileInfo.name);
			item->setText(COLUMN_HASH, colFileInfo.hash);
			item->setData(COLUMN_HASH, ROLE_NAME, colFileInfo.name);
			item->setData(COLUMN_HASH, ROLE_PATH, colFileInfo.path);
			item->setData(COLUMN_HASH, ROLE_TYPE, colFileInfo.type);
			QFont font = item->font(COLUMN_FILE);
			if (colFileInfo.type==DIR_TYPE_DIR) {
				item->setToolTip(COLUMN_FILE,tr("This is a directory. Double-click to expand it."));
				font.setBold(true);
				//Size calculated after for added child after init
				item->setText(COLUMN_SIZE, misc::friendlyUnit(0));
				item->setToolTip(COLUMN_SIZE,tr("Real Size: Waiting child..."));
				item->setData(COLUMN_SIZE, ROLE_SIZE, 0);
				item->setData(COLUMN_SIZE, ROLE_SELSIZE, 0);

				item->setText(COLUMN_FILEC, "");
				item->setToolTip(COLUMN_FILEC, tr("Real File Count: Waiting child..."));
				item->setData(COLUMN_FILEC, ROLE_FILEC, 0);
				item->setData(COLUMN_FILEC, ROLE_SELFILEC, 0);
			} else {
				font.setBold(false);
				item->setText(COLUMN_SIZE, misc::friendlyUnit(colFileInfo.size));
				item->setToolTip(COLUMN_SIZE, tr("Real Size=%1").arg(misc::friendlyUnit(colFileInfo.size)));
				item->setData(COLUMN_SIZE, ROLE_SIZE, colFileInfo.size);
				item->setData(COLUMN_SIZE, ROLE_SELSIZE, colFileInfo.size);

				item->setText(COLUMN_FILEC, "1");
				item->setToolTip(COLUMN_FILEC, tr("Real File Count=%1").arg(1));
				item->setData(COLUMN_FILEC, ROLE_FILEC, 1);
				item->setData(COLUMN_FILEC, ROLE_SELFILEC, 1);
			}
			item->setFont(COLUMN_FILE, font);
			item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);

			if (colFileInfo.filename_has_wrong_characters)
			{
				//TODO (Phenom): Add qproperty for these text colors in stylesheets
				wrong_chars = true ;
				item->setData(COLUMN_FILE, Qt::ForegroundRole, QColor(255,80,120)) ;
			}

			if (parentsFounds.empty()) {
				parent->addChild(item);
			} else {
				parentsFounds.at(0)->addChild(item);
			}

			if (colFileInfo.type == DIR_TYPE_FILE) {
				//update parents size only for file children
				QTreeWidgetItem *itemParent = item->parent();
				if (itemParent) {
					while (itemParent) {
						qulonglong parentSize = itemParent->data(COLUMN_SIZE, ROLE_SIZE).toULongLong() + colFileInfo.size;
						itemParent->setData(COLUMN_SIZE, ROLE_SIZE, parentSize);
						itemParent->setToolTip(COLUMN_SIZE, tr("Real Size=%1").arg(misc::friendlyUnit(parentSize)));
						qulonglong parentSelSize = itemParent->data(COLUMN_SIZE, ROLE_SELSIZE).toULongLong() + colFileInfo.size;
						itemParent->setData(COLUMN_SIZE, ROLE_SELSIZE, parentSelSize);
						itemParent->setText(COLUMN_SIZE, misc::friendlyUnit(parentSelSize));

						qulonglong parentFileCount = itemParent->data(COLUMN_FILEC, ROLE_FILEC).toULongLong() + 1;
						itemParent->setData(COLUMN_FILEC, ROLE_FILEC, parentFileCount);
						itemParent->setToolTip(COLUMN_FILEC, tr("Real File Count=%1").arg(parentFileCount));
						qulonglong parentSelFileCount = itemParent->data(COLUMN_FILEC, ROLE_SELFILEC).toULongLong() + 1;
						itemParent->setData(COLUMN_FILEC, ROLE_SELFILEC, parentSelFileCount);
						itemParent->setText(COLUMN_FILEC, QString("%1").arg(parentSelFileCount));

						itemParent = itemParent->parent();
					}
				}
			}

			founds.push_back(item);
		}

		if (!founds.empty()) {

			if (colFileInfo.type == DIR_TYPE_DIR) {
				wrong_chars |= addChild(founds.at(0), colFileInfo.children);
			}
		}
	}
	return wrong_chars;
}
#endif

/**
 * @brief RsCollectionDialog::directoryLoaded: called when ui._treeView have load an directory as QFileSystemModel don't load all in one time
 * @param dirLoaded: Name of directory just loaded
 */
void RsCollectionDialog::directoryLoaded(QString dirLoaded)
{
    if(!_dirLoaded)
    {

        QFileInfo lastDir = Settings->getLastDir(RshareSettings::LASTDIR_EXTRAFILE);
        if (lastDir.absoluteFilePath() == "") return;
        if (lastDir.absoluteFilePath() == dirLoaded) _dirLoaded = true;

        // Check all parent directory to see if it is loaded
        //as QFileSystemModel don't load all
        do {
            if(lastDir.absolutePath()==dirLoaded) {
                //Only expand loaded directory
                QModelIndex lastDirIdx = _dirModel->index(lastDir.absoluteFilePath());
                //Select LASTDIR_EXTRAFILE or last parent found when loaded
                if (!lastDirIdx.isValid()){
                    _dirLoaded = true;
                    break;
                }

                //Ask dirModel to load next parent directory
                while (_dirModel->canFetchMore(lastDirIdx)) _dirModel->fetchMore(lastDirIdx);

                //Ask to Expand last loaded parent
                lastDirIdx = _tree_proxyModel->mapFromSource(lastDirIdx);
                if (lastDirIdx.isValid()){
                    ui._systemFileTW->expand(lastDirIdx);
                    ui._systemFileTW->setCurrentIndex(lastDirIdx);
                }
                break;
            }
            //std::cerr << "lastDir = " << lastDir.absoluteFilePath().toStdString() << std::endl;

            QDir c = lastDir.dir() ;
            if(!c.cdUp())
                break ;

            lastDir = QFileInfo(c.path());
        } while (true); //(lastDir.absoluteFilePath() != lastDir.absolutePath());
    }
}

/**
 * @brief RsCollectionDialog::updateSizes: Update Column size of ui._fileEntriesTW)
 */
void RsCollectionDialog::updateSizes()
{
    ui._selectedFiles_TL->setText(QString::number(mCollectionModel->totalSelected()));
    ui._totalSize_TL->setText(misc::friendlyUnit(mCollectionModel->totalSize()));
}

/**
 * @brief purifyFileName: Check if name of file is good or not.
 * @param input
 * @param bad
 * @return
 */
static QString purifyFileName(const QString& input, bool& bad)
{
	static const QString bad_chars = "/\\\"*:?<>|" ;
	bad = false ;
	QString output = input ;

	for(int i=0;i<output.length();++i)
		for(int j=0;j<bad_chars.length();++j)
			if(output[i] == bad_chars[j])
			{
				output[i] = '_' ;
				bad = true ;
			}

	return output ;
}

/**
 * @brief RsCollectionDialog::changeFileName: Change the name of saved file
 */
void RsCollectionDialog::changeFileName()
{
	QString fileName;
	if(!misc::getSaveFileName(this, RshareSettings::LASTDIR_EXTRAFILE
														, QApplication::translate("RsCollectionFile", "Create collection file")
														, QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollection::ExtensionString + ")"
														, fileName,0, QFileDialog::DontConfirmOverwrite))
		return;

	if (!fileName.endsWith("." + RsCollection::ExtensionString))
		fileName += "." + RsCollection::ExtensionString ;

	std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

	QFile file(fileName) ;
    RsCollection::RsCollectionErrorCode err;

    if(file.exists() && RsCollection::checkFile(fileName,err))
    {
		QMessageBox mb;
		mb.setText(tr("Save Collection File."));
		mb.setInformativeText(tr("File already exists.")+"\n"+tr("What do you want to do?"));
		QAbstractButton *btnOwerWrite = mb.addButton(tr("Overwrite"), QMessageBox::YesRole);
        QAbstractButton *btnMerge     = mb.addButton(tr("Merge"),     QMessageBox::NoRole);
        QAbstractButton *btnCancel    = mb.addButton(tr("Cancel"),    QMessageBox::ResetRole);
		mb.setIcon(QMessageBox::Question);
		mb.exec();

		if (mb.clickedButton()==btnOwerWrite) {
			//Nothing to do
        }
        else if(mb.clickedButton()==btnMerge)
        {
			//Open old file to merge it with RsCollection

            RsCollection qddOldFileCollection(fileName,err);

            if(err != RsCollection::RsCollectionErrorCode::NO_ERROR)
            {
                mCollectionModel->preMods();
                mCollection->merge_in(qddOldFileCollection.fileTree());
                mCollectionModel->postMods();
            }
        }
        else if(mb.clickedButton()==btnCancel)
			return;
        else
			return;

    }
    else
    {
		//create a new empty file to check if name if good.
        if (!file.open(QFile::WriteOnly))
            return;
		file.remove();
	}

    ui._filename_TL->setText(fileName);

	updateList();
}

/**
 * @brief RsCollectionDialog::add:  */
void RsCollectionDialog::addSelection()
{
    addSelection(false);
}

/**
 * @brief RsCollectionDialog::addRecursive:
 */
void RsCollectionDialog::addSelectionRecursive()
{
    addSelection(true);
}

static void recursBuildFileTree(const QString& path,RsFileTree& tree,RsFileTree::DirIndex dir_index,bool recursive,std::map<QString,RsFileHash>& paths_to_hash)
{
    QFileInfo fileInfo = path;

    if (fileInfo.isDir())
    {
        auto di = tree.addDirectory(dir_index,fileInfo.fileName().toUtf8().constData());

        if(recursive)
        {
            QDir dirParent = fileInfo.absoluteFilePath();
            dirParent.setFilter(QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            QFileInfoList childrenList = dirParent.entryInfoList();

            for(QFileInfo f:childrenList)
                recursBuildFileTree(f.absoluteFilePath(),tree,di,recursive,paths_to_hash);
        }
    }
    else
    {
        // Here we use a temporary hash that serves two purposes:
        // 1 - identify the file in the RsFileTree of the collection so that we can update its hash when calculated
        // 2 - mark the file as being processed
        // The hash s is computed to be the hash of the path of the file. The collection must take care of multiple instances.

        Sha1CheckSum s = RsDirUtil::sha1sum((uint8_t*)(fileInfo.filePath().toUtf8().constData()),fileInfo.filePath().toUtf8().size());

        tree.addFile(dir_index,fileInfo.fileName().toUtf8().constData(),s,fileInfo.size());

        paths_to_hash.insert(std::make_pair(fileInfo.filePath(),s));
    }
}
/**
 * @brief RsCollectionDialog::addRecursive: Add Selected item to RSCollection
 *   -Add File seperatly if parent folder not selected
 *   -Add File in folder if selected
 *   -Get root folder the selected one
 * @param recursive: If true, add all selected directory childrens
 */
void RsCollectionDialog::addSelection(bool recursive)
{
	QMap<QString, QString > dirToAdd;
    int count=0;//to not scan all items on list .count()

	QModelIndexList milSelectionList =	ui._systemFileTW->selectionModel()->selectedIndexes();

    mCollectionModel->preMods();

    std::map<QString,RsFileHash> paths_to_hash;	// sha1sum of the paths to hash.

	foreach (QModelIndex index, milSelectionList)
        if(index.column()==0)    //Get only FileName
        {
            RsFileTree tree;
            recursBuildFileTree(_dirModel->filePath(_tree_proxyModel->mapToSource(index)),tree,tree.root(),recursive,paths_to_hash);

            mCollection->merge_in(tree);

#ifdef TO_REMOVE
			QFileInfo fileInfo = filePath;
			if (fileInfo.isDir()) {
				dirToAdd.insert(fileInfo.absoluteFilePath(),fileInfo.absolutePath());
				++count;
				if (recursive) {
					if (!addAllChild(fileInfo, dirToAdd, fileToHash, count)) return;
				} else {
					continue;
				}
			}
			if (fileInfo.isFile()){
				fileToHash.append(fileInfo.absoluteFilePath());
				++count;
				if (dirToAdd.contains(fileInfo.absolutePath()))
					_listOfFilesAddedInDir.insert(fileInfo.absoluteFilePath(),fileInfo.absolutePath());
				else
					_listOfFilesAddedInDir.insert(fileInfo.absoluteFilePath(),"");
			}
#endif
		}

#ifdef TO_REMOVE
	// Process Dirs
	QTreeWidgetItem *item = NULL;
	if (!ui._fileEntriesTW->selectedItems().empty())
		item= ui._fileEntriesTW->selectedItems().at(0);
	if (item) {
		while (item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR) {
			item = item->parent();//Only Dir as Parent
		}
	}

	int index = 0;
	while (index < dirToAdd.count())
	{
		ColFileInfo root;
		if (item && (item != getRootItem())) {
			root.name = "";
			root.path = item->text(COLUMN_FILEPATH);
		} else {
			root.name = "";
			root.path = "";
		}
		//QMap is ordered, so we get parent before child
		//Iterator is moved inside this function
		processItem(dirToAdd, index, root);
	}

	//Update liste before attach files to be sure when file is hashed, parent directory exists.
	updateList();

	for (QHash<QString,QString>::Iterator it = _listOfFilesAddedInDir.begin(); it != _listOfFilesAddedInDir.end() ; ++it)
	{
		QString path = it.value();
		//it.value() = "";//Don't reset value, could be an older attachment not terminated.
		if (dirToAdd.contains(path)){
			it.value() = dirToAdd.value(path);
		} else if(item) {
			if (item->data(COLUMN_HASH, ROLE_NAME) != "") {
				it.value() = item->text(COLUMN_FILEPATH);
			}
		}
	}

	// Process Files once all done
	ui._hashBox->addAttachments(fileToHash,RS_FILE_REQ_ANONYMOUS_ROUTING /*, 0*/);
#endif

    mFilesBeingHashed.insert(paths_to_hash.begin(),paths_to_hash.end());

    QStringList paths;
    std::list<RsFileHash> hashes;

    for(auto it:paths_to_hash)
    {
        paths.push_back(it.first);
        hashes.push_back(it.second);

        std::cerr << "Setting file has being hased: ID=" << it.second << " - " << it.first.toUtf8().constData() << std::endl;
    }

    mCollectionModel->notifyFilesBeingHashed(hashes);
    mCollectionModel->postMods();

    ui._hashBox->addAttachments(paths,RS_FILE_REQ_ANONYMOUS_ROUTING /*, 0*/);

    if(!mFilesBeingHashed.empty())
    {
        ui._save_PB->setToolTip(tr("Please wait for all files to be properly processed before saving."));
        ui._save_PB->setEnabled(false);
    }
}

#ifdef TO_REMOVE
/**
 * @brief RsCollectionDialog::addAllChild: Add children to RsCollection
 * @param fileInfoParent: Parent's QFileInfo to scan
 * @param dirToAdd: QMap where directories are added
 * @param fileToHash: QStringList where files are added
 * @return false if too many items is selected and aborted
 */
bool RsCollectionDialog::addAllChild(QFileInfo &fileInfoParent
                                     , QMap<QString, QString > &dirToAdd
                                     , QStringList &fileToHash
                                     , int &count)
{
	//Save count only first time to not scan all items on list .count()
	if (count == 0) count = (dirToAdd.count() + fileToHash.count());
	QDir dirParent = fileInfoParent.absoluteFilePath();
	dirParent.setFilter(QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	QFileInfoList childrenList = dirParent.entryInfoList();
	foreach (QFileInfo fileInfo, childrenList)
	{
		if (count == MAX_FILE_ADDED_BEFORE_ASK)
		{
			QMessageBox msgBox;
			msgBox.setText(tr("Warning, selection contains more than %1 items.").arg(MAX_FILE_ADDED_BEFORE_ASK));
			msgBox.setInformativeText("Do you want to continue?");
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			msgBox.setDefaultButton(QMessageBox::No);
			int ret = msgBox.exec();
			switch (ret) {
				case QMessageBox::Yes:
				break;
				case QMessageBox::No:
					return false;
				break;
				break;
				default:
					// should never be reached
				break;
			}
		}
		if (fileInfo.isDir()) {
			dirToAdd.insert(fileInfo.absoluteFilePath(),fileInfo.absolutePath());
			++count;
			if (!addAllChild(fileInfo, dirToAdd, fileToHash, count)) return false;
		}
		if (fileInfo.isFile()){
			fileToHash.append(fileInfo.absoluteFilePath());
			++count;
			if (dirToAdd.contains(fileInfo.absolutePath()))
				_listOfFilesAddedInDir.insert(fileInfo.absoluteFilePath(),fileInfo.absolutePath());
			else
				_listOfFilesAddedInDir.insert(fileInfo.absoluteFilePath(),"");
		}
	}
	return true;
}
#endif

/**
 * @brief RsCollectionDialog::remove: Remove selected Items in RSCollection
 */
void RsCollectionDialog::remove()
{
    QMap<QString, QString > dirToRemove;
    int count=0;//to not scan all items on list .count()

    QModelIndexList milSelectionList =	ui._fileEntriesTW->selectionModel()->selectedIndexes();

    mCollectionModel->preMods();

    foreach (QModelIndex index, milSelectionList)
        if(index.column()==0)    //Get only FileName
        {
            auto indx = mCollectionModel->getIndex(index);
            auto parent_indx = mCollectionModel->getIndex(index.parent());

            if(indx.is_file)
                mCollection->removeFile(indx.index,parent_indx.index);
            else
                mCollection->removeDirectory(indx.index,parent_indx.index);
        }

    mCollectionModel->postMods();

#ifdef TODO_COLLECTION
	bool removeOnlyFile=false;
	QString listDir;
	// First, check if selection contains directories
	for (int curs = 0; curs < ui._fileEntriesTW->selectedItems().count(); ++curs)
	{// Have to call ui._fileEntriesTW->selectedItems().count() each time as selected could change
		QTreeWidgetItem *item = NULL;
		item= ui._fileEntriesTW->selectedItems().at(curs);

		//Uncheck child directory item if parent is checked
		if (item != getRootItem()){
			if (item->data(COLUMN_HASH, ROLE_TYPE).toUInt() == DIR_TYPE_DIR) {
				QString path = item->data(COLUMN_HASH, ROLE_PATH).toString();
				if (listDir.contains(path) && !path.isEmpty()) {
					item->setSelected(false);
				} else {
					listDir += item->data(COLUMN_HASH, ROLE_NAME).toString() +"<br>";
				}
			}
		}
	}

	//If directories, ask to remove them or not
	if (!listDir.isEmpty()){
		QMessageBox* msgBox = new QMessageBox(QMessageBox::Information, "", "");
		msgBox->setText("Warning, selection contains directories.");
		//msgBox->setInformativeText(); If text too long, no scroll, so I add an text edit
		QGridLayout* layout = qobject_cast<QGridLayout*>(msgBox->layout());
		if (layout) {
			int newRow = 1;
			for (int row = layout->count()-1; row >= 0; --row) {
				for (int col = layout->columnCount()-1; col >= 0; --col) {
					QLayoutItem *item = layout->itemAtPosition(row, col);
					if (item) {
						int index = layout->indexOf(item->widget());
						int r=0, c=0, rSpan=0, cSpan=0;
						layout->getItemPosition(index, &r, &c, &rSpan, &cSpan);
						if (r>0) {
							layout->removeItem(item);
							layout->addItem(item, r+3, c, rSpan, cSpan);
						} else if (rSpan>1) {
							newRow = rSpan + 1;
						}
					}
				}
			}
			QLabel *label = new QLabel(tr("Do you want to remove them and all their children, too?"));
			layout->addWidget(label,newRow, 0, 1, layout->columnCount(), Qt::AlignHCenter );
			QTextEdit *edit = new QTextEdit(listDir);
			edit->setReadOnly(true);
			edit->setWordWrapMode(QTextOption::NoWrap);
			layout->addWidget(edit,newRow+1, 0, 1, layout->columnCount(), Qt::AlignHCenter );
		}

		msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		msgBox->setDefaultButton(QMessageBox::Yes);
		int ret = msgBox->exec();
		switch (ret) {
			case QMessageBox::Yes:
			break;
			case QMessageBox::No:
				removeOnlyFile = true;
			break;
			case QMessageBox::Cancel: {
				delete msgBox;
				return;
			}
			break;
			default:
				// should never be reached
			break;
		}
		delete msgBox;
	}

	//Remove wanted items
	int leftItem = 0;
	// Have to call ui._fileEntriesTW->selectedItems().count() each time as selected change
	while (ui._fileEntriesTW->selectedItems().count() > leftItem) {
		QTreeWidgetItem *item = ui._fileEntriesTW->selectedItems().at(leftItem);

		if (item != getRootItem()){
			if (!removeItem(item, removeOnlyFile)) {
				++leftItem;
			}
		} else {
			//Get Root change index
			++leftItem;
		}
	}

	updateSizes() ;
#endif
}

#ifdef TODO_COLLECTION
bool RsCollectionDialog::removeItem(QTreeWidgetItem *item, bool &removeOnlyFile)
{
	if (item){
		if ((item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR) || !removeOnlyFile) {
			int leftItem = 0;
			while (item->childCount() > leftItem) {
				if (!removeItem(item->child(0), removeOnlyFile)) {
					++leftItem;
				}
			}
			if (leftItem == 0) {
				//First uncheck item to update parent informations
				item->setCheckState(COLUMN_FILE,Qt::Unchecked);
				QTreeWidgetItem *parent = item->parent();
				if (parent) {
					parent->removeChild(item);
				} else {
					getRootItem()->removeChild(item);
				}
				return true;
			} else {
				if (!removeOnlyFile) {
					std::cerr << "(EE) RsCollectionDialog::removeItem This could never happen." << std::endl;
				}
			}
		}
	}
	return false;
}
#endif

#ifdef TO_REMOVE
/** Process each item to make a new RsCollection item */
void RsCollectionDialog::processItem(QMap<QString, QString> &dirToAdd
                                     , int &index
                                     , ColFileInfo &parent
                                     )
{
	ColFileInfo newChild;
	int count = dirToAdd.count();
	if (index < count) {
		QString key=dirToAdd.keys().at(index);
		bool bad_chars_detected = false;
		QFileInfo fileInfo=key;
		QString cleanDirName = purifyFileName(fileInfo.fileName(),bad_chars_detected);
		newChild.name = cleanDirName;
		newChild.filename_has_wrong_characters = bad_chars_detected;
		newChild.size = fileInfo.isDir()? 0: fileInfo.size();
		newChild.type = fileInfo.isDir()? DIR_TYPE_DIR: DIR_TYPE_FILE ;
		if (parent.name != "") {
			newChild.path = parent.path + "/" + parent.name;
		} else {
			newChild.path = parent.path;
		}
		dirToAdd[key] = newChild.path + "/" + newChild.name;
		//Move to next item
		++index;
		if (index < count){
			QString newKey = dirToAdd.keys().at(index);
			while ((dirToAdd.value(newKey) == key)
			       && (index < count)) {
				processItem(dirToAdd, index, newChild);
				if (index < count)newKey = dirToAdd.keys().at(index);
			}
		}

		//Save parent when child are processed
		if (parent.name != "") {
			parent.children.push_back(newChild);
			parent.size += newChild.size;
		} else {
			_newColFileInfos.push_back(newChild);
		}
	}
}
#endif

/**
 * @brief RsCollectionDialog::addDir: Add new empty dir to list
 */
void RsCollectionDialog::makeDir()
{
    QModelIndexList selected_indices = ui._fileEntriesTW->selectionModel()->selectedIndexes();

    if(selected_indices.size() > 1)
    {
        QMessageBox::information(nullptr,tr("Too many places selected"),tr("Please select at most one directory where to create the new folder"));
        return;
    }

    QModelIndex place_index;

    if(!selected_indices.empty())
        place_index = selected_indices.first();

    RsCollectionModel::EntryIndex e = mCollectionModel->getIndex(place_index);

    if(e.is_file)
    {
        QMessageBox::information(nullptr,tr("Selected place cannot be a file"),tr("Please select at most one directory where to create the new folder"));
        return;
    }
    QString childName = QInputDialog::getText(this, tr("New Directory"), tr("Enter the new directory's name"), QLineEdit::Normal);

    mCollectionModel->preMods();
    mCollection->merge_in(*RsFileTree::fromDirectory(childName.toUtf8().constData()),e.index);
    mCollectionModel->postMods();

#ifdef TODO_COLLECTION
	QString childName="";
	bool ok, badChar, nameOK = false;
	// Ask for name
	while (!nameOK)
	{
		childName = QInputDialog::getText(this, tr("New Directory")
		                                  , tr("Enter the new directory's name")
		                                  , QLineEdit::Normal, childName, &ok);
		if (ok && !childName.isEmpty())
		{
			childName = purifyFileName(childName, badChar);
			nameOK = !badChar;
			if (badChar)
			{
				QMessageBox msgBox;
				msgBox.setText("The name contains bad characters.");
				msgBox.setInformativeText("Do you want to use the corrected one?\n" + childName);
				msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Retry | QMessageBox::Cancel);
				msgBox.setDefaultButton(QMessageBox::Ok);
				int ret = msgBox.exec();
				switch (ret) {
					case QMessageBox::Ok:
						nameOK = true;
					break;
					case QMessageBox::Retry:
					break;
					case QMessageBox::Cancel:
						return;
					break;
					default:
						// should never be reached
					break;
				}
			}
		} else {//if (ok && !childName.isEmpty())
			return;
		}

	}


	QList<QTreeWidgetItem*> selected = ui._fileEntriesTW->selectedItems();

	if(selected.empty())
	{
		QTreeWidgetItem *item = getRootItem();

		ColFileInfo newChild;
		newChild.name = childName;
		newChild.filename_has_wrong_characters = false;
		newChild.size = 0;
		newChild.type = DIR_TYPE_DIR;
		newChild.path = item->data(COLUMN_HASH, ROLE_PATH).toString()
		        + "/" + item->data(COLUMN_HASH, ROLE_NAME).toString();
		if (item == getRootItem()) newChild.path = "";

		_newColFileInfos.push_back(newChild);
	}
	else
		for(auto it(selected.begin());it!=selected.end();++it)
		{
			QTreeWidgetItem *item = *it;

			while(item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR)
				item = item->parent();//Only Dir as Parent

			ColFileInfo newChild;
			newChild.name = childName;
			newChild.filename_has_wrong_characters = false;
			newChild.size = 0;
			newChild.type = DIR_TYPE_DIR;

			if (item == getRootItem())
				newChild.path = "";
			else
				newChild.path = item->data(COLUMN_HASH, ROLE_PATH).toString() + "/" + item->data(COLUMN_HASH, ROLE_NAME).toString();

			_newColFileInfos.push_back(newChild);
		}

	updateList();
#endif
}

/**
 * @brief RsCollectionDialog::fileHashingFinished: Connected to ui._hashBox.fileHashingFinished
 *  Add finished File to collection in respective directory
 * @param hashedFiles: List of the file finished
 */
void RsCollectionDialog::fileHashingFinished(QList<HashedFile> hashedFiles)
{
#ifdef TO_REMOVE
	std::cerr << "RsCollectionDialog::fileHashingFinished() started." << std::endl;

	QString message;

	QList<HashedFile>::iterator it;
	for (it = hashedFiles.begin(); it != hashedFiles.end(); ++it) {
		HashedFile& hashedFile = *it;

		ColFileInfo colFileInfo;
		colFileInfo.name=hashedFile.filename;
		colFileInfo.path="";
		colFileInfo.size=hashedFile.size;
		colFileInfo.hash=QString::fromStdString(hashedFile.hash.toStdString());
		colFileInfo.filename_has_wrong_characters=false;
		colFileInfo.type=DIR_TYPE_FILE;

		if(_listOfFilesAddedInDir.value(hashedFile.filepath,"")!="") {
			//File Added in directory, find its parent
			colFileInfo.path = _listOfFilesAddedInDir.value(hashedFile.filepath,"");
			_listOfFilesAddedInDir.remove(hashedFile.filepath);
		}
#ifdef TODO_COLLECTION
		_newColFileInfos.push_back(colFileInfo);
#endif
	}
#endif
    // build a map of old-hash to new-hash for the hashed files, so that it can be passed to the mCollection for update

    mCollectionModel->preMods();
    std::map<RsFileHash,RsFileHash> old_to_new_hashes;

    for(auto f:hashedFiles)
    {
        auto it = mFilesBeingHashed.find(f.filepath);

        if(it == mFilesBeingHashed.end())
        {
            RsErr() << "Could not find hash-ID correspondence for path " << f.filepath.toUtf8().constData() << ". This is a bug." << std::endl;
            continue;
        }
        std::cerr << "Will update old hash " << it->second << " to new hash " << f.hash << std::endl;

        old_to_new_hashes.insert(std::make_pair(it->second,f.hash));
        mFilesBeingHashed.erase(it);
        mCollectionModel->fileHashingFinished(it->second);
    }
    mCollection->updateHashes(old_to_new_hashes);
    mCollectionModel->postMods();

    if(mFilesBeingHashed.empty())
    {
        ui._save_PB->setToolTip(tr(""));
        ui._save_PB->setEnabled(true);
    }
	updateList();
}

#ifdef TO_REMOVE
void RsCollectionDialog::itemChanged(QTreeWidgetItem *item, int col)
{
	if (col != COLUMN_FILE) return;

	if (item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_FILE) return;

	//In COLUMN_FILE, normaly, only checkState could change...
	qulonglong size = item->data(COLUMN_SIZE, ROLE_SIZE).toULongLong();
	bool unchecked = (item->checkState(COLUMN_FILE) == Qt::Unchecked);
	item->setData(COLUMN_SIZE, ROLE_SELSIZE, unchecked?0:size);
	item->setText(COLUMN_SIZE, misc::friendlyUnit(unchecked?0:size));
	item->setData(COLUMN_FILEC, ROLE_SELFILEC, unchecked?0:1);
	item->setText(COLUMN_FILEC, QString("%1").arg(unchecked?0:1));

	//update parents size
	QTreeWidgetItem *itemParent = item->parent();
	while (itemParent) {
		//When unchecked only remove selected size
		qulonglong parentSize = itemParent->data(COLUMN_SIZE, ROLE_SELSIZE).toULongLong() + (unchecked?0-size:size);
		itemParent->setData(COLUMN_SIZE, ROLE_SELSIZE, parentSize);
		itemParent->setText(COLUMN_SIZE, misc::friendlyUnit(parentSize));

		qulonglong parentFileCount = itemParent->data(COLUMN_FILEC, ROLE_SELFILEC).toULongLong() + (unchecked?0-1:1);
		itemParent->setData(COLUMN_FILEC, ROLE_SELFILEC, parentFileCount);
		itemParent->setText(COLUMN_FILEC, QString("%1").arg(parentFileCount));

		itemParent = itemParent->parent();
	}

	updateSizes() ;

}
#endif

/**
 * @brief RsCollectionDialog::updateRemoveDuplicate Remove all duplicate file when checked.
 * @param checked
 */
void RsCollectionDialog::updateRemoveDuplicate(bool checked)
{
#ifdef TODO_COLLECTION
	if (checked) {
		bool bRemoveAll = false;
		QTreeWidgetItemIterator it(ui._fileEntriesTW);
		QTreeWidgetItem *item;
		while ((item = *it) != NULL) {
			++it;
			if (item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR) {
				QList<QTreeWidgetItem*> founds;
				founds << ui._fileEntriesTW->findItems(item->text(COLUMN_HASH), Qt::MatchExactly | Qt::MatchRecursive, COLUMN_HASH);
				if (founds.count() > 1) {
					bool bRemove = false;
					if (!bRemoveAll) {
						QMessageBox* msgBox = new QMessageBox(QMessageBox::Information, "", "");
						msgBox->setText("Warning, duplicate file found.");
						//msgBox->setInformativeText(); If text too long, no scroll, so I add an text edit
						msgBox->setStandardButtons(QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
						msgBox->setDefaultButton(QMessageBox::Yes);

						QGridLayout* layout = qobject_cast<QGridLayout*>(msgBox->layout());
						if (layout) {
							int newRow = 1;
							for (int row = layout->count()-1; row >= 0; --row) {
								for (int col = layout->columnCount()-1; col >= 0; --col) {
									QLayoutItem *item = layout->itemAtPosition(row, col);
									if (item) {
										int index = layout->indexOf(item->widget());
										int r=0, c=0, rSpan=0, cSpan=0;
										layout->getItemPosition(index, &r, &c, &rSpan, &cSpan);
										if (r>0) {
											layout->removeItem(item);
											layout->addItem(item, r+3, c, rSpan, cSpan);
										} else if (rSpan>1) {
											newRow = rSpan + 1;
										}
									}
								}
							}
							QLabel *label = new QLabel(tr("Do you want to remove this file from the list?"));
							layout->addWidget(label,newRow, 0, 1, layout->columnCount(), Qt::AlignHCenter );
							QTextEdit *edit = new QTextEdit(item->text(COLUMN_FILEPATH));
							edit->setReadOnly(true);
							edit->setWordWrapMode(QTextOption::NoWrap);
							layout->addWidget(edit,newRow+1, 0, 1, layout->columnCount(), Qt::AlignHCenter );
						}

						int ret = msgBox->exec();
						switch (ret) {
							case QMessageBox::YesToAll: {
								bRemoveAll = true;
							}
							break;
							case QMessageBox::Yes: {
								bRemove = true;
							}
							break;
							case QMessageBox::No:
							break;
							case QMessageBox::Cancel: {
								delete msgBox;
								ui._removeDuplicate_CB->setChecked(false);
								return;
							}
							break;
							default:
								// should never be reached
							break;
						}
						delete msgBox;
					}

					if (bRemove || bRemoveAll) {
							//First uncheck item to update parent informations
							item->setCheckState(COLUMN_FILE,Qt::Unchecked);
							item->parent()->removeChild(item);
					}

				}
			}
		}
	}
#endif
}

/**
 * @brief RsCollectionDialog::cancel: Cancel RScollection editing or donwloading
 */
void RsCollectionDialog::cancel()
{
	std::cerr << "Canceling!" << std::endl;
	close() ;
}

/**
 * @brief RsCollectionDialog::download: Start downloading checked items
 */
void RsCollectionDialog::download()
{
	std::cerr << "Downloading!" << std::endl;

	QString dldir = ui.downloadFolder_LE->text();

	std::cerr << "downloading all these files:" << std::endl;

    std::function<void(RsFileTree::DirIndex,const std::string&)> recursDL = [&](RsFileTree::DirIndex index,const std::string& path)
    {
        const auto& dirdata(mCollection->fileTree().directoryData(index));
        RsCollectionModel::EntryIndex e;

        for(uint32_t i=0;i<dirdata.subdirs.size();++i)
        {
            e.index = dirdata.subdirs[i];
            e.is_file = false;

            if(!mCollectionModel->isChecked(e))
                continue;

            const auto& sdd = mCollection->fileTree().directoryData(e.index);
            std::string subpath = RsDirUtil::makePath(path,sdd.name);

            std::cerr << "Creating subdir " << sdd.name << " to directory " << path << std::endl;

            if(!QDir(QApplication::applicationDirPath()).mkpath(QString::fromUtf8(subpath.c_str())))
                QMessageBox::warning(NULL,tr("Unable to make path"),tr("Unable to make path:")+"<br>  "+QString::fromUtf8(subpath.c_str())) ;

            recursDL(dirdata.subdirs[i],subpath);
        }
        for(uint32_t i=0;i<dirdata.subfiles.size();++i)
        {
            e.index = dirdata.subfiles[i];
            e.is_file = true;

            if(!mCollectionModel->isChecked(e))
                continue;

            std::string subpath = RsDirUtil::makePath(path,dirdata.name);
            const auto& f(mCollection->fileTree().fileData(dirdata.subfiles[i]));

            std::cerr << "Requesting file " << f.name << " to directory " << path << std::endl;

            rsFiles->FileRequest(f.name,f.hash,f.size,path,RS_FILE_REQ_ANONYMOUS_ROUTING,std::list<RsPeerId>());
        }
    };

    recursDL(mCollection->fileTree().root(),dldir.toUtf8().constData());
    close();
#ifdef TO_REMOVE
	while ((item = *itemIterator) != NULL) {
		++itemIterator;

		if (item->checkState(COLUMN_FILE) == Qt::Checked) {
			std::cerr << item->data(COLUMN_HASH,ROLE_NAME).toString().toStdString()
			          << " " << item->text(COLUMN_HASH).toStdString()
			          << " " << item->text(COLUMN_SIZE).toStdString()
			          << " " << item->data(COLUMN_HASH,ROLE_PATH).toString().toStdString() << std::endl;
			ColFileInfo colFileInfo;
			colFileInfo.hash = item->text(COLUMN_HASH);
			colFileInfo.name = item->data(COLUMN_HASH,ROLE_NAME).toString();
			colFileInfo.path = item->data(COLUMN_HASH,ROLE_PATH).toString();
			colFileInfo.type = item->data(COLUMN_HASH,ROLE_TYPE).toUInt();
			colFileInfo.size = item->data(COLUMN_SIZE,ROLE_SELSIZE).toULongLong();

			QString cleanPath = dldir + colFileInfo.path ;
			std::cerr << "making directory " << cleanPath.toStdString() << std::endl;

			if(!QDir(QApplication::applicationDirPath()).mkpath(cleanPath))
				QMessageBox::warning(NULL,QObject::tr("Unable to make path"),QObject::tr("Unable to make path:")+"<br>  "+cleanPath) ;

			if (colFileInfo.type==DIR_TYPE_FILE)
				rsFiles->FileRequest(colFileInfo.name.toUtf8().constData(),
				                     RsFileHash(colFileInfo.hash.toStdString()),
				                     colFileInfo.size,
				                     cleanPath.toUtf8().constData(),
				                     RS_FILE_REQ_ANONYMOUS_ROUTING,
				                     std::list<RsPeerId>());
		} else {//if (item->checkState(COLUMN_FILE) == Qt::Checked)
			std::cerr<<"Skipping file : " << item->data(COLUMN_HASH,ROLE_NAME).toString().toStdString() << std::endl;
		}
	}

	close();
#endif
}

/**
 * @brief RsCollectionDialog::save: Update collection to save it in caller.
 */
void RsCollectionDialog::save()
{
    if(ui._filename_TL->text().isNull())
        changeFileName();
    if(ui._filename_TL->text().isNull())
        return;

    mCollectionModel->preMods();
    mCollection->cleanup();
    mCollection->save(ui._filename_TL->text());
    close();
#ifdef TO_REMOVE
	std::cerr << "Saving!" << std::endl;
	_newColFileInfos.clear();
	QTreeWidgetItem* root = getRootItem();
	if (root) {
		saveChild(root);
		emit saveColl(_newColFileInfos, _fileName);
	}
	close();
#endif
}

#ifdef TO_REMOVE
/**
 * @brief RsCollectionDialog::saveChild: Save each child in _newColFileInfos
 * @param parent
 */
void RsCollectionDialog::saveChild(QTreeWidgetItem *parentItem, ColFileInfo *parentInfo)
{
	ColFileInfo parent;
	if (!parentInfo) parentInfo = &parent;

	parentInfo->checked = (parentItem->checkState(COLUMN_FILE)==Qt::Checked);
	parentInfo->hash = parentItem->text(COLUMN_HASH);
	parentInfo->name = parentItem->data(COLUMN_HASH,ROLE_NAME).toString();
	parentInfo->path = parentItem->data(COLUMN_HASH,ROLE_PATH).toString();
	parentInfo->type = parentItem->data(COLUMN_HASH,ROLE_TYPE).toUInt();
	parentInfo->size = parentItem->data(COLUMN_SIZE,ROLE_SELSIZE).toULongLong();

	for (int i=0; i<parentItem->childCount(); ++i) {
		ColFileInfo child;
		saveChild(parentItem->child(i), &child);
		if (parentInfo->name != "") {
			parentInfo->children.push_back(child);
		} else {
			_newColFileInfos.push_back(child);
		}
	}
}
#endif

bool RsCollectionDialog::editExistingCollection(const QString& fileName, bool showError /* = true*/)
{
    return RsCollectionDialog(fileName,EDIT).exec();
}

bool RsCollectionDialog::openExistingCollection(const QString& fileName, bool showError /* = true*/)
{
    return RsCollectionDialog(fileName,DOWNLOAD).exec();
}

bool RsCollectionDialog::openNewCollection(const RsFileTree& tree)
{
    RsCollection collection(tree);
    QString fileName;

    if(!misc::getSaveFileName(nullptr, RshareSettings::LASTDIR_EXTRAFILE
                                                        , QApplication::translate("RsCollectionFile", "Create collection file")
                                                        , QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollection::ExtensionString + ")"
                                                        , fileName,0, QFileDialog::DontConfirmOverwrite))
        return false;

    if (!fileName.endsWith("." + RsCollection::ExtensionString))
        fileName += "." + RsCollection::ExtensionString ;

    std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

    QMessageBox mb;
    mb.setText(tr("Save Collection File."));
    mb.setInformativeText(tr("File already exists.")+"\n"+tr("What do you want to do?"));
    QAbstractButton *btnOwerWrite = mb.addButton(tr("Overwrite"), QMessageBox::YesRole);
    QAbstractButton *btnCancel = mb.addButton(tr("Cancel"), QMessageBox::ResetRole);
    mb.setIcon(QMessageBox::Question);
    mb.exec();

    if (mb.clickedButton()==btnCancel)
        return false;

    if(!collection.save(fileName))
        return false;

    return RsCollectionDialog(fileName,EDIT).exec();

#ifdef TODO_COLLECTION
    QString fileName = proposed_file_name;

    if(!misc::getSaveFileName(nullptr, RshareSettings::LASTDIR_EXTRAFILE
                                                        , QApplication::translate("RsCollectionFile", "Create collection file")
                                                        , QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollection::ExtensionString + ")"
                                                        , fileName,0, QFileDialog::DontConfirmOverwrite))
        return false;

    if (!fileName.endsWith("." + RsCollection::ExtensionString))
        fileName += "." + RsCollection::ExtensionString ;

    std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

    QFile file(fileName) ;

    if(file.exists())
    {
        RsCollection::RsCollectionErrorCode err;
        if (!RsCollection::checkFile(fileName,err))
        {
            QMessageBox::information(nullptr,tr("Error openning collection"),RsCollection::errorString(err));
            return false;
        }

        QMessageBox mb;
        mb.setText(tr("Save Collection File."));
        mb.setInformativeText(tr("File already exists.")+"\n"+tr("What do you want to do?"));
        QAbstractButton *btnOwerWrite = mb.addButton(tr("Overwrite"), QMessageBox::YesRole);
        QAbstractButton *btnMerge = mb.addButton(tr("Merge"), QMessageBox::NoRole);
        QAbstractButton *btnCancel = mb.addButton(tr("Cancel"), QMessageBox::ResetRole);
        mb.setIcon(QMessageBox::Question);
        mb.exec();

        if (mb.clickedButton()==btnOwerWrite) {
            //Nothing to do _xml_doc already up to date
        } else if (mb.clickedButton()==btnMerge) {
            //Open old file to merge it with _xml_doc
            QDomDocument qddOldFile("RsCollection");
            if (qddOldFile.setContent(&file)) {
                QDomElement docOldElem = qddOldFile.elementsByTagName("RsCollection").at(0).toElement();
                std::vector<ColFileInfo> colOldFileInfos;
                recursCollectColFileInfos(docOldElem,colOldFileInfos,QString(),false);

                QDomElement root = _xml_doc.elementsByTagName("RsCollection").at(0).toElement();
                for(uint32_t i = 0;i<colOldFileInfos.size();++i){
                    recursAddElements(_xml_doc,colOldFileInfos[i],root) ;
                }
            }

        } else if (mb.clickedButton()==btnCancel) {
            return false;
        } else {
            return false;
        }

    }//if(file.exists())

    _fileName=fileName;
    std::vector<ColFileInfo> colFileInfos ;

    recursCollectColFileInfos(_xml_doc.documentElement(),colFileInfos,QString(),false) ;

    RsCollectionDialog* rcd = new RsCollectionDialog(fileName, colFileInfos,true);
    connect(rcd,SIGNAL(saveColl(std::vector<ColFileInfo>, QString)),this,SLOT(saveColl(std::vector<ColFileInfo>, QString))) ;
    _saved=false;
    rcd->exec() ;
    delete rcd;

#endif
    return true;
}

