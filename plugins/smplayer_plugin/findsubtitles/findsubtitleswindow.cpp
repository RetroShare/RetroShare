/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "findsubtitleswindow.h"
#include "simplehttp.h"
#include "osparser.h"
#include "languages.h"
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QMap>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QSettings>

#ifdef DOWNLOAD_SUBS
#include "filedownloader.h"
#include "subchooserdialog.h"
#include "quazip.h"
#include "quazipfile.h"
#include <QTemporaryFile>
#include <QBuffer>
#endif

#define COL_LANG 0
#define COL_NAME 1
#define COL_FORMAT 2
#define COL_FILES 3
#define COL_DATE 4
#define COL_USER 5

FindSubtitlesWindow::FindSubtitlesWindow( QWidget * parent, Qt::WindowFlags f )
	: QDialog(parent,f)
{
	setupUi(this);

	set = 0; // settings

	subtitles_for_label->setBuddy(file_chooser->lineEdit());

	progress->hide();

	connect( file_chooser, SIGNAL(fileChanged(QString)),
             this, SLOT(setMovie(QString)) );
	connect( file_chooser->lineEdit(), SIGNAL(textChanged(const QString &)),
             this, SLOT(updateRefreshButton()) );

	connect( refresh_button, SIGNAL(clicked()),
             this, SLOT(refresh()) );

	connect( download_button, SIGNAL(clicked()),
             this, SLOT(download()) );

	/*
	connect( language_filter, SIGNAL(editTextChanged(const QString &)),
             this, SLOT(applyFilter(const QString &)) );
	*/
	connect( language_filter, SIGNAL(activated(int)),
             this, SLOT(applyCurrentFilter()) );

	table = new QStandardItemModel(this);
	table->setColumnCount(COL_USER + 1);

	proxy_model = new QSortFilterProxyModel(this);
	proxy_model->setSourceModel(table);
	proxy_model->setFilterKeyColumn(COL_LANG);
	proxy_model->setFilterRole(Qt::UserRole);

	view->setModel(proxy_model);
	view->setRootIsDecorated(false);
	view->setSortingEnabled(true);
	view->setAlternatingRowColors(true);
	view->header()->setSortIndicator(COL_LANG, Qt::AscendingOrder);
	view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	view->setContextMenuPolicy( Qt::CustomContextMenu );

	connect(view, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(itemActivated(const QModelIndex &)) );
	connect(view->selectionModel(), SIGNAL(currentChanged(const QModelIndex &,const QModelIndex &)),
            this, SLOT(currentItemChanged(const QModelIndex &,const QModelIndex &)) );

	connect(view, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenu(const QPoint &)) );

	downloader = new SimpleHttp(this);

	connect( downloader, SIGNAL(downloadFailed(QString)),
             this, SLOT(showError(QString)) );
	connect( downloader, SIGNAL(downloadFinished(QByteArray)), 
             this, SLOT(downloadFinished()) );
	connect( downloader, SIGNAL(downloadFinished(QByteArray)), 
             this, SLOT(parseInfo(QByteArray)) );
	connect( downloader, SIGNAL(stateChanged(int)),
             this, SLOT(updateRefreshButton()) );

	connect( downloader, SIGNAL(connecting(QString)),
             this, SLOT(connecting(QString)) );
	connect( downloader, SIGNAL(dataReadProgress(int, int)),
             this, SLOT(updateDataReadProgress(int, int)) );

#ifdef DOWNLOAD_SUBS
	include_lang_on_filename = true;

	file_downloader = new FileDownloader(this);
	file_downloader->setModal(true);
	connect( file_downloader, SIGNAL(downloadFailed(QString)),
             this, SLOT(showError(QString)), Qt::QueuedConnection );
	connect( file_downloader, SIGNAL(downloadFinished(const QByteArray &)),
             this, SLOT(archiveDownloaded(const QByteArray &)), Qt::QueuedConnection );
