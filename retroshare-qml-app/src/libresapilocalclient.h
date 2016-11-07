/*
 * libresapi local socket client
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
 * Copyright (C) 2016  Manu Pineda <manu@cooperativa.cat>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBRESAPILOCALCLIENT_H
#define LIBRESAPILOCALCLIENT_H

#include <QLocalSocket>
#include <QDir>
#include <QJsonDocument>
#include <QVector>

class LibresapiLocalClient : public QObject
{
	Q_OBJECT

public:
	LibresapiLocalClient() : mLocalSocket(this) {}

	// potser abstreure el següent amb QUrl urlPath (path) i amb QJson jsonData.
	Q_INVOKABLE int request(const QString & path, const QString & jsonData);
	const QJsonDocument & getJson();
	Q_INVOKABLE void openConnection(QString socketPath);

private:
	QLocalSocket mLocalSocket;
	QByteArray receivedBytes;
	QJsonDocument json;
	//QVector<QJsonDocument> responses;

	bool parseResponse(); //std::string msg);

private slots:
	void socketError(QLocalSocket::LocalSocketError error);
	void read();

signals:
	void goodResponseReceived(const QString & msg);//, int requestId);
};

#endif // LIBRESAPILOCALCLIENT_H
