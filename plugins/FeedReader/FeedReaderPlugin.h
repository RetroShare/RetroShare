/****************************************************************
 *  RetroShare GUI is distributed under the following license:
 *
 *  Copyright (C) 2012 by Thunder
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#pragma once

#include <rsitems/rsserviceids.h>
#include <retroshare/rsplugin.h>
#include <retroshare-gui/mainpage.h>
#include "services/p3FeedReader.h"

class p3FeedReader;
class FeedReaderNotify;
class RsForums;

class FeedReaderPlugin: public RsPlugin
{
public:
	FeedReaderPlugin();

	virtual uint16_t rs_service_id() const { return RS_SERVICE_TYPE_PLUGIN_FEEDREADER; }
	virtual p3Service *p3_service() const { return mFeedReader; }
	virtual p3Config *p3_config() const { return mFeedReader; }
	virtual void stop();

	virtual MainPage *qt_page() const;
	virtual QIcon *qt_icon() const;
	virtual std::string qt_stylesheet() { return "FeedReader"; }
	virtual QTranslator *qt_translator(QApplication *app, const QString& languageCode, const QString& externalDir) const;

	virtual void getPluginVersion(int &major, int &minor, int &build, int &svn_rev) const;
	virtual void setPlugInHandler(RsPluginHandler *pgHandler);

	virtual std::string configurationFileName() const { return "feedreader.cfg" ; }

	virtual std::string getShortPluginDescription() const;
	virtual std::string getPluginName() const;
	virtual void getLibraries(std::list<RsLibraryInfo> &libraries);
	virtual void setInterfaces(RsPlugInInterfaces& interfaces);
	virtual ConfigPage *qt_config_page() const;

	virtual FeedNotify *qt_feedNotify();

private:
	RsPlugInInterfaces mInterfaces;
	mutable p3FeedReader *mFeedReader;
	mutable FeedReaderNotify *mNotify;
	mutable RsPluginHandler *mPlugInHandler;
	mutable MainPage *mainpage;
	mutable QIcon *mIcon;
	mutable FeedNotify *mFeedNotify;
};
