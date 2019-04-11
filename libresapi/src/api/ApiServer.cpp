/*******************************************************************************
 * libresapi/api/ApiServer.cpp                                                 *
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
#include "ApiServer.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include <time.h>
#include <unistd.h>
#include <sstream>
#include <algorithm>
#include "json.h"

#include <retroshare/rsservicecontrol.h>
#include "JsonStream.h"
#include "StateTokenServer.h" // for the state token serialisers

#include "ApiPluginHandler.h"
#include "ChannelsHandler.h"
#include "StatsHandler.h"

#ifdef LIBRESAPI_SETTINGS
    #include "SettingsHandler.h"
#endif

/*
data types in json       http://json.org/
string (utf-8 unicode)
number (int and float)
object (key value pairs, key must be a string)
true
false
null

data types in lua        http://www.lua.org/pil/2.html
nil
boolean
number  (double)
string  (8-bit)
table   (key value pairs, keys can be anything except nil)

data types in QML       http://qt-project.org/doc/qt-5/qtqml-typesystem-basictypes.html
bool
string
real/double
int
list
object types?

QML has many more types with special meaning like date


C++ delivers
std::string
bool
int
(double? i don't know)
enum
bitflags
raw binary data

objects
std::vector
std::list

different types of ids/hashes
-> convert to/from string with a generic operator
-> the operator signals ok/fail to the stream


*/

/*

data types to handle:
- bool
- string
- bitflags
- enums

containers:
- arrays: collection of objects or values without name, usually of the same type
- objects: objects and values with names

careful: the json lib has many asserts, so retroshare will stop if the smalles thing goes wrong
-> check type of json before usage

there are two possible implementations:
- with a virtual base class for the serialisation targets
  - better documentation of the interface
- with templates

*/

/*

the general idea is:
want output in many different formats, while the retrival of the source data is always the same

get, put

ressource adress like
.org.retroshare.api.peers

generic router from adress to the ressource handler object

data formats for input and output:
- json
- lua
- QObject
- maybe just a typed c++ object

rest inspired / resource based interface
- have resources with adresses
- requests:
  - put
  - get
- request has parameters
- response:
  - returncode:
    - ok
    - not modified
    - error
  - data = object or list of objects

want to have a typesafe interface at the top?
probably not, a system with generic return values is enough
this interface is for scripting languages which don't have types

the interface at the top should look like this:
template <class RequestDataFormatT, class ResponseDataFormatT>
processRequest(const RequestMeta& req, const RequestDataFormatT& reqData,
               ResponseMeta& respMeta, ResponseDataFormatT& respData);

idea: pass all the interfaces to the retroshare core to this function,
or have this function as part of an object

the processor then applies all members of the request and response data to the data format like this:
reqData << "member1" << member1
        << "member2" << member2 ... ;

these operators have to be implemented for common things like boolean, int, std::string, std::vector, std::list ...
request data gets only deserialised
response data gets only serialised

response and request meta contains things like resource address, method and additional parameters

want generic resource caching mechanism
- on first request a request handler is created
- request handler is stored with its input data
- if a request handler for a given resource adress and parameters exists
  then the request handler is asked if the result is still valid
  if yes the result from the existing handler is used
- request handler gets deleted after timeout
- can have two types of resource handlers: static handlers and dynamic handlers
  - static handlers don't get deleted, because they don't contain result data
  - dynamic handlers contain result data, and thus get deleted after a while

it is even possible to implement a resource-changed check at the highest level
this allows to compute everything on the server side and only send changes to the client
the different resource providers don't have to implement a resource changed check then
a top level change detector will poll them
of course this does not work with a deep resource tree with millions of nodes

for this we have the dynamic handlers,
they are created on demand and know how to listen for changes which affect them

*/

