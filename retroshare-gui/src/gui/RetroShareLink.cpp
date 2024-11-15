/*******************************************************************************
 * gui/RetroshareLink.cpp                                                      *
 *                                                                             *
 * Copyright (c) 2009 Cyril Soler      <retroshare.project@gmail.com>          *
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

#include "RetroShareLink.h"

#include "ChatLobbyWidget.h"
#include "MainWindow.h"
#include "HomePage.h"
#include "chat/ChatDialog.h"
#include "common/PeerDefs.h"
#include "common/RsCollectionDialog.h"
#include "common/RsUrlHandler.h"
#include "connect/ConfCertDialog.h"
#include "connect/ConnectFriendWizard.h"
#include "connect/PGPKeyDialog.h"
#include "FileTransfer/SearchDialog.h"
#include "gxschannels/GxsChannelDialog.h"
#include "gxsforums/GxsForumsDialog.h"
#include "msgs/MessageComposer.h"
#include "Posted/PostedDialog.h"
#include "util/misc.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rsgxsforums.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QIcon>
#include <QMessageBox>
#include <QMimeData>
#include <QObject>
#include <QRegExp>
#include <QStringList>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QUrlQuery>
#endif

#include <iostream>
#include <time.h>

//#define DEBUG_RSLINK 1

#define HOST_FILE        "file"
#define HOST_PERSON      "person"
#define HOST_FORUM       "forum"
#define HOST_CHANNEL     "channel"
#define HOST_SEARCH      "search"
#define HOST_MESSAGE     "message"
#define HOST_CERTIFICATE "certificate"
#define HOST_EXTRAFILE   "extra"
//#define HOST_PRIVATE_CHAT"private_chat"
#define HOST_PUBLIC_MSG  "public_msg"
#define HOST_POSTED      "posted"
#define HOST_IDENTITY    "identity"
#define HOST_FILE_TREE   "collection"
#define HOST_CHAT_ROOM   "chat_room"
#define HOST_REGEXP      "file|person|forum|channel|search|message|certificate|extra|private_chat|public_msg|posted|identity|collection|chat_room"

#define FILE_NAME       "name"
#define FILE_SIZE       "size"
#define FILE_HASH       "hash"
#define FILE_SOURCE     "src"

#define PERSON_NAME     "name"
#define PERSON_HASH     "hash"

#define FORUM_NAME      "name"
#define FORUM_ID        "id"
#define FORUM_MSGID     "msgid"

#define CHANNEL_NAME    "name"
#define CHANNEL_ID      "id"
#define CHANNEL_MSGID   "msgid"

#define SEARCH_KEYWORDS "keywords"

#define MESSAGE_ID      "id"
#define MESSAGE_SUBJECT "subject"

#define CERTIFICATE_SSLID        "sslid"
#define CERTIFICATE_GPG_ID       "gpgid"
#define CERTIFICATE_GPG_BASE64   "gpgbase64"
#define CERTIFICATE_GPG_CHECKSUM "gpgchecksum"
#define CERTIFICATE_LOCATION     "location"
#define CERTIFICATE_NAME         "name"
#define CERTIFICATE_EXT_IPPORT   "extipp"
#define CERTIFICATE_LOC_IPPORT   "locipp"
#define CERTIFICATE_DYNDNS       "dyndns"
#define CERTIFICATE_RADIX        "radix"

#define EXTRAFILE_NAME       "name"
#define EXTRAFILE_SIZE       "size"
#define EXTRAFILE_HASH       "hash"
#define EXTRAFILE_SOURCE     "src"

#define PUBLIC_MSG_TIME_STAMP  "time_stamp"
#define PUBLIC_MSG_HASH        "hash"
#define PUBLIC_MSG_SRC_PGP_ID  "gpgid"

#define POSTED_NAME    "name"
#define POSTED_ID      "id"
#define POSTED_MSGID   "msgid"

#define IDENTITY_ID    "gxsid"
#define IDENTITY_NAME  "name"
#define IDENTITY_GROUP "groupdata"

#define FILE_TREE_NAME "name"
#define FILE_TREE_SIZE "size"
#define FILE_TREE_DATA "radix"
#define FILE_TREE_COUNT "files"

#define CHAT_ROOM_NAME "name"
#define CHAT_ROOM_ID   "id"

RetroShareLink::RetroShareLink(const QUrl& url)
{
	fromUrl(url);
}

RetroShareLink::RetroShareLink(const QString& url)
{
	fromString(url);
}

void RetroShareLink::fromString(const QString& url)
{
	clear();

	// parse
#ifdef DEBUG_RSLINK
	std::cerr << "got new RS link \"" << url.toStdString() << "\"" << std::endl ;
#endif

	if ((url.startsWith(QString(RSLINK_SCHEME) + "://" + QString(HOST_FILE)) && url.count("|") == 3) ||
	    (url.startsWith(QString(RSLINK_SCHEME) + "://" + QString(HOST_PERSON)) && url.count("|") == 2))
	{
		/* Old link, we try it */
		QStringList list = url.split ("|");

		if (list.size() >= 1) {
			if (list.size() == 4 && list[0] == QString(RSLINK_SCHEME) + "://" + QString(HOST_FILE)) {
				bool ok ;

				_type = TYPE_FILE;
				_name = list[1] ;
				_size = list[2].toULongLong(&ok) ;
				_hash = list[3].left(40) ;	// normally not necessary, but it's a security.

				if (ok) {
#ifdef DEBUG_RSLINK
					std::cerr << "New RetroShareLink forged:" << std::endl ;
					std::cerr << "  name = \"" << _name.toStdString() << "\"" << std::endl ;
					std::cerr << "  hash = \"" << _hash.toStdString() << "\"" << std::endl ;
					std::cerr << "  size = " << _size << std::endl ;
#endif
					check();
					return;
				}
			} else if (list.size() == 3 && list[0] == QString(RSLINK_SCHEME) + "://" + QString(HOST_PERSON)) {
				_type = TYPE_PERSON;
				_name = list[1] ;
				_hash = list[2].left(40) ;	// normally not necessary, but it's a security.
				_size = 0;
				check();
				return;
			}

			// bad link
		}
	}

	/* Now try QUrl */
	fromUrl(QUrl::fromEncoded(url.toUtf8().constData()));
}

// Qt 4 and Qt5 use different classes
// to make it work with Qt4 and Qt5, use a template
template<class QUrl_or_QUrlQuery>
QString decodedQueryItemValue(const QUrl_or_QUrlQuery& urlQuery, const QString& key)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// Qt5 needs a additional flag to properly decode everything
	// we have to decode, because we want only decoded stuff in our QString
	return urlQuery.queryItemValue(key, QUrl::FullyDecoded);
#else
	return urlQuery.queryItemValue(key);
#endif
}

void RetroShareLink::fromUrl(const QUrl& url)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	const QUrlQuery urlQuery(url);
#else
	const QUrl &urlQuery(url);
#endif

	clear();

	// parse
#ifdef DEBUG_RSLINK
	std::cerr << "got new RS link \"" << url.toString().toStdString() << "\"" << std::endl ;
