/*
 * RetroShare Web User Interface
 *
 * Copyright (C) 2019  Cyril Soler <csoler@users.sourceforge.net>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>
 *
 * SPDX-FileCopyrightText: 2004-2019 RetroShare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

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
