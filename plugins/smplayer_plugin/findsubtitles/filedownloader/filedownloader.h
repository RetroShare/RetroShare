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

#ifndef _FILEDOWNLOADER_H_
#define _FILEDOWNLOADER_H_

#include <QProgressDialog>
#include <QUrl>
#include <QBuffer>
#include <QNetworkProxy>

class QHttp;
class QHttpResponseHeader;

class FileDownloader : public QProgressDialog
{
    Q_OBJECT

public:
	FileDownloader(QWidget *parent = 0);
	~FileDownloader();

	void setProxy(QNetworkProxy proxy);

public slots:
	void download(QUrl url);
	void cancelDownload();

signals:
	void downloadFinished(const QByteArray & buffer);
	void downloadFailed(const QString & reason);

private slots:
	void httpRequestFinished(int request_id, bool error);
	void readResponseHeader(const QHttpResponseHeader &responseHeader);
	void updateDataReadProgress(int bytes_read, int total_bytes);

private:
	QHttp * http;
	int http_get_id;
	bool http_request_aborted;
	QBuffer buffer;
};

#endif