#endif

	// Actions
	downloadAct = new QAction(this);
	downloadAct->setEnabled(false);
	connect( downloadAct, SIGNAL(triggered()), this, SLOT(download()) );

	copyLinkAct = new QAction(this);
	copyLinkAct->setEnabled(false);
	connect( copyLinkAct, SIGNAL(triggered()), this, SLOT(copyLink()) );

	context_menu = new QMenu(this);
	context_menu->addAction(downloadAct);
	context_menu->addAction(copyLinkAct);

	retranslateStrings();

	language_filter->setCurrentIndex(0);
}

FindSubtitlesWindow::~FindSubtitlesWindow() {
	if (set) saveSettings();
}

void FindSubtitlesWindow::setSettings(QSettings * settings) {
	set = settings;
	loadSettings();
}

void FindSubtitlesWindow::setProxy(QNetworkProxy proxy) {
	downloader->abort();
	downloader->setProxy(proxy);

#ifdef DOWNLOAD_SUBS
	file_downloader->setProxy(proxy);
#endif

	qDebug("FindSubtitlesWindow::setProxy: host: '%s' port: %d type: %d",
           proxy.hostName().toUtf8().constData(), proxy.port(), proxy.type());
}

void FindSubtitlesWindow::retranslateStrings() {
	retranslateUi(this);

	QStringList labels;
	labels << tr("Language") << tr("Name") << tr("Format") 
           << tr("Files") << tr("Date") << tr("Uploaded by");

	table->setHorizontalHeaderLabels( labels );

	// Language combobox
	//int language_index = language_filter->currentIndex();
	QString current_language = language_filter->itemData(language_filter->currentIndex()).toString();
	language_filter->clear();

	QMap<QString,QString> l = Languages::list();
	QMapIterator<QString, QString> i(l);
	while (i.hasNext()) {
		i.next();
		language_filter->addItem( i.value() + " (" + i.key() + ")", i.key() );
	}
	language_filter->model()->sort(0);
	language_filter->insertItem( 0, tr("All"), "*" );
	//language_filter->setCurrentIndex(language_index);
	language_filter->setCurrentIndex(language_filter->findData(current_language));

#if QT_VERSION < 0x040300
	QPushButton * close_button = buttonBox->button(QDialogButtonBox::Close);
	close_button->setText( tr("Close") );
#endif

	// Actions
	downloadAct->setText( tr("&Download") );
	copyLinkAct->setText( tr("&Copy link to clipboard") );
}

void FindSubtitlesWindow::setMovie(QString filename) {
	qDebug("FindSubtitlesWindow::setMovie: '%s'", filename.toLatin1().constData());

	if (filename == last_file) {
		return;
	}

	file_chooser->setText(filename);
	table->setRowCount(0);

	QString hash = OSParser::calculateHash(filename);
	if (hash.isEmpty()) {
		qWarning("FindSubtitlesWindow::setMovie: hash invalid. Doing nothing.");
	} else {
		QString link = "http://www.opensubtitles.org/search/sublanguageid-all/moviehash-" + hash + "/simplexml";
		qDebug("FindSubtitlesWindow::setMovie: link: '%s'", link.toLatin1().constData());
		downloader->download(link);
		last_file = filename;
	}
}

void FindSubtitlesWindow::refresh() {
	last_file = "";
	setMovie(file_chooser->text());
}

void FindSubtitlesWindow::updateRefreshButton() {
	QString file = file_chooser->lineEdit()->text();
	bool enabled = ( (!file.isEmpty()) && (QFile::exists(file)) && 
                     (downloader->state()==QHttp::Unconnected) );
	refresh_button->setEnabled(enabled);
}

void FindSubtitlesWindow::currentItemChanged(const QModelIndex & current, const QModelIndex & /*previous*/) {
	qDebug("FindSubtitlesWindow::currentItemChanged: row: %d, col: %d", current.row(), current.column());
	download_button->setEnabled(current.isValid());
	downloadAct->setEnabled(current.isValid());
	copyLinkAct->setEnabled(current.isValid());
}

void FindSubtitlesWindow::applyFilter(const QString & filter) {
	proxy_model->setFilterWildcard(filter);
}

