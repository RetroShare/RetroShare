/*******************************************************************************
 * libretroshare/src/jsonapi/: restbedservice.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2019-2019 Cyril Soler                                             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#pragma once

#include <restbed>
#include "util/rsthreads.h"

class RestbedThread;

class RestbedService: public RsThread
{
public:
    RestbedService() ;
	virtual ~RestbedService();

    bool restart();
    bool stop();
    bool isRunning() const;

    void setListeningPort(uint16_t port) ;
	void setBindAddress(const std::string& bind_address);
	uint16_t listeningPort() const ;

    // should be overloaded by sub-class in order to provide resources to the restbed Service.

    virtual std::vector<std::shared_ptr<restbed::Resource> > getResources() const = 0;

protected:
	void runloop() override;

    std::shared_ptr<restbed::Service> mService;	// managed by RestbedService because it needs to be properly deleted when restarted.

private:
    uint16_t mListeningPort;
    std::string mBindingAddress;
};

