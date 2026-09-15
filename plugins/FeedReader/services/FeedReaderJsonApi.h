/*******************************************************************************
 * plugins/FeedReader/services/FeedReaderJsonApi.h                             *
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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "jsonapi/jsonapi.h"

class RsFeedReader;

class FeedReaderJsonApi final: public JsonApiResourceProvider
{
public:
	FeedReaderJsonApi(RsFeedReader& feedReader, RsJsonApi& jsonApi);

	std::vector<std::shared_ptr<restbed::Resource>> getResources() const override;
	std::string getName() const override { return "FeedReader"; }

private:
	RsFeedReader& mFeedReader;
	RsJsonApi& mJsonApi;
};