void FindSubtitlesWindow::applyCurrentFilter() {
	//proxy_model->setFilterWildcard(language_filter->currentText());
	QString filter = language_filter->itemData( language_filter->currentIndex() ).toString();
	applyFilter(filter);
}

void FindSubtitlesWindow::setLanguage(const QString & lang) {
	int idx = language_filter->findData(lang);
	if (idx < 0) idx = 0;
	language_filter->setCurrentIndex(idx);
}

QString FindSubtitlesWindow::language() {
	int idx = language_filter->currentIndex();
	return language_filter->itemData(idx).toString();
}

void FindSubtitlesWindow::showError(QString error) {
	QMessageBox::information(this, tr("Error"),
                             tr("Download failed: %1.")
                             .arg(error));
}

void FindSubtitlesWindow::connecting(QString host) {
	status->setText( tr("Connecting to %1...").arg(host) );
}

void FindSubtitlesWindow::updateDataReadProgress(int done, int total) {
	qDebug("FindSubtitlesWindow::updateDataReadProgress: %d, %d", done, total);

	status->setText( tr("Downloading...") );

	if (!progress->isVisible()) progress->show();
	progress->setMaximum(total);
	progress->setValue(done);
}

void FindSubtitlesWindow::downloadFinished() {
	status->setText( tr("Done.") );
	progress->setMaximum(1);
	progress->setValue(0);
	progress->hide();
}

void FindSubtitlesWindow::parseInfo(QByteArray xml_text) {
	OSParser osparser;
	bool ok = osparser.parseXml(xml_text);

	table->setRowCount(0);

	QMap <QString,QString> language_list = Languages::list();

	if (ok) {
		QList<OSSubtitle> l = osparser.subtitleList();
		for (int n=0; n < l.count(); n++) {

			QString title_name = l[n].movie;
			if (!l[n].releasename.isEmpty()) {
				title_name += " - " + l[n].releasename;
			}

			QStandardItem * i_name = new QStandardItem(title_name);
			i_name->setData( l[n].link );
			#if QT_VERSION < 0x040400
			i_name->setToolTip( l[n].link );
			#endif

			QStandardItem * i_lang = new QStandardItem(l[n].language);
			i_lang->setData(l[n].iso639, Qt::UserRole);
			#if QT_VERSION < 0x040400
			i_lang->setToolTip(l[n].iso639);
			#endif
			if (language_list.contains(l[n].iso639)) {
				i_lang->setText( language_list[ l[n].iso639 ] );
			}

			table->setItem(n, COL_LANG, i_lang);
			table->setItem(n, COL_NAME, i_name);
			table->setItem(n, COL_FORMAT, new QStandardItem(l[n].format));
			table->setItem(n, COL_FILES, new QStandardItem(l[n].files));
			table->setItem(n, COL_DATE, new QStandardItem(l[n].date));
			table->setItem(n, COL_USER, new QStandardItem(l[n].user));

		}
		status->setText( tr("%1 files available").arg(l.count()) );
		applyCurrentFilter();

		qDebug("sort column: %d", view->header()->sortIndicatorSection());
		qDebug("sort indicator: %d", view->header()->sortIndicatorOrder());

		table->sort( view->header()->sortIndicatorSection(),
                     view->header()->sortIndicatorOrder() );
	} else {
		status->setText( tr("Failed to parse the received data.") );
	}

	view->resizeColumnToContents(COL_NAME);
}

void FindSubtitlesWindow::itemActivated(const QModelIndex & index ) {
	qDebug("FindSubtitlesWindow::itemActivated: row: %d, col %d", proxy_model->mapToSource(index).row(), proxy_model->mapToSource(index).column());

	QString download_link = table->item(proxy_model->mapToSource(index).row(), COL_NAME)->data().toString();

	qDebug("FindSubtitlesWindow::itemActivated: download link: '%s'", download_link.toLatin1().constData());

#ifdef DOWNLOAD_SUBS
	file_downloader->download( QUrl(download_link) );
	file_downloader->show();
#else
	QDesktopServices::openUrl( QUrl(download_link) );
#endif
}

void FindSubtitlesWindow::download() {
	qDebug("FindSubtitlesWindow::download");
	if (view->currentIndex().isValid()) {
		itemActivated(view->currentIndex());
	}
}

