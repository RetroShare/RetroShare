#include "StateTokenServer.h"

#include <algorithm>

namespace resource_api
{

// maybe it would be good to make this part of state token or friend, to be able to directly access the value
StreamBase& operator <<(StreamBase& left, KeyValueReference<StateToken> kv)
{
    if(left.serialise())
    {
        // have to make a variable, to be able to pass it by reference
        // (cant pass return value of a function by refernce to another function)
        int value = kv.value.getValue();
        left << makeKeyValueReference(kv.key, value);
    }
    else
    {
        int value;
        left << makeKeyValueReference(kv.key, value);
        kv.value = StateToken(value);
    }
    return left;
}

StreamBase& operator<<(StreamBase& left, StateToken& token)
{
    if(left.serialise())
    {
        // have to make a variable, to be able to pass it by reference
        // (cant pass return value of a function by refernce to another function)
        int value = token.getValue();
        left << value;
    }
    else
    {
        int value;
        left << value;
        token = StateToken(value);
    }
    return left;
}

bool operator==(const StateToken& left, const StateToken& right)
{
    if(left.getValue() == right.getValue())
    {
        return true;
    }
    else
    {
        return false;
    }
}

StateTokenServer::StateTokenServer():
    mMtx("StateTokenServer mMtx"),
    mNextToken(1),
    mClientsMtx("StateTokenServer mClientsMtx")
{
    addResourceHandler("*", this, &StateTokenServer::handleWildcard);
}

StateToken StateTokenServer::getNewToken()
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    return locked_getNewToken();
}

void StateTokenServer::discardToken(StateToken token)
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    locked_discardToken(token);
}

void StateTokenServer::replaceToken(StateToken &token)
{
    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    locked_discardToken(token);
    token = locked_getNewToken();
}

void StateTokenServer::registerTickClient(Tickable *c)
{
    // extra service: tick it to let it init its ticking stuff
    c->tick();

    // avoid double registration
    unregisterTickClient(c);

    RsStackMutex stack(mClientsMtx); /********** STACK LOCKED MTX ***********/
    mTickClients.push_back(c);
}

void StateTokenServer::unregisterTickClient(Tickable *c)
{
    RsStackMutex stack(mClientsMtx); /********** STACK LOCKED MTX ***********/
    std::vector<Tickable*>::iterator vit = std::find(mTickClients.begin(), mTickClients.end(), c);
    if(vit != mTickClients.end())
        mTickClients.erase(vit);
}

void StateTokenServer::handleWildcard(Request &req, Response &resp)
{
    {
        RsStackMutex stack(mClientsMtx); /********** STACK LOCKED MTX ***********/
        for(std::vector<Tickable*>::iterator vit = mTickClients.begin(); vit != mTickClients.end(); ++vit)
        {
            (*vit)->tick();
        }
    }

    RsStackMutex stack(mMtx); /********** STACK LOCKED MTX ******/
    // want to lookpup many tokens at once, return a list of invalid tokens
    // TODO: make generic list serialiser/deserialiser
    while(req.mStream.hasMore())
    {
        StateToken token;
        req.mStream << token;
        // lookup if token is valid
        if(std::find(mValidTokens.begin(), mValidTokens.end(), token) == mValidTokens.end())
        {
            // if invalid, add to response list
            resp.mDataStream << token;
        }
    }
    resp.setOk();
}

StateToken StateTokenServer::locked_getNewToken()
{
    StateToken token(mNextToken);
    mValidTokens.push_back(token);
    mNextToken++;
    if(mNextToken == 0) // 0 is a reserved value, don't ever use it
        mNextToken = 1;
    return token;
}

void StateTokenServer::locked_discardToken(StateToken token)
{
    std::vector<StateToken>::iterator toDelete = std::find(mValidTokens.begin(), mValidTokens.end(), token);
    if(toDelete != mValidTokens.end())
    {
        mValidTokens.erase(toDelete);
    }
}

} // namespace resource_api
