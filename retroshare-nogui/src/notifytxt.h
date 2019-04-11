/*******************************************************************************
 * retroshare-nogui/src/notifytxt.h                                            *
 *                                                                             *
 * retroshare-nogui: headless version of retroshare                            *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare.project@gmail.com>         *
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

#ifndef RSIFACE_NOTIFY_TXT_H
#define RSIFACE_NOTIFY_TXT_H

#include <retroshare/rsiface.h>
#include <retroshare/rsturtle.h>
#include "util/rsthreads.h"

#include <string>

class NotifyTxt: public NotifyClient
{
	public:
		NotifyTxt():mNotifyMtx("NotifyMtx") { return; }
		virtual ~NotifyTxt() { return; }

		virtual void notifyListChange(int list, int type);
		virtual void notifyErrorMsg(int list, int sev, std::string msg);
		virtual void notifyChat();
		virtual bool askForPassword(const std::string& title, const std::string& question, bool prev_is_bad, std::string& password,bool& cancel);
		virtual bool askForPluginConfirmation(const std::string& plugin_file, const std::string& plugin_hash,bool first_time);

		virtual void notifyTurtleSearchResult(const RsPeerId& pid,uint32_t search_id,const std::list<TurtleFileInfo>& found_files);

		/* interface for handling SearchResults */
		void getSearchIds(std::list<uint32_t> &searchIds);

		int getSearchResultCount(uint32_t id);
		int getSearchResults(uint32_t id, std::list<TurtleFileInfo> &searchResults);

		// only collect results for selected searches.
		// will drop others.
		int collectSearchResults(uint32_t searchId);
		int clearSearchId(uint32_t searchId);


	private:

		void displayNeighbours();
		void displayFriends();
		void displayDirectories();
		void displaySearch();
		void displayMessages();
		void displayChannels();
		void displayTransfers();

		/* store TurtleSearchResults */
		RsMutex mNotifyMtx;

		std::map<uint32_t, std::list<TurtleFileInfo> > mSearchResults;
};

#endif