void FindSubtitlesWindow::copyLink() {
	qDebug("FindSubtitlesWindow::copyLink");
	if (view->currentIndex().isValid()) {
		const QModelIndex & index = view->currentIndex();
		QString download_link = table->item(proxy_model->mapToSource(index).row(), COL_NAME)->data().toString();
		qDebug("FindSubtitlesWindow::copyLink: link: '%s'", download_link.toLatin1().constData());
		qApp->clipboard()->setText(download_link);
	}
}

void FindSubtitlesWindow::showContextMenu(const QPoint & pos) {
	qDebug("FindSubtitlesWindow::showContextMenu");

	context_menu->move( view->viewport()->mapToGlobal(pos) );
	context_menu->show();
}

// Language change stuff
void FindSubtitlesWindow::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#ifdef DOWNLOAD_SUBS
void FindSubtitlesWindow::archiveDownloaded(const QByteArray & buffer) {
	qDebug("FindSubtitlesWindow::archiveDownloaded");

	QString temp_dir = QDir::tempPath();
	if (!temp_dir.endsWith("/")) temp_dir += "/";

	QTemporaryFile file(temp_dir + "archive_XXXXXX.zip");
	file.setAutoRemove(false);

	qDebug("FindSubtitlesWindow::archiveDownloaded: a temporary file will be saved in folder '%s'", temp_dir.toUtf8().constData());

	if (file.open()) {
		QString filename = file.fileName();
		file.write( buffer );
		file.close();

		qDebug("FindSubtitlesWindow::archiveDownloaded: file saved as: %s", filename.toUtf8().constData());

		/*
		QMessageBox::information(this, tr("Downloaded"), tr("File saved as %1").arg(filename));
		return;
		*/

		status->setText(tr("Temporary file %1").arg(filename));

		QString lang = "unknown";
		QString extension = "unknown";
		if (view->currentIndex().isValid()) {
			const QModelIndex & index = view->currentIndex();
			lang = table->item(proxy_model->mapToSource(index).row(), COL_LANG)->data(Qt::UserRole).toString();
			extension = table->item(proxy_model->mapToSource(index).row(), COL_FORMAT)->text();
		}

		QFileInfo fi(file_chooser->text());
		QString output_name = fi.completeBaseName();
		if (include_lang_on_filename) output_name += "_"+ lang;
		output_name += "." + extension;

		if (!uncompressZip(filename, fi.absolutePath(), output_name)) {
			status->setText(tr("Download failed"));
		}
		file.remove();
	}
	else {
		qWarning("FindSubtitlesWindow::archiveDownloaded: can't write temporary file");
		QMessageBox::warning(this, tr("Error saving file"),
                             tr("It wasn't possible to save the downloaded\n"
                                "file in folder %1\n"
                                "Please check the permissions of that folder.").arg(temp_dir));
	}
}


