/***************************************************************************
 *   Copyright (C) 2009 Cyril Soler                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

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
#include <stdint.h>
#include <QString>
#include <QVector>
#include <QUrl>

#define RSLINK_PROCESS_NOTIFY_SUCCESS	  1 // notify on success
#define RSLINK_PROCESS_NOTIFY_ERROR		  2 // notify on error
#define RSLINK_PROCESS_NOTIFY_ASK		  4 // ask for add the links
#define RSLINK_PROCESS_NOTIFY_BAD_CHARS  8 // / or \\ characters in a filename

#define RSLINK_PROCESS_NOTIFY_ALL      15

#define RSLINK_SCHEME 	"retroshare"

#define RSLINK_SUBTYPE_CERTIFICATE_USER_REQUEST 1
#define RSLINK_SUBTYPE_FILE_EXTRA               2

class RetroShareLink
{
	public:
		enum enumType { 	TYPE_UNKNOWN       = 0x00, 
								TYPE_FILE          = 0x01, 
								TYPE_PERSON        = 0x02, 
								TYPE_FORUM         = 0x03, 
								TYPE_CHANNEL       = 0x04, 
								TYPE_SEARCH        = 0x05, 
								TYPE_MESSAGE       = 0x06, 
								TYPE_CERTIFICATE   = 0x07,
								TYPE_EXTRAFILE     = 0x08, 
								TYPE_PRIVATE_CHAT  = 0x09,
								TYPE_PUBLIC_MSG    = 0x0a
		};

	public:
		RetroShareLink();
		RetroShareLink(const QUrl& url);
		RetroShareLink(const QString& url);

		bool createFile(const QString& name, uint64_t size, const QString& hash);
		bool createExtraFile(const QString& name, uint64_t size, const QString& hash, const QString& ssl_id);
		bool createPerson(const std::string& id);
		bool createForum(const std::string& id, const std::string& msgId);
		bool createChannel(const std::string& id, const std::string& msgId);
		bool createSearch(const QString& keywords);
		bool createMessage(const std::string& peerId, const QString& subject);
		bool createCertificate(const std::string& ssl_or_gpg_id) ;
		bool createPrivateChatInvite(time_t time_stamp,const QString& gpg_id,const QString& encrypted_chat_info) ;
		bool createPublicMsgInvite(time_t time_stamp,const QString& pgp_id,const QString& hash) ;
		bool createUnknwonSslCertificate(const std::string& sslId, const std::string& gpgId = "") ;

		enumType type() const {return _type; }
		uint64_t size() const { return _size ; }
		const QString& name() const { return _name ; }
		const QString& hash() const { return _hash ; }
		const QString& id() const { return _hash ; }
		const QString& msgId() const { return _msgId ; }
		const QString& subject() const { return _subject ; }
		const QString& GPGRadix64Key() const { return _GPGBase64String ; }
		const QString& GPGBase64CheckSum() const { return _GPGBase64CheckSum ; }
		const QString& SSLId() const { return _SSLid ; }
		const QString& GPGId() const { return _GPGid ; }
		const QString& localIPAndPort() const { return _loc_ip_port ; }
		const QString& externalIPAndPort() const { return _ext_ip_port ; }
		const QString& location() const { return _location ; }
		time_t timeStamp() const { return _time_stamp ; }
		const QString& encryptedPrivateChatInfo() const { return _encrypted_chat_info ; }
		QString title() const;

		unsigned int subType() const { return _subType; }
		void setSubType(unsigned int subType) { _subType = subType; }

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

		bool valid() const { return _valid; }

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
		QString  _hash;  // or id (forum, channel, message)
		QString  _msgId; // id of the message (forum, channel)
		QString  _subject;
		QString  _SSLid ; // ssl id for rs links
		QString  _GPGid ; // ssl id for rs links
		QString  _GPGBase64String ; // GPG Cert
		QString  _GPGBase64CheckSum ; // GPG Cert
		QString  _location ;	// location 
		QString  _ext_ip_port ;
		QString  _loc_ip_port ;
		QString  _encrypted_chat_info ; // encrypted data string for the recipient of a chat invite
		time_t   _time_stamp ; 				// time stamp at which the link will expire.

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
		static void pasteLinks(QList<RetroShareLink> &links) ;

		// Produces a list of links with no html structure.
		static QString toString() ;

		// produces a list of html links that displays with the file names only
		//
		static QString toHtml();

		// produces a list of html links that displays the full links
		//
		static QString toHtmlFull();
		
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

	private:
		static void parseClipboard(QList<RetroShareLink> &links) ;
};

#endif
