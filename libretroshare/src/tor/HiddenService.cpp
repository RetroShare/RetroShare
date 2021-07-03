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

#include "HiddenService.h"
#include "TorControl.h"
#include "TorSocket.h"
#include "CryptoKey.h"
#include "Useful.h"
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QDebug>

using namespace Tor;

HiddenService::HiddenService(HiddenServiceClient *client)
    : m_status(NotCreated), m_client(client)
{
}

HiddenService::HiddenService(HiddenServiceClient *client,const QString &path)
    : m_dataPath(path), m_status(NotCreated), m_client(client)
{
    /* Set the initial status and, if possible, load the hostname */
    if (QDir(m_dataPath).exists(QLatin1String("private_key"))) {
        loadPrivateKey();
        if (!m_hostname.isEmpty())
            m_status = Offline;
    }
}

HiddenService::HiddenService(HiddenServiceClient *client,const CryptoKey &privateKey, const QString &path)
    : m_dataPath(path), m_status(NotCreated), m_client(client)
{
    setPrivateKey(privateKey);
    m_status = Offline;
}

void HiddenService::setStatus(Status newStatus)
{
    if (m_status == newStatus)
        return;

    Status old = m_status;
    m_status = newStatus;

    if(m_client)
        m_client->hiddenServiceStatusChanged(m_status,old); //emit statusChanged(m_status, old);

    if (m_status == Online)
        if(m_client)
            m_client->hiddenServiceOnline(); //emit serviceOnline();
}

void HiddenService::addTarget(const Target &target)
{
    m_targets.append(target);
}

void HiddenService::addTarget(quint16 servicePort, QHostAddress targetAddress, quint16 targetPort)
{
    Target t = { targetAddress, servicePort, targetPort };
    m_targets.append(t);
}

void HiddenService::setServiceId(const QByteArray& sid)
{
    m_service_id = sid;
    m_hostname = sid + ".onion";

    if(m_client)
        m_client->hiddenServiceHostnameChanged(); // emit hostnameChanged();
}
void HiddenService::setPrivateKey(const CryptoKey &key)
{
    if (m_privateKey.isLoaded()) {
        BUG() << "Cannot change the private key on an existing HiddenService";
        return;
    }

#ifdef TO_REMOVE
    if (!key.isPrivate()) {
        BUG() << "Cannot create a hidden service with a public key";
        return;
    }
#endif

    m_privateKey = key;

    if(m_client)
        m_client->hiddenServicePrivateKeyChanged(); //emit privateKeyChanged();
}

void HiddenService::loadPrivateKey()
{
    if (m_privateKey.isLoaded() || m_dataPath.isEmpty())
        return;

    bool ok = m_privateKey.loadFromFile(m_dataPath + QLatin1String("/private_key"));

    if (!ok) {
        qWarning() << "Failed to load hidden service key";
        return;
    }

    if(m_client)
        m_client->hiddenServicePrivateKeyChanged(); // emit privateKeyChanged();
}

void HiddenService::servicePublished()
{
    loadPrivateKey();

    if (m_hostname.isEmpty()) {
        std::cerr << "Failed to read hidden service hostname" << std::endl;
        return;
    }

    std::cerr << "Hidden service published successfully" << std::endl;
    setStatus(Online);
}

