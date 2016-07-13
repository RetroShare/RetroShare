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
    static const uint32_t FLAG_PRIVACY_PRIVATE    = 0x00000001; // pub key encrypted
    static const uint32_t FLAG_PRIVACY_RESTRICTED = 0x00000002; // publish private key needed to publish
    static const uint32_t FLAG_PRIVACY_PUBLIC     = 0x00000004; // anyone can publish, publish key pair not needed

    /** END privacy **/

    /** END authentication **/
    
    /** START author authentication flags **/
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_MASK           = 0x0000ff00;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_NONE           = 0x00000000;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_GPG            = 0x00000100;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_REQUIRED       = 0x00000200;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_IFNOPUBSIGN    = 0x00000400;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES = 0x00000800;
    static const uint32_t FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN      = 0x00001000;

    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_MASK       = 0x000000ff;
    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_ENCRYPTED  = 0x00000001;
    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_ALLSIGNED  = 0x00000002;
    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_THREADHEAD = 0x00000004;
    static const uint32_t FLAG_GROUP_SIGN_PUBLISH_NONEREQ    = 0x00000008;
    
    /** START msg authentication flags **/

    static const uint8_t MSG_AUTHEN_MASK               = 0x0f;
    static const uint8_t MSG_AUTHEN_ROOT_PUBLISH_SIGN  = 0x01;
    static const uint8_t MSG_AUTHEN_CHILD_PUBLISH_SIGN = 0x02;
    static const uint8_t MSG_AUTHEN_ROOT_AUTHOR_SIGN   = 0x04;
    static const uint8_t MSG_AUTHEN_CHILD_AUTHOR_SIGN  = 0x08;

    /** END msg authentication flags **/

    /** START group options flag **/

    static const uint8_t GRP_OPTION_AUTHEN_AUTHOR_SIGN = 0x01;

    /** END group options flag **/

    /** START Subscription Flags. (LOCAL) **/

    static const uint32_t GROUP_SUBSCRIBE_ADMIN          = 0x01;
    static const uint32_t GROUP_SUBSCRIBE_PUBLISH        = 0x02;
    static const uint32_t GROUP_SUBSCRIBE_SUBSCRIBED     = 0x04;
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
    static const uint32_t GXS_MSG_STATUS_UNPROCESSED = 0x00000001;
    static const uint32_t GXS_MSG_STATUS_GUI_UNREAD  = 0x00000002;
    static const uint32_t GXS_MSG_STATUS_GUI_NEW     = 0x00000004;
    static const uint32_t GXS_MSG_STATUS_KEEP        = 0x00000008;
    static const uint32_t GXS_MSG_STATUS_DELETE      = 0x00000020;

    /** END GXS Msg status flags **/

    /** START GXS Grp status flags **/

    static const uint32_t GXS_GRP_STATUS_UNPROCESSED = 0x000000100;

    static const uint32_t GXS_GRP_STATUS_UNREAD = 0x00000200;

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
