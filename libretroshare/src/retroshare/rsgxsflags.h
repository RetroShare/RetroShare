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

    static const uint32_t FLAG_PRIVACY_MASK = 0x0000000f;

    // pub key encrypted
    static const uint32_t FLAG_PRIVACY_PRIVATE = 0x00000001;

    // publish private key needed to publish
    static const uint32_t FLAG_PRIVACY_RESTRICTED = 0x00000002;

    // anyone can publish, publish key pair not needed
    static const uint32_t FLAG_PRIVACY_PUBLIC = 0x00000004;

    /** END privacy **/

    /** START authentication **/

    static const uint32_t FLAG_AUTHEN_MASK = 0x000000f0;

    // identity
    static const uint32_t FLAG_AUTHEN_IDENTITY = 0x000000010;

    // publish key
    static const uint32_t FLAG_AUTHEN_PUBLISH = 0x000000020;

    // admin key
    static const uint32_t FLAG_AUTHEN_ADMIN = 0x00000040;

    // pgp sign identity
    static const uint32_t FLAG_AUTHEN_PGP_IDENTITY = 0x00000080;

    /** END authentication **/

    /** START msg authentication flags **/

    static const uint8_t MSG_AUTHEN_MASK = 0x0f;

    static const uint8_t MSG_AUTHEN_ROOT_PUBLISH_SIGN = 0x01;

    static const uint8_t MSG_AUTHEN_CHILD_PUBLISH_SIGN = 0x02;

    static const uint8_t MSG_AUTHEN_ROOT_AUTHOR_SIGN = 0x04;

    static const uint8_t MSG_AUTHEN_CHILD_AUTHOR_SIGN = 0x08;

    /** END msg authentication flags **/

    /** START group options flag **/

    static const uint8_t GRP_OPTION_AUTHEN_AUTHOR_SIGN = 0x01;

    /** END group options flag **/

    /** START Subscription Flags. (LOCAL) **/

    static const uint32_t GROUP_SUBSCRIBE_ADMIN = 0x01;

    static const uint32_t GROUP_SUBSCRIBE_PUBLISH = 0x02;

    static const uint32_t GROUP_SUBSCRIBE_SUBSCRIBED = 0x04;

    static const uint32_t GROUP_SUBSCRIBE_NOT_SUBSCRIBED = 0x08;

    /*!
     * Simply defines the range of bits that deriving services
     * should not use
     */
    static const uint32_t GROUP_SUBSCRIBE_MASK = 0x0000000f;

    /** END Subscription Flags. (LOCAL) **/

    /** START GXS Msg status flags **/

    static const uint32_t GXS_MSG_STATUS_UNPROCESSED = 0x0000001;

    static const uint32_t GXS_MSG_STATUS_UNREAD = 0x000002;

    static const uint32_t GXS_MSG_STATUS_READ = 0x000004;

    static const uint32_t GXS_MSG_STATUS_KEEP = 0x000008;

    static const uint32_t GXS_MSG_STATUS_DELETE = 0x0000020;

    /** END GXS Msg status flags **/

    /** START GXS Grp status flags **/

    static const uint32_t GXS_GRP_STATUS_UNPROCESSED = 0x000000100;

    static const uint32_t GXS_GRP_STATUS_UNREAD = 0x00000200;

    /** END GXS Grp status flags **/
}


// GENERIC GXS MACROS
#define IS_MSG_NEW(status)                      (status & GXS_SERV::GXS_MSG_STATUS_UNPROCESSED)

#define IS_MSG_UNREAD(status)                   (status & GXS_SERV::GXS_MSG_STATUS_UNREAD)
#define IS_GROUP_ADMIN(subscribeFlags)          (subscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
#define IS_GROUP_PUBLISHER(subscribeFlags)          (subscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)
#define IS_GROUP_SUBSCRIBED(subscribeFlags)     (subscribeFlags & (GXS_SERV::GROUP_SUBSCRIBE_ADMIN | GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED))
#define IS_GROUP_NOT_SUBSCRIBED(subscribeFlags)     (subscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED)


#define IS_MSG_UNPROCESSED(status)              (status & GXS_SERV::GXS_MSG_STATUS_UNPROCESSED)


#endif // RSGXSFLAGS_H
