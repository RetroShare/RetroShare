/*******************************************************************************
 * gui/RetroshareLink.h                                                        *
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

#ifndef RETROSHARE_LINK_ANALYZER
#define RETROSHARE_LINK_ANALYZER

// This class implements a RS link.
//
// The link is done in a way that if valid()==true, then the link can be used.
//
// The following combinations have been tested:
//  copy   paste->   Transfers (DL) | Message compose (forums) | Private msg      | Public chat | private Chat
//    -------------+----------------+--------------------------+------------------+-------------+-------------
//          search |      Y         |            Y             | Y (send RS link) | Paste menu? | Paste menu?
//    -------------+----------------+--------------------------+------------------+-------------+-------------
//          shared |      Y         |            Y             | Y (send RS link) | Paste menu? | Paste menu?
//    -------------+----------------+--------------------------+------------------+-------------+-------------
//

#include <retroshare/rsgxsifacetypes.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rstypes.h>

#include <QString>
#include <QUrl>
#include <QVector>

#include <stdint.h>

#define RSLINK_PROCESS_NOTIFY_SUCCESS     0x01u // notify on success
#define RSLINK_PROCESS_NOTIFY_ERROR       0x02u // notify on error
#define RSLINK_PROCESS_NOTIFY_ASK         0x04u // ask for add the links
#define RSLINK_PROCESS_NOTIFY_BAD_CHARS   0x08u // / or \\ characters in a filename

#define RSLINK_PROCESS_NOTIFY_ALL         0x0Fu

#define RSLINK_SCHEME          "retroshare"

#define RSLINK_SUBTYPE_CERTIFICATE_USER_REQUEST 1
#define RSLINK_SUBTYPE_FILE_EXTRA               2

class RetroShareLink
{
	public:
		enum enumType {
			TYPE_UNKNOWN       = 0x00,
			TYPE_FILE          = 0x01,
			TYPE_PERSON        = 0x02,
			TYPE_FORUM         = 0x03,
			TYPE_CHANNEL       = 0x04,
			TYPE_SEARCH        = 0x05,
			TYPE_MESSAGE       = 0x06,
			TYPE_CERTIFICATE   = 0x07,
			TYPE_EXTRAFILE     = 0x08,
			TYPE_PRIVATE_CHAT  = 0x09,//Deprecated
			TYPE_PUBLIC_MSG    = 0x0a,
			TYPE_POSTED        = 0x0b,
			TYPE_IDENTITY      = 0x0c,
			TYPE_FILE_TREE     = 0x0d,
            TYPE_CHAT_ROOM     = 0x0e,
            TYPE_WIRE          = 0x0f
		};

	public:
		RetroShareLink();
		RetroShareLink(const QUrl& url);
		RetroShareLink(const QString& url);

		static RetroShareLink createFile(const QString& name, uint64_t size, const QString& hash);
		static RetroShareLink createPerson(const RsPgpId &id);
		static RetroShareLink createGxsGroupLink(const RetroShareLink::enumType &linkType, const RsGxsGroupId &groupId, const QString &groupName);
		static RetroShareLink createGxsMessageLink(const RetroShareLink::enumType &linkType, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &msgName);
		static RetroShareLink createSearch(const QString& keywords);
		static RetroShareLink createMessage(const RsPeerId &peerId, const QString& subject);
		static RetroShareLink createMessage(const RsGxsId &peerId, const QString& subject);
		static RetroShareLink createCertificate(const RsPeerId &ssl_id) ;
		static RetroShareLink createUnknownSslCertificate(const RsPeerId &sslId, const RsPgpId &gpgId = RsPgpId()) ;
		static RetroShareLink createExtraFile(const QString& name, uint64_t size, const QString& hash, const QString& ssl_id);
		static RetroShareLink createPublicMsgInvite(time_t time_stamp,const QString& pgp_id,const QString& hash) ;
		static RetroShareLink createIdentity(const RsGxsId& gxs_id,const QString& name,const QString& radix_data) ;
		static RetroShareLink createFileTree(const QString& name, uint64_t size,uint32_t count,const QString& radix_data);
		static RetroShareLink createChatRoom(const ChatId &chatId, const QString& name);

		bool valid() const { return _valid; }
		enumType type() const {return _type; }
		const QString& name() const { return _name ; }
		uint64_t size() const { return _size ; }
		const QString& hash() const { return _hash ; }
		const QString& id() const { return _hash ; }
		const QString& msgId() const { return _msgId ; }
		const QString& subject() const { return _subject ; }
		const QString& SSLId() const { return _SSLid ; }
		const QString& GPGId() const { return _GPGid ; }
		//const QString& GPGRadix64Key() const { return _GPGBase64String ; }
		//const QString& GPGBase64CheckSum() const { return _GPGBase64CheckSum ; }
		const QString& location() const { return _location ; }
		//const QString& externalIPAndPort() const { return _ext_ip_port ; }
		//const QString& localIPAndPort() const { return _loc_ip_port ; }
		//const QString& dyndns() const { return _dyndns_name ; }
		const QString& radix() const { return _radix ; }
		time_t timeStamp() const { return _time_stamp ; }
		QString radixGroupData() const { return _radix_group_data ;}
		uint32_t count() const { return _count ; }

		unsigned int subType() const { return _subType; }
		void setSubType(unsigned int subType) { _subType = subType; }

		// get title depends link's type
		QString title() const;
		// get nice name for anchor
		QString niceName() const;

		/// returns the string retroshare://file?name=&size=&hash=
		///                    retroshare://person?name=&hash=
		QString toString() const;
		/// returns the string <a href="retroshare://file?name=&size=&hash=">name</a>
		///                    <a href="retroshare://person?name=&hash=">name@hash</a>
		QString toHtml() const ;
		/// returns the string <a href="retroshare://file?name=&size=&hash=">retroshare://file?name=&size=&hash=</a>
		///                    <a href="retroshare://person?name=&hash=">retroshare://person?name=&hash=</a>
		QString toHtmlFull() const ;
		
		QString toHtmlSize() const ;

		QUrl toUrl() const ;

		bool operator==(const RetroShareLink& l) const { return _type == l._type && _hash == l._hash ; }

		static int process(const QStringList &urls, RetroShareLink::enumType type = RetroShareLink::TYPE_UNKNOWN, uint flag = RSLINK_PROCESS_NOTIFY_ALL);
		static int process(const QList<RetroShareLink> &links, uint flag = RSLINK_PROCESS_NOTIFY_ALL);

	private:
		void fromString(const QString &url);
		void fromUrl(const QUrl &url);
		void clear();
		void check();
		static bool checkHash(const QString& hash);
		static bool checkRadix64(const QString& s);
		static bool checkName(const QString& name);
		static bool checkSSLId(const QString& name);
		static bool checkPGPId(const QString& name);

		bool     _valid;
		enumType _type;
		QString  _name;
		uint64_t _size;
		QString  _hash;  // or id (forum, channel, message, chat room)
		QString  _msgId; // id of the message (forum, channel)
		QString  _subject;
		QString  _SSLid ; // ssl id for rs links
		QString  _GPGid ; // ssl id for rs links
		//QString  _GPGBase64String ; // GPG Cert
		//QString  _GPGBase64CheckSum ; // GPG Cert
		QString  _location ;	// location 
		//QString  _ext_ip_port ;
		//QString  _loc_ip_port ;
		//QString  _dyndns_name ;
		QString  _radix ;
		//QString  _encrypted_chat_info ; // encrypted data string for the recipient of a chat invite
		time_t   _time_stamp ; 				// time stamp at which the link will expire.
		QString  _radix_group_data;
		uint32_t _count ;

		unsigned int _subType; // for general use as sub type for _type (RSLINK_SUBTYPE_...)
};

/// This class handles the copy/paste of links. Every member is static to ensure unicity.
/// I put no mutex, because all calls supposely com from the GUI thread.
/// 
/// All links are stored in html format into the clipboard. Why? Because this allows to import
/// links from both the clipboard and the RS application, e.g. paste links from an internet forum.
/// This requires many clipboard parsing operations, but this is not a problem because this code is
/// not performances-critical.
//
class RSLinkClipboard
{
	public:
		// Copy these links to the RS clipboard. Also copy them to the system clipboard
		//
		static void copyLinks(const QList<RetroShareLink>& links) ;

		// Get the liste of pasted links, either from the internal RS links, or by default
		// from the clipboard.
		//
		static void pasteLinks(QList<RetroShareLink> &links,RetroShareLink::enumType type = RetroShareLink::TYPE_UNKNOWN) ;

		// Produces a list of links with no html structure.
		static QString toString() ;

		// produces a list of html links that displays with the file names only
		//
		static QString toHtml();

		// produces a list of html links that displays with the file name + filesize
		//
		static QString toHtmlSize();		

		// Returns true is no links are found to paste.
		// Useful for menus.
		//
		static bool empty(RetroShareLink::enumType type = RetroShareLink::TYPE_UNKNOWN);

		// Returns the count of processed links
		// Useful for menus.
		//
		static int process(RetroShareLink::enumType type = RetroShareLink::TYPE_UNKNOWN, uint flag = RSLINK_PROCESS_NOTIFY_ALL);

		static void parseText(QString text, QList<RetroShareLink> &links, RetroShareLink::enumType type = RetroShareLink::TYPE_UNKNOWN) ;

	private:
		static void parseClipboard(QList<RetroShareLink> &links, RetroShareLink::enumType type = RetroShareLink::TYPE_UNKNOWN) ;
};

#endif