#endif

	if (url.scheme() != RSLINK_SCHEME) {
		/* No RetroShare-Link */
#ifdef DEBUG_RSLINK
		std::cerr << "Not a RS link: scheme=" << url.scheme().toStdString() << std::endl;
#endif
		return;
	}

	if (url.host() == HOST_FILE) {
		bool ok ;

		_type = TYPE_FILE;
		_name = decodedQueryItemValue(urlQuery, FILE_NAME);
		_size = urlQuery.queryItemValue(FILE_SIZE).toULongLong(&ok);
		_hash = urlQuery.queryItemValue(FILE_HASH).left(40);	// normally not necessary, but it's a security.

		if (ok) {
#ifdef DEBUG_RSLINK
			std::cerr << "New RetroShareLink forged:" << std::endl ;
			std::cerr << "  name = \"" << _name.toStdString() << "\"" << std::endl ;
			std::cerr << "  hash = \"" << _hash.toStdString() << "\"" << std::endl ;
			std::cerr << "  size = " << _size << std::endl ;
#endif
			check();
			return;
		}
	}

	if (url.host() == HOST_PERSON) {
		_type = TYPE_PERSON;
		_name = decodedQueryItemValue(urlQuery, PERSON_NAME);
		_hash = urlQuery.queryItemValue(PERSON_HASH).left(40);	// normally not necessary, but it's a security.

#ifdef DEBUG_RSLINK
		std::cerr << "Got a person link!!" << std::endl;
#endif
		check();
		return;
	}

	if (url.host() == HOST_FORUM) {
		_type = TYPE_FORUM;
		_name = decodedQueryItemValue(urlQuery, FORUM_NAME);
		_hash = urlQuery.queryItemValue(FORUM_ID);
		_msgId = urlQuery.queryItemValue(FORUM_MSGID);

#ifdef DEBUG_RSLINK
		std::cerr << "Got a forum link!!" << std::endl;
#endif
		check();
		return;
	}

	if (url.host() == HOST_CHANNEL) {
		_type = TYPE_CHANNEL;
		_name = decodedQueryItemValue(urlQuery, CHANNEL_NAME);
		_hash = urlQuery.queryItemValue(CHANNEL_ID);
		_msgId = urlQuery.queryItemValue(CHANNEL_MSGID);

#ifdef DEBUG_RSLINK
		std::cerr << "Got a channel link!!" << std::endl;
#endif
		check();
		return;
	}

	if (url.host() == HOST_SEARCH) {
		_type = TYPE_SEARCH;
		_name = decodedQueryItemValue(urlQuery, SEARCH_KEYWORDS);

#ifdef DEBUG_RSLINK
		std::cerr << "Got a search link!!" << std::endl;
#endif
		check();
		return;
	}

	if (url.host() == HOST_MESSAGE) {
		std::string id = urlQuery.queryItemValue(MESSAGE_ID).toStdString();
		*this = createMessage(RsPeerId(id), urlQuery.queryItemValue(MESSAGE_SUBJECT));

#ifdef DEBUG_RSLINK
		std::cerr << "Got a message link!!" << std::endl;
#endif
		return;
	}

	if (url.host() == HOST_CERTIFICATE) {
		_type = TYPE_CERTIFICATE;
		_radix = decodedQueryItemValue(urlQuery, CERTIFICATE_RADIX);

#ifdef DEBUG_RSLINK
		std::cerr << "Got a certificate link!!" << std::endl;
#endif
		check() ;
		return;
	}

	if (url.host() == HOST_EXTRAFILE) {
		bool ok ;

		_type = TYPE_EXTRAFILE;
		_name = decodedQueryItemValue(urlQuery, EXTRAFILE_NAME);
		_size = urlQuery.queryItemValue(EXTRAFILE_SIZE).toULongLong(&ok);
		_hash = urlQuery.queryItemValue(EXTRAFILE_HASH).left(40);	// normally not necessary, but it's a security.
		_SSLid = urlQuery.queryItemValue(EXTRAFILE_SOURCE);

		if (ok) {
#ifdef DEBUG_RSLINK
			std::cerr << "New RetroShareLink forged:" << std::endl ;
			std::cerr << "  name = \"" << _name.toStdString() << "\"" << std::endl ;
			std::cerr << "  hash = \"" << _hash.toStdString() << "\"" << std::endl ;
			std::cerr << "  size = " << _size << std::endl ;
			std::cerr << "  src  = " << _SSLid.toStdString() << std::endl ;
#endif
			check();
			return;
		}
	}

	//TYPE_PRIVATE_CHAT deprecated

	if(url.host() == HOST_PUBLIC_MSG)
	{
		bool ok ;
		_type = TYPE_PUBLIC_MSG ;
		_time_stamp = urlQuery.queryItemValue(PUBLIC_MSG_TIME_STAMP).toUInt(&ok) ;
		_hash = urlQuery.queryItemValue(PUBLIC_MSG_HASH) ;
		_GPGid = urlQuery.queryItemValue(PUBLIC_MSG_SRC_PGP_ID) ;

#ifdef DEBUG_RSLINK
		std::cerr << "Got a public message link!!" << std::endl;
#endif
		check() ;
		return;
	}

	if (url.host() == HOST_POSTED) {
		_type = TYPE_POSTED;
		_name = decodedQueryItemValue(urlQuery, POSTED_NAME);
		_hash = urlQuery.queryItemValue(POSTED_ID);
		_msgId = urlQuery.queryItemValue(POSTED_MSGID);

#ifdef DEBUG_RSLINK
		std::cerr << "Got a posted link!!" << std::endl;
#endif
		check();
		return;
	}

	if(url.host() == HOST_IDENTITY) {
		_type = TYPE_IDENTITY ;
		QString gxsid= urlQuery.queryItemValue(IDENTITY_ID) ;
		QString name = decodedQueryItemValue(urlQuery, IDENTITY_NAME) ;
		QString radix= decodedQueryItemValue(urlQuery, IDENTITY_GROUP) ;

		RsGxsId id(gxsid.toStdString()) ;

		if(!id.isNull())
			*this = createIdentity(id,name,radix) ;
#ifdef DEBUG_RSLINK
		std::cerr << "Got an identity link!!" << std::endl;
#endif
		else
			std::cerr << "(EE) identity link is not valid." << std::endl;
		return ;
	}

	if (url.host() == HOST_FILE_TREE) {
		bool ok;
		_type  = TYPE_FILE_TREE;
		_name  = decodedQueryItemValue(urlQuery, FILE_TREE_NAME);
		_size  = urlQuery.queryItemValue(FILE_TREE_SIZE).toULongLong(&ok);
		_radix = decodedQueryItemValue(urlQuery, FILE_TREE_DATA);
		_count = urlQuery.queryItemValue(FILE_TREE_COUNT).toUInt(&ok);

#ifdef DEBUG_RSLINK
		std::cerr << "Got a file tree link!!" << std::endl;
#endif
		check() ;
		return;
	}

	if (url.host() == HOST_CHAT_ROOM) {
		_type = TYPE_CHAT_ROOM;
		_name = decodedQueryItemValue(urlQuery, CHAT_ROOM_NAME);
		_hash = urlQuery.queryItemValue(CHAT_ROOM_ID);

#ifdef DEBUG_RSLINK
		std::cerr << "Got a chat room link!!" << std::endl;
#endif
		check() ;
		return;
	}

	// bad link

#ifdef DEBUG_RSLINK
	std::cerr << "Wrongly formed RS link. Can't process." << std::endl ;
#endif
	clear();
}

RetroShareLink::RetroShareLink()
{
	clear();
}

RetroShareLink RetroShareLink::createFile(const QString& name, uint64_t size, const QString& hash)
{
	RetroShareLink link;
	link.clear();

	link._name = name;
	link._size = size;
	link._hash = hash;

	link._type = TYPE_FILE;

	link.check();

	return link;
}

RetroShareLink RetroShareLink::createPerson(const RsPgpId& id)
{
	RetroShareLink link;
	link.clear();

	RsPeerDetails detail;
	if (rsPeers->getGPGDetails(id, detail) == false) {
		std::cerr << "RetroShareLink::createPerson() Couldn't find peer id " << id << std::endl;
	} else {

		link._name = QString::fromUtf8(detail.name.c_str());
		link._hash = QString::fromStdString(id.toStdString());

		link._type = TYPE_PERSON;
	}

	link.check();

	return link;
}

//For Forum, Channel & Posted
RetroShareLink RetroShareLink::createGxsGroupLink(const RetroShareLink::enumType &linkType, const RsGxsGroupId &groupId, const QString &groupName)
{
	RetroShareLink link;
	link.clear();

	if (!groupId.isNull()) {
		link._name = groupName;
		link._hash = QString::fromStdString(groupId.toStdString());

		link._type = linkType;
	}

	link.check();

	return link;
}
RetroShareLink RetroShareLink::createGxsMessageLink(const RetroShareLink::enumType &linkType, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &msgName)
{
	RetroShareLink link;
	link.clear();

	if (!groupId.isNull() && !msgId.isNull()) {
		link._name = msgName;
		link._hash = QString::fromStdString(groupId.toStdString());
		link._msgId = QString::fromStdString(msgId.toStdString());

		link._type = linkType;
	}

	link.check();

	return link;
}

