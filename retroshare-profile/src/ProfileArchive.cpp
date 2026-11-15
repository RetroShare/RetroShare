/*******************************************************************************
 * retroshare-profile/src/ProfileArchive.cpp                                   *
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

#include "ProfileArchive.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QTemporaryDir>

#include <cstring>
#include <list>

#include "retroshare/rsinit.h"
#include "util/rsdir.h"

#define MANIFEST_NAME     "retroshare-profile.ini"
#define PGP_IDENTITY_NAME "pgp_identity.asc"
#define ARCHIVE_FORMAT    1

static const qint64 TAR_BLOCK = 512;
static const qint64 COPY_CHUNK = 256 * 1024;

static QString tr(const char* s) { return QCoreApplication::translate("ProfileArchive", s); }

/******************************************************************************
 *  Minimal GNU tar writer / reader (regular files and directories only)     *
 ******************************************************************************/

namespace {

void putOctal(char* field, int len, quint64 value)
{
	// len includes the terminating NUL. Values that don't fit use the GNU
	// base-256 encoding (only needed for the size of files > 8 GiB).
	QByteArray s = QByteArray::number(value, 8);

	if(s.size() <= len - 1)
	{
		s = s.rightJustified(len - 1, '0');
		memcpy(field, s.constData(), len - 1);
		field[len - 1] = 0;
		return;
	}

	memset(field, 0, len);
	field[0] = char(0x80);
	for(int i = len - 1; i > 0 && value; --i, value >>= 8)
		field[i] = char(value & 0xff);
}

quint64 getOctal(const char* field, int len)
{
	if(static_cast<unsigned char>(field[0]) & 0x80)	// base-256
	{
		quint64 v = 0;
		for(int i = 1; i < len; ++i)
			v = (v << 8) | static_cast<unsigned char>(field[i]);
		return v;
	}

	quint64 v = 0;
	for(int i = 0; i < len; ++i)
	{
		char c = field[i];
		if(c == ' ' && v == 0) continue;
		if(c < '0' || c > '7') break;
		v = (v << 3) | quint64(c - '0');
	}
	return v;
}

class TarWriter
{
public:
	explicit TarWriter(QIODevice& dev) : mDev(dev), mPending(0) {}

	bool addDirectory(const QString& name, qint64 mtime)
	{
		QString n = name.endsWith('/') ? name : name + '/';
		return writeHeader(n, 0, mtime, '5');
	}

	bool addData(const QString& name, const QByteArray& data, qint64 mtime)
	{
		return beginFile(name, data.size(), mtime) && write(data.constData(), data.size()) && endFile();
	}

	bool beginFile(const QString& name, qint64 size, qint64 mtime)
	{
		mPending = size;
		return writeHeader(name, size, mtime, '0');
	}

	bool write(const char* data, qint64 len)
	{
		if(len > mPending) return false;
		mPending -= len;
		return mDev.write(data, len) == len;
	}

	bool endFile()
	{
		if(mPending != 0) return false;
		return pad();
	}

	bool finish()
	{
		QByteArray zero(2 * TAR_BLOCK, 0);
		return mDev.write(zero) == zero.size();
	}

private:
	bool pad()
	{
		qint64 rem = mDev.pos() % TAR_BLOCK;
		if(rem == 0) return true;
		QByteArray zero(TAR_BLOCK - rem, 0);
		return mDev.write(zero) == zero.size();
	}

	bool writeHeader(const QString& name, qint64 size, qint64 mtime, char type)
	{
		QByteArray utf8 = name.toUtf8();

		if(utf8.size() > 99)	// GNU long name extension
		{
			QByteArray longName = utf8;
			longName.append('\0');
			if(!writeRawHeader("././@LongLink", longName.size(), 0, 'L')) return false;
			if(mDev.write(longName) != longName.size()) return false;
			if(!pad()) return false;
			utf8.truncate(99);
		}
		return writeRawHeader(utf8, size, mtime, type);
	}

