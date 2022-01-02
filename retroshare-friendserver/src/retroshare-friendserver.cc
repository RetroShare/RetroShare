/*
 * RetroShare Service
 * Copyright (C) 2021-2021  retroshare team <retroshare.project@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "util/stacktrace.h"
#include "util/rsdir.h"
#include "util/argstream.h"
#include "util/rstime.h"
#include "util/rsdebug.h"

#include "retroshare/rstor.h"
#include "retroshare/rsinit.h"

#include "friendserver.h"

// debug
#include "friend_server/fsitem.h"
#include "friend_server/fsclient.h"

int main(int argc, char* argv[])
{
	RsInfo() << "\n" <<
	    "+================================================================+\n"
	    "|     o---o                                             o        |\n"
	    "|      \\ /         - Retroshare Friend Server -        / \\       |\n"
	    "|       o                                             o---o      |\n"
	    "+================================================================+"
	         << std::endl << std::endl;

	//RsControl::earlyInitNotificationSystem();

    std::string base_directory = "FSData";

	argstream as(argc,argv);

    as >> parameter( 'c',"base-dir", base_directory, "set base directory to store data files (keys, etc)", false )
	   >> help( 'h', "help", "Display this Help" );

	as.defaultErrorHandling(true, true);

    RsConfigOptions conf;
    conf.main_executable_path = argv[0];

    RsInit::InitRsConfig();
    RsInit::InitRetroShare(conf);

    // Create the base directory if needed

    if(!RsDirUtil::checkCreateDirectory(base_directory))
    {
        RsErr() << "Cannot create base directory \"" << base_directory << "\". Check permissions, paths, etc." ;
        return 1;
    }
    // Create/start TorManager

    RsTor::setTorDataDirectory(RsDirUtil::makePath(base_directory,"tor"));
    RsTor::setHiddenServiceDirectory(RsDirUtil::makePath(base_directory,"hidden_service"));

    if(! RsTor::start() || RsTor::hasError())
    {
        RsErr() << "Tor cannot be started on your system: " << RsTor::errorMessage() ;
        return 1 ;
    }

    std::string service_id;

    while(RsTor::torStatus() != RsTorStatus::READY || RsTor::getHiddenServiceStatus(service_id) != RsTorHiddenServiceStatus::ONLINE)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    std::string onion_address,service_target_address;
    uint16_t service_port,target_port;

    RsTor::getHiddenServiceInfo(service_id,onion_address,service_port,service_target_address,target_port) ;

    RsDbg() << "Tor properly started: " ;
    RsDbg() << "  Hidden service address: " << onion_address << ":" << service_port;
    RsDbg() << "  Target address        : " << service_target_address << ":" << target_port;

    // Now start the real thing.

    FriendServer fs(base_directory,service_target_address,target_port);
    fs.start();

    while(fs.isRunning())
        std::this_thread::sleep_for(std::chrono::seconds(2));

	return 0;
}