RetroShareLink RetroShareLink::createSearch(const QString& keywords)
{
	RetroShareLink link;
	link.clear();

	link._name = keywords;

	link._type = TYPE_SEARCH;

	link.check();

	return link;
}

RetroShareLink RetroShareLink::createMessage(const RsPeerId& peerId, const QString& subject)
{
	RetroShareLink link;
	link.clear();

	link._hash = QString::fromStdString(peerId.toStdString());
	PeerDefs::rsidFromId(peerId, &link._name);
	link._subject = subject;

	link._type = TYPE_MESSAGE;

	link.check();

	return link;
}
RetroShareLink RetroShareLink::createMessage(const RsGxsId& peerId, const QString& subject)
{
	RetroShareLink link;
	link.clear();

	link._hash = QString::fromStdString(peerId.toStdString());
	PeerDefs::rsidFromId(peerId, &link._name);
	link._subject = subject;

	link._type = TYPE_MESSAGE;

	link.check();

	return link;
}

RetroShareLink RetroShareLink::createCertificate(const RsPeerId& ssl_id)
{
    RetroShareLink link;
    link.clear();

#ifndef RS_NO_WARN_CPP
#pragma message("csoler 2012-08-14: This is baaaaaad code")
#endif

    // 	- we should not need to parse and re-read a cert in old format.
    //
    RsPeerDetails detail;
    if (rsPeers->getPeerDetails(ssl_id, detail) == false)
        std::cerr << "RetroShareLink::createPerson() Couldn't find peer id " << ssl_id << std::endl;
    else
    {
        link._type = TYPE_CERTIFICATE;

        if(rsPeers->getOwnId() == ssl_id)	// in this case, use application-wide parameters set in HomePage
        {
            QString description;
            auto invite_flags = static_cast<HomePage*>(MainWindow::getPage(MainWindow::Home))->currentInviteFlags();

            invite_flags &= ~RetroshareInviteFlags::SLICE_TO_80_CHARS;
            invite_flags |=  RetroshareInviteFlags::RADIX_FORMAT;

            link._radix = QString::fromUtf8(rsPeers->GetRetroshareInvite(ssl_id,invite_flags).c_str());
        }
        else
            link._radix = QString::fromUtf8(rsPeers->GetRetroshareInvite(ssl_id).c_str());

        link._name = QString::fromUtf8(detail.name.c_str());
        link._location = QString::fromUtf8(detail.location.c_str());
        link._radix.replace("\n","");

        std::cerr << "Found radix                = " << link._radix.toStdString() << std::endl;
    }

    link.check();

    return link;
}
RetroShareLink RetroShareLink::createUnknownSslCertificate(const RsPeerId& sslId, const RsPgpId& gpgId)
{
	RetroShareLink link;
	link.clear();

	// first try ssl id
	link = createCertificate(sslId);
	if (link.valid()) {
		if (gpgId.isNull() || link._GPGid.toStdString() == gpgId.toStdString()) {
			return link;
		}
		// wrong gpg id
		link.clear();
		return link;
	}

	// then gpg id
	link = createPerson(gpgId);
	if (link.valid()) {
		if (!link._SSLid.isEmpty()) {
			link.clear();
			return link;
		}
		if (sslId.isNull()) {
			link.check();
			return link;
		}
		link._SSLid = QString::fromStdString(sslId.toStdString());
		if (link._location.isEmpty()) {
			link._location = link._name;
		}
		link.check();
		return link;
	}

	link.clear();
	return link;
}

RetroShareLink RetroShareLink::createExtraFile(const QString& name, uint64_t size, const QString& hash,const QString& ssl_id)
{
	RetroShareLink link;
	link.clear();

	link._name = name;
	link._size = size;
	link._hash = hash;
	link._SSLid = ssl_id;

	link._type = TYPE_EXTRAFILE;

	link.check();

	return link;
}

//Private Chat deprecated

RetroShareLink RetroShareLink::createPublicMsgInvite(time_t time_stamp,const QString& issuer_pgp_id,const QString& hash)
{
	RetroShareLink link;
	link.clear() ;

	link._type = TYPE_PUBLIC_MSG ;
	link._time_stamp = time_stamp ;
	link._hash = hash ;
	link._GPGid = issuer_pgp_id ;

	link.check() ;

	return link;
}

RetroShareLink RetroShareLink::createIdentity(const RsGxsId& id, const QString& name, const QString& radix_data)
{
	RetroShareLink link;
	link.clear();

	link._name = name;
	link._hash = QString::fromStdString(id.toStdString());
	link._radix_group_data = radix_data ;

	link._type = TYPE_IDENTITY;

	link.check();

	return link;
}

RetroShareLink RetroShareLink::createFileTree(const QString& name, uint64_t size, uint32_t count, const QString& radix_data)
{
	RetroShareLink link;
	link.clear();

	link._name  = name;
	link._size  = size;
	link._radix = radix_data ;
	link._count = count;

	link._type = TYPE_FILE_TREE;

	link.check();

	return link;
}

RetroShareLink RetroShareLink::createChatRoom(const ChatId &chatId, const QString &name)
{
	RetroShareLink link;
	link.clear();

	link._name = name;
	link._hash = QString::fromStdString(chatId.toStdString());

	link._type = TYPE_CHAT_ROOM;

	link.check();

	return link;
}

void RetroShareLink::clear()
{
	_valid = false ;
	_type = TYPE_UNKNOWN ;
	_name = "" ;
	_size = 0 ;
	_hash = "" ;
	_msgId = "" ;
	_subject = "" ;
	_SSLid = "" ;
	_GPGid = "" ;
	_location = "" ;
	_radix = "" ;
	_subType = 0 ;
	_time_stamp = 0 ;
	//_encrypted_chat_info = "" ;
	_radix_group_data = "" ;
	_count = 0 ;
}

void RetroShareLink::check()
{
	_valid = true;

	switch (_type)
	{
		case TYPE_UNKNOWN:
			/* fallthrough */
		case TYPE_PRIVATE_CHAT:
			_valid = false;
		break;
		case TYPE_EXTRAFILE:
			if(!checkSSLId(_SSLid))
				_valid = false;			// no break! We also test file stuff below.
			/* fallthrough */
			[[clang::fallthrough]];
		case TYPE_FILE_TREE:
			/* fallthrough */
		case TYPE_FILE:
			if(_size > ((static_cast<uint64_t>(1))<<40))	// 1TB. Who has such large files?
				_valid = false;

			if(!checkName(_name))
				_valid = false;

			if(!checkRadix64(_radix))
				_valid = false;
		break;

		case TYPE_PERSON:
			if(_size != 0)
				_valid = false;

			if(_name.isEmpty())
				_valid = false;

			if(_hash.isEmpty())
				_valid = false;
		break;

		case TYPE_FORUM:
			if(_size != 0)
				_valid = false;

			if(_name.isEmpty())
				_valid = false;

			if(_hash.isEmpty())
				_valid = false;
		break;

		case TYPE_CHANNEL:
			if(_size != 0)
				_valid = false;

			if(_name.isEmpty())
				_valid = false;

			if(_hash.isEmpty())
				_valid = false;
		break;

		case TYPE_SEARCH:
			if(_size != 0)
				_valid = false;

			if(_name.isEmpty())
				_valid = false;

			if(!_hash.isEmpty())
				_valid = false;
		break;

		case TYPE_MESSAGE:
			if(_size != 0)
				_valid = false;

			if(_hash.isEmpty())
				_valid = false;
		break;

		case TYPE_CERTIFICATE:
		break;

		//TYPE_EXTRAFILE done upper
		//TYPE_PRIVATE_CHAT done upper (deprecated)

		case TYPE_PUBLIC_MSG:
			if(!checkHash(_hash)) _valid = false ;
			if(!checkPGPId(_GPGid)) _valid = false ;
		break ;

		case TYPE_POSTED:
			if(_size != 0)
				_valid = false;

			if(_name.isEmpty())
				_valid = false;

			if(_hash.isEmpty())
				_valid = false;
		break;

		case TYPE_IDENTITY:
			if(_name.isEmpty())
				_valid = false ;

			if(_radix_group_data.isEmpty())
				_valid = false ;

			if(_hash.isEmpty())
				_valid = false ;
		break ;

		//TYPE_FILE_TREE done upper

		case TYPE_CHAT_ROOM:
			if(_size != 0)
				_valid = false;

			if(_name.isEmpty())
				_valid = false;

			if(_hash.isEmpty())
				_valid = false;
		break;
	}

	if (!_valid) {
		clear();
	}
}