	bool writeRawHeader(const QByteArray& name, qint64 size, qint64 mtime, char type)
	{
		char h[TAR_BLOCK];
		memset(h, 0, sizeof(h));

		memcpy(h, name.constData(), qMin(name.size(), 100));
		putOctal(h + 100, 8, type == '5' ? 0755 : 0644);	// mode
		putOctal(h + 108, 8, 0);							// uid
		putOctal(h + 116, 8, 0);							// gid
		putOctal(h + 124, 12, quint64(size));
		putOctal(h + 136, 12, quint64(qMax<qint64>(mtime, 0)));
		memset(h + 148, ' ', 8);							// checksum placeholder
		h[156] = type;
		memcpy(h + 257, "ustar  ", 8);						// GNU magic + version
		memcpy(h + 265, "retroshare", 10);					// uname
		memcpy(h + 297, "retroshare", 10);					// gname

		unsigned int sum = 0;
		for(int i = 0; i < TAR_BLOCK; ++i) sum += static_cast<unsigned char>(h[i]);
		QByteArray chk = QByteArray::number(sum, 8).rightJustified(6, '0');
		memcpy(h + 148, chk.constData(), 6);
		h[154] = 0;
		h[155] = ' ';

		return mDev.write(h, TAR_BLOCK) == TAR_BLOCK;
	}

	QIODevice& mDev;
	qint64 mPending;
};

class TarReader
{
public:
	struct Entry
	{
		QString name;
		qint64 size = 0;
		char type = 0;
		bool isFile() const { return type == '0' || type == 0 || type == '7'; }
		bool isDir() const  { return type == '5'; }
	};

	explicit TarReader(QIODevice& dev) : mDev(dev), mRemaining(0), mPadding(0) {}

	/// returns false at the end of the archive or on error (error set).
	bool next(Entry& e, QString& error)
	{
		if(!skipRest()) { error = tr("Unexpected end of archive."); return false; }

		QString longName;

		for(;;)
		{
			char h[TAR_BLOCK];
			if(mDev.read(h, TAR_BLOCK) != TAR_BLOCK) return false;	// truncated: treated as end

			bool allZero = true;
			for(int i = 0; i < TAR_BLOCK && allZero; ++i) allZero = (h[i] == 0);
			if(allZero) return false;

			unsigned int expected = unsigned(getOctal(h + 148, 8));
			unsigned int sum = 0;
			for(int i = 0; i < TAR_BLOCK; ++i)
				sum += (i >= 148 && i < 156) ? ' ' : static_cast<unsigned char>(h[i]);
			if(sum != expected) { error = tr("The archive is damaged (bad header checksum)."); return false; }

			e.type = h[156];
			e.size = qint64(getOctal(h + 124, 12));

			if(e.size < 0) { error = tr("The archive is damaged."); return false; }

			mRemaining = e.size;
			mPadding = (TAR_BLOCK - e.size % TAR_BLOCK) % TAR_BLOCK;

			if(e.type == 'L')
			{
				if(e.size > 64 * 1024) { error = tr("The archive is damaged."); return false; }
				QByteArray n = mDev.read(e.size);
				mRemaining = 0;
				if(n.size() != e.size || !skipRest()) { error = tr("Unexpected end of archive."); return false; }
				int nul = n.indexOf('\0');
				longName = QString::fromUtf8(nul >= 0 ? n.left(nul) : n);
				continue;
			}

			if(!longName.isEmpty())
				e.name = longName;
			else
			{
				QByteArray name(h, int(strnlen(h, 100)));
				QByteArray prefix(h + 345, int(strnlen(h + 345, 155)));
				if(!prefix.isEmpty() && memcmp(h + 257, "ustar\0", 6) == 0)
					name = prefix + '/' + name;
				e.name = QString::fromUtf8(name);
			}
			return true;
		}
	}

