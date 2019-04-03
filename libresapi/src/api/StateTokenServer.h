/*******************************************************************************
 * libresapi/api/StateTokenServer.h                                            *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <util/rsthreads.h>
#include "ResourceRouter.h"

namespace resource_api
{

//class StreamBase;
//class StateToken;

// serialiser/deserialiser (depends on stream type)
// for single value
StreamBase& operator <<(StreamBase& left, KeyValueReference<StateToken> kv);
// for lists
StreamBase& operator <<(StreamBase& left, StateToken& token);
bool operator ==(const StateToken& left, const StateToken& right);

class Tickable{
public:
    virtual void tick() = 0;
};


class StateTokenServer: public ResourceRouter
{
public:
    StateTokenServer();

    // thread safe
    // this allows tokens to be created and destroyed from arbitrary threads
    StateToken getNewToken();
    void discardToken(StateToken token);
    // discard the token and fill in a new one
    void replaceToken(StateToken& token);

    void registerTickClient(Tickable* c);
    void unregisterTickClient(Tickable* c);

private:
    void handleWildcard(Request& req, Response& resp);

    StateToken locked_getNewToken();
    void locked_discardToken(StateToken token);

    RsMutex mMtx;

    uint32_t mNextToken;
    // not sure what the most efficient data structure for simple token storage is
    // have to:
    // - add elements
    // - remove elements by value
    // - store many values
    // vector: expensive token erase, could make this better with a invalidate flag
    // and periodic cleanup
    // list: lots of overhead for pointers
    // a set would offer cheap lookup
    // we have to lookup often, so maybe this would be an option
    // have to see where the bottleneck is in practice
    // idea: invalidate all tokens after x minutes/hours, to limit the range of the token values
    // then store the token states in a circular bitbuffer
    std::vector<StateToken> mValidTokens;

    // classes which want to be ticked
    RsMutex mClientsMtx; // needs extra mutex, because clients may call back to modify get/delete tokens
    std::vector<Tickable*> mTickClients;
};

} // namespace resource_api