namespace resource_api{

// old code, only to copy and paste from
// to be removed
/*
class ChatlobbiesHandler
{
public:
    ChatlobbiesHandler(RsMsgs* msgs): mMsgs(msgs) {}

    template <class InputT, class OutputT>
    void handleRequest(Request& req, InputT& reqData, Response& resp, OutputT& respData)
    {
        if(req.mMethod == "GET")
        {
            typename OutputT::Array result;
            // subscribed lobbies
            std::list<ChatLobbyInfo> slobbies;
            mMsgs->getChatLobbyList(slobbies);
            for(std::list<ChatLobbyInfo>::iterator lit = slobbies.begin(); lit != slobbies.end(); lit++)
            {
                typename OutputT::Object lobby;
                ChatLobbyInfo& lobbyRecord = *lit;
                lobby["name"] = lobbyRecord.lobby_name;
                RsPeerId pid;
                mMsgs->getVirtualPeerId(lobbyRecord.lobby_id, pid);
                lobby["id"] = pid.toStdString();
                lobby["subscribed"] = true;
                result.push_back(lobby);
            }
            // unsubscirbed lobbies
            std::vector<VisibleChatLobbyRecord> ulobbies;
            mMsgs->getListOfNearbyChatLobbies(ulobbies);
            for(std::vector<VisibleChatLobbyRecord>::iterator vit = ulobbies.begin(); vit != ulobbies.end(); vit++)
            {
                typename OutputT::Object lobby;
                VisibleChatLobbyRecord& lobbyRecord = *vit;
                lobby["name"] = lobbyRecord.lobby_name;
                RsPeerId pid;
                mMsgs->getVirtualPeerId(lobbyRecord.lobby_id, pid);
                lobby["id"] = pid.toStdString();
                lobby["subscribed"] = false;
                result.push_back(lobby);
            }
            respData = result;
        }
        else if(req.mMethod == "PUT")
        {
            RsPeerId id = RsPeerId(req.mAdress.substr(1));

            if(!id.isNull() && reqData.HasKey("msg"))
            {
                // for now can send only id as message
                mMsgs->sendPrivateChat(id, reqData["msg"]);
            }
        }
    }

    RsMsgs* mMsgs;
};
*/

class ApiServerMainModules
{
public:
    ApiServerMainModules(ResourceRouter& router, StateTokenServer* sts, const RsPlugInInterfaces &ifaces):
        mPeersHandler(sts, ifaces.mNotify, ifaces.mPeers, ifaces.mMsgs),
        mIdentityHandler(sts, ifaces.mNotify, ifaces.mIdentity),
        mForumHandler(ifaces.mGxsForums),
        mServiceControlHandler(ifaces.mServiceControl),
        mFileSearchHandler(sts, ifaces.mNotify, ifaces.mTurtle, ifaces.mFiles),
	    mFileSharingHandler(sts, ifaces.mFiles, *ifaces.mNotify),
	    mTransfersHandler(sts, ifaces.mFiles, ifaces.mPeers, *ifaces.mNotify),
        mChatHandler(sts, ifaces.mNotify, ifaces.mMsgs, ifaces.mPeers, ifaces.mIdentity, &mPeersHandler),
        mApiPluginHandler(sts, ifaces),
	    mChannelsHandler(*ifaces.mGxsChannels),
	    mStatsHandler()
#ifdef LIBRESAPI_SETTINGS
	    ,mSettingsHandler(sts)
#endif
    {
        // the dynamic cast is to not confuse the addResourceHandler template like this:
        // addResourceHandler(derived class, parent class)
        // the template would then be instantiated using derived class as parameter
        // and then parent class would not match the type
        router.addResourceHandler("peers",dynamic_cast<ResourceRouter*>(&mPeersHandler),
                                   &PeersHandler::handleRequest);
        router.addResourceHandler("identity", dynamic_cast<ResourceRouter*>(&mIdentityHandler),
                                   &IdentityHandler::handleRequest);
        router.addResourceHandler("forums", dynamic_cast<ResourceRouter*>(&mForumHandler),
                                   &ForumHandler::handleRequest);
        router.addResourceHandler("servicecontrol", dynamic_cast<ResourceRouter*>(&mServiceControlHandler),
                                   &ServiceControlHandler::handleRequest);
        router.addResourceHandler("filesearch", dynamic_cast<ResourceRouter*>(&mFileSearchHandler),
                                   &FileSearchHandler::handleRequest);
		router.addResourceHandler("filesharing", dynamic_cast<ResourceRouter*>(&mFileSharingHandler),
		                           &FileSharingHandler::handleRequest);
        router.addResourceHandler("transfers", dynamic_cast<ResourceRouter*>(&mTransfersHandler),
                                   &TransfersHandler::handleRequest);
        router.addResourceHandler("chat", dynamic_cast<ResourceRouter*>(&mChatHandler),
                                  &ChatHandler::handleRequest);
        router.addResourceHandler("apiplugin", dynamic_cast<ResourceRouter*>(&mApiPluginHandler),
                                  &ChatHandler::handleRequest);
        router.addResourceHandler("channels", dynamic_cast<ResourceRouter*>(&mChannelsHandler),
                                  &ChannelsHandler::handleRequest);
		router.addResourceHandler("stats", dynamic_cast<ResourceRouter*>(&mStatsHandler),
		                          &StatsHandler::handleRequest);
#ifdef LIBRESAPI_SETTINGS
		router.addResourceHandler("settings", dynamic_cast<ResourceRouter*>(&mSettingsHandler),
		                                  &SettingsHandler::handleRequest);
#endif
	}