	qint64 read(char* data, qint64 len)
	{
		len = qMin(len, mRemaining);
		if(len <= 0) return 0;
		qint64 r = mDev.read(data, len);
		if(r > 0) mRemaining -= r;
		return r;
	}

	QByteArray readAll(qint64 maxSize)
	{
		if(mRemaining > maxSize) return QByteArray();
		QByteArray d = mDev.read(mRemaining);
		mRemaining -= d.size();
		return d;
	}

	bool skipRest()
	{
		qint64 toSkip = mRemaining + mPadding;
		mRemaining = 0;
		mPadding = 0;
		if(toSkip == 0) return true;
		if(!mDev.isSequential())
			return mDev.seek(mDev.pos() + toSkip);
		return mDev.read(toSkip).size() == toSkip;
	}

private:
	QIODevice& mDev;
	qint64 mRemaining;
	qint64 mPadding;
};

QByteArray makeManifest(const ProfileArchive::Info& info)
{
	QStringList l;
	l << "[RetroShareProfile]";
	l << QString("format=%1").arg(info.formatVersion);
	l << "ssl_id=" + QString::fromStdString(info.sslId.toStdString());
	l << "pgp_id=" + QString::fromStdString(info.pgpId.toStdString());
	l << "pgp_name=" + info.pgpName;
	l << "location=" + info.locationName;
	l << "account_dir=" + info.accountDirName;
	l << "created=" + info.createdAt;
	l << "created_by=" + info.createdBy;
	return (l.join('\n') + '\n').toUtf8();
}

bool parseManifest(const QByteArray& data, ProfileArchive::Info& info)
{
	const QStringList lines = QString::fromUtf8(data).split('\n');

	for(const QString& raw : lines)
	{
		QString line = raw.trimmed();
		int eq = line.indexOf('=');
		if(line.startsWith('[') || eq <= 0) continue;

		QString key = line.left(eq);
		QString value = line.mid(eq + 1);

		if(key == "format")           info.formatVersion = value.toInt();
		else if(key == "ssl_id")      info.sslId = RsPeerId(value.toStdString());
		else if(key == "pgp_id")      info.pgpId = RsPgpId(value.toStdString());
		else if(key == "pgp_name")    info.pgpName = value;
		else if(key == "location")    info.locationName = value;
		else if(key == "account_dir") info.accountDirName = value;
		else if(key == "created")     info.createdAt = value;
		else if(key == "created_by")  info.createdBy = value;
	}
	return info.formatVersion > 0;
}

bool validateInfo(const ProfileArchive::Info& info, QString& error)
{
	if(info.formatVersion > ARCHIVE_FORMAT)
	{
		error = tr("This backup was made by a newer RetroShare version. Please update RetroShare first.");
		return false;
	}

	static const QRegularExpression dirRx("^(LOC06_|HID06_)([0-9a-f]{32})$");
	QRegularExpressionMatch m = dirRx.match(info.accountDirName);

	if(!m.hasMatch() || info.sslId.isNull() || info.pgpId.isNull() ||
	        m.captured(2) != QString::fromStdString(info.sslId.toStdString()))
	{
		error = tr("The archive does not describe a valid RetroShare node.");
		return false;
	}
	return true;
}

/// Make sure an archive path stays inside the node directory.
bool safeRelativePath(const QString& rel, QString& clean)
{
	if(rel.isEmpty() || rel.startsWith('/') || rel.startsWith('\\') || rel.contains(':') || rel.contains('\\'))
		return false;

	clean = QDir::cleanPath(rel);
	if(clean == "." || clean.isEmpty()) { clean.clear(); return true; }	// the root itself

	const QStringList parts = clean.split('/');
	for(const QString& p : parts)
		if(p == ".." || p.isEmpty())
			return false;
	return true;
}

qint64 mtimeOf(const QFileInfo& fi)
{
	return fi.lastModified().toMSecsSinceEpoch() / 1000;
}

} // anonymous namespace

/******************************************************************************
 *                              ProfileArchive                               *
 ******************************************************************************/

