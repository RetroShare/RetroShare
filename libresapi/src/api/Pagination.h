#pragma once

#include "ApiTypes.h"

namespace resource_api
{

// C must be a type with STL like iterator, a begin() and an end() method
// additionally a function id() which gives a unique value for every container element
// the type of the id should be string
// the type returned by dereferencing the iterator should have a stream operator for StreamBase
// the stream operator must not add an element "id", this is done by the pagination handler
template<class C>
void handlePaginationRequest(Request& req, Response& resp, const C& data)
{
    if(!req.isGet()){
        resp.mDebug << "unsupported method. only GET is allowed." << std::endl;
        resp.setFail();
        return;
    }
    if(data.begin() == data.end()){
        // set result type to list
        resp.mDataStream.getStreamToMember();
        resp.mDebug << "note: list is empty" << std::endl;
        return;
    }

    std::string first;
    std::string last;
    req.mStream << makeKeyValueReference("first", first) << makeKeyValueReference("last", last);

    C::iterator it_first = data.begin();
    if(first != "begin")
    {
        while(it_first != data.end() && id(*it_first) != first)
            it_first++;
        if(it_first == data.end())
        {
            resp.setFail("Error: first id did not match any element");
            return;
        }
    }

    C::iterator it_last = data.begin();
    if(last == "end")
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

    int count = 0;
    for(C::iterator it = it_first; it != it_last; ++it)
    {
        StreamBase& stream = resp.mDataStream.getStreamToMember();
        stream << *it;
        stream << makeKeyValue("id", id(*it));

        // todo: also handle the case when the last element is specified and the first element is begin
        // then want to return the elements near the specified element
        count++;
        if(count > 20){
            resp.mDebug << "limited the number of returned items to 20" << std::endl;
            break;
        }
    }
    resp.setOk();
}

} // namespace resource_api
