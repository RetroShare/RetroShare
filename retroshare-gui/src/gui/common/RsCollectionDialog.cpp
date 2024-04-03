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

    if(err_code != RsCollection::RsCollectionErrorCode::COLLECTION_NO_ERROR)
    {
        QMessageBox::information(nullptr,tr("Could not load collection file"),tr("Could not load collection file"));
        close();
    }

    init(collectionFileName);
}

RsCollectionDialog::RsCollectionDialog(const RsCollection& coll, RsCollectionDialogMode mode)
  : _mode(mode)
{
    mCollection = new RsCollection(coll);
    init(QString());
}
void RsCollectionDialog::init(const QString& collectionFileName)
{
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

	// 2 - connect necessary signals/slots

	connect(ui._changeFile, SIGNAL(clicked()), this, SLOT(changeFileName()));
    connect(ui._add_PB, SIGNAL(clicked()), this, SLOT(addSelection()));
    connect(ui._addRecur_PB, SIGNAL(clicked()), this, SLOT(addSelectionRecursive()));
	connect(ui._remove_PB, SIGNAL(clicked()), this, SLOT(remove()));
	connect(ui._makeDir_PB, SIGNAL(clicked()), this, SLOT(makeDir()));
	connect(ui._cancel_PB, SIGNAL(clicked()), this, SLOT(cancel()));
	connect(ui._save_PB, SIGNAL(clicked()), this, SLOT(save()));
	connect(ui._download_PB, SIGNAL(clicked()), this, SLOT(download()));
	connect(ui._hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

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
    ui._save_PB->setVisible(_mode == EDIT);
    ui._treeViewFrame->setVisible(_mode == EDIT);
    ui._download_PB->setVisible(_mode == DOWNLOAD);

	ui._systemFileTW->installEventFilter(this);

	// 6 Add HashBox
	setAcceptDrops(true);
	ui._hashBox->setDropWidget(this);
	ui._hashBox->setAutoHide(true);
	ui._hashBox->setDefaultTransferRequestFlags(RS_FILE_REQ_ANONYMOUS_ROUTING) ;
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
static bool checkFileName(const std::string& input, std::string& corrected)
{
    static const std::string bad_chars ( "/\\\"*:?<>|" );
    bool ok = true ;
    corrected = input ;

    for(uint32_t i=0;i<input.length();++i)
        for(uint32_t j=0;j<bad_chars.length();++j)
            if(input[i] == bad_chars[j])
			{
                corrected[i] = '_' ;
                ok = false ;
			}

    return ok ;
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

            if(err != RsCollection::RsCollectionErrorCode::COLLECTION_NO_ERROR)
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
	QModelIndexList milSelectionList =	ui._systemFileTW->selectionModel()->selectedIndexes();

    mCollectionModel->preMods();

    std::map<QString,RsFileHash> paths_to_hash;	// sha1sum of the paths to hash.

	foreach (QModelIndex index, milSelectionList)
        if(index.column()==0)    //Get only FileName
        {
            RsFileTree tree;
            recursBuildFileTree(_dirModel->filePath(_tree_proxyModel->mapToSource(index)),tree,tree.root(),recursive,paths_to_hash);

            mCollection->merge_in(tree);
		}

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
}

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
}

/**
 * @brief RsCollectionDialog::fileHashingFinished: Connected to ui._hashBox.fileHashingFinished
 *  Add finished File to collection in respective directory
 * @param hashedFiles: List of the file finished
 */
void RsCollectionDialog::fileHashingFinished(QList<HashedFile> hashedFiles)
{
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
    bool auto_correct = false;
    bool auto_skip = false;

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

            std::string corrected_name;

            if(!checkFileName(f.name,corrected_name) && !auto_correct)
            {
                if(auto_skip)
                    continue;

                QMessageBox mb;
                mb.setText(tr("Incompatible filename."));
                mb.setInformativeText(tr("This filename is not usable on your system.")+"\n"+tr("Retroshare can replace every problematic chars by '_'.")
                                      +"\n"+tr("What do you want to do?"));
                QAbstractButton *btnCorrect = mb.addButton(tr("Correct filename"), QMessageBox::YesRole);
                QAbstractButton *btnCorrectAll = mb.addButton(tr("Correct all"), QMessageBox::AcceptRole);
                QAbstractButton *btnSkip = mb.addButton(tr("Skip this file"), QMessageBox::NoRole);
                QAbstractButton *btnSkipAll = mb.addButton(tr("Skip all"), QMessageBox::RejectRole);
                mb.setIcon(QMessageBox::Question);
                mb.exec();

                if(mb.clickedButton() == btnSkipAll)
                {
                    auto_skip = true;
                    continue;
                }
                if(mb.clickedButton() == btnSkip)
                    continue;

                if(mb.clickedButton() == btnCorrectAll)
                    auto_correct = true;
            }

            std::cerr << "Requesting file " << corrected_name << " to directory " << path << std::endl;

            rsFiles->FileRequest(corrected_name,f.hash,f.size,path,RS_FILE_REQ_ANONYMOUS_ROUTING,std::list<RsPeerId>());
        }
    };

    recursDL(mCollection->fileTree().root(),dldir.toUtf8().constData());
    close();
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
}

bool RsCollectionDialog::editExistingCollection(const QString& fileName, bool showError /* = true*/)
{
    return RsCollectionDialog(fileName,EDIT).exec();
}

bool RsCollectionDialog::openExistingCollection(const QString& fileName, bool showError /* = true*/)
{
    return RsCollectionDialog(fileName,DOWNLOAD).exec();
}

bool RsCollectionDialog::downloadFiles(const RsCollection &collection)
{
    return RsCollectionDialog(collection,DOWNLOAD).exec();
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

    if(QFile(fileName).exists())
    {
        QMessageBox mb;
        mb.setText(tr("Save Collection File."));
        mb.setInformativeText(tr("File already exists.")+"\n"+tr("What do you want to do?"));
        QAbstractButton *btnOwerWrite = mb.addButton(tr("Overwrite"), QMessageBox::YesRole);
        QAbstractButton *btnCancel = mb.addButton(tr("Cancel"), QMessageBox::ResetRole);
        mb.setIcon(QMessageBox::Question);
        mb.exec();

        if (mb.clickedButton()==btnCancel)
            return false;
    }

    if(!collection.save(fileName))
        return false;

    return RsCollectionDialog(fileName,EDIT).exec();
}

