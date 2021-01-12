/*******************************************************************************
 * bitdht/bdstore.h                                                            *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <bitdht@lunamutt.com>                       *
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
#ifndef BITDHT_STORE_H
#define BITDHT_STORE_H

#include <string>
#include "bitdht/bdiface.h"
#include "bitdht/bdpeer.h"

class bdStore
{
public:

    bdStore(std::string file, std::string backupfile, bdDhtFunctions *fns);

    int 	reloadFromStore(); /* for restarts */
    int 	reloadFromStore(std::string file);
    int		filterIpList(const std::list<struct sockaddr_in> &filteredIPs);
    int 	clear();

    int 	getPeer(bdPeer *peer);
    void	addStore(bdPeer *peer);
    void	writeStore(std::string file);
    void	writeStore();

protected:
    std::string mStoreFile;
    std::string mStoreFileBak;
    std::list<bdPeer> store;
    int mIndex;
    bdDhtFunctions *mFns;
};


#endif
