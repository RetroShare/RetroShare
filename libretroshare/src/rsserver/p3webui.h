/*******************************************************************************
 * libretroshare/src/rsserver: p3webui.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2019-2019 Cyril Soler                                             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License version 3 as    *
 * published by the Free Software Foundation.                                  *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <string>
#include <vector>
#include <memory>
#include "retroshare/rswebui.h"
#include "jsonapi/jsonapi.h"

class p3WebUI: public RsWebUi, public JsonApiResourceProvider
{
public:
	~p3WebUI() override = default;

    // implements RsWebUI

    virtual void setHtmlFilesDirectory(const std::string& html_dir) override;
    virtual void setUserPassword(const std::string& passwd) override;

    virtual bool restart() override ;
    virtual bool stop() override ;
	bool isRunning() const override;
    // implements JsonApiResourceProvider

    virtual std::vector<std::shared_ptr<restbed::Resource> > getResources() const override;
};