QString RetroShareLink::title() const
{
	if (!valid()) {
		return "";
	}

	switch (_type) {
		case TYPE_UNKNOWN:
			/* fallthrough */
		case TYPE_PRIVATE_CHAT:
		break;

		case TYPE_FILE:
		return QString("Size: %2  hash: %1").arg(hash()).arg(misc::friendlyUnit(size()));

		case TYPE_PERSON:
		return PeerDefs::rsidFromId(RsPgpId(hash().toStdString()));

		case TYPE_FORUM:
			return QString("Forum id: %1").arg(hash());
		case TYPE_CHANNEL:
			return QString("Channel id: %1").arg(hash());
		case TYPE_SEARCH:
			return QString("Search files");

		case TYPE_MESSAGE:
		return PeerDefs::rsidFromId(RsPeerId(hash().toStdString()));

		case TYPE_CERTIFICATE:
		{
			RsPeerDetails details ;
			uint32_t error_code ;

			if(!rsPeers->loadDetailsFromStringCert(_radix.toStdString(),details,error_code))
				return QObject::tr("This cert is malformed. Error code:")+" "+QString::number(error_code) ;
			else
				return QObject::tr("Click to add this RetroShare cert to your PGP keyring\nand open the Make Friend Wizard.\n")
				    + QString("PGP Id =")+" " + QString::fromStdString(details.gpg_id.toStdString()) + QString("\nSSLId =")+" "+QString::fromStdString(details.id.toStdString());
		}

		case TYPE_EXTRAFILE:
		return QObject::tr("%1 (%2, Extra - Source included)").arg(hash()).arg(misc::friendlyUnit(size()));

		//TYPE_PRIVATE_CHAT done upper (deprecated)

		case TYPE_PUBLIC_MSG:
		{
			RsPeerDetails detail;
			rsPeers->getGPGDetails(RsPgpId(_GPGid.toStdString()), detail) ;
			return QObject::tr("Click to send a private message to %1 (%2).").arg(QString::fromUtf8(detail.name.c_str())).arg(_GPGid) ;
		}

		case TYPE_POSTED:
		break;

		case TYPE_IDENTITY:
		return _name ;

		case TYPE_FILE_TREE:
		return QObject::tr("Click to browse/download this file collection");

		case TYPE_CHAT_ROOM:
		return _name ;
	}

	return "";
}

static QString encodeItem(QString item)
{
	return QUrl::toPercentEncoding(item);
}

QString RetroShareLink::toString() const
{
	QUrl url;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QUrlQuery urlQuery;
#else
	QUrl &urlQuery(url);
#endif

	switch (_type) {
		case TYPE_UNKNOWN:
			/* fallthrough */
		case TYPE_PRIVATE_CHAT:
		return "";

		case TYPE_FILE:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_FILE);
			urlQuery.addQueryItem(FILE_NAME, encodeItem(_name));
			urlQuery.addQueryItem(FILE_SIZE, QString::number(_size));
			urlQuery.addQueryItem(FILE_HASH, _hash);

		break;

		case TYPE_PERSON:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_PERSON);
			urlQuery.addQueryItem(PERSON_NAME, encodeItem(_name));
			urlQuery.addQueryItem(PERSON_HASH, _hash);

		break;

		case TYPE_FORUM:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_FORUM);
			urlQuery.addQueryItem(FORUM_NAME, encodeItem(_name));
			urlQuery.addQueryItem(FORUM_ID, _hash);
			if (!_msgId.isEmpty()) {
				urlQuery.addQueryItem(FORUM_MSGID, _msgId);
			}

		break;

		case TYPE_CHANNEL:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_CHANNEL);
			urlQuery.addQueryItem(CHANNEL_NAME, encodeItem(_name));
			urlQuery.addQueryItem(CHANNEL_ID, _hash);
			if (!_msgId.isEmpty()) {
				urlQuery.addQueryItem(CHANNEL_MSGID, _msgId);
			}

		break;

		case TYPE_SEARCH:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_SEARCH);
			urlQuery.addQueryItem(SEARCH_KEYWORDS, encodeItem(_name));

		break;

		case TYPE_MESSAGE:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_MESSAGE);
			urlQuery.addQueryItem(MESSAGE_ID, _hash);
			if (_subject.isEmpty() == false) {
				urlQuery.addQueryItem(MESSAGE_SUBJECT, encodeItem(_subject));
			}

		break;

		case TYPE_CERTIFICATE:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_CERTIFICATE) ;
			urlQuery.addQueryItem(CERTIFICATE_RADIX, encodeItem(_radix));
			urlQuery.addQueryItem(CERTIFICATE_NAME, encodeItem(_name));
			urlQuery.addQueryItem(CERTIFICATE_LOCATION, encodeItem(_location));
		break;

		case TYPE_EXTRAFILE:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_EXTRAFILE);
			urlQuery.addQueryItem(FILE_NAME, encodeItem(_name));
			urlQuery.addQueryItem(FILE_SIZE, QString::number(_size));
			urlQuery.addQueryItem(FILE_HASH, _hash);
			urlQuery.addQueryItem(FILE_SOURCE, _SSLid);

		break;

		//TYPE_PRIVATE_CHAT done upper (deprecated)

		case TYPE_PUBLIC_MSG:
			url.setScheme(RSLINK_SCHEME) ;
			url.setHost(HOST_PUBLIC_MSG) ;
			urlQuery.addQueryItem(PUBLIC_MSG_TIME_STAMP,QString::number(_time_stamp)) ;
			urlQuery.addQueryItem(PUBLIC_MSG_HASH,_hash) ;
			urlQuery.addQueryItem(PUBLIC_MSG_SRC_PGP_ID,_GPGid) ;

		break;

		case TYPE_POSTED:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_POSTED);
			urlQuery.addQueryItem(POSTED_NAME, encodeItem(_name));
			urlQuery.addQueryItem(POSTED_ID, _hash);
			if (!_msgId.isEmpty()) {
				urlQuery.addQueryItem(POSTED_MSGID, _msgId);
			}

		break;

		case TYPE_IDENTITY:
			url.setScheme(RSLINK_SCHEME) ;
			url.setHost(HOST_IDENTITY) ;
			urlQuery.addQueryItem(IDENTITY_ID,_hash) ;
			urlQuery.addQueryItem(IDENTITY_NAME,encodeItem(_name)) ;
			urlQuery.addQueryItem(IDENTITY_GROUP,encodeItem(_radix_group_data)) ;
		break ;

		case TYPE_FILE_TREE:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_FILE_TREE) ;
			urlQuery.addQueryItem(FILE_TREE_NAME, encodeItem(_name));
			urlQuery.addQueryItem(FILE_TREE_SIZE, QString::number(_size));
			urlQuery.addQueryItem(FILE_TREE_DATA, encodeItem(_radix));
			urlQuery.addQueryItem(FILE_TREE_COUNT, QString::number(_count));
		break;

		case TYPE_CHAT_ROOM:
			url.setScheme(RSLINK_SCHEME);
			url.setHost(HOST_CHAT_ROOM);
			urlQuery.addQueryItem(CHAT_ROOM_NAME, encodeItem(_name));
			urlQuery.addQueryItem(CHAT_ROOM_ID, _hash);

		break;

	}

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	url.setQuery(urlQuery);
	return url.toString(QUrl::EncodeSpaces | QUrl::EncodeUnicode);
#else
	return url.toString().replace(" ","%20");//Seems to be already done with Qt4 but to be sure.
#endif
}

