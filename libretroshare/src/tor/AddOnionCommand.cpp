/* Ricochet - https://ricochet.im/
 * Copyright (C) 2016, John Brooks <john.brooks@dereferenced.net>
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

#include "AddOnionCommand.h"
#include "HiddenService.h"
#include "CryptoKey.h"
#include "StrUtil.h"

using namespace Tor;

AddOnionCommand::AddOnionCommand(HiddenService *service)
    : m_service(service), mSucceeded([](){}), mFailed([](int){})
{
    assert(m_service);
}

bool AddOnionCommand::isSuccessful() const
{
    return statusCode() == 250 && m_errorMessage.empty();
}

ByteArray AddOnionCommand::build()
{
    ByteArray out("ADD_ONION");

    if (m_service->privateKey().isLoaded()) {
        out += " ";
        out += m_service->privateKey().bytes();
    } else {
        //out += " NEW:RSA1024";	// this is v2. For v3, use NEW:BEST, or NEW:ED25519-V3
        //out += " NEW:ED25519-V3";	// this is v3.
        out += " NEW:BEST";		// this is v3, but without control of key type. Generates a RSA1024 key on older Tor versions.
    }

    for(const HiddenService::Target& target: m_service->targets())
    {
        out += " Port=";
        out += RsUtil::NumberToString(target.servicePort);
        out += ",";
        out += target.targetAddress;
        out += ":";
        out += RsUtil::NumberToString(target.targetPort);
    }

    out.append("\r\n");
    return out;
}

void AddOnionCommand::onReply(int statusCode, const ByteArray &data)
{
    TorControlCommand::onReply(statusCode, data);
    if (statusCode != 250) {
        m_errorMessage = data.toString();
        return;
    }

    const ByteArray keyPrefix("PrivateKey=");
    const ByteArray sidPrefix("ServiceID=");

    if(data.startsWith(sidPrefix))
    {
        ByteArray service_id = data.mid(sidPrefix.size());
        m_service->setServiceId(service_id);
    }

    if (data.startsWith(keyPrefix))
    {
        ByteArray keyData(data.mid(keyPrefix.size()));
        CryptoKey key;

        if (!key.loadFromTorMessage(keyData)) {
            m_errorMessage = "Key structure check failed";
            return;
        }

        m_service->setPrivateKey(key);
    }
}

void AddOnionCommand::onFinished(int statusCode)
{
    TorControlCommand::onFinished(statusCode);
    if (isSuccessful())
        mSucceeded();
    else
        mFailed(statusCode);
}


