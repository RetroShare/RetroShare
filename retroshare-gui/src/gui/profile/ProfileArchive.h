/*******************************************************************************
 * gui/profile/ProfileArchive.h                                                *
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

#ifndef RS_PROFILE_ARCHIVE_H
#define RS_PROFILE_ARCHIVE_H

#include <QString>
#include <functional>

#include "retroshare/rsids.h"

/**
 * Backup and restore of a complete RetroShare node (location).
 *
 * A profile archive (*.rsprofile) is a plain ustar/GNU tar file that holds:
 *   - retroshare-profile.ini   : manifest (ids, names, format version)
 *   - pgp_identity.asc         : the PGP key pair (secret key stays encrypted
 *                                with the user's PGP passphrase)
 *   - LOC06_<id>/... or HID06_<id>/... : the node directory (configs, keys,
 *                                GXS databases, GUI settings, hidden service...)
 *
 * Nothing is decrypted while exporting or importing: the archive is exactly as
 * safe as the profile folder itself, and still needs the PGP passphrase.
 * The archive can be opened with any tar tool (7-Zip, tar, ...).
 */
class ProfileArchive
{
public:
	struct Info
	{
		int formatVersion = 0;
		RsPeerId sslId;
		RsPgpId pgpId;
		QString pgpName;
		QString locationName;
		QString accountDirName;   // LOC06_<id> or HID06_<id>
		QString createdAt;
		QString createdBy;
		bool isHidden() const { return accountDirName.startsWith("HID06_"); }
	};

	/// progress callback: bytes done, bytes total, current file. Return false to cancel.
	typedef std::function<bool(qint64 done, qint64 total, const QString& file)> ProgressCallback;

	static QString fileExtension() { return QStringLiteral("rsprofile"); }

	/// Absolute path of the directory of the given node, empty if not found.
	static QString accountDirectory(const RsPeerId& sslId);

	/** Export the given node and its PGP key into @p archivePath.
	 *  The node must not be running (call it before logging in). */
	static bool exportProfile(const RsPeerId& sslId, const QString& archivePath,
	                          QString& error, ProgressCallback progress = ProgressCallback());

	/// Read only the manifest of an archive.
	static bool readInfo(const QString& archivePath, Info& info, QString& error);

	/** Import an archive: adds the PGP key to the keyring (if missing), restores
	 *  the node directory and reloads the list of available nodes.
	 *  Fails if a node with the same id already exists. */
	static bool importProfile(const QString& archivePath, Info& info, QString& error,
	                          ProgressCallback progress = ProgressCallback());

	/// Reload the accounts list after a node directory was added on disk.
	static bool reloadAccounts(QString& error);

	/** Directories (relative to the node directory) that are not part of a backup:
	 *  the lock file, the Tor runtime data and the default download folders. */
	static bool isExcludedFromBackup(const QString& relativePath);
};

#endif // RS_PROFILE_ARCHIVE_H
