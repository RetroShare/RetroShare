/*******************************************************************************
 * retroshare-nogui/src/introserver.h                                          *
 *                                                                             *
 * retroshare-nogui: headless version of retroshare                            *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare.project@gmail.com>         *
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

#ifndef RS_INTRO_SERVER_H
#define RS_INTRO_SERVER_H

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <stdio.h>
#include <iostream>
                      
#define MIN_CERT_SIZE	100

class RsIntroStore;

class RsIntroServer
{
	public:

	RsIntroServer();

int 	tick();
int 	displayPeers();

	private:

int 	addCertificateFile(std::string);
int 	addNewUser(std::string certificate);
int 	setupChatLobbies(std::string &genericLobbyName);
int 	createConfigFiles(std::string peersDir, std::string lobbyName);
int 	restoreStoredPeers();
int 	removeAllPeers();

int 	checkForNewCerts();
int 	cleanOldPeers();

time_t mStartTS;
time_t mLastCheck;
int    mMaxPeerAge;

	std::string mCertLoadPath;
	time_t mCertCheckTime;
	
	std::string mPeersFile;
	RsIntroStore *mStorePeers;

	ChatLobbyId mEnglishLobby;
	ChatLobbyId mFrenchLobby;
	ChatLobbyId mGermanLobby;
	ChatLobbyId mSpanishLobby;
};


class storeData
{
	public:
	time_t mAdded;
	uint32_t mFlags;
};


class RsIntroStore
{
	public:
	RsIntroStore(std::string storefile);
bool	loadPeers();
bool	addPeer(const RsPeerDetails &pd);
bool	removePeer(std::string gpgId);
bool	savePeers();
bool	getPeersBeforeTS(time_t ts, std::list<std::string> &plist);

bool	isValid(std::string gpgId);
uint32_t getFlags(std::string gpgId);

	private:

	std::string mStoreFile;
	std::string mTempStoreFile;
	std::map<std::string, storeData> mGpgData;
};



#endif
