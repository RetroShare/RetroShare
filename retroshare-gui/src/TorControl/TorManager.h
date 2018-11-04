/* Ricochet - https://ricochet.im/
 * Copyright (C) 2014, John Brooks <john.brooks@dereferenced.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This code has been further modified to fit Retroshare context.

#ifndef TORMANAGER_H
#define TORMANAGER_H

#include <QObject>
#include <QStringList>
#include <QHostAddress>

namespace Tor
{

class TorProcess;
class TorControl;
class TorManagerPrivate;

/* Run/connect to an instance of Tor according to configuration, and manage
 * UI interaction, first time configuration, etc. */
class TorManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool configurationNeeded READ configurationNeeded NOTIFY configurationNeededChanged)
    Q_PROPERTY(QStringList logMessages READ logMessages CONSTANT)
    Q_PROPERTY(Tor::TorProcess* process READ process CONSTANT)
    Q_PROPERTY(Tor::TorControl* control READ control CONSTANT)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QString torDataDirectory READ torDataDirectory WRITE setTorDataDirectory)

public:
    static bool isTorAvailable() ;
    static TorManager *instance();

    TorProcess *process();
    TorControl *control();


    QString torDataDirectory() const;
    void setTorDataDirectory(const QString &path);

    QString hiddenServiceDirectory() const;
    void setHiddenServiceDirectory(const QString &path);

	// Starts a hidden service, loading it from the config directory that has been set earlier.
	bool setupHiddenService() ;

    // True on first run or when the Tor configuration wizard needs to be shown
    bool configurationNeeded() const;

    QStringList logMessages() const;

    bool hasError() const;
    QString errorMessage() const;

	bool getHiddenServiceInfo(QString& service_id,QString& service_onion_address,uint16_t& service_port, QHostAddress& service_target_address,uint16_t& target_port);
	bool getProxyServerInfo(QHostAddress& proxy_server_adress,uint16_t& proxy_server_port);

public slots:
    bool start();

private slots:
	void hiddenServicePrivateKeyChanged();
	void hiddenServiceStatusChanged(int old_status,int new_status);

signals:
    void configurationNeededChanged();
    void errorChanged();

private:
    explicit TorManager(QObject *parent = 0);
    TorManagerPrivate *d;
};

}

#endif
#
