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
#include "jsonapi/jsonapi.h"

class p3WebUI: public RsWebUI, public JsonApiResourceProvider
{
public:
    p3WebUI(){}
    virtual ~p3WebUI(){}

    // implements RsWebUI

    virtual void setHtmlFilesDirectory(const std::string& html_dir) override;
    virtual void setUserPassword(const std::string& passwd) override;

    virtual bool restart() override ;
    virtual bool stop() override ;
    virtual int  status() const override;

    // implements JsonApiResourceProvider

    virtual std::string getName() const override { return "Web Interface" ;}
    virtual std::vector<std::shared_ptr<restbed::Resource> > getResources() const override;
};


