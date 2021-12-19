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
#include "CryptoKey.h"
#include "util/rsdir.h"

#include <fstream>

using namespace Tor;

HiddenService::HiddenService(HiddenServiceClient *client)
    : m_status(NotCreated), m_client(client)
{
}

HiddenService::HiddenService(HiddenServiceClient *client,const std::string& path)
    : m_dataPath(path), m_status(NotCreated), m_client(client)
{
    /* Set the initial status and, if possible, load the hostname */
    if(RsDirUtil::fileExists(m_dataPath + "/private_key"))
    {
        loadPrivateKey();
        if (!m_hostname.empty())
            m_status = Offline;
    }
}

HiddenService::HiddenService(HiddenServiceClient *client,const CryptoKey &privateKey, const std::string &path)
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
    m_targets.push_back(target);
}

void HiddenService::addTarget(uint16_t servicePort, std::string targetAddress, uint16_t targetPort)
{
    Target t = { targetAddress, servicePort, targetPort };
    m_targets.push_back(t);
}

void HiddenService::setServiceId(const ByteArray& sid)
{
    m_service_id = sid;
    m_hostname = sid.toString() + ".onion";

    if(m_client)
        m_client->hiddenServiceHostnameChanged(); // emit hostnameChanged();
}
void HiddenService::setPrivateKey(const CryptoKey &key)
{
    if (m_privateKey.isLoaded()) {
        RsErr() << "Cannot change the private key on an existing HiddenService";
        return;
    }

    m_privateKey = key;

    if(m_client)
        m_client->hiddenServicePrivateKeyChanged(); //emit privateKeyChanged();
}

bool HiddenService::loadPrivateKey()
{
    if (m_privateKey.isLoaded() || m_dataPath.empty())
        return false;

    bool ok = m_privateKey.loadFromFile(m_dataPath + "/private_key");

    if (!ok) {
        RsWarn() << "Failed to load hidden service key";
        return false;
    }

    // Also load the onion address stored in "hostname" file. This is not needed, except for early display
    // of the onion address, since the onion address will be re-computed by Tor (to the same value) when the
    // service is published.

    std::ifstream i((m_dataPath + "/hostname").c_str());

    if(i)
    {
        std::string s;
        i >> s;
        if(ByteArray(s).endsWith(ByteArray(".onion")))
        {
            m_hostname = s;
            m_service_id = s.substr(0,s.length() - std::string(".onion").length());

            RsDbg() << "Read existing hostname: " << m_hostname;
        }
        i.close();
    }

    if(m_client)
        m_client->hiddenServicePrivateKeyChanged(); // emit privateKeyChanged();

    return true;
}

void HiddenService::servicePublished()
{
    loadPrivateKey();

    if (m_hostname.empty()) {
        std::cerr << "Failed to read hidden service hostname" << std::endl;
        return;
    }

    std::cerr << "Hidden service published successfully" << std::endl;
    setStatus(Online);
}

