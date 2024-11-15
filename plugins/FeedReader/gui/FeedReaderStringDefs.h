/*******************************************************************************
 * plugins/FeedReader/gui/FeedReaderStringDefs.h                               *
 *                                                                             *
 * Copyright (C) 2012 by Thunder <retroshare.project@gmail.com>                *
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

#ifndef FEEDREADER_STRINGDEFS_H
#define FEEDREADER_STRINGDEFS_H

#include <QString>

#include "interface/rsFeedReader.h"

class QWidget;

class FeedReaderStringDefs
{
public:
	static bool showError(QWidget *parent, RsFeedResult result, const QString &title, const QString &text);
	static QString workState(FeedInfo::WorkState state);
	static QString errorString(const FeedInfo &feedInfo);
	static QString errorString(RsFeedReaderErrorState errorState, const std::string &errorString);
	static QString transforationTypeString(RsFeedTransformationType transformationType);
};

#endif
