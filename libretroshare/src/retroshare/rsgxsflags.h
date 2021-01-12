/*******************************************************************************
 * libretroshare/src/retroshare: rsgxsflags.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2018 by Retroshare Team <retroshare.project@gmail.com>       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RSGXSFLAGS_H
#define RSGXSFLAGS_H

#include "inttypes.h"

/**
 * The GXS_SERV namespace serves a single point of reference for definining grp and msg flags
 * Declared and defined here are:
 * - privacy flags which define the level of privacy that can be given \n
 *   to a group
 * - authentication types which defined types of authentication needed for a given message to
 *   confirm its authenticity
 * - subscription flags: This used only locally by the peer to subscription status to a \n
 *   a group
 * -
 */
namespace GXS_SERV {

    /** START privacy **/

    static const uint32_t FLAG_PRIVACY_MASK       = 0x0000000f;
    static const uint32_t FLAG_PRIVACY_PRIVATE    = 0x00000001; // pub key encrypted. No-one can read unless he has the key to decrypt the publish key.
    static const uint32_t FLAG_PRIVACY_RESTRICTED = 0x00000002; // publish private key needed to publish. Typical usage: channels.
    static const uint32_t FLAG_PRIVACY_PUBLIC     = 0x00000004; // anyone can publish, publish key pair not needed. Typical usage: forums.

    /** END privacy **/

    /** END authentication **/
    
    /** START author authentication flags **/
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_MASK           = 0x0000ff00;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_NONE           = 0x00000000;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_GPG            = 0x00000100;   // Anti-spam feature. Allows to ask higher reputation to anonymous IDs
	static const uint32_t FLAG_AUTHOR_AUTHENTICATION_REQUIRED       = 0x00000200;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_IFNOPUBSIGN    = 0x00000400;	// ???
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES = 0x00000800;	// not used anymore
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN      = 0x00001000;   // Anti-spam feature. Allows to ask higher reputation to unknown IDs and anonymous IDs

    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_MASK       = 0x000000ff;
    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_ENCRYPTED  = 0x00000001;
    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_ALLSIGNED  = 0x00000002;	// unused
    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_THREADHEAD = 0x00000004;
    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_NONEREQ    = 0x00000008;
    
    /** START msg authentication flags **/

    static const uint8_t MSG_AUTHEN_MASK               = 0x0f;
    static const uint8_t MSG_AUTHEN_ROOT_PUBLISH_SIGN  = 0x01;  // means: new threads need to be signed by the publish signature of the group. Typical use: posts in channels.
    static const uint8_t MSG_AUTHEN_CHILD_PUBLISH_SIGN = 0x02;  // means: all messages need to be signed by the publish signature of the group. Typical use: channels were comments are restricted to the publisher.
    static const uint8_t MSG_AUTHEN_ROOT_AUTHOR_SIGN   = 0x04;  // means: new threads need to be signed by the author of the message. Typical use: forums, since posts are signed.
    static const uint8_t MSG_AUTHEN_CHILD_AUTHOR_SIGN  = 0x08;  // means: all messages need to be signed by the author of the message. Typical use: forums since response to posts are signed, and signed comments in channels.

    /** END msg authentication flags **/

    /** START group options flag **/

    static const uint8_t GRP_OPTION_AUTHEN_AUTHOR_SIGN = 0x01;	// means: the group options (serialised grp data) needs to be signed by a specific author stored in GroupMeta.mAuthorId
    															// note that it is always signed by the *admin* (means the creator) of the group. This author signature is just an option here.

    /** END group options flag **/

    /** START Subscription Flags. (LOCAL) **/

    static const uint32_t GROUP_SUBSCRIBE_ADMIN          = 0x01;// means: you have the admin key for this group
    static const uint32_t GROUP_SUBSCRIBE_PUBLISH        = 0x02;// means: you have the publish key for thiss group. Typical use: publish key in channels are shared with specific friends.
    static const uint32_t GROUP_SUBSCRIBE_SUBSCRIBED     = 0x04;// means: you are subscribed to a group, which makes you a source for this group to your friend nodes.
    static const uint32_t GROUP_SUBSCRIBE_NOT_SUBSCRIBED = 0x08;

    /*!
     * Simply defines the range of bits that deriving services
     * should not use
     */
    static const uint32_t GROUP_SUBSCRIBE_MASK = 0x0000000f;

    /** END Subscription Flags. (LOCAL) **/

    /** START GXS Msg status flags **/

    /*!
     * Two lower bytes are reserved for Generic STATUS Flags listed here.
     * Services are free to use the two upper bytes. (16 flags).
     *
     * NOTE: RsGxsCommentService uses 0x000f0000.
     */
    static const uint32_t GXS_MSG_STATUS_GEN_MASK    = 0x0000ffff;
    static const uint32_t GXS_MSG_STATUS_UNPROCESSED = 0x00000001;	// Flags to store the read/process status of group messages.
    static const uint32_t GXS_MSG_STATUS_GUI_UNREAD  = 0x00000002;	// The actual meaning may depend on the type of service.
    static const uint32_t GXS_MSG_STATUS_GUI_NEW     = 0x00000004;	//
    static const uint32_t GXS_MSG_STATUS_KEEP_FOREVER = 0x00000008; // Do not delete message even if older then group maximum storage time
    static const uint32_t GXS_MSG_STATUS_DELETE      = 0x00000020;	//

    /** END GXS Msg status flags **/

    /** START GXS Grp status flags **/

    static const uint32_t GXS_GRP_STATUS_UNPROCESSED = 0x000000100;
    static const uint32_t GXS_GRP_STATUS_UNREAD      = 0x000000200;

    /** END GXS Grp status flags **/
}


// GENERIC GXS MACROS
#define IS_MSG_NEW(status)                      (status & GXS_SERV::GXS_MSG_STATUS_GUI_NEW)
#define IS_MSG_UNREAD(status)                   (status & GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD)
#define IS_MSG_UNPROCESSED(status)              (status & GXS_SERV::GXS_MSG_STATUS_UNPROCESSED)

#define IS_GROUP_PGP_AUTHED(signFlags)          (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG)
#define IS_GROUP_PGP_KNOWN_AUTHED(signFlags)    (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN)
#define IS_GROUP_MESSAGE_TRACKING(signFlags)    (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES)

#define IS_GROUP_ADMIN(subscribeFlags)          (subscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
#define IS_GROUP_PUBLISHER(subscribeFlags)      (subscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)
#define IS_GROUP_SUBSCRIBED(subscribeFlags)     (subscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
#define IS_GROUP_NOT_SUBSCRIBED(subscribeFlags) (subscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED)

#endif // RSGXSFLAGS_H