QString RetroShareLink::niceName() const
{
	if(type() == TYPE_FILE)
		return QObject::tr("%1 (%2)").arg(_name).arg(misc::friendlyUnit(_size));

	if (type() == TYPE_PERSON)
		return PeerDefs::rsid(name().toUtf8().constData(), RsPgpId(hash().toStdString()));

	if(type() == TYPE_CERTIFICATE)
	{
		RsPeerDetails details ;
		uint32_t error_code ;

		if(!rsPeers->loadDetailsFromStringCert(_radix.toStdString(),details,error_code))
			return QObject::tr("This cert is malformed. Error code:")+" "+QString::number(error_code) ;
		else
			return QObject::tr("RetroShare Certificate (%1, @%2)").arg(QString::fromUtf8(details.name.c_str()), QString::fromUtf8(details.location.c_str()));	// should add SSL id there
	}

	if(type() == TYPE_PUBLIC_MSG) {
		RsPeerDetails detail;
		rsPeers->getGPGDetails(RsPgpId(_GPGid.toStdString()), detail) ;
		return QObject::tr("Click this link to send a private message to %1 (%2)").arg(QString::fromUtf8(detail.name.c_str())).arg(_GPGid) ;
	}

	if(type() == TYPE_IDENTITY)
		return QObject::tr("Identity link (name=%1, ID=%2)").arg(_name).arg(_hash) ;

	if(type() == TYPE_FILE_TREE)
		return QObject::tr("%1 (%2 files, %3)").arg(_name).arg(_count).arg(misc::friendlyUnit(_size));

	return name();
}

QString RetroShareLink::toHtml() const
{
	QString html = "<a href=\"" + toString() + "\"";

	QString linkTitle = title();
	if (!linkTitle.isEmpty()) {
		html += " title=\"" + linkTitle + "\"";
	}
	html += ">" + niceName() + "</a>" ;

	return html;
}

QString RetroShareLink::toHtmlFull() const
{
	return QString("<a href=\"") + toString() + "\">" + toString() + "</a>" ;
}

QString RetroShareLink::toHtmlSize() const
{
	QString size = QString("(%1)").arg(misc::friendlyUnit(_size));

	if (type() == TYPE_FILE && RsCollection::isCollectionFile(name())) {
		FileInfo finfo;
        if (rsFiles->FileDetails(RsFileHash(hash().toStdString()), RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL, finfo))
        {
            RsCollection::RsCollectionErrorCode code;
            RsCollection collection(QString::fromUtf8(finfo.path.c_str()), code) ;

            if(code == RsCollection::RsCollectionErrorCode::COLLECTION_NO_ERROR)
				size += QString(" [%1]").arg(misc::friendlyUnit(collection.size()));
		}
	}
	QString link = QString("<a href=\"%1\">%2</a> <font color=\"blue\">%3</font>").arg(toString()).arg(name()).arg(size);
	return link;
}

bool RetroShareLink::checkName(const QString& name)
{
	if(name.isEmpty())
		return false ;

	for(int i=0;i<name.length();++i)
	{
		QChar::Category cat( name[i].category() ) ;

		if(	cat == QChar::Separator_Line
		    || cat == QChar::Other_NotAssigned
		    )
		{
#ifdef DEBUG_RSLINK
			std::cerr <<"Unwanted category " << cat << " at place " << i << " in string \"" << name.toStdString() << "\"" << std::endl ;
#endif
			return false ;
		}
	}

	return true ;
}

QUrl RetroShareLink::toUrl() const
{
	return QUrl::fromEncoded(toString().toUtf8().constData());
}

bool RetroShareLink::checkSSLId(const QString& ssl_id)
{
	if(ssl_id.length() != 32)
		return false ;

	QByteArray qb(ssl_id.toLatin1()) ;

	for(int i=0;i<qb.length();++i)
	{
		char b(qb[i]) ;

		if(!((b>='0' && b<='9') || (b>='a' && b<='f')))
			return false ;
	}

	return true ;
}

bool RetroShareLink::checkPGPId(const QString& pgp_id)
{
	if(pgp_id.length() != 16)
		return false ;

	QByteArray qb(pgp_id.toLatin1()) ;

	for(int i=0;i<qb.length();++i)
	{
		char b(qb[i]) ;

		if(!((b>='0' && b<='9') || (b>='A' && b<='F')))
			return false ;
	}

	return true ;
}

