/*******************************************************************************
 * gui/profile/ProfileUpgrader.cpp                                             *
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

// libretroshare internals first: openssl headers must come before any
// Windows header pulled in by Qt (X509_NAME clash with wincrypt.h).
#include "pqi/authssl.h"
#include "pqi/pqibin.h"
#include "retroshare/rsinit.h"
#include "rsserver/rsloginhandler.h"
#include "util/rsprint.h"

#include "ProfileUpgrader.h"
#include "ProfileArchive.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include <cstdlib>
#include <vector>

static QString tr(const char* s) { return QCoreApplication::translate("ProfileUpgrader", s); }

namespace {

struct ConfigFile
{
	QString name;		// file name inside config/, e.g. "peers.cfg"
	QByteArray data;	// decrypted content
};

/// Read, decrypt and check the signature of one config file (old keys loaded).
bool loadConfigFile(const QString& cfgPath, const QString& signPath, QByteArray& out)
{
	if(!QFile::exists(cfgPath) || !QFile::exists(signPath))
		return false;

	BinFileInterface bio(cfgPath.toUtf8().constData(), BIN_FLAGS_HASH_DATA | BIN_FLAGS_READABLE);

	const uint64_t size64 = bio.getFileSize();
	if(size64 > 512ull * 1024 * 1024)
		return false;

	out.clear();

	if(size64 > 0)	// an empty file is a valid, empty configuration
	{
		std::vector<char> encrypted(size64);
		if(bio.readdata(encrypted.data(), int(size64)) != int(size64))
			return false;

		void* decrypted = nullptr;
		int decryptedLen = 0;
		if(!AuthSSL::instance().decrypt(decrypted, decryptedLen, encrypted.data(), int(size64)))
			return false;

		out = QByteArray(static_cast<const char*>(decrypted), decryptedLen);
		free(decrypted);
	}

	const RsFileHash hash = bio.gethash();

	// Signature: hex string of the SSL signature of the hash of the encrypted data.
	QFile signFile(signPath);
	if(!signFile.open(QIODevice::ReadOnly))
		return false;

	const std::string signHex = signFile.readAll().toStdString();
	if(signHex.empty() || signHex.size() % 2)
		return false;

	std::vector<unsigned char> sign(signHex.size() / 2);
	if(!RsUtil::HexToBin(signHex, sign.data(), uint32_t(sign.size())))
		return false;

	return AuthSSL::instance().VerifyOwnSignBin(hash.toByteArray(), RsFileHash::SIZE_IN_BYTES,
	                                            sign.data(), uint32_t(sign.size()));
}

/// Encrypt and sign one config file with the keys of the node now loaded.
bool saveConfigFile(const QString& cfgPath, const QByteArray& data)
{
	RsFileHash hash;
	{
		BinEncryptedFileInterface bio(cfgPath.toUtf8().constData(), BIN_FLAGS_HASH_DATA | BIN_FLAGS_WRITEABLE);
		QByteArray copy(data);	// senddata() takes a non-const pointer
		if(!copy.isEmpty() && bio.senddata(copy.data(), copy.size()) != copy.size())
			return false;
		hash = bio.gethash();
	}	// file closed here

	std::string signature;
	if(!AuthSSL::instance().SignData(hash.toByteArray(), RsFileHash::SIZE_IN_BYTES, signature))
		return false;

	QFile signFile(cfgPath + ".sgn");
	if(!signFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	return signFile.write(signature.data(), qint64(signature.size())) == qint64(signature.size());
}

bool isConfigWorkFile(const QString& name)
{
	return name.endsWith(".cfg") || name.endsWith(".sgn") || name.endsWith(".tmp") || name.endsWith("_new");
}

/// Files that must not be copied from the old node to the new one.
bool skipWhenCopying(const QString& rel)
{
	const QString first = rel.section('/', 0, 0);

	if(first == "keys")		// new keys are generated
		return rel != "keys/ssl_passphrase.pgp";
	if(first == "config")	// re-encrypted separately, only extra plain files are copied
		return isConfigWorkFile(QFileInfo(rel).fileName());

	return ProfileArchive::isExcludedFromBackup(rel);
}

QString loginErrorText(RsInit::LoadCertificateStatus status)
{
	switch(status)
	{
	case RsInit::ERR_ALREADY_RUNNING:
		return tr("This node is already in use by another running RetroShare. Close it first.");
	case RsInit::ERR_CANT_ACQUIRE_LOCK:
		return tr("Cannot lock the node directory.");
	case RsInit::ERR_CERT_CRYPTO_IS_TOO_WEAK:
		return tr("Your OpenSSL library refuses the old certificate of this node completely, "
		          "so its configuration cannot be decrypted.");
	default:
		return tr("Cannot log in. Please check your password.");
	}
}

} // anonymous namespace

ProfileUpgrader::Result ProfileUpgrader::upgrade(const RsPeerId& oldSslId, const QString& pgpPassphrase,
                                                 const QString& newLocationName, LogCallback log)
{
	Result res;
	auto say = [&](const QString& m) { if(log) log(m); };

	struct PassphraseGuard {
		explicit PassphraseGuard(const QString& p) { RsLoginHelper::cachePgpPassphrase(p.toUtf8().constData()); }
		~PassphraseGuard() { RsLoginHelper::clearPgpPassphrase(); }
	} passphraseGuard(pgpPassphrase);

	RsPgpId pgpId;
	{
		std::string pgpName, pgpEmail, location;
		if(!RsAccounts::GetAccountDetails(oldSslId, pgpId, pgpName, pgpEmail, location))
		{
			res.error = tr("Cannot read the details of this node.");
			return res;
		}
	}

	const QString oldDir = ProfileArchive::accountDirectory(oldSslId);
	if(oldDir.isEmpty())
	{
		res.error = tr("Cannot find the directory of this node.");
		return res;
	}

	// 1 - log in to the old node

	say(tr("Logging in to the old node..."));

	if(!RsAccounts::SelectAccount(oldSslId))
	{
		res.error = tr("Cannot select this node.");
		return res;
	}

	std::string lockFile;
	RsInit::LoadCertificateStatus status = RsInit::LockAndLoadCertificates(false, lockFile);
	if(status != RsInit::OK)
	{
		res.error = loginErrorText(status);
		return res;
	}
	res.oldNodeStillLoaded = true;

	bool isHidden = false, isAutoTor = false, isFirstTime = false;
	RsAccounts::getCurrentAccountOptions(isHidden, isAutoTor, isFirstTime);

	// 2 - decrypt the config files

	say(tr("Decrypting the configuration..."));

	std::vector<ConfigFile> configs;
	{
		QDir cfgDir(oldDir + "/config");
		const QStringList names = cfgDir.entryList(QStringList() << "*.cfg", QDir::Files, QDir::Name);

		for(const QString& name : names)
		{
			ConfigFile cf;
			cf.name = name;
			const QString cfg = cfgDir.absoluteFilePath(name);

			if(loadConfigFile(cfg, cfg + ".sgn", cf.data) ||
			        loadConfigFile(cfg + ".tmp", cfg + ".sgn.tmp", cf.data))	// p3Config's backup copy
			{
				configs.push_back(cf);
				say("  " + name + ": " + tr("OK"));
			}
			else
			{
				++res.configFilesFailed;
				say("  " + name + ": " + tr("cannot be decrypted, it will not be migrated"));
			}
		}
	}

	if(configs.empty())
	{
		res.error = tr("No configuration file of this node could be decrypted.");
		return res;
	}

	// The SSL password protects the SSL key and the GXS databases. Keeping
	// it for the new node keeps the databases readable without converting them.
	std::string sslPassword;
	if(!RsLoginHandler::getSSLPassword(oldSslId, true, sslPassword) || sslPassword.empty())
	{
		res.error = tr("Cannot read the SSL password of this node.");
		return res;
	}

	AuthSSL::instance().CloseAuth();
	RsAccounts::unlockPreferredAccount();

	// 3 - create the new node

	say(tr("Creating the new node with new SSL keys (this can take a moment)..."));

	RsPeerId newSslId;
	std::string err;
	if(!RsAccounts::createNewAccount(pgpId, "", newLocationName.toUtf8().constData(), "",
	                                 isHidden, isAutoTor, sslPassword, newSslId, err))
	{
		res.error = tr("Cannot create the new node:") + " " + QString::fromStdString(err);
		return res;
	}
	res.newSslId = newSslId;

	const QString newDir = ProfileArchive::accountDirectory(newSslId);
	if(newDir.isEmpty())
	{
		res.error = tr("Cannot find the directory of the new node.");
		return res;
	}

	// 4 - copy the data

	say(tr("Copying databases and settings..."));
	{
		QDir oldRoot(oldDir);
		QDirIterator it(oldDir, QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
		                QDirIterator::Subdirectories);
		int copied = 0;

		while(it.hasNext())
		{
			it.next();
			if(it.fileInfo().isSymLink())
				continue;

			const QString rel = oldRoot.relativeFilePath(it.filePath());
			if(skipWhenCopying(rel))
				continue;

			const QString dest = newDir + '/' + rel;
			if(QFile::exists(dest))
				continue;	// created by the new node itself

			QDir().mkpath(QFileInfo(dest).absolutePath());
			if(QFile::copy(it.filePath(), dest))
				++copied;
			else
				say("  " + tr("cannot copy") + " " + rel);
		}
		say("  " + tr("%1 files copied").arg(copied));
	}

	// 5 - log in to the new node and write the configuration

	say(tr("Logging in to the new node..."));

	status = RsInit::LockAndLoadCertificates(false, lockFile);
	if(status != RsInit::OK)
	{
		res.error = loginErrorText(status);
		return res;
	}

	say(tr("Encrypting the configuration with the new keys..."));

	const QString newCfgDir = newDir + "/config";
	QDir().mkpath(newCfgDir);

	for(const ConfigFile& cf : configs)
	{
		if(saveConfigFile(newCfgDir + '/' + cf.name, cf.data))
			++res.configFilesMigrated;
		else
		{
			++res.configFilesFailed;
			say("  " + cf.name + ": " + tr("cannot be written"));
		}
	}

	res.ok = true;
	say(tr("Done."));
	return res;
}
