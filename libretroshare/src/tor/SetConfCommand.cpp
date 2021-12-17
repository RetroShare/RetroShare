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

#include "SetConfCommand.h"
#include "StrUtil.h"

using namespace Tor;

SetConfCommand::SetConfCommand()
    : m_resetMode(false), mConfSucceeded([](){}), mConfFailed([](int){})
{
}

void SetConfCommand::setResetMode(bool enabled)
{
    m_resetMode = enabled;
}

bool SetConfCommand::isSuccessful() const
{
    return statusCode() == 250;
}

ByteArray SetConfCommand::build(const std::string &key, const std::string &value)
{
    return build(std::list<std::pair<std::string, std::string> > { std::make_pair(key, value) } );
}

// ByteArray SetConfCommand::build(const std::list<std::pair<ByteArray,ByteArray> > &data)
// {
//     QList<QPair<QByteArray, QByteArray> > out;
//
//     for (QVariantMap::ConstIterator it = data.begin(); it != data.end(); it++) {
//         QByteArray key = it.key().toLatin1();
//
//         if (static_cast<QMetaType::Type>(it.value().type()) == QMetaType::QVariantList) {
//             QVariantList values = it.value().value<QVariantList>();
//             foreach (const QVariant &value, values)
//                 out.append(qMakePair(key, value.toString().toLatin1()));
//         } else {
//             out.append(qMakePair(key, it.value().toString().toLatin1()));
//         }
//     }
//
//     return build(out);
// }

ByteArray SetConfCommand::build(const std::list<std::pair<std::string, std::string> >& data)
{
    ByteArray out(m_resetMode ? "RESETCONF" : "SETCONF");

    for (auto& p:data)
    {
        out += " " ;
        out += p.first;

        if (!p.second.empty())
        {
            out += "=" ;
            out += quotedString(p.second);
        }
    }

    out.append("\r\n");
    return out;
}

void SetConfCommand::onReply(int statusCode, const ByteArray &data)
{
    TorControlCommand::onReply(statusCode, data);
    if (statusCode != 250)
        m_errorMessage = data.toString();
}

void SetConfCommand::onFinished(int statusCode)
{
    TorControlCommand::onFinished(statusCode);
    if (isSuccessful())
        mConfSucceeded();
    else
        mConfFailed(statusCode);
}

