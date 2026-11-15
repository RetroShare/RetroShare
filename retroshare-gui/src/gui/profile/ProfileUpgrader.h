/*******************************************************************************
 * gui/profile/ProfileUpgrader.h                                               *
 *                                                                             *
 * Copyright (c) 2023 hunbernd                                                 *
 * Copyright (c) 2026 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef RS_PROFILE_UPGRADER_H
#define RS_PROFILE_UPGRADER_H

#include <QString>
#include <functional>

#include "retroshare/rsids.h"

/**
 * Upgrade a node (location) to new SSL keys while keeping all of its data.
 *
 * Old nodes have SSL certificates that recent OpenSSL versions consider too
 * weak. This creates a new node of the same PGP profile with fresh SSL keys
 * and moves everything over. Based on hunbernd's retroshare-profileupgradetool.
 *
 *  1. log in to the old node (SSL keys only, no services are started)
 *  2. decrypt all config files into memory and check their signatures
 *  3. create a new node with the same PGP key and the same SSL password,
 *     so that the GXS databases (encrypted with that password) stay readable
 *  4. copy the rest of the node directory (GXS databases, GUI settings,
 *     hidden service keys, file lists, stickers...)
 *  5. log in to the new node and write the configs, encrypted and signed
 *     with the new keys
 *
 * On success the new node is logged in (certificates loaded), exactly as after
 * RsApplication::loadCertificate(), so start-up can simply continue.
 * The old node directory is left untouched.
 */
class ProfileUpgrader
{
public:
	typedef std::function<void(const QString& message)> LogCallback;

	struct Result
	{
		bool ok = false;
		bool oldNodeStillLoaded = false;	// a restart is needed before logging in again
		RsPeerId newSslId;
		int configFilesMigrated = 0;
		int configFilesFailed = 0;
		QString error;
	};

	static Result upgrade(const RsPeerId& oldSslId, const QString& pgpPassphrase,
	                      const QString& newLocationName, LogCallback log);
};

#endif // RS_PROFILE_UPGRADER_H