QString ProfileArchive::accountDirectory(const RsPeerId& sslId)
{
	if(sslId.isNull())
		return QString();

	QDir base(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str()));
	const QString hex = QString::fromStdString(sslId.toStdString());

	for(const QString& prefix : { QString("LOC06_"), QString("HID06_") })
		if(base.exists(prefix + hex))
			return base.absoluteFilePath(prefix + hex);

	return QString();
}

bool ProfileArchive::isExcludedFromBackup(const QString& relativePath)
{
	const QString first = relativePath.section('/', 0, 0);

	return relativePath == "lock"
	        || first == "tor"          // Tor runtime data, recreated (keys are in hidden_service/)
	        || first == "Downloads"    // default download folders: can be huge
	        || first == "Partials";
}

bool ProfileArchive::exportProfile(const RsPeerId& sslId, const QString& archivePath,
                                   QString& error, ProgressCallback progress)
{
	const QString accountDir = accountDirectory(sslId);
	if(accountDir.isEmpty())
	{
		error = tr("Cannot find the directory of this node.");
		return false;
	}

	Info info;
	{
		std::string pgpName, pgpEmail, location;
		if(!RsAccounts::GetAccountDetails(sslId, info.pgpId, pgpName, pgpEmail, location))
		{
			error = tr("Cannot read the details of this node.");
			return false;
		}
		info.formatVersion = ARCHIVE_FORMAT;
		info.sslId = sslId;
		info.pgpName = QString::fromUtf8(pgpName.c_str());
		info.locationName = QString::fromUtf8(location.c_str());
		info.accountDirName = QFileInfo(accountDir).fileName();
		info.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
		info.createdBy = QString("RetroShare (libretroshare %1)").arg(QString::fromUtf8(RsInit::libRetroShareVersion()));
	}

	// Export the PGP key pair through a temporary file: the RNP backend does
	// not implement exportIdentityToString().
	QByteArray pgpData;
	{
		QTemporaryDir tmpDir;
		const QString keyFile = tmpDir.path() + "/identity.asc";

		if(tmpDir.isValid() && RsAccounts::ExportIdentity(keyFile.toUtf8().constData(), info.pgpId))
		{
			QFile f(keyFile);
			if(f.open(QIODevice::ReadOnly))
				pgpData = f.readAll();
		}
	}
	if(pgpData.isEmpty())
	{
		error = tr("Cannot export the PGP key of this profile.");
		return false;
	}

	// Refuse to copy a node that is currently running: databases would be inconsistent.
	rs_lock_handle_t lockHandle;
	const std::string lockFile = (accountDir + "/lock").toUtf8().constData();
	if(RsDirUtil::createLockFile(lockFile, lockHandle) != 0)
	{
		error = tr("This node is in use by another running RetroShare. Close it first.");
		return false;
	}

	struct LockReleaser {
		rs_lock_handle_t h;
		~LockReleaser() { RsDirUtil::releaseLockFile(h); }
	} releaser { lockHandle };

	// Collect the files first, so that we know the total size.
	QStringList dirs, files;
	qint64 total = 0;
	{
		QDir root(accountDir);
		QDirIterator it(accountDir, QDir::Files | QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
		                QDirIterator::Subdirectories);
		while(it.hasNext())
		{
			it.next();
			const QFileInfo fi = it.fileInfo();
			const QString rel = root.relativeFilePath(fi.absoluteFilePath());

			if(isExcludedFromBackup(rel) || fi.isSymLink())
				continue;

			if(fi.isDir())
				dirs << rel;
			else
			{
				files << rel;
				total += fi.size();
			}
		}
		dirs.sort();
		files.sort();
	}

	const QString partPath = archivePath + ".part";
	QFile out(partPath);
	if(!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		error = tr("Cannot write the file %1").arg(QDir::toNativeSeparators(archivePath));
		return false;
	}

	TarWriter tar(out);
	const qint64 now = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000;
	qint64 done = 0;
	bool ok = true;

	ok = ok && tar.addData(MANIFEST_NAME, makeManifest(info), now);
	ok = ok && tar.addData(PGP_IDENTITY_NAME, pgpData, now);
	ok = ok && tar.addDirectory(info.accountDirName, now);

	for(const QString& d : dirs)
	{
		if(!ok) break;
		ok = tar.addDirectory(info.accountDirName + '/' + d, mtimeOf(QFileInfo(accountDir + '/' + d)));
	}

	QByteArray buffer(int(COPY_CHUNK), 0);

	for(const QString& f : files)
	{
		if(!ok) break;

		QFile in(accountDir + '/' + f);
		if(!in.open(QIODevice::ReadOnly))
		{
			error = tr("Cannot read the file %1").arg(QDir::toNativeSeparators(in.fileName()));
			ok = false;
			break;
		}

		const qint64 size = in.size();
		if(!tar.beginFile(info.accountDirName + '/' + f, size, mtimeOf(QFileInfo(in))))
		{
			ok = false;
			break;
		}

		qint64 left = size;
		while(ok && left > 0)
		{
			qint64 r = in.read(buffer.data(), qMin<qint64>(COPY_CHUNK, left));
			if(r <= 0 || !tar.write(buffer.constData(), r))
			{
				error = tr("Error while copying %1").arg(QDir::toNativeSeparators(in.fileName()));
				ok = false;
				break;
			}
			left -= r;
			done += r;

			if(progress && !progress(done, total, f))
			{
				error = tr("Backup cancelled.");
				ok = false;
			}
		}
		ok = ok && tar.endFile();
	}

	ok = ok && tar.finish();
	out.close();

	if(ok && out.error() == QFileDevice::NoError)
	{
		QFile::remove(archivePath);
		if(QFile::rename(partPath, archivePath))
			return true;

		error = tr("Cannot write the file %1").arg(QDir::toNativeSeparators(archivePath));
	}
	else if(error.isEmpty())
		error = tr("Cannot write the file %1").arg(QDir::toNativeSeparators(archivePath));

	QFile::remove(partPath);
	return false;
}

