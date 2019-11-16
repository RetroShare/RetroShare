/*******************************************************************************
 * libretroshare/src/rsserver: p3webui.h                                       *
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

#include <string>
#include <vector>
#include <memory>
#include "retroshare/rswebui.h"
#include "jsonapi/restbedservice.h"

class p3WebUI: public RsWebUI, public RestbedService
{
public:
    p3WebUI(){}
    virtual ~p3WebUI(){}

    virtual void setHtmlFilesDirectory(const std::string& html_dir) override;

    virtual bool restart() override { return RestbedService::restart();}
    virtual bool stop() override { return RestbedService::stop();}
    virtual void setListeningPort(uint16_t port) override { RestbedService::setListeningPort(port) ;}
    virtual void setBindingAddress(const std::string& address) override { RestbedService::setBindAddress(address) ;}
    virtual void setUserPassword(const std::string& passwd) override;

    virtual int status() const override;
    virtual std::vector<std::shared_ptr<restbed::Resource> > getResources() const override;
};


