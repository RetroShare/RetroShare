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

#include <QApplication>
#include <QIcon>

#include <retroshare/rsplugin.h>
#include <QTranslator>

#include "FeedReaderPlugin.h"
#include "gui/FeedReaderDialog.h"
#include "gui/FeedReaderConfig.h"
#include "services/p3FeedReader.h"

#define IMAGE_FEEDREADER ":/images/FeedReader.png"

static void *inited = new FeedReaderPlugin();

extern "C" {
#ifdef WIN32
	__declspec(dllexport)
#endif
	void *RETROSHARE_PLUGIN_provide()
	{
		static FeedReaderPlugin *p = new FeedReaderPlugin();

		return (void*)p;
	}
}

void FeedReaderPlugin::getPluginVersion(int& major,int& minor,int& svn_rev) const
{
	major = 5;
	minor = 1;
	svn_rev = 4350;
}

FeedReaderPlugin::FeedReaderPlugin()
{
	mainpage = NULL ;
	mIcon = NULL ;
	mPlugInHandler = NULL;
	mFeedReader = NULL;
}

void FeedReaderPlugin::setInterfaces(RsPlugInInterfaces &/*interfaces*/)
{
}

ConfigPage *FeedReaderPlugin::qt_config_page() const
{
	return new FeedReaderConfig();
}

MainPage *FeedReaderPlugin::qt_page() const
{
	if (mainpage == NULL) {
		mainpage = new FeedReaderDialog(mFeedReader);
	}

	return mainpage;
}

RsPQIService *FeedReaderPlugin::rs_pqi_service() const
{
	if (mFeedReader == NULL) {
		mFeedReader = new p3FeedReader(mPlugInHandler);
		rsFeedReader = mFeedReader;
	}

	return mFeedReader;
}

void FeedReaderPlugin::stop()
{
	if (mFeedReader) {
		mFeedReader->stop();
	}
}

void FeedReaderPlugin::setPlugInHandler(RsPluginHandler *pgHandler)
{
	mPlugInHandler = pgHandler;
}

QIcon *FeedReaderPlugin::qt_icon() const
{
	if (mIcon == NULL) {
		Q_INIT_RESOURCE(FeedReader_images);

		mIcon = new QIcon(IMAGE_FEEDREADER);
	}

	return mIcon;
}

std::string FeedReaderPlugin::getShortPluginDescription() const
{
	return QApplication::translate("FeedReaderPlugin", "This plugin provides a Feedreader.").toUtf8().constData();
}

std::string FeedReaderPlugin::getPluginName() const
{
	return QApplication::translate("FeedReaderPlugin", "FeedReader").toUtf8().constData();
}

QTranslator* FeedReaderPlugin::qt_translator(QApplication */*app*/, const QString& languageCode) const
{
	if (languageCode == "en") {
		return NULL;
	}

	QTranslator* translator = new QTranslator();
	if (translator->load(":/lang/FeedReader_" + languageCode + ".qm")) {
		return translator;
	}

	delete(translator);
	return NULL;
}
