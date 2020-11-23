/*
 * RetroShare JSON API public header
 *
 * Copyright (C) 2018-2020  Gioacchino Mazzurco <gio@eigenlab.org>
 * Copyright (C) 2019  Cyril Soler <csoler@users.sourceforge.net>
 * Copyright (C) 2020  Asociación Civil Altermundi <info@altermundi.net>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 * SPDX-FileCopyrightText: 2004-2019 RetroShare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <map>
#include <string>
#include <cstdint>
#include <system_error>

#include "util/rsdebug.h"
#include "util/rsmemory.h"

class RsJsonApi;

/**
 * Pointer to global instance of RsJsonApi service implementation
 * @jsonapi{development}
 */
extern RsJsonApi* rsJsonApi;

enum class RsJsonApiErrorNum : int32_t
{
	TOKEN_FORMAT_INVALID             = 2004,
	UNKNOWN_API_USER                 = 2005,
	WRONG_API_PASSWORD               = 2006,
	API_USER_CONTAIN_COLON           = 2007,
	AUTHORIZATION_REQUEST_DENIED     = 2008,
	CANNOT_EXECUTE_BEFORE_RS_LOGIN   = 2009,
	NOT_A_MACHINE_GUN                = 2010
};

struct RsJsonApiErrorCategory: std::error_category
{
	const char* name() const noexcept override
	{ return "RetroShare JSON API"; }

	std::string message(int ev) const override
	{
		switch (static_cast<RsJsonApiErrorNum>(ev))
		{
		case RsJsonApiErrorNum::TOKEN_FORMAT_INVALID:
			return "Invalid token format, must be alphanumeric_user:password";
		case RsJsonApiErrorNum::UNKNOWN_API_USER:
			return "Unknown API user";
		case RsJsonApiErrorNum::WRONG_API_PASSWORD:
			return "Wrong API password";
		case RsJsonApiErrorNum::API_USER_CONTAIN_COLON:
			return "API user cannot contain colon character";
		case RsJsonApiErrorNum::AUTHORIZATION_REQUEST_DENIED:
			return "User denied new token autorization";
		case RsJsonApiErrorNum::CANNOT_EXECUTE_BEFORE_RS_LOGIN:
			return "This operation cannot be executed bedore RetroShare login";
		case RsJsonApiErrorNum::NOT_A_MACHINE_GUN:
			return "Method must not be called in burst";
		default:
			return rsErrorNotInCategory(ev, name());
		}
	}

	std::error_condition default_error_condition(int ev) const noexcept override;

	const static RsJsonApiErrorCategory instance;
};


namespace std
{
/** Register RsJsonApiErrorNum as an error condition enum, must be in std
 * namespace */
template<> struct is_error_condition_enum<RsJsonApiErrorNum> : true_type {};
}

/** Provide RsJsonApiErrorNum conversion to std::error_condition, must be in
 * same namespace of RsJsonApiErrorNum */
inline std::error_condition make_error_condition(RsJsonApiErrorNum e) noexcept
{
	return std::error_condition(
	            static_cast<int>(e), RsJsonApiErrorCategory::instance );
};


class p3ConfigMgr;
class JsonApiResourceProvider;

class RsJsonApi
{
public:
	static const uint16_t    DEFAULT_PORT = 9092;
	static const std::string DEFAULT_BINDING_ADDRESS; // 127.0.0.1

	/**
	 * @brief Restart RsJsonApi server.
	 * This method is asyncronous when called from JSON API.
	 * @jsonapi{development,manualwrapper}
	 * @return Success or error details
	 */
	virtual std::error_condition restart() = 0;

	/** @brief Request RsJsonApi to stop and wait until it has stopped.
	 * Do not expose this method to JSON API as fullstop must not be called from
	 * the same thread of service execution.
	 */
	virtual void fullstop() = 0;

	/**
	 * @brief Request RsJsonApi to stop asynchronously.
	 * @jsonapi{development}
	 * Be expecially carefull to call this from JSON API because you will loose
	 * access to the API.
	 * If you need to wait until stopping has completed @see isRunning().
	 */
	virtual void askForStop() = 0;

