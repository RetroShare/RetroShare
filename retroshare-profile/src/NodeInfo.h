/*******************************************************************************
 * retroshare-profile/src/NodeInfo.h                                           *
 *                                                                             *
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

#ifndef RS_PROFILE_NODE_INFO_H
#define RS_PROFILE_NODE_INFO_H

#include <QDateTime>
#include <QString>

/**
 * Information read from a node folder without logging in: nothing is decrypted.
 */
namespace NodeInfo
{
	struct Certificate
	{
		bool valid = false;
		QString keyType;		// "RSA", "EC", ...
		int keyBits = 0;
		QString signature;		// "SHA-1", "SHA-256", ...
		bool weak = false;		// refused by OpenSSL security level 2 (current default)

		/// e.g. "RSA 4096, SHA-256"
		QString summary() const;
	};

	/// Reads <node folder>/keys/user_cert.pem
	Certificate readCertificate(const QString& nodeFolder);

	/// Last time the node saved its configuration (newest file in config/).
	QDateTime lastUsed(const QString& nodeFolder);

	enum class Keyring { Unknown, Rnp, RnpConvertedFromOpenPgpSdk, OpenPgpSdk };

	/** Which PGP keyring the data folder uses. @p rnpExistedAtStart tells
	 *  whether the RNP keyring was there before this tool started (RetroShare,
	 *  and this tool, create it from the old OpenPGP-SDK one when missing). */
	Keyring keyring(const QString& dataFolder, bool rnpExistedAtStart);

	/// true if the RNP keyring file exists in @p dataFolder
	bool hasRnpKeyring(const QString& dataFolder);
}

#endif // RS_PROFILE_NODE_INFO_H
