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

class RestbedThread;

class RestbedService
{
public:
    RestbedService() ;
	virtual ~RestbedService();

    bool isRunning() const ;

    virtual bool restart();
    virtual bool stop();

    virtual void setListeningPort(uint16_t port) ;

    virtual std::vector<std::shared_ptr<restbed::Resource> > getResources()const =0;

private:
    RestbedThread *_restbed_thread;
};

