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

#ifndef _SIMPLEHTTP_H_
#define _SIMPLEHTTP_H_

#include <QHttp>

class QHttp;
class QHttpResponseHeader;

class SimpleHttp : public QHttp
{
	Q_OBJECT

public:
	SimpleHttp( QObject* parent = 0 );
	~SimpleHttp();

	void download(const QString & url);

signals:
	void connecting(QString host);
	void downloadFinished(QByteArray downloaded_text);
	void downloadFailed(QString error);

protected slots:
	void readResponseHeader(const QHttpResponseHeader &responseHeader);
	void httpRequestFinished(int request_id, bool error);

protected:
	QByteArray downloaded_text;
	int http_get_id;
};

#endif

