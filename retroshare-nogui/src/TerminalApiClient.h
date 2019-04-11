/*******************************************************************************
 * retroshare-nogui/src/TerminalApiClient.h                                    *
 *                                                                             *
 * retroshare-nogui: headless version of retroshare                            *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare.project@gmail.com>         *
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
#include <api/ApiServer.h>
#include <util/rsthreads.h>

namespace resource_api {

// allows basic control from stdin/stdout
// - account selection
// - login
// - shutdown
class TerminalApiClient: public RsTickingThread{
public:
    // zero setup: create an instance of this class and destroy it when not needed anymore
    // no need to call start or stop or something
    // parameter api must not be null
    TerminalApiClient(ApiServer* api);
    ~TerminalApiClient();
protected:
    // from RsThread
    virtual void data_tick(); /* called once the thread is started. Should be overloaded by subclasses. */
private:
	struct AccountInfo
	{
		std::string name ;
		std::string location ;
		RsPeerId    ssl_id ;
	};


    void waitForResponse(ApiServer::RequestId id) const;
    bool isTokenValid(StateToken st) const;
    ApiServer* mApiServer;

	// Methods to talk to the ApiServer

	void sendPassword(const std::string& passwd) const;
	void sendSelectedAccount(const std::string& ssl_id) const;
	void readAvailableAccounts(std::vector<AccountInfo>& accounts) const;
	void getRunningState() const ;
	void readPasswordState(StateToken& password_state_token,bool& ask_for_password,std::string& key_name,bool& prev_is_bad) const;
	void readRunState(StateToken& runstate_state_token, std::string& runstate) const;
};

} // namespace resource_api
