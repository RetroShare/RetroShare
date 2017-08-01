#pragma once
/*
 * libresapi
 * Copyright (C) 2015  electron128 <electron128@yahoo.com>
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>
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

#include "ApiTypes.h"

namespace resource_api
{

// C must be a type with STL like iterator, a begin() and an end() method
// additionally a function id() which gives a unique value for every container element
// the type of the id should be string
// the type returned by dereferencing the iterator should have a stream operator for StreamBase
// the stream operator must not add an element "id", this is done by the pagination handler
template<class C>
void handlePaginationRequest(Request& req, Response& resp, C& data)
{
    if(data.begin() == data.end()){
        // set result type to list
        resp.mDataStream.getStreamToMember();
        resp.mDebug << "note: list is empty" << std::endl;
        resp.setOk();
        return;
    }

    std::string begin_after;
    std::string last;
	req.mStream << makeKeyValueReference("begin_after", begin_after)
	            << makeKeyValueReference("last", last);

    typename C::iterator it_first = data.begin();
    if(begin_after != "begin" && begin_after != "")
    {
        while(it_first != data.end() && id(*it_first) != begin_after)
            it_first++;
        it_first++; // get after the specified element
        if(it_first == data.end())
        {
            resp.setFail("Error: first id did not match any element");
            return;
        }
    }

    typename C::iterator it_last = data.begin();
    if(last == "end" || last == "")
    {
        it_last = data.end();
    }
    else
    {
        while(it_last != data.end() && id(*it_last) != last)
            it_last++;
        if(it_last == data.end())
        {
            resp.setFail("Error: last id did not match any element");
            return;
        }
        ++it_last; // increment to get iterator to element after the last wanted element
    }

/* G10h4ck: Guarded message count limitation with
 * JSON_API_LIMIT_CHAT_MSG_COUNT_BY_DEFAULT as ATM it seems that handling a
 * big bunch of messages hasn't been a problem for client apps, and even in
 * that case the client can specify +begin_after+ and +last+ in the request,
 * this way we don't make more difficult the life of those who just want get
 * the whole list of chat messages that seems to be a common usecase
 */
#ifdef JSON_API_LIMIT_CHAT_MSG_COUNT_BY_DEFAULT
	int count = 0;
#endif

    for(typename C::iterator it = it_first; it != it_last; ++it)
    {
        StreamBase& stream = resp.mDataStream.getStreamToMember();
        stream << *it;
        stream << makeKeyValue("id", id(*it));

        // todo: also handle the case when the last element is specified and the first element is begin
        // then want to return the elements near the specified element

// G10h4ck: @see first comment about JSON_API_LIMIT_CHAT_MSG_COUNT_BY_DEFAULT
#ifdef JSON_API_LIMIT_CHAT_MSG_COUNT_BY_DEFAULT
		++count;
		if(count > 20)
		{
			resp.mDebug << "limited the number of returned items to 20";
			break;
		}
#endif
    }
    resp.setOk();
}

} // namespace resource_api