bool ProfileArchive::readInfo(const QString& archivePath, Info& info, QString& error)
{
	QFile in(archivePath);
	if(!in.open(QIODevice::ReadOnly))
	{
		error = tr("Cannot open the file %1").arg(QDir::toNativeSeparators(archivePath));
		return false;
	}

	TarReader tar(in);
	TarReader::Entry e;

	if(!tar.next(e, error) || e.name != MANIFEST_NAME || !e.isFile() || !parseManifest(tar.readAll(64 * 1024), info))
	{
		if(error.isEmpty())
			error = tr("This file is not a RetroShare node backup.");
		return false;
	}
	return validateInfo(info, error);
}

bool ProfileArchive::importProfile(const QString& archivePath, Info& info, QString& error,
                                   ProgressCallback progress)
{
	if(!readInfo(archivePath, info, error))
		return false;

	if(!accountDirectory(info.sslId).isEmpty())
	{
		error = tr("This node (%1) already exists on this computer.").arg(info.locationName);
		return false;
	}

	QDir base(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str()));
	const QString target = base.absoluteFilePath(info.accountDirName);
	const QString tmp = target + ".import";

	QDir(tmp).removeRecursively();	// left-over of a previous failed import
	if(!base.mkpath(tmp))
	{
		error = tr("Cannot create the directory %1").arg(QDir::toNativeSeparators(tmp));
		return false;
	}

	QFile in(archivePath);
	if(!in.open(QIODevice::ReadOnly))
	{
		error = tr("Cannot open the file %1").arg(QDir::toNativeSeparators(archivePath));
		QDir(tmp).removeRecursively();
		return false;
	}

	const qint64 total = in.size();
	const QString dirPrefix = info.accountDirName + '/';
	QByteArray pgpData;
	QByteArray buffer(int(COPY_CHUNK), 0);
	TarReader tar(in);
	TarReader::Entry e;
	bool ok = true;
	QString readError;

	while(ok && tar.next(e, readError))
	{
		if(e.name == PGP_IDENTITY_NAME && e.isFile())
		{
			pgpData = tar.readAll(16 * 1024 * 1024);
			continue;
		}

		if(!e.name.startsWith(dirPrefix))
			continue;	// the node directory itself, or an unknown entry: ignore

		const QString rawRel = e.name.mid(dirPrefix.size());
		if(rawRel.isEmpty())
			continue;

		QString rel;
		if(!safeRelativePath(rawRel, rel))
		{
			error = tr("The archive contains an invalid path: %1").arg(e.name);
			ok = false;
			break;
		}
		if(rel.isEmpty())
			continue;

		const QString dest = tmp + '/' + rel;

		if(e.isDir())
		{
			if(!QDir().mkpath(dest))
			{
				error = tr("Cannot create the directory %1").arg(QDir::toNativeSeparators(dest));
				ok = false;
			}
			continue;
		}
		if(!e.isFile())
			continue;	// links etc. are never written by the export

		QDir().mkpath(QFileInfo(dest).absolutePath());
		QFile out(dest);
		if(!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			error = tr("Cannot write the file %1").arg(QDir::toNativeSeparators(dest));
			ok = false;
			break;
		}

		qint64 left = e.size;
		while(left > 0)
		{
			qint64 r = tar.read(buffer.data(), qMin<qint64>(COPY_CHUNK, left));
			if(r <= 0 || out.write(buffer.constData(), r) != r)
			{
				error = tr("Error while restoring %1").arg(QDir::toNativeSeparators(dest));
				ok = false;
				break;
			}
			left -= r;

			if(progress && !progress(in.pos(), total, rel))
			{
				error = tr("Restore cancelled.");
				ok = false;
				break;
			}
		}
		out.close();
	}

	if(ok && !readError.isEmpty())
	{
		error = readError;
		ok = false;
	}

	if(ok && !QFile::exists(tmp + "/keys/user_cert.pem"))
	{
		error = tr("The archive does not contain the keys of the node.");
		ok = false;
	}

	if(ok && pgpData.isEmpty())
	{
		error = tr("The archive does not contain the PGP key of the profile.");
		ok = false;
	}

	// Add the PGP key pair to the keyring, unless it is already there
	// (another node of the same profile on this computer).
	if(ok)
	{
		std::list<RsPgpId> pgpIds;
		RsAccounts::GetPGPLogins(pgpIds);

		bool havePgp = false;
		for(const RsPgpId& id : pgpIds)
			if(id == info.pgpId) havePgp = true;

		if(!havePgp)
		{
			RsPgpId imported;
			std::string err;
			if(!RsAccounts::importIdentityFromString(pgpData.toStdString(), imported, err))
			{
				error = tr("Cannot import the PGP key of the profile:") + " " + QString::fromStdString(err);
				ok = false;
			}
			else if(imported != info.pgpId)
			{
				error = tr("The PGP key in the archive does not match the node.");
				ok = false;
			}
		}
	}

	if(ok && !QDir().rename(tmp, target))
	{
		error = tr("Cannot create the directory %1").arg(QDir::toNativeSeparators(target));
		ok = false;
	}

	if(!ok)
	{
		QDir(tmp).removeRecursively();
		return false;
	}

	return reloadAccounts(error);
}

bool ProfileArchive::reloadAccounts(QString& error)
{
	// RsAccounts has no public "reload": re-running init() scans the base
	// directory again. The PGP keyring is already loaded and is kept as is.
	int errorCode = 0;
	if(!RsAccounts::init(RsAccounts::ConfigDirectory(), errorCode))
	{
		error = tr("The node was restored, but the list of nodes could not be reloaded (error %1). Please restart RetroShare.").arg(errorCode);
		return false;
	}
	return true;
}