    PeersHandler mPeersHandler;
    IdentityHandler mIdentityHandler;
    ForumHandler mForumHandler;
    ServiceControlHandler mServiceControlHandler;
    FileSearchHandler mFileSearchHandler;
	FileSharingHandler mFileSharingHandler;
    TransfersHandler mTransfersHandler;
    ChatHandler mChatHandler;
    ApiPluginHandler mApiPluginHandler;
    ChannelsHandler mChannelsHandler;
	StatsHandler mStatsHandler;

#ifdef LIBRESAPI_SETTINGS
	SettingsHandler mSettingsHandler;
#endif
};

ApiServer::ApiServer():
    mMtx("ApiServer mMtx"),
    mStateTokenServer(),
    mLivereloadhandler(&mStateTokenServer),
    mTmpBlobStore(&mStateTokenServer),
    mMainModules(0)
{
    mRouter.addResourceHandler("statetokenservice", dynamic_cast<ResourceRouter*>(&mStateTokenServer),
                               &StateTokenServer::handleRequest);
    mRouter.addResourceHandler("livereload", dynamic_cast<ResourceRouter*>(&mLivereloadhandler),
                               &LivereloadHandler::handleRequest);
}

ApiServer::~ApiServer()
{
    RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    for(std::vector<RequestId>::iterator vit = mRequests.begin(); vit != mRequests.end(); ++vit)
        delete vit->task;
    mRequests.clear();

    if(mMainModules)
        delete mMainModules;
}

void ApiServer::loadMainModules(const RsPlugInInterfaces &ifaces)
{
    RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    if(mMainModules == 0)
        mMainModules = new ApiServerMainModules(mRouter, &mStateTokenServer, ifaces);
}

std::string ApiServer::handleRequest(Request &request)
{
    resource_api::JsonStream outstream;
    std::stringstream debugString;

    StreamBase& data = outstream.getStreamToMember("data");
    resource_api::Response resp(data, debugString);

    ResponseTask* task = 0;
    {
        RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
        task = mRouter.handleRequest(request, resp);
    }

    //time_t start = time(NULL);
    bool morework = true;
    while(task && morework)
    {
        {
            RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
            morework = task->doWork(request, resp);
        }
        if(morework)
            usleep(10*1000);
        /*if(time(NULL) > (start+5))
        {
            std::cerr << "ApiServer::handleRequest() Error: task timed out" << std::endl;
            resp.mDebug << "Error: task timed out." << std::endl;
            resp.mReturnCode = resource_api::Response::FAIL;
            break;
        }*/
    }
    if(task)
        delete task;

    std::string returncode;
    switch(resp.mReturnCode){
    case resource_api::Response::NOT_SET:
        returncode = "not_set";
        break;
    case resource_api::Response::OK:
        returncode = "ok";
        break;
    case resource_api::Response::WARNING:
        returncode = "warning";
        break;
    case resource_api::Response::FAIL:
        returncode = "fail";
        break;
    }

    // evil HACK, remove this
    if(data.isRawData())
        return data.getRawData();

    if(!resp.mCallbackName.empty())
        outstream << resource_api::makeKeyValueReference("callback_name", resp.mCallbackName);

    outstream << resource_api::makeKeyValue("debug_msg", debugString.str());
    outstream << resource_api::makeKeyValueReference("returncode", returncode);
    if(!resp.mStateToken.isNull())
        outstream << resource_api::makeKeyValueReference("statetoken", resp.mStateToken);
    return outstream.getJsonString();
}

ApiServer::RequestId ApiServer::handleRequest(Request &request, Response &response)
{
    RequestId id;
    ResponseTask* task = 0;
    {
        RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
        task = mRouter.handleRequest(request, response);
    }
    if(task == 0)
    {
        id.done = true;
        return id;
    }
    id.done = false,
    id.task = task;
    id.request = &request;
    id.response = &response;
    {
        RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
        mRequests.push_back(id);
    }
    return id;
}

bool ApiServer::isRequestDone(RequestId id)
{
    if(id.done)
        return true;

    RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    std::vector<RequestId>::iterator vit = std::find(mRequests.begin(), mRequests.end(), id);
    // Request id not found, maybe the id is old and was removed from the list
    if(vit == mRequests.end())
        return true;

    if(id.task->doWork(*id.request, *id.response))
        return false;

    // if we reach this point, the request is in the list and done
    // remove the id from the list of valid ids
    // delete the ResponseTask object

    *vit = mRequests.back();
    mRequests.pop_back();

    delete id.task;
    return true;
}

} // namespace resource_api
