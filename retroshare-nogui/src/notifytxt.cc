/*******************************************************************************
 * retroshare-nogui/src/notifytxt.cc                                           *
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

#include <stdio.h>
#include "notifytxt.h"
#include <retroshare/rspeers.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#ifdef WINDOWS_SYS
#include <conio.h>
#include <stdio.h>

#define PASS_MAX 512

char *getpass (const char *prompt)
{
    static char getpassbuf [PASS_MAX + 1];
    size_t i = 0;
    int c;

    if (prompt) {
        fputs (prompt, stderr);
        fflush (stderr);
    }

    for (;;) {
        c = _getch ();
        if (c == '\r') {
            getpassbuf [i] = '\0';
            break;
        }
        else if (i < PASS_MAX) {
            getpassbuf[i++] = c;
        }

        if (i >= PASS_MAX) {
            getpassbuf [i] = '\0';
            break;
        }
    }

    if (prompt) {
        fputs ("\r\n", stderr);
        fflush (stderr);
    }

    return getpassbuf;
}
#endif

void NotifyTxt::notifyErrorMsg(int /* list */, int /* type */, std::string /* msg */)
{
	return;
}

void NotifyTxt::notifyChat()
{
	return;
}

bool NotifyTxt::askForPluginConfirmation(const std::string& plugin_file_name, const std::string& plugin_file_hash, bool /* first_time */)
{
	std::cerr << "The following plugin is not registered as accepted or denied. You probably upgraded the main executable or the plugin itself." << std::endl;
	std::cerr << "   Hash: " << plugin_file_hash << std::endl;
	std::cerr << "   File: " << plugin_file_name << std::endl;

	char a = 0 ;
	while(a != 'y' && a != 'n')
	{
		std::cerr << "Enable this plugin ? (y/n) :" ;
		std::cerr.flush() ;

		a = fgetc(stdin) ;
	}
	return a == 'y' ;
}

bool NotifyTxt::askForPassword(const std::string& title, const std::string& question, bool /* prev_is_bad */, std::string& password,bool& cancel)
{
	std::string question1=title + "\nPlease enter your PGP password for key:\n    " + question + " :";
	char *passwd = getpass(question1.c_str()) ;
	password = passwd;
    cancel = false ;

	return !password.empty();
}


void NotifyTxt::notifyListChange(int list, int /* type */)
{
	//std::cerr << "NotifyTxt::notifyListChange()" << std::endl;
	switch(list)
	{
//		case NOTIFY_LIST_NEIGHBOURS:
//			displayNeighbours();
//			break;
		case NOTIFY_LIST_FRIENDS:
			displayFriends();
			break;
		case NOTIFY_LIST_DIRLIST_FRIENDS:
			displayDirectories();
			break;
		case NOTIFY_LIST_SEARCHLIST:
			displaySearch();
			break;
		case NOTIFY_LIST_MESSAGELIST:
			displayMessages();
			break;
		case NOTIFY_LIST_CHANNELLIST:
			displayChannels();
			break;
		case NOTIFY_LIST_TRANSFERLIST:
			displayTransfers();
			break;
		default:
			break;
	}
	return;
}
			
void NotifyTxt::displayNeighbours()
{
	std::list<RsPgpId> neighs;
	std::list<RsPgpId>::iterator it;

	rsPeers->getGPGAllList(neighs);

	std::ostringstream out;
	for(it = neighs.begin(); it != neighs.end(); ++it)
	{
		RsPeerDetails detail;
		rsPeers->getGPGDetails(*it, detail);

		out << "Neighbour: ";
		out << detail;
		out << std::endl;
	}
	std::cerr << out.str();
}

void NotifyTxt::displayFriends()
{
	std::list<RsPeerId> ids;
	std::list<RsPeerId>::iterator it;

	rsPeers->getFriendList(ids);

	std::ostringstream out;
	for(it = ids.begin(); it != ids.end(); ++it)
	{
		RsPeerDetails detail;
		rsPeers->getPeerDetails(*it, detail);

		out << "Neighbour: ";
		out << detail;
		out << std::endl;
	}
	std::cerr << out.str();
}

void NotifyTxt::displayDirectories()
{
	std::ostringstream out;
	std::cerr << out.str();
}


