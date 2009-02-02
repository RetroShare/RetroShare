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

/* Based on the Qt network/http example */

#include "filedownloader.h"
#include <QHttp>
#include <QTimer>

FileDownloader::FileDownloader(QWidget *parent) : QProgressDialog(parent)
{
	setMinimumDuration(0);

	http = new QHttp(this);

	connect(http, SIGNAL(requestFinished(int, bool)),
            this, SLOT(httpRequestFinished(int, bool)));
	connect(http, SIGNAL(dataReadProgress(int, int)),
            this, SLOT(updateDataReadProgress(int, int)));
	connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
	connect(this, SIGNAL(canceled()), this, SLOT(cancelDownload()));

	setWindowTitle(tr("Downloading..."));
}

FileDownloader::~FileDownloader() {
	//qDebug("FileDownloader::~FileDownloader");
	delete http;
}

void FileDownloader::setProxy(QNetworkProxy proxy) {
	http->abort();
	http->setProxy(proxy);

	qDebug("FileDownloader::setProxy: host: '%s' port: %d type: %d",
           proxy.hostName().toUtf8().constData(), proxy.port(), proxy.type());
}

void FileDownloader::download(QUrl url) {
	QHttp::ConnectionMode mode = url.scheme().toLower() == "https" ? QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp;
	http->setHost(url.host(), mode, url.port() == -1 ? 0 : url.port());
    
	if (!url.userName().isEmpty())
		http->setUser(url.userName(), url.password());

	http_request_aborted = false;
	http_get_id = http->get(url.path(), &buffer);

	setLabelText(tr("Downloading %1").arg(url.toString()));
}

void FileDownloader::cancelDownload() {
	http_request_aborted = true;
	http->abort();
}

void FileDownloader::httpRequestFinished(int request_id, bool error) {
	qDebug("FileDownloader::httpRequestFinished: request_id %d, error %d", request_id, error);

	if (request_id != http_get_id) return;

	if (http_request_aborted) {
		hide();
		return;
	}

	hide();

	if (error) {
		emit downloadFailed(http->errorString());
	} else {
		emit downloadFinished(buffer.data());
	}
}

void FileDownloader::readResponseHeader(const QHttpResponseHeader &responseHeader) {
	if (responseHeader.statusCode() != 200) {
		emit downloadFailed(responseHeader.reasonPhrase());
		http_request_aborted = true;
		hide();
		http->abort();
		return;
	}
}

void FileDownloader::updateDataReadProgress(int bytes_read, int total_bytes) {
	if (http_request_aborted) return;

	setMaximum(total_bytes);
	setValue(bytes_read);
}

#include "moc_filedownloader.cpp"

