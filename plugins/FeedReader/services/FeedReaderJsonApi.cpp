/*******************************************************************************
 * plugins/FeedReader/services/FeedReaderJsonApi.cpp                           *
 *                                                                             *
 * Copyright (C) 2026 RetroShare Team                                          *
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

#include "FeedReaderJsonApi.h"

#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "interface/rsFeedReader.h"
#include "retroshare/rsjsonapi.h"
#include "util/radix64.h"

namespace
{
using Document = rapidjson::Document;
using Value = rapidjson::Value;
using Allocator = Document::AllocatorType;

const std::multimap<std::string, std::string> jsonHeaders = {
	{"Access-Control-Allow-Origin", "*"},
	{"Access-Control-Allow-Headers", "Authorization, Content-Type"},
	{"Access-Control-Allow-Methods", "POST, OPTIONS"},
	{"Content-Type", "application/json"}
};

void closeJson(const std::shared_ptr<restbed::Session>& session, int status, const Document& doc)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	auto headers = jsonHeaders;
	headers.emplace("Content-Length", std::to_string(buffer.GetSize()));
	session->close(status, buffer.GetString(), headers);
}

void closeError(const std::shared_ptr<restbed::Session>& session, int status, const std::string& message)
{
	Document doc;
	doc.SetObject();
	doc.AddMember("ok", false, doc.GetAllocator());
	doc.AddMember("error", Value(message.c_str(), doc.GetAllocator()), doc.GetAllocator());
	closeJson(session, status, doc);
}

bool authenticate(const std::shared_ptr<restbed::Session>& session, RsJsonApi& jsonApi)
{
	std::istringstream header(session->get_request()->get_header("Authorization"));
	std::string scheme, encoded;
	header >> scheme >> encoded;
	if(scheme != "Basic" || encoded.empty()) return false;

	const auto decoded = Radix64::decode(encoded);
	if(decoded.empty()) return false;
	const std::string token(reinterpret_cast<const char*>(decoded.data()), decoded.size());
	return jsonApi.isAuthTokenValid(token);
}

void addString(Value& object, const char* name, const std::string& value, Allocator& allocator)
{
	object.AddMember(Value(name, allocator), Value(value.c_str(), value.size(), allocator), allocator);
}

Value feedToJson(const FeedInfo& feed, Allocator& allocator)
{
	Value out(rapidjson::kObjectType);
	out.AddMember("feedId", feed.feedId, allocator);
	out.AddMember("parentId", feed.parentId, allocator);
	addString(out, "url", feed.url, allocator);
	addString(out, "name", feed.name, allocator);
	addString(out, "description", feed.description, allocator);
	addString(out, "icon", feed.icon, allocator);
	out.AddMember("updateInterval", feed.updateInterval, allocator);
	out.AddMember("lastUpdate", static_cast<int64_t>(feed.lastUpdate), allocator);
	out.AddMember("storageTime", feed.storageTime, allocator);
	out.AddMember("workState", static_cast<unsigned>(feed.workstate), allocator);
	out.AddMember("errorState", static_cast<unsigned>(feed.errorState), allocator);
	addString(out, "errorString", feed.errorString, allocator);
	out.AddMember("folder", feed.flag.folder, allocator);
	out.AddMember("deactivated", feed.flag.deactivated, allocator);
	out.AddMember("authentication", feed.flag.authentication, allocator);
	return out;
}

Value messageToJson(const FeedMsgInfo& msg, Allocator& allocator)
{
	Value out(rapidjson::kObjectType);
	addString(out, "msgId", msg.msgId, allocator);
	out.AddMember("feedId", msg.feedId, allocator);
	addString(out, "title", msg.title, allocator);
	addString(out, "link", msg.link, allocator);
	addString(out, "author", msg.author, allocator);
	addString(out, "description", msg.description, allocator);
	addString(out, "descriptionTransformed", msg.descriptionTransformed, allocator);
	out.AddMember("pubDate", static_cast<int64_t>(msg.pubDate), allocator);
	addString(out, "attachmentLink", msg.attachmentLink, allocator);
	addString(out, "attachmentMimeType", msg.attachmentMimeType, allocator);
	out.AddMember("isNew", msg.flag.isnew, allocator);
	out.AddMember("read", msg.flag.read, allocator);
	out.AddMember("deleted", msg.flag.deleted, allocator);
	return out;
}

bool uintMember(const Document& doc, const char* name, uint32_t& value)
{
	if(!doc.HasMember(name) || !doc[name].IsUint()) return false;
	value = doc[name].GetUint();
	return true;
}

bool stringMember(const Document& doc, const char* name, std::string& value)
{
	if(!doc.HasMember(name) || !doc[name].IsString()) return false;
	value = doc[name].GetString();
	return true;
}

template<typename Handler>
std::shared_ptr<restbed::Resource> makeResource(
        const std::string& path, RsJsonApi& jsonApi, Handler handler )
{
	auto resource = std::make_shared<restbed::Resource>();
	resource->set_path(path);
	resource->set_method_handler("OPTIONS", [](const std::shared_ptr<restbed::Session> session)
	{
		session->close(restbed::NO_CONTENT, jsonHeaders);
	});
	resource->set_authentication_handler([jsonApiPtr = &jsonApi](
	        const std::shared_ptr<restbed::Session> session,
	        const std::function<void(const std::shared_ptr<restbed::Session>)>& callback)
	{
		if( session->get_request()->get_method() == "OPTIONS" ||
		        authenticate(session, *jsonApiPtr) ) callback(session);
		else session->close(restbed::UNAUTHORIZED, jsonHeaders);
	});
	resource->set_method_handler("POST", [handler](const std::shared_ptr<restbed::Session> session)
	{
		const auto size = session->get_request()->get_header("Content-Length", 0);
		session->fetch(static_cast<size_t>(size), [handler](
		        const std::shared_ptr<restbed::Session> fetched, const restbed::Bytes& body)
		{
			Document request;
			request.Parse(reinterpret_cast<const char*>(body.data()), body.size());
			if(request.HasParseError() || !request.IsObject())
			{
				closeError(fetched, restbed::BAD_REQUEST, "Invalid JSON request body");
				return;
			}
			handler(fetched, request);
		});
	});
	return resource;
}
}

FeedReaderJsonApi::FeedReaderJsonApi(
        RsFeedReader& feedReader, RsJsonApi& jsonApi ):
    mFeedReader(feedReader), mJsonApi(jsonApi) {}

std::vector<std::shared_ptr<restbed::Resource>> FeedReaderJsonApi::getResources() const
{
	std::vector<std::shared_ptr<restbed::Resource>> resources;
	auto resource = [this](const std::string& path, auto handler)
	{
		return makeResource(path, mJsonApi, handler);
	};

	resources.push_back(resource("/rsFeedReader/getFeeds", [this](auto session, const Document& request)
	{
		uint32_t parentId = 0;
		if(request.HasMember("parentId") && !uintMember(request, "parentId", parentId))
			return closeError(session, restbed::BAD_REQUEST, "parentId must be an unsigned integer");
		std::list<FeedInfo> feeds;
		mFeedReader.getFeedList(parentId, feeds);
		Document response;
		response.SetObject();
		response.AddMember("ok", true, response.GetAllocator());
		Value array(rapidjson::kArrayType);
		for(const auto& feed: feeds) array.PushBack(feedToJson(feed, response.GetAllocator()), response.GetAllocator());
		response.AddMember("feeds", array, response.GetAllocator());
		closeJson(session, restbed::OK, response);
	}));

	resources.push_back(resource("/rsFeedReader/getMessages", [this](auto session, const Document& request)
	{
		uint32_t feedId = 0;
		if(!uintMember(request, "feedId", feedId)) return closeError(session, restbed::BAD_REQUEST, "feedId is required");
		std::list<FeedMsgInfo> messages;
		if(!mFeedReader.getFeedMsgList(feedId, messages)) return closeError(session, restbed::NOT_FOUND, "Feed not found");
		Document response;
		response.SetObject();
		response.AddMember("ok", true, response.GetAllocator());
		Value array(rapidjson::kArrayType);
		for(const auto& msg: messages) array.PushBack(messageToJson(msg, response.GetAllocator()), response.GetAllocator());
		response.AddMember("messages", array, response.GetAllocator());
		closeJson(session, restbed::OK, response);
	}));

	resources.push_back(resource("/rsFeedReader/addFolder", [this](auto session, const Document& request)
	{
		uint32_t parentId = 0, feedId = 0;
		std::string name;
		if(!stringMember(request, "name", name) || name.empty()) return closeError(session, restbed::BAD_REQUEST, "name is required");
		if(request.HasMember("parentId") && !uintMember(request, "parentId", parentId)) return closeError(session, restbed::BAD_REQUEST, "Invalid parentId");
		const auto result = mFeedReader.addFolder(parentId, name, feedId);
		Document response;
		response.SetObject();
		response.AddMember("ok", result == RS_FEED_RESULT_SUCCESS, response.GetAllocator());
		response.AddMember("result", static_cast<unsigned>(result), response.GetAllocator());
		response.AddMember("feedId", feedId, response.GetAllocator());
		closeJson(session, result == RS_FEED_RESULT_SUCCESS ? restbed::OK : restbed::BAD_REQUEST, response);
	}));

	resources.push_back(resource("/rsFeedReader/addFeed", [this](auto session, const Document& request)
	{
		FeedInfo feed;
		uint32_t feedId = 0;
		if(!stringMember(request, "url", feed.url) || feed.url.empty()) return closeError(session, restbed::BAD_REQUEST, "url is required");
		stringMember(request, "name", feed.name);
		if(request.HasMember("parentId") && !uintMember(request, "parentId", feed.parentId)) return closeError(session, restbed::BAD_REQUEST, "Invalid parentId");
		if(request.HasMember("updateInterval")) uintMember(request, "updateInterval", feed.updateInterval);
		else feed.flag.standardUpdateInterval = true;
		if(request.HasMember("storageTime")) uintMember(request, "storageTime", feed.storageTime);
		else feed.flag.standardStorageTime = true;
		if(request.HasMember("deactivated") && request["deactivated"].IsBool()) feed.flag.deactivated = request["deactivated"].GetBool();
		if(request.HasMember("user") && request["user"].IsString()) feed.user = request["user"].GetString();
		if(request.HasMember("password") && request["password"].IsString()) feed.password = request["password"].GetString();
		feed.flag.authentication = !feed.user.empty();
		const auto result = mFeedReader.addFeed(feed, feedId);
		Document response;
		response.SetObject();
		response.AddMember("ok", result == RS_FEED_RESULT_SUCCESS, response.GetAllocator());
		response.AddMember("result", static_cast<unsigned>(result), response.GetAllocator());
		response.AddMember("feedId", feedId, response.GetAllocator());
		closeJson(session, result == RS_FEED_RESULT_SUCCESS ? restbed::OK : restbed::BAD_REQUEST, response);
	}));

	resources.push_back(resource("/rsFeedReader/updateFeed", [this](auto session, const Document& request)
	{
		uint32_t feedId = 0;
		if(!uintMember(request, "feedId", feedId)) return closeError(session, restbed::BAD_REQUEST, "feedId is required");
		FeedInfo feed;
		if(!mFeedReader.getFeedInfo(feedId, feed)) return closeError(session, restbed::NOT_FOUND, "Feed not found");
		if(feed.flag.folder)
		{
			std::string name;
			if(!stringMember(request, "name", name) || name.empty()) return closeError(session, restbed::BAD_REQUEST, "name is required");
			const auto result = mFeedReader.setFolder(feedId, name);
			Document response; response.SetObject(); response.AddMember("ok", result == RS_FEED_RESULT_SUCCESS, response.GetAllocator());
			return closeJson(session, result == RS_FEED_RESULT_SUCCESS ? restbed::OK : restbed::BAD_REQUEST, response);
		}
		stringMember(request, "url", feed.url);
		stringMember(request, "name", feed.name);
		if(request.HasMember("updateInterval")) uintMember(request, "updateInterval", feed.updateInterval);
		if(request.HasMember("storageTime")) uintMember(request, "storageTime", feed.storageTime);
		if(request.HasMember("deactivated") && request["deactivated"].IsBool()) feed.flag.deactivated = request["deactivated"].GetBool();
		if(request.HasMember("user") && request["user"].IsString()) feed.user = request["user"].GetString();
		if(request.HasMember("password") && request["password"].IsString()) feed.password = request["password"].GetString();
		feed.flag.authentication = !feed.user.empty();
		const auto result = mFeedReader.setFeed(feedId, feed);
		Document response; response.SetObject(); response.AddMember("ok", result == RS_FEED_RESULT_SUCCESS, response.GetAllocator());
		response.AddMember("result", static_cast<unsigned>(result), response.GetAllocator());
		closeJson(session, result == RS_FEED_RESULT_SUCCESS ? restbed::OK : restbed::BAD_REQUEST, response);
	}));

	auto idAction = [this, &resource](const std::string& path, auto action)
	{
		return resource(path, [this, action](auto session, const Document& request)
		{
			uint32_t feedId = 0;
			if(!uintMember(request, "feedId", feedId)) return closeError(session, restbed::BAD_REQUEST, "feedId is required");
			const bool ok = action(feedId);
			Document response; response.SetObject(); response.AddMember("ok", ok, response.GetAllocator());
			closeJson(session, ok ? restbed::OK : restbed::NOT_FOUND, response);
		});
	};
	resources.push_back(idAction("/rsFeedReader/removeFeed", [this](uint32_t id){ return mFeedReader.removeFeed(id); }));
	resources.push_back(idAction("/rsFeedReader/refreshFeed", [this](uint32_t id){ return mFeedReader.processFeed(id); }));

	resources.push_back(resource("/rsFeedReader/setMessageRead", [this](auto session, const Document& request)
	{
		uint32_t feedId = 0; std::string msgId;
		if(!uintMember(request, "feedId", feedId) || !stringMember(request, "msgId", msgId) || !request.HasMember("read") || !request["read"].IsBool())
			return closeError(session, restbed::BAD_REQUEST, "feedId, msgId and read are required");
		const bool ok = mFeedReader.setMessageRead(feedId, msgId, request["read"].GetBool());
		Document response; response.SetObject(); response.AddMember("ok", ok, response.GetAllocator());
		closeJson(session, ok ? restbed::OK : restbed::NOT_FOUND, response);
	}));

	resources.push_back(resource("/rsFeedReader/removeMessage", [this](auto session, const Document& request)
	{
		uint32_t feedId = 0; std::string msgId;
		if(!uintMember(request, "feedId", feedId) || !stringMember(request, "msgId", msgId)) return closeError(session, restbed::BAD_REQUEST, "feedId and msgId are required");
		const bool ok = mFeedReader.removeMsg(feedId, msgId);
		Document response; response.SetObject(); response.AddMember("ok", ok, response.GetAllocator());
		closeJson(session, ok ? restbed::OK : restbed::NOT_FOUND, response);
	}));

	resources.push_back(resource("/rsFeedReader/getSettings", [this](auto session, const Document&)
	{
		Document response; response.SetObject(); response.AddMember("ok", true, response.GetAllocator());
		response.AddMember("storageTime", mFeedReader.getStandardStorageTime(), response.GetAllocator());
		response.AddMember("updateInterval", mFeedReader.getStandardUpdateInterval(), response.GetAllocator());
		response.AddMember("saveInBackground", mFeedReader.getSaveInBackground(), response.GetAllocator());
		closeJson(session, restbed::OK, response);
	}));

	resources.push_back(resource("/rsFeedReader/setSettings", [this](auto session, const Document& request)
	{
		uint32_t value = 0;
		if(request.HasMember("storageTime") && uintMember(request, "storageTime", value)) mFeedReader.setStandardStorageTime(value);
		if(request.HasMember("updateInterval") && uintMember(request, "updateInterval", value)) mFeedReader.setStandardUpdateInterval(value);
		if(request.HasMember("saveInBackground") && request["saveInBackground"].IsBool()) mFeedReader.setSaveInBackground(request["saveInBackground"].GetBool());
		Document response; response.SetObject(); response.AddMember("ok", true, response.GetAllocator()); closeJson(session, restbed::OK, response);
	}));

	return resources;
}