void NotifyTxt::displaySearch()
{
	std::ostringstream out;
	std::cerr << out.str();
}


void NotifyTxt::displayMessages()
{
}

void NotifyTxt::displayChannels()
{
	std::ostringstream out;
	std::cerr << out.str();
}


void NotifyTxt::displayTransfers()
{
	std::ostringstream out;
	std::cerr << out.str();
}



/******************* Turtle Search Interface **********/

void NotifyTxt::notifyTurtleSearchResult(const RsPeerId &pid, uint32_t search_id, const std::list<TurtleFileInfo>& found_files)
{
//	std::cerr << "NotifyTxt::notifyTurtleSearchResult() " << found_files.size();
//	std::cerr << " new results for Id: " << search_id;
//	std::cerr << std::endl;

	RsStackMutex stack(mNotifyMtx); /****** LOCKED *****/

        std::map<uint32_t, std::list<TurtleFileInfo> >::iterator it;
	it = mSearchResults.find(search_id);
	if (it == mSearchResults.end())
	{
		std::cerr << "NotifyTxt::notifyTurtleSearchResult() " << found_files.size();
		std::cerr << "ERROR: new results for Id: " << search_id;
		std::cerr << std::endl;
		std::cerr << "But list not installed...";
		std::cerr << " DROPPING SEARCH RESULTS";
		std::cerr << std::endl;

		/* new entry */
		//mSearchResults[search_id] = found_files;
		return;
	}

	/* add to existing entry */
        std::list<TurtleFileInfo>::const_iterator fit;
	for(fit = found_files.begin(); fit != found_files.end(); ++fit)
	{
		it->second.push_back(*fit);
	}
	return;
}


                /* interface for handling SearchResults */
void NotifyTxt::getSearchIds(std::list<uint32_t> &searchIds)
{
	RsStackMutex stack(mNotifyMtx); /****** LOCKED *****/

        std::map<uint32_t, std::list<TurtleFileInfo> >::iterator it;
	for(it = mSearchResults.begin(); it != mSearchResults.end(); ++it)
	{
		searchIds.push_back(it->first);
	}
	return;
}


int NotifyTxt::getSearchResults(uint32_t id, std::list<TurtleFileInfo> &searchResults)
{
	RsStackMutex stack(mNotifyMtx); /****** LOCKED *****/

        std::map<uint32_t, std::list<TurtleFileInfo> >::iterator it;
	it = mSearchResults.find(id);
	if (it == mSearchResults.end())
	{
		return 0;
	}

	searchResults = it->second;
	return 1;
}


int NotifyTxt::getSearchResultCount(uint32_t id)
{
	RsStackMutex stack(mNotifyMtx); /****** LOCKED *****/

        std::map<uint32_t, std::list<TurtleFileInfo> >::iterator it;
	it = mSearchResults.find(id);
	if (it == mSearchResults.end())
	{
		return 0;
	}
	return it->second.size();
}

                // only collect results for selected searches.
                // will drop others.
int NotifyTxt::collectSearchResults(uint32_t searchId)
{
	std::cerr << "NotifyTxt::collectSearchResult(" << searchId << ")";
	std::cerr << std::endl;

	RsStackMutex stack(mNotifyMtx); /****** LOCKED *****/

        std::map<uint32_t, std::list<TurtleFileInfo> >::iterator it;
	it = mSearchResults.find(searchId);
	if (it == mSearchResults.end())
	{
        	std::list<TurtleFileInfo> emptyList;
		mSearchResults[searchId] = emptyList;
		return 1;
	}

	std::cerr << "NotifyTxt::collectSearchResult() ERROR Id exists";
	std::cerr << std::endl;
	return 1;
}

int NotifyTxt::clearSearchId(uint32_t searchId)
{
	std::cerr << "NotifyTxt::clearSearchId(" << searchId << ")";
	std::cerr << std::endl;

	RsStackMutex stack(mNotifyMtx); /****** LOCKED *****/

        std::map<uint32_t, std::list<TurtleFileInfo> >::iterator it;
	it = mSearchResults.find(searchId);
	if (it == mSearchResults.end())
	{
		std::cerr << "NotifyTxt::clearSearchId() ERROR Id not there";
		std::cerr << std::endl;
		return 0;
	}

	mSearchResults.erase(it);
	return 1;
}


