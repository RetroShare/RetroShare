#ifndef RSGXSFLAGS_H
#define RSGXSFLAGS_H

#include "inttypes.h"

// this serves a single point of call for definining grp and msg modes
// GXS. These modes say
namespace GXS_SERV {



    /** privacy **/

    static const uint32_t FLAG_PRIVACY_MASK = 0x0000000f;

    // pub key encrypted
    static const uint32_t FLAG_PRIVACY_PRIVATE = 0x00000001;

    // publish private key needed to publish
    static const uint32_t FLAG_PRIVACY_RESTRICTED = 0x00000002;

    // anyone can publish, publish key pair not needed
    static const uint32_t FLAG_PRIVACY_PUBLIC = 0x00000004;

    /** privacy **/

    /** authentication **/

    static const uint32_t FLAG_AUTHEN_MASK = 0x000000f0;

    // identity
    static const uint32_t FLAG_AUTHEN_IDENTITY = 0x000000010;

    // publish key
    static const uint32_t FLAG_AUTHEN_PUBLISH = 0x000000020;

    // admin key
    static const uint32_t FLAG_AUTHEN_ADMIN = 0x00000040;

    // pgp sign identity
    static const uint32_t FLAG_AUTHEN_PGP_IDENTITY = 0x00000080;

    /** authentication **/


    // Subscription Flags. (LOCAL)

    static const uint32_t GROUP_SUBSCRIBE_ADMIN = 0x00000001;

    static const uint32_t GROUP_SUBSCRIBE_PUBLISH = 0x00000002;

    static const uint32_t GROUP_SUBSCRIBE_SUBSCRIBED = 0x00000004;

    static const uint32_t GROUP_SUBSCRIBE_NOT_SUBSCRIBED = 0x00000008;

    static const uint32_t GROUP_SUBSCRIBE_MASK = 0x0000000f;

}


#endif // RSGXSFLAGS_H
