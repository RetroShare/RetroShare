/*******************************************************************************
 * retroshare-profile/src/NodeInfo.cpp                                         *
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

// openssl first: avoids the X509_NAME clash with wincrypt.h on Windows
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include "NodeInfo.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace NodeInfo
{

QString Certificate::summary() const
{
	if(!valid)
		return QCoreApplication::translate("NodeInfo", "unknown");

	QString s = QString("%1 %2, %3").arg(keyType).arg(keyBits).arg(signature);
	if(weak)
		s += " " + QCoreApplication::translate("NodeInfo", "(old)");
	return s;
}

static QString signatureName(int nid)
{
	switch(nid)
	{
	case NID_md5WithRSAEncryption:    return "MD5";
	case NID_sha1WithRSAEncryption:
	case NID_ecdsa_with_SHA1:         return "SHA-1";
	case NID_sha224WithRSAEncryption: return "SHA-224";
	case NID_sha256WithRSAEncryption:
	case NID_ecdsa_with_SHA256:       return "SHA-256";
	case NID_sha384WithRSAEncryption:
	case NID_ecdsa_with_SHA384:       return "SHA-384";
	case NID_sha512WithRSAEncryption:
	case NID_ecdsa_with_SHA512:       return "SHA-512";
	default:                          return QString::fromLatin1(OBJ_nid2sn(nid));
	}
}

Certificate readCertificate(const QString& nodeFolder)
{
	Certificate c;

	QFile f(nodeFolder + "/keys/user_cert.pem");
	if(!f.open(QIODevice::ReadOnly))
		return c;

	const QByteArray pem = f.readAll();
	BIO *bio = BIO_new_mem_buf(pem.constData(), pem.size());
	if(!bio)
		return c;

	X509 *x509 = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
	BIO_free(bio);
	if(!x509)
		return c;

	const int sigNid = X509_get_signature_nid(x509);
	c.signature = signatureName(sigNid);

	EVP_PKEY *key = X509_get0_pubkey(x509);
	if(key)
	{
		c.keyBits = EVP_PKEY_bits(key);
		switch(EVP_PKEY_base_id(key))
		{
		case EVP_PKEY_RSA: c.keyType = "RSA"; break;
		case EVP_PKEY_DSA: c.keyType = "DSA"; break;
		case EVP_PKEY_EC:  c.keyType = "EC";  break;
		default:           c.keyType = QCoreApplication::translate("NodeInfo", "key"); break;
		}

		// OpenSSL security level 2 (default of current versions) needs
		// 112 bits of security: RSA >= 2048 bits, and no MD5 / SHA-1 signature.
		c.weak = EVP_PKEY_security_bits(key) < 112
		        || c.signature == "MD5" || c.signature == "SHA-1";
		c.valid = true;
	}

	X509_free(x509);
	return c;
}

QDateTime lastUsed(const QString& nodeFolder)
{
	QDateTime newest;

	const QFileInfoList files = QDir(nodeFolder + "/config").entryInfoList(QDir::Files);
	for(const QFileInfo& fi : files)
		if(!newest.isValid() || fi.lastModified() > newest)
			newest = fi.lastModified();

	return newest;
}

bool hasRnpKeyring(const QString& dataFolder)
{
	return QFileInfo::exists(dataFolder + "/rnp/retroshare_secret_keyring.gpg");
}

Keyring keyring(const QString& dataFolder, bool rnpExistedAtStart)
{
	const bool rnp = hasRnpKeyring(dataFolder);
	const bool old = QFileInfo::exists(dataFolder + "/pgp/retroshare_secret_keyring.gpg");

	if(rnp && !rnpExistedAtStart && old)
		return Keyring::RnpConvertedFromOpenPgpSdk;
	if(rnp)
		return Keyring::Rnp;
	if(old)
		return Keyring::OpenPgpSdk;
	return Keyring::Unknown;
}

} // namespace NodeInfo
