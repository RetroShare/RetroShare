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
#include <QQueue>
#include <QJSValue>

class LibresapiLocalClient : public QObject
{
	Q_OBJECT

public:
	LibresapiLocalClient() : mLocalSocket(this) {}

	Q_INVOKABLE int request( const QString& path, const QString& jsonData = "",
	                         QJSValue callback = QJSValue::NullValue);
	Q_INVOKABLE void openConnection(QString socketPath);

private:
	QLocalSocket mLocalSocket;
	QQueue<QJSValue> callbackQueue;

private slots:
	void socketError(QLocalSocket::LocalSocketError error);
	void read();

signals:
	/// @deprecated @see LibresapiLocalClient::responseReceived instead
	void goodResponseReceived(const QString & msg);

	/**
	 * @brief responseReceived emitted when a response is received
	 * @param msg
	 */
	void responseReceived(const QString & msg);
};

#endif // LIBRESAPILOCALCLIENT_H
