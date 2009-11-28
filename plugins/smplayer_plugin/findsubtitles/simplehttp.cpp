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

#include "simplehttp.h"
#include <QUrl>

SimpleHttp::SimpleHttp( QObject * parent ) : QHttp(parent)
{
	connect( this, SIGNAL(requestFinished(int, bool)),
             this, SLOT(httpRequestFinished(int, bool)) );

	connect( this, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
             this, SLOT(readResponseHeader(const QHttpResponseHeader &)) );
}

SimpleHttp::~SimpleHttp() {
}

void SimpleHttp::download(const QString & url) {
	downloaded_text.clear();

	QUrl u(url);
	setHost( u.host() );
	http_get_id = get( u.path() );

	emit connecting(u.host());
}

void SimpleHttp::readResponseHeader(const QHttpResponseHeader &responseHeader) {
	qDebug("SimpleHttp::readResponseHeader: statusCode: %d", responseHeader.statusCode());

	if (responseHeader.statusCode() == 301)  {
		QString new_url = responseHeader.value("Location");
		qDebug("SimpleHttp::readResponseHeader: Location: '%s'", new_url.toLatin1().constData());
		download(new_url);
	}
	else
	if (responseHeader.statusCode() != 200) {
		qDebug("SimpleHttp::readResponseHeader: error: '%s'", responseHeader.reasonPhrase().toLatin1().constData());
		emit downloadFailed(responseHeader.reasonPhrase());
		abort();
	}
}

void SimpleHttp::httpRequestFinished(int request_id, bool error) {
	qDebug("SimpleHttp::httpRequestFinished: %d, %d", request_id, error);

    if (request_id != http_get_id) return;

	downloaded_text += readAll();

	if (!downloaded_text.isEmpty()) {
		emit downloadFinished(downloaded_text);
	}
}

#include "moc_simplehttp.cpp"