bool RetroShareLink::checkRadix64(const QString& s)
{
	QByteArray qb(s.toLatin1()) ;

	for(int i=0;i<qb.length();++i)
	{
		char b(qb[i]) ;

		if(!(  (b >= '0' && b <= '9') || (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || b=='/' || b=='+' || b=='='))
		{
			std::cerr << "Character not allowed in radix: " << b << std::endl;
			return false;
		}
	}
	return true ;
}

bool RetroShareLink::checkHash(const QString& hash)
{
	if(hash.length() != 40)
		return false ;

	QByteArray qb(hash.toLatin1()) ;

	for(int i=0;i<qb.length();++i)
	{
		char b(qb[i]) ;

		if(!((b>='0' && b<='9') || (b>='a' && b<='f')))
			return false ;
	}

	return true ;
}

static void processList(const QStringList &list, const QString &textSingular, const QString &textPlural, QString &result)
{
	if (list.isEmpty()) {
		return;
	}
	if (list.size() == 1) {
		result += "" + textSingular + ":";
	} else {
		result += "" + textPlural + ":";
	}
	result += "<p style='margin-left: 5px; margin-top: 0px'>";
	QStringList::const_iterator it;
	for (it = list.begin(); it != list.end(); ++it) {
		if (it != list.begin()) {
			result += ", ";
		}
		result += *it;
	}
	result += "</p>";
}

/*static*/ int RetroShareLink::process(const QList<RetroShareLink> &linksIn, uint flag /* = RSLINK_PROCESS_NOTIFY_ALL*/)
{
	QList<RetroShareLink>::const_iterator linkIt;
	flag &= ~RSLINK_PROCESS_NOTIFY_BAD_CHARS ;	// this will be set below when needed

	/* filter dublicate links */
	QList<RetroShareLink> links;
	for (linkIt = linksIn.begin(); linkIt != linksIn.end(); ++linkIt) {
		if (links.contains(*linkIt)) {
			continue;
		}

		links.append(*linkIt);
	}

	if (flag & RSLINK_PROCESS_NOTIFY_ASK)
	{
		/* ask for some types of link */
		//QStringList fileAdd;
		QStringList personAdd;

		for (linkIt = links.begin(); linkIt != links.end(); ++linkIt) {
			const RetroShareLink &link = *linkIt;

			if (link.valid() == false) {
				continue;
			}

			switch (link.type()) {
				case TYPE_UNKNOWN:
				case TYPE_FILE:
				case TYPE_FORUM:
				case TYPE_CHANNEL:
				case TYPE_SEARCH:
				case TYPE_MESSAGE:
				case TYPE_CERTIFICATE:
				case TYPE_EXTRAFILE:
				case TYPE_PRIVATE_CHAT:
				case TYPE_PUBLIC_MSG:
				case TYPE_POSTED:
				case TYPE_IDENTITY:
				case TYPE_FILE_TREE:
				case TYPE_CHAT_ROOM:
					// no need to ask
				break;

				case TYPE_PERSON:
					personAdd.append(PeerDefs::rsid(link.name().toUtf8().constData(), RsPgpId(link.hash().toStdString())));
				break;
			}
		}
	}

	int countInvalid = 0;
	int countUnknown = 0;
	int countFileOpened = 0;
	bool needNotifySuccess = false;
	bool dontOpenNextFile = false;

	// file
	QStringList fileAdded;
	QStringList fileExist;

	// person
	QStringList personAdded;
	QStringList personExist;
	QStringList personFailed;
	QStringList personNotFound;

	// forum
	QStringList forumFound;
	QStringList forumMsgFound;
	QStringList forumUnknown;
	QStringList forumMsgUnknown;

	// channel
	QStringList channelFound;
	QStringList channelMsgFound;
	QStringList channelUnknown;
	QStringList channelMsgUnknown;

	// search
	QStringList searchStarted;

	// message
	QStringList messageStarted;
	QStringList messageReceipientNotAccepted;
	QStringList messageReceipientUnknown;

	// Certificate
	//QStringList GPGBase64Strings ;
	//QStringList SSLIds ;

	// posted
	QStringList postedFound;
	QStringList postedMsgFound;
	QStringList postedUnknown;
	QStringList postedMsgUnknown;

	// chat room
	QStringList chatroomFound;
	QStringList chatroomUnknown;

	// for summary at end
	QList<QStringList*> processedList;
	QList<QStringList*> errorList;

	processedList << &fileAdded << &personAdded << &forumFound << &channelFound << &searchStarted << &messageStarted << &postedFound << &chatroomFound;
	errorList << &fileExist << &personExist << &personFailed << &personNotFound << &forumUnknown << &forumMsgUnknown << &channelUnknown << &channelMsgUnknown << &messageReceipientNotAccepted << &messageReceipientUnknown << &postedUnknown << &postedMsgUnknown << &chatroomUnknown;
	// not needed: forumFound, channelFound, messageStarted

	// we want to merge all single file links into one collection
	// if a collection tree link is found it is processed independen for the other file links
	RsCollection col;
	bool fileLinkFound = false;

	for (linkIt = links.begin(); linkIt != links.end(); ++linkIt) {
		const RetroShareLink &link = *linkIt;

		if (link.valid() == false) {
			std::cerr << " RetroShareLink::process invalid request" << std::endl;
			++countInvalid;
			continue;
		}

		switch (link.type())
		{
			case TYPE_UNKNOWN:
				++countUnknown;
			break;

			case TYPE_PERSON:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process FriendRequest : name : " << link.name().toStdString() << ". id : " << link.hash().toStdString() << std::endl;
#endif

				RsPeerDetails detail;
				if (rsPeers->getGPGDetails(RsPgpId(link.hash().toStdString()), detail))
					PGPKeyDialog::showIt(detail.gpg_id,PGPKeyDialog::PageDetails) ;
				else
					personNotFound.append(PeerDefs::rsid(link.name().toUtf8().constData(), RsPgpId(link.hash().toStdString())));
			}
			break;

			case TYPE_FORUM:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process ForumRequest : name : " << link.name().toStdString() << ". id : " << link.hash().toStdString() << ". msgId : " << link.msgId().toStdString() << std::endl;
#endif

				MainWindow::showWindow(MainWindow::Forums);
				GxsForumsDialog *forumsDialog = dynamic_cast<GxsForumsDialog*>(MainWindow::getPage(MainWindow::Forums));
				if (!forumsDialog) {
					return false;
				}

				if (forumsDialog->navigate(RsGxsGroupId(link.id().toStdString()), RsGxsMessageId(link.msgId().toStdString()))) {
					if (link.msgId().isEmpty()) {
						forumFound.append(link.name());
					} else {
						forumMsgFound.append(link.name());
					}
				} else {
					if (link.msgId().isEmpty()) {
						forumUnknown.append(link.name());
					} else {
						forumMsgUnknown.append(link.name());
					}
				}
			}
			break;

			case TYPE_CHANNEL:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process ChannelRequest : name : " << link.name().toStdString() << ". id : " << link.hash().toStdString() << ". msgId : " << link.msgId().toStdString() << std::endl;
#endif

				MainWindow::showWindow(MainWindow::Channels);
				GxsChannelDialog *channelDialog = dynamic_cast<GxsChannelDialog*>(MainWindow::getPage(MainWindow::Channels));
				if (!channelDialog) {
					return false;
				}

				if (channelDialog->navigate(RsGxsGroupId(link.id().toStdString()), RsGxsMessageId(link.msgId().toStdString()))) {
					if (link.msgId().isEmpty()) {
						channelFound.append(link.name());
					} else {
						channelMsgFound.append(link.name());
					}
				} else {
					if (link.msgId().isEmpty()) {
						channelUnknown.append(link.name());
					} else {
						channelMsgUnknown.append(link.name());
					}
				}
			}
			break;

			case TYPE_SEARCH:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process SearchRequest : string : " << link.name().toStdString() << std::endl;
#endif
				SearchDialog *searchDialog = dynamic_cast<SearchDialog*>(MainWindow::getPage(MainWindow::Search));
				if (!searchDialog)
				{
					std::cerr << "Retrieve of search dialog failed. Please debug!" << std::endl;
					break;
				}

				MainWindow::showWindow(MainWindow::Search);
				searchDialog->searchKeywords(link.name());
				searchStarted.append(link.name());
			}
			break;

			case TYPE_MESSAGE:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process MessageRequest : id : " << link.hash().toStdString() << ", subject : " << link.name().toStdString() << std::endl;
#endif
				RsPeerDetails detail;

				// This is awful, but apparently the hash can be multiple different types. Let's check!

				RsPeerId  ssl_id(link.hash().toStdString()) ;

				if(!ssl_id.isNull() && rsPeers->getPeerDetails(ssl_id,detail) && detail.accept_connection)
				{
					MessageComposer *msg = MessageComposer::newMsg();
					msg->addRecipient(MessageComposer::TO, detail.id);
					if (link.subject().isEmpty() == false) {
						msg->setTitleText(link.subject());
					}
					msg->show();
					messageStarted.append(PeerDefs::nameWithLocation(detail));
					break ;
				}

				RsIdentityDetails gxs_details ;
				RsGxsId gxs_id(link.hash().toStdString()) ;

				if(!gxs_id.isNull() && rsIdentity->getIdDetails(gxs_id,gxs_details))
				{
					if(gxs_details.mFlags & RS_IDENTITY_FLAGS_IS_OWN_ID)
					{
						QMessageBox::warning(nullptr,QString("Cannot send message to yourself"),QString("This identity is owned by you. You wouldn't want to send yourself a message right?"));
						break ;
					}

					MessageComposer *msg = MessageComposer::newMsg();
					msg->addRecipient(MessageComposer::TO, gxs_id) ;

					if (link.subject().isEmpty() == false)
						msg->setTitleText(link.subject());

					msg->show();
					messageStarted.append(PeerDefs::nameWithLocation(gxs_details));

					break ;
				}
				messageReceipientUnknown.append(PeerDefs::rsidFromId(RsPeerId(link.hash().toStdString())));
			}
			break;

			case TYPE_CERTIFICATE:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process certificate." << std::endl;
#endif
				needNotifySuccess = true;

				std::cerr << "Usign this certificate:" << std::endl;
				std::cerr << link.radix().toStdString() << std::endl;

				ConnectFriendWizard connectFriendWizard;
				connectFriendWizard.setCertificate(link.radix(), (link.subType() == RSLINK_SUBTYPE_CERTIFICATE_USER_REQUEST) ? true : false);
				connectFriendWizard.exec();
				needNotifySuccess = false;
			}
			break ;

			case TYPE_FILE:
			{
				FileInfo fi1;
				if(links.size()==1 && rsFiles->alreadyHaveFile(RsFileHash(link.hash().toStdString()), fi1)
					&& !link.name().endsWith(RsCollection::ExtensionString))
				{
					/* fallthrough */
				}
				else
				{
					col.merge_in(link.name(),link.size(),RsFileHash(link.hash().toStdString())) ;
					fileLinkFound = true;
					break;
				}
			}
			//break;
			/* fallthrough */
			[[clang::fallthrough]];
			case TYPE_EXTRAFILE:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process FileRequest : fileName : " << link.name().toUtf8().constData() << ". fileHash : " << link.hash().toStdString() << ". fileSize : " << link.size() << std::endl;