	/**
	 * @brief Get status of the json api server
	 * @return Returns true if the server is running
	 */
	virtual bool isRunning() = 0;

	/**
	 * Sets the binding address of the JSON API server. Will only take effect
	 * after the server is restarted.
	 * @jsonapi{development}
	 * @param[in] address Binding address in IPv4 or IPv6 format.
	 */
	virtual void setBindingAddress(const std::string& address) = 0;

	/**
	 * Get the binding address of the JSON API server.
	 * @jsonapi{development}
	 * @return string representing binding address
	 */
	virtual std::string getBindingAddress() const = 0;

	/*!
	 * Set port on which JSON API server will listen. Will only take effect
	 * after the server is restarted.
	 * @jsonapi{development}
	 * @param[in] port Must be available otherwise the binding will fail
	 */
	virtual void setListeningPort(uint16_t port) = 0;

	/*!
	 * Get port on which JSON API server will listen.
	 * @jsonapi{development}
	 */
	virtual uint16_t listeningPort() const = 0;

	/*!
	 * Should be called after creating the JsonAPI object so that it publishes
	 * itself with the proper config file.
	 * Since JsonAPI is created *before* login, the config manager does not
	 * exist at that time.
	 * @param cfgmgr
	 */
	virtual void connectToConfigManager(p3ConfigMgr& cfgmgr) = 0;

	/**
	 * This is used to add/remove new web services to JsonAPI. The client
	 * should take care of not using a path range already used by the jsonAPI
	 * server
	 */
	virtual void registerResourceProvider(const JsonApiResourceProvider&) = 0;
	virtual void unregisterResourceProvider(const JsonApiResourceProvider&) = 0;
	virtual bool hasResourceProvider(const JsonApiResourceProvider&) = 0;

	/**
	 * @brief This function should be used by JSON API clients that aren't
	 * authenticated yet, to ask their token to be authorized, the success or
	 * failure will depend on mNewAccessRequestCallback return value, and it
	 * will likely need human user interaction in the process.
	 * @jsonapi{development,unauthenticated}
	 * @param[in] user user name to authorize
	 * @param[in] password password for the new user
	 * @return if an error occurred details about it.
	 */
	virtual std::error_condition requestNewTokenAutorization(
	        const std::string& user, const std::string& password) = 0;

	/** Split a token in USER:PASSWORD format into user and password */
	static bool parseToken(
	        const std::string& clear_token,
	        std::string& user, std::string& passwd );

	/**
	 * Add new API auth user, passwd to the authorized set.
	 * @jsonapi{development}
	 * @param[in] user user name to autorize, must not contain ':'
	 * @param[in] password password for the user
	 * @return if some error occurred return details about it
	 */
	virtual std::error_condition authorizeUser(
	        const std::string& user, const std::string& password ) = 0;

	/**
	 * @brief Revoke given auth token
	 * @jsonapi{development}
	 * @param[in] user user name of which to revoke authorization
	 * @return true if the token has been revoked, false otherwise
	 */
	virtual bool revokeAuthToken(const std::string& user)=0;

	/**
	 * @brief Get authorized tokens
	 * @jsonapi{development}
	 * @return the set of authorized encoded tokens
	 */
	virtual std::map<std::string,std::string> getAuthorizedTokens() = 0;

	/**
	 * @brief Check if given JSON API auth token is authorized
	 * @jsonapi{development,unauthenticated}
	 * @param[in] token decoded
	 * @param[out] error optional storage for error details
	 * @return true if authorized, false otherwise
	 */
	virtual bool isAuthTokenValid(
	        const std::string& token,
	        std::error_condition& error = RS_DEFAULT_STORAGE_PARAM(std::error_condition)
	        ) = 0;

	/**
	 * @brief Write version information to given paramethers
	 * @jsonapi{development,unauthenticated}
	 * @param[out] major storage
	 * @param[out] minor storage
	 * @param[out] mini storage
	 * @param[out] extra storage
	 * @param[out] human storage
	 */
	static void version( uint32_t& major, uint32_t& minor, uint32_t& mini,
	                     std::string& extra, std::string& human );

	virtual ~RsJsonApi() = default;
};

