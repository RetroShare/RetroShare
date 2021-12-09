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

#include "GetConfCommand.h"
#include "StrUtil.h"

using namespace Tor;

GetConfCommand::GetConfCommand(Type t)
    : type(t)
{
}

ByteArray GetConfCommand::build(const std::string &key)
{
    return build(std::list<std::string> { key } );
}

ByteArray GetConfCommand::build(const std::list<std::string> &keys)
{
    ByteArray out;
    if (type == GetConf) {
        out = "GETCONF";
    } else if (type == GetInfo) {
        out = "GETINFO";
    } else {
        assert(false);
        return out;
    }

    for(const ByteArray &key: keys) {
        out.append(' ');
        out.append(key);
    }

    out.append("\r\n");
    return out;
}

void GetConfCommand::onReply(int statusCode, const ByteArray &data)
{
    TorControlCommand::onReply(statusCode, data);
    if (statusCode != 250)
        return;

    int kep = data.indexOf('=');
    std::string key = data.mid(0, kep).toString();
    std::string value;
    if (kep >= 0)
        value = unquotedString(data.mid(kep + 1)).toString();

    m_lastKey = key;
    m_results[key].push_back(value);
}

void GetConfCommand::onDataLine(const ByteArray &data)
{
    if (m_lastKey.empty()) {
        RsWarn() << "torctrl: Unexpected data line in GetConf command";
        return;
    }
    m_results[m_lastKey].push_back(data.toString());
}

void GetConfCommand::onDataFinished()
{
    m_lastKey.clear();
}

std::list<std::string> GetConfCommand::get(const std::string& key) const
{
    auto it = m_results.find(key);

    if(it != m_results.end())
        return it->second;
    else
        return std::list<std::string>();
}