#endif

				needNotifySuccess = true;
				std::list<RsPeerId> srcIds;

				// Add the link built-in source. This is needed for EXTRA files, where the source is specified in the link.

				if(link.type() == TYPE_EXTRAFILE)
				{
#ifdef DEBUG_RSLINK
					std::cerr << " RetroShareLink::process Adding built-in source " << link.SSLId().toStdString() << std::endl;
#endif
					srcIds.push_back(RsPeerId(link.SSLId().toStdString())) ;
				}

				// Get a list of available direct sources, in case the file is browsable only.
				//
				FileInfo finfo ;
				rsFiles->FileDetails(RsFileHash(link.hash().toStdString()), RS_FILE_HINTS_REMOTE, finfo) ;

				for(std::vector<TransferInfo>::const_iterator it(finfo.peers.begin());it!=finfo.peers.end();++it)
				{
#ifdef DEBUG_RSLINK
					std::cerr << "  adding peerid " << (*it).peerId << std::endl ;
#endif
					srcIds.push_back((*it).peerId) ;
				}

				QString cleanname = link.name() ;
				static const QString bad_chars_str = "/\\\"*:?<>|" ;

				for(int i=0;i<cleanname.length();++i)
					for(int j=0;j<bad_chars_str.length();++j)
						if(cleanname[i] == bad_chars_str[j])
						{
							cleanname[i] = '_';
							flag |= RSLINK_PROCESS_NOTIFY_BAD_CHARS ;
						}

				bool bFileOpened = false;
				FileInfo fi;
				if (rsFiles->alreadyHaveFile(RsFileHash(link.hash().toStdString()), fi)) {
					/* make path for downloaded file */
					std::string path;
					path = fi.path;//Shared files has path with filename included

					//Seems that all FileInfo get .path==filepath+filename
					//if (fi.downloadStatus == FT_STATE_COMPLETE)
					//	path = fi.path + "/" + fi.fname;

					QFileInfo qinfo;
					qinfo.setFile(QString::fromUtf8(path.c_str()));
					if (qinfo.exists() && qinfo.isFile() && !dontOpenNextFile) {
						QString question = "<html><body>";
						question += QObject::tr("Warning: Retroshare is about to ask your system to open this file. ");
						question += QObject::tr("Before you do so, please make sure that this file does not contain malicious executable code.");
						question += "<br><br>" + cleanname + "</body></html>";

						QMessageBox mb(QObject::tr("Confirmation"), question, QMessageBox::Warning, QMessageBox::Yes, QMessageBox::No, links.size()>1 ? QMessageBox::NoToAll : 0, nullptr);
						int ret = mb.exec();
						if(ret == QMessageBox::Yes) {
							++countFileOpened;
							bFileOpened = true;
							/* open file with a suitable application */
							if (!RsUrlHandler::openUrl(QUrl::fromLocalFile(qinfo.absoluteFilePath()))) {
								std::cerr << "RetroShareLink::process(): can't open file " << path << std::endl;
							}
						} else if (ret == QMessageBox::NoToAll) {
							dontOpenNextFile = true;
						}
						needNotifySuccess = false;
					}
				}

				if (rsFiles->FileRequest(cleanname.toUtf8().constData(), RsFileHash(link.hash().toStdString()), link.size(), "", RS_FILE_REQ_ANONYMOUS_ROUTING, srcIds)) {
					fileAdded.append(link.name());
				} else {
					if (!bFileOpened && links.size()>1) fileExist.append(link.name());}
			}
			break;

			//TYPE_PRIVATE_CHAT

			case TYPE_PUBLIC_MSG:
			{
				std::cerr << "(!!) Distant messages from links is disabled for now" << std::endl;
				//		std::cerr << "Opening a public msg window " << std::endl;
				//		std::cerr << "      time_stamp = " << link._time_stamp << std::endl;
				//		std::cerr << "      hash       = " << link._hash.toStdString() << std::endl;
				//		std::cerr << "      Issuer Id  = " << link._GPGid.toStdString() << std::endl;
				//
				//		if(link._time_stamp < time(NULL))
				//		{
				//			QMessageBox::information(NULL,QObject::tr("Messaging link is expired"),QObject::tr("This Messaging link is expired. The destination peer will not receive it.")) ;
				//			break ;
				//		}
				//
				//		 MessageComposer::msgDistantPeer(link._hash.toStdString(),link._GPGid.toStdString()) ;
			}
			break ;

			case TYPE_POSTED:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process PostedRequest : name : " << link.name().toStdString() << ". id : " << link.hash().toStdString() << ". msgId : " << link.msgId().toStdString() << std::endl;