bool FindSubtitlesWindow::uncompressZip(const QString & filename, const QString & output_path, const QString & preferred_output_name) {
	qDebug("FindSubtitlesWindow::uncompressZip: zip file '%s', output_path '%s', save subtitle as '%s'", 
           filename.toUtf8().constData(), output_path.toUtf8().constData(),
           preferred_output_name.toUtf8().constData());

	QuaZip zip(filename);

	if (!zip.open(QuaZip::mdUnzip)) {
	    qWarning("FindSubtitlesWindow::uncompressZip: open zip failed: %d", zip.getZipError());
	    return false;
	}

	zip.setFileNameCodec("IBM866");
	qDebug("FindSubtitlesWindow::uncompressZip: %d entries", zip.getEntriesCount());
	qDebug("FindSubtitlesWindow::uncompressZip: global comment: '%s'", zip.getComment().toUtf8().constData());

	QStringList sub_files;
	QuaZipFileInfo info;

	for (bool more=zip.goToFirstFile(); more; more=zip.goToNextFile()) {
		if (!zip.getCurrentFileInfo(&info)) {
			qWarning("FindSubtitlesWindow::uncompressZip: getCurrentFileInfo(): %d\n", zip.getZipError());
			return false;
		}
	    qDebug("FindSubtitlesWindow::uncompressZip: file '%s'", info.name.toUtf8().constData());
		if (QFileInfo(info.name).suffix() != "nfo") sub_files.append(info.name);
	}

	qDebug("FindSubtitlesWindow::uncompressZip: list of subtitle files:");
	for (int n=0; n < sub_files.count(); n++) {
		qDebug("FindSubtitlesWindow::uncompressZip: subtitle file %d '%s'", n, sub_files[n].toUtf8().constData());
	}

	if (sub_files.count() == 1) {
		// If only one file, just extract it
		QString output_name = output_path +"/"+ preferred_output_name;
		if (extractFile(zip, sub_files[0], output_name )) {
			status->setText(tr("Subtitle saved as %1").arg(preferred_output_name));
			emit subtitleDownloaded(output_name);
		} else {
			return false;
		}
	} else {
		// More than one file
		SubChooserDialog d(this);

		for (int n=0; n < sub_files.count(); n++) {
			d.addFile(sub_files[n]);
		}

		if (d.exec() == QDialog::Rejected) return false;

		QStringList files_to_extract = d.selectedFiles();
		int extracted_count = 0;
		for (int n=0; n < files_to_extract.count(); n++) {
			QString file = files_to_extract[n];
			bool ok = extractFile(zip, file, output_path +"/"+ file);
			qDebug("FindSubtitlesWindow::uncompressZip: extracted %s ok: %d", file.toUtf8().constData(), ok);
			if (ok) extracted_count++;
		}
		status->setText(tr("%1 subtitle(s) extracted","", extracted_count).arg(extracted_count));
		if (extracted_count > 0) {
			emit subtitleDownloaded( output_path +"/"+ files_to_extract[0] );
		}
	}

	zip.close();
	return true;
}

bool FindSubtitlesWindow::extractFile(QuaZip & zip, const QString & filename, const QString & output_name) {
	qDebug("FindSubtitlesWindow::extractFile: '%s', save as '%s'", filename.toUtf8().constData(), output_name.toUtf8().constData());

	if (QFile::exists(output_name)) {
		if (QMessageBox::question(this, tr("Overwrite?"), 
            tr("The file %1 already exits, overwrite?").arg(output_name), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		{
			return false;
		}
	}

	if (!zip.setCurrentFile(filename)) {
		qDebug("FindSubtitlesWindow::extractFile: can't select file %s", filename.toUtf8().constData());
		return false;
	}

	// Saving
	char c;
	QuaZipFile file(&zip);
	QFile out(output_name);

	if (!file.open(QIODevice::ReadOnly)) {
		qWarning("FindSubtitlesWindow::extractFile: can't open file for reading: %d", file.getZipError());
		return false;
    }

	if (out.open(QIODevice::WriteOnly)) {
		// Slow like hell (on GNU/Linux at least), but it is not my fault.
		// Not ZIP/UNZIP package's fault either.
		// The slowest thing here is out.putChar(c).
		while(file.getChar(&c)) out.putChar(c);
		out.close();

		file.close();
	} else {
		qWarning("FindSubtitlesWindow::extractFile: can't open %s for writing", output_name.toUtf8().constData());
		return false;
	}

	return true;
}

#endif

void FindSubtitlesWindow::saveSettings() {
	qDebug("FindSubtitlesWindow::saveSettings");

	set->beginGroup("findsubtitles");

	set->setValue("language", language());
#ifdef DOWNLOAD_SUBS
	set->setValue("include_lang_on_filename", includeLangOnFilename());
#endif

	set->endGroup();
}

void FindSubtitlesWindow::loadSettings() {
	qDebug("FindSubtitlesWindow::loadSettings");

	set->beginGroup("findsubtitles");

	setLanguage( set->value("language", language()).toString() );
#ifdef DOWNLOAD_SUBS
	setIncludeLangOnFilename( set->value("include_lang_on_filename", includeLangOnFilename()).toBool() );
#endif

	set->endGroup();
}

#include "moc_findsubtitleswindow.cpp"

