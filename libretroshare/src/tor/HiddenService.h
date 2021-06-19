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

#ifndef HIDDENSERVICE_H
#define HIDDENSERVICE_H

#include <string>
#include <list>

#include "tor/CryptoKey.h"
#include "tor/TorTypes.h"

namespace Tor
{
class TorSocket;

class HiddenService : public NonCopiable
{
    friend class TorControlPrivate;

public:
    struct Target
    {
        TorHostAddress targetAddress;
        unsigned short servicePort, targetPort;
    };

    enum Status
    {
        NotCreated = -1, /* Service has not been created yet */
        Offline = 0, /* Data exists, but service is not published */
        Online /* Published */
    };

    HiddenService();
    HiddenService(const std::string& dataPath);
    HiddenService(const CryptoKey& privateKey, const std::string& dataPath = std::string());

    Status status() const { return m_status; }

    const std::string&  hostname()  const { return m_hostname; }
    const std::string   serviceId() const { return std::string(m_service_id.data()); }
    const std::string&  dataPath()  const { return m_dataPath; }

    CryptoKey privateKey() { return m_privateKey; }
    void setPrivateKey(const CryptoKey &privateKey);
    void setServiceId(const TorByteArray& sid);

    const std::list<Target>& targets() const { return m_targets; }
    void addTarget(const Target &target);
    void addTarget(unsigned short servicePort, TorHostAddress targetAddress, unsigned short targetPort);

#ifdef NO_TOR_CONTROL_SIGNALS
signals:
    void statusChanged(int newStatus, int oldStatus);
    void serviceOnline();
    void privateKeyChanged();
    void hostnameChanged();

private slots:
    void servicePublished();
#endif

private:
    std::string m_dataPath;
    std::list<Target> m_targets;
    std::string m_hostname;
    Status m_status;
    CryptoKey m_privateKey;
    TorByteArray m_service_id;

    void loadPrivateKey();
    void setStatus(Status newStatus);
};

}

#endif // HIDDENSERVICE_H