#endif

				MainWindow::showWindow(MainWindow::Posted);
				PostedDialog *postedDialog = dynamic_cast<PostedDialog*>(MainWindow::getPage(MainWindow::Posted));

				if (!postedDialog) {
					return false;
				}

				if (postedDialog->navigate(RsGxsGroupId(link.id().toStdString()), RsGxsMessageId(link.msgId().toStdString()))) {
					if (link.msgId().isEmpty()) {
						postedFound.append(link.name());
					} else {
						postedMsgFound.append(link.name());
					}
				} else {
					if (link.msgId().isEmpty()) {
						postedUnknown.append(link.name());
					} else {
						postedMsgUnknown.append(link.name());
					}
				}
			}
			break;

			case TYPE_IDENTITY:
			{
				if(rsIdentity->deserialiseIdentityFromMemory(link.radixGroupData().toStdString()))
					QMessageBox::information(nullptr,QObject::tr("Identity added to People"),QObject::tr("The identity was added to people. You can now chat with it, send messages to it, etc.")) ;
				else
					QMessageBox::warning(nullptr,QObject::tr("Identity cannot be added to People"),QObject::tr("The identity was not added to people. Some error occured. The link is probably corrupted.")) ;
			}
			break;

		case TYPE_FILE_TREE:
		{
            auto ft = RsFileTree::fromRadix64(link.radix().toStdString() );
            RsCollectionDialog::downloadFiles(RsCollection(*ft));
            break;
		}

			case TYPE_CHAT_ROOM:
			{
#ifdef DEBUG_RSLINK
				std::cerr << " RetroShareLink::process ChatRoom Request : name : " << link.name().toStdString() << ". id : " << link.hash().toStdString() << std::endl;
#endif
				ChatId chatId(link.id().toStdString());

				if (chatId.isBroadcast())
				{
					MainWindow::showWindow(MainWindow::Network);
					chatroomFound.append(link.name());
				}
				else if (chatId.isNotSet())
					chatroomUnknown.append(link.name());
				else if (chatId.isLobbyId())
				{
					MainWindow::showWindow(MainWindow::ChatLobby);
					ChatLobbyWidget *chatLobbyDialog = dynamic_cast<ChatLobbyWidget*>(MainWindow::getPage(MainWindow::ChatLobby));
					if (!chatLobbyDialog) {
						return false;
					}
					if (chatLobbyDialog->showLobbyAnchor(chatId.toLobbyId(),"")) {
						chatroomFound.append(link.name());
					} else {
						chatroomUnknown.append(link.name());
					}
				}
				else
				{
					ChatDialog* chatDialog = ChatDialog::getChat(chatId, Settings->getChatFlags());
					if (chatDialog) {
						chatroomFound.append(link.name());
					} else {
						chatroomUnknown.append(link.name());
					}
				}

			}
			break;

			default:
				std::cerr << " RetroShareLink::process unknown type: " << link.type() << std::endl;
				++countUnknown;
		}
	}

	// were single file links found?
	if (fileLinkFound)
        RsCollectionDialog::downloadFiles(col);

	int countProcessed = 0;
	int countError = 0;

	QList<QStringList*>::iterator listIt;
	for (listIt = processedList.begin(); listIt != processedList.end(); ++listIt) {
		countProcessed += (*listIt)->size();
	}
	for (listIt = errorList.begin(); listIt != errorList.end(); ++listIt) {
		countError += (*listIt)->size();
	}

	// success notify needed ?
	if (needNotifySuccess == false) {
		flag &= ~RSLINK_PROCESS_NOTIFY_SUCCESS;
	}
	// error notify needed ?
	if (countError == 0) {
		flag &= ~RSLINK_PROCESS_NOTIFY_ERROR;
	}

	QString result;

	if (flag & (RSLINK_PROCESS_NOTIFY_SUCCESS | RSLINK_PROCESS_NOTIFY_ERROR)) {
		result += (links.count() == 1 ? QObject::tr("%1 of %2 RetroShare link processed.") : QObject::tr("%1 of %2 RetroShare links processed.")).arg(countProcessed).arg(links.count()) + "<br><br>";
	}

	// file
	if (flag & RSLINK_PROCESS_NOTIFY_SUCCESS) {
		if (!fileAdded.isEmpty()) {
			processList(fileAdded, QObject::tr("File added"), QObject::tr("Files added"), result);
		}
	}
	if (flag & RSLINK_PROCESS_NOTIFY_ERROR) {
		if (!fileExist.isEmpty()) {
			processList(fileExist, QObject::tr("File exist"), QObject::tr("Files exist"), result);
		}
	}

	// person
	if (flag & RSLINK_PROCESS_NOTIFY_SUCCESS) {
		if (!personAdded.isEmpty()) {
			processList(personAdded, QObject::tr("Friend added"), QObject::tr("Friends added"), result);
		}
	}
	if (flag & RSLINK_PROCESS_NOTIFY_ERROR) {
		if (!personExist.isEmpty()) {
			processList(personExist, QObject::tr("Friend exist"), QObject::tr("Friends exist"), result);
		}
		if (!personFailed.isEmpty()) {
			processList(personFailed, QObject::tr("Friend not added"), QObject::tr("Friends not added"), result);
		}
		if (!personNotFound.isEmpty()) {
			processList(personNotFound, QObject::tr("Friend not found"), QObject::tr("Friends not found"), result);
		}
	}

	// forum
	if (flag & RSLINK_PROCESS_NOTIFY_ERROR) {
		if (!forumUnknown.isEmpty()) {
			processList(forumUnknown, QObject::tr("Forum not found"), QObject::tr("Forums not found"), result);
		}
		if (!forumMsgUnknown.isEmpty()) {
			processList(forumMsgUnknown, QObject::tr("Forum message not found"), QObject::tr("Forum messages not found"), result);
		}
	}

	// channel
	if (flag & RSLINK_PROCESS_NOTIFY_ERROR) {
		if (!channelUnknown.isEmpty()) {
			processList(channelUnknown, QObject::tr("Channel not found"), QObject::tr("Channels not found"), result);
		}
		if (!channelMsgUnknown.isEmpty()) {
			processList(channelMsgUnknown, QObject::tr("Channel message not found"), QObject::tr("Channel messages not found"), result);
		}
	}

	// message
	if (flag & RSLINK_PROCESS_NOTIFY_ERROR) {
		if (!messageReceipientNotAccepted.isEmpty()) {
			processList(messageReceipientNotAccepted, QObject::tr("Recipient not accepted"), QObject::tr("Recipients not accepted"), result);
		}
		if (!messageReceipientUnknown.isEmpty()) {
			processList(messageReceipientUnknown, QObject::tr("Unkown recipient"), QObject::tr("Unkown recipients"), result);
		}
	}

	// posted
	if (flag & RSLINK_PROCESS_NOTIFY_ERROR) {
		if (!postedUnknown.isEmpty()) {
			processList(postedUnknown, QObject::tr("Posted not found"), QObject::tr("Posted not found"), result);
		}
		if (!postedMsgUnknown.isEmpty()) {
			processList(postedMsgUnknown, QObject::tr("Posted message not found"), QObject::tr("Posted messages not found"), result);
		}
	}

	// chat room
	if (flag & RSLINK_PROCESS_NOTIFY_ERROR) {
		if (!chatroomUnknown.isEmpty()) {
			processList(chatroomUnknown, QObject::tr("Chat room not found"), QObject::tr("Chat room not found"), result);
		}
	}

	if (flag & RSLINK_PROCESS_NOTIFY_ERROR) {
		if (countUnknown) {
			result += QString("<br>%1: %2").arg(QObject::tr("Malformed links")).arg(countUnknown);
		}
		if (countInvalid) {
			result += QString("<br>%1: %2").arg(QObject::tr("Invalid links")).arg(countInvalid);
		}
	}
	if(flag & RSLINK_PROCESS_NOTIFY_BAD_CHARS)
		result += QString("<br>%1").arg(QObject::tr("Warning: forbidden characters found in filenames. \nCharacters <b>\",|,/,\\,&lt;,&gt;,*,?</b> will be replaced by '_'.")) ;

	if ((result.isEmpty() == false) && (links.count() > countFileOpened)) { //Don't count files opened
		QMessageBox mb(QObject::tr("Result"), "<html><body>" + result + "</body></html>", QMessageBox::Information, QMessageBox::Ok, 0, 0);
		mb.exec();
	}

	return 0;
}

/*static*/ int RetroShareLink::process(const QStringList &urls, RetroShareLink::enumType type /* = RetroShareLink::TYPE_UNKNOWN*/, uint flag /* = RSLINK_PROCESS_NOTIFY_ALL*/)
{
	QList<RetroShareLink> links;

	for (QStringList::const_iterator it = urls.begin(); it != urls.end(); ++it) {
		RetroShareLink link(*it);
		if (link.valid() && (type == RetroShareLink::TYPE_UNKNOWN || link.type() == type)) {
			links.append(link);
		}
	}

	return process(links, flag);
}

/**** RSLinkClipboard ********************************************/

void RSLinkClipboard::copyLinks(const QList<RetroShareLink>& links)
{
	QString res ;
	for (int i = 0; i < links.size(); ++i)
		res += links[i].toString() + "\n" ;

	QApplication::clipboard()->setText(res) ;
}

void RSLinkClipboard::pasteLinks(QList<RetroShareLink> &links,RetroShareLink::enumType type)
{
	return parseClipboard(links,type);
}

void RSLinkClipboard::parseClipboard(QList<RetroShareLink> &links,RetroShareLink::enumType type)
{
	// parse clipboard for links.
	//
	QString text = QApplication::clipboard()->text() ;
	parseText(text, links,type);
}

QString RSLinkClipboard::toString()
{
	QList<RetroShareLink> links;
	parseClipboard(links);

	QString res ;
	for(int i = 0; i < links.size(); ++i)
		res += links[i].toString() + "\n" ;

	return res ;
}

QString RSLinkClipboard::toHtml()
{
	QList<RetroShareLink> links;
	parseClipboard(links);

	QString res ;
	for(int i = 0; i < links.size(); ++i)
		res += links[i].toHtml() + "<br>" ;

	return res ;
}

bool RSLinkClipboard::empty(RetroShareLink::enumType type /* = RetroShareLink::TYPE_UNKNOWN*/)
{
	QList<RetroShareLink> links;
	parseClipboard(links);

	if (type == RetroShareLink::TYPE_UNKNOWN) {
		return links.empty();
	}

	for (QList<RetroShareLink>::iterator link = links.begin(); link != links.end(); ++link) {
		if (link->type() == type) {
			return false;
		}
	}

	return true;
}

int RSLinkClipboard::process(RetroShareLink::enumType type /* = RetroShareLink::TYPE_UNKNOWN*/, uint flag /* = RSLINK_PROCESS_NOTIFY_ALL*/)
{
	QList<RetroShareLink> links;
	pasteLinks(links,type);

	if (links.isEmpty())
		return 0;

	return RetroShareLink::process(links, flag);
}

void RSLinkClipboard::parseText(QString text, QList<RetroShareLink> &links,RetroShareLink::enumType type )
{
	links.clear();

#ifdef DEBUG_RSLINK
	std::cerr << "Parsing text:" << text.toStdString() << std::endl ;
#endif

	QRegExp rx(QString("retroshare://(%1)[^\r\n]+").arg(HOST_REGEXP));

	int pos = 0;

	while((pos = rx.indexIn(text, pos)) != -1)
	{
		QString url(text.mid(pos, rx.matchedLength()));
		RetroShareLink link(url);

		if(link.valid() && (type == RetroShareLink::TYPE_UNKNOWN || type == link.type()))
		{
			// check that the link is not already in the list:
			bool already = false ;
			for (int i = 0; i <links.size(); ++i)
				if(links[i] == link)
				{
					already = true ;
					break ;
				}

			if(!already)
			{
				links.push_back(link) ;
#ifdef DEBUG_RSLINK
				std::cerr << "captured link: " << link.toString().toStdString() << std::endl ;
#endif
			}
		}
#ifdef DEBUG_RSLINK
		else
			std::cerr << "invalid link" << std::endl ;
#endif

		pos += rx.matchedLength();
	}
}

